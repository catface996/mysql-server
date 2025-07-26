/*****************************************************************************

Copyright (c) 2025, Oracle and/or its affiliates.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License, version 2.0,
as published by the Free Software Foundation.

*****************************************************************************/

/** @file test_file_operations_simple.cc
 Simple Test for SBT File Operations Interface

 This test verifies basic file operations without MySQL dependencies
 
 Created 2025-01-26
 *******************************************************/

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <cassert>
#include <unistd.h>
#include <sys/stat.h>

// Minimal includes to avoid MySQL plugin dependencies
#include "my_config.h"
#include "my_inttypes.h"
#include "my_sys.h"
#include "my_io.h"
#include "my_alloc.h"
#include <fcntl.h>

// Test configuration
const char* TEST_FILE_PATH = "/tmp/sbt_file_test.sbt";

// Simplified error codes for testing
enum sbt_error_t {
  SBT_SUCCESS = 0,
  SBT_ERR_OUT_OF_MEMORY,
  SBT_ERR_FILE_NOT_FOUND,
  SBT_ERR_CORRUPTED_DATA,
  SBT_ERR_IO_ERROR,
  SBT_ERR_INVALID_ARGUMENT
};

// Simplified file header for testing
struct SBT_header {
  char magic[4];                    // "SBT\0"
  uint32_t version;                 // File format version
  uint64_t record_count;            // Number of records
  uint64_t next_insert_id;          // Next insert ID
  uint64_t tree_root_offset;        // Offset to tree data
  uint32_t tree_data_size;          // Size of tree data
  uint32_t header_size;             // Header size
  uint32_t checksum;                // Header checksum
  uint64_t created_time;            // Creation time
  uint64_t modified_time;           // Modification time
  char reserved[16];                // Reserved space
};

// File format constants
#define SBT_FILE_MAGIC "SBT\0"
#define SBT_FILE_MAGIC_SIZE 4
#define SBT_FILE_VERSION 1
#define SBT_HEADER_SIZE sizeof(SBT_header)

// Simple CRC32 implementation for testing
uint32_t simple_crc32(const uchar *data, size_t length) {
  uint32_t crc = 0xFFFFFFFF;
  for (size_t i = 0; i < length; i++) {
    crc ^= data[i];
    for (int j = 0; j < 8; j++) {
      if (crc & 1) {
        crc = (crc >> 1) ^ 0xEDB88320;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc ^ 0xFFFFFFFF;
}

// Simple file operations class for testing
class Simple_SBT_file {
private:
  File fd;
  char *file_name;
  bool is_open;

public:
  Simple_SBT_file() : fd(-1), file_name(nullptr), is_open(false) {}
  
  ~Simple_SBT_file() {
    if (is_open) {
      close();
    }
    if (file_name) {
      my_free(file_name);
    }
  }

  int create(const char *name) {
    if (!name) {
      return SBT_ERR_INVALID_ARGUMENT;
    }

    fd = my_create(name, 0, O_RDWR | O_TRUNC, MYF(MY_WME));
    if (fd < 0) {
      return SBT_ERR_IO_ERROR;
    }

    size_t name_len = strlen(name);
    file_name = (char *)my_malloc(PSI_NOT_INSTRUMENTED, name_len + 1, MYF(MY_WME));
    if (!file_name) {
      my_close(fd, MYF(0));
      fd = -1;
      return SBT_ERR_OUT_OF_MEMORY;
    }
    strcpy(file_name, name);
    is_open = true;

    // Write initial header
    SBT_header header;
    memset(&header, 0, sizeof(header));
    memcpy(header.magic, SBT_FILE_MAGIC, SBT_FILE_MAGIC_SIZE);
    header.version = SBT_FILE_VERSION;
    header.record_count = 0;
    header.next_insert_id = 1;
    header.tree_root_offset = SBT_HEADER_SIZE;
    header.tree_data_size = 0;
    header.header_size = SBT_HEADER_SIZE;
    header.created_time = 1234567890; // Dummy timestamp
    header.modified_time = 1234567890;
    
    // Calculate checksum
    header.checksum = 0;
    header.checksum = simple_crc32((const uchar*)&header, sizeof(header) - sizeof(header.checksum));

    if (my_pwrite(fd, (uchar *)&header, sizeof(header), 0, MYF(MY_NABP)) != 0) {
      return SBT_ERR_IO_ERROR;
    }

    return SBT_SUCCESS;
  }

  int open(const char *name) {
    if (!name) {
      return SBT_ERR_INVALID_ARGUMENT;
    }

    fd = my_open(name, O_RDWR, MYF(MY_WME));
    if (fd < 0) {
      return SBT_ERR_FILE_NOT_FOUND;
    }

    size_t name_len = strlen(name);
    file_name = (char *)my_malloc(PSI_NOT_INSTRUMENTED, name_len + 1, MYF(MY_WME));
    if (!file_name) {
      my_close(fd, MYF(0));
      fd = -1;
      return SBT_ERR_OUT_OF_MEMORY;
    }
    strcpy(file_name, name);
    is_open = true;

    // Read and validate header
    SBT_header header;
    if (my_pread(fd, (uchar *)&header, sizeof(header), 0, MYF(MY_NABP)) != 0) {
      close();
      return SBT_ERR_IO_ERROR;
    }

    // Validate magic number
    if (memcmp(header.magic, SBT_FILE_MAGIC, SBT_FILE_MAGIC_SIZE) != 0) {
      close();
      return SBT_ERR_CORRUPTED_DATA;
    }

    // Validate version
    if (header.version != SBT_FILE_VERSION) {
      close();
      return SBT_ERR_CORRUPTED_DATA;
    }

    return SBT_SUCCESS;
  }

  int close() {
    if (is_open && fd >= 0) {
      my_close(fd, MYF(0));
      fd = -1;
      is_open = false;
    }
    return SBT_SUCCESS;
  }

  my_off_t get_file_size() {
    if (!is_open || fd < 0) {
      return -1;
    }

    my_off_t current_pos = my_tell(fd, MYF(0));
    if (current_pos == MY_FILEPOS_ERROR) {
      return -1;
    }

    my_off_t file_size = my_seek(fd, 0, MY_SEEK_END, MYF(0));
    if (file_size == MY_FILEPOS_ERROR) {
      return -1;
    }

    if (my_seek(fd, current_pos, MY_SEEK_SET, MYF(0)) == MY_FILEPOS_ERROR) {
      return -1;
    }

    return file_size;
  }

  static int delete_file(const char *name) {
    if (!name) {
      return SBT_ERR_INVALID_ARGUMENT;
    }

    if (my_delete(name, MYF(0)) != 0) {
      return SBT_ERR_IO_ERROR;
    }

    return SBT_SUCCESS;
  }

  static bool file_exists(const char *name) {
    if (!name) {
      return false;
    }

    File test_fd = my_open(name, O_RDONLY, MYF(0));
    if (test_fd < 0) {
      return false;
    }

    my_close(test_fd, MYF(0));
    return true;
  }
};

// Helper function to clean up test files
void cleanup_test_files() {
  unlink(TEST_FILE_PATH);
}

// Helper function to check if file exists
bool file_exists(const char* path) {
  struct stat buffer;
  return (stat(path, &buffer) == 0);
}

// Test utilities
class FileOperationsTest {
private:
    int test_count;
    int passed_count;
    
public:
    FileOperationsTest() : test_count(0), passed_count(0) {}
    
    void run_test(const std::string& test_name, bool (*test_func)()) {
        test_count++;
        std::cout << "Running: " << test_name << "... ";
        
        if (test_func()) {
            std::cout << "PASSED" << std::endl;
            passed_count++;
        } else {
            std::cout << "FAILED" << std::endl;
        }
    }
    
    void print_summary() {
        std::cout << "\n=== Test Summary ===" << std::endl;
        std::cout << "Total tests: " << test_count << std::endl;
        std::cout << "Passed: " << passed_count << std::endl;
        std::cout << "Failed: " << (test_count - passed_count) << std::endl;
        
        if (passed_count == test_count) {
            std::cout << "\n🎉 ALL FILE OPERATIONS TESTS PASSED! 🎉" << std::endl;
        } else {
            std::cout << "\n❌ Some tests failed!" << std::endl;
        }
    }
    
    bool all_passed() const {
        return passed_count == test_count;
    }
};

// Test 1: Basic file creation and deletion
bool test_file_creation_deletion() {
    cleanup_test_files();
    
    Simple_SBT_file file;
    
    // Test file creation
    int result = file.create(TEST_FILE_PATH);
    if (result != SBT_SUCCESS) {
        std::cerr << "Failed to create file: " << result << std::endl;
        return false;
    }
    
    // Verify file exists
    if (!file_exists(TEST_FILE_PATH)) {
        std::cerr << "File was not created on disk" << std::endl;
        return false;
    }
    
    // Test file closing
    result = file.close();
    if (result != SBT_SUCCESS) {
        std::cerr << "Failed to close file: " << result << std::endl;
        return false;
    }
    
    // Test file deletion
    result = Simple_SBT_file::delete_file(TEST_FILE_PATH);
    if (result != SBT_SUCCESS) {
        std::cerr << "Failed to delete file: " << result << std::endl;
        return false;
    }
    
    // Verify file is deleted
    if (file_exists(TEST_FILE_PATH)) {
        std::cerr << "File was not deleted from disk" << std::endl;
        return false;
    }
    
    return true;
}

// Test 2: File opening and header validation
bool test_file_opening_validation() {
    cleanup_test_files();
    
    Simple_SBT_file file;
    
    // Create a file first
    int result = file.create(TEST_FILE_PATH);
    if (result != SBT_SUCCESS) {
        return false;
    }
    file.close();
    
    // Test opening existing file
    result = file.open(TEST_FILE_PATH);
    if (result != SBT_SUCCESS) {
        std::cerr << "Failed to open existing file: " << result << std::endl;
        cleanup_test_files();
        return false;
    }
    
    file.close();
    
    // Test opening non-existent file
    result = file.open("/tmp/non_existent_file.sbt");
    if (result == SBT_SUCCESS) {
        std::cerr << "Should have failed to open non-existent file" << std::endl;
        cleanup_test_files();
        return false;
    }
    
    cleanup_test_files();
    return true;
}

// Test 3: File corruption detection
bool test_file_corruption_detection() {
    cleanup_test_files();
    
    Simple_SBT_file file;
    
    // Create a valid file first
    int result = file.create(TEST_FILE_PATH);
    if (result != SBT_SUCCESS) {
        return false;
    }
    file.close();
    
    // Corrupt the file by writing invalid magic number
    FILE* corrupt_file = fopen(TEST_FILE_PATH, "r+b");
    if (!corrupt_file) {
        cleanup_test_files();
        return false;
    }
    
    // Write invalid magic number
    const char* invalid_magic = "XXXX";
    fwrite(invalid_magic, 1, 4, corrupt_file);
    fclose(corrupt_file);
    
    // Try to open corrupted file
    result = file.open(TEST_FILE_PATH);
    if (result == SBT_SUCCESS) {
        std::cerr << "Should have detected file corruption" << std::endl;
        file.close();
        cleanup_test_files();
        return false;
    }
    
    cleanup_test_files();
    return true;
}

// Test 4: File metadata verification
bool test_file_metadata() {
    cleanup_test_files();
    
    Simple_SBT_file file;
    
    // Create file
    int result = file.create(TEST_FILE_PATH);
    if (result != SBT_SUCCESS) {
        return false;
    }
    
    // Check file size
    my_off_t file_size = file.get_file_size();
    if (file_size <= 0) {
        std::cerr << "Invalid file size: " << file_size << std::endl;
        file.close();
        cleanup_test_files();
        return false;
    }
    
    // Should be at least the size of the header
    if (file_size < (my_off_t)SBT_HEADER_SIZE) {
        std::cerr << "File size too small. Expected at least: " << SBT_HEADER_SIZE 
                  << ", Got: " << file_size << std::endl;
        file.close();
        cleanup_test_files();
        return false;
    }
    
    file.close();
    
    // Test file_exists utility
    if (!Simple_SBT_file::file_exists(TEST_FILE_PATH)) {
        std::cerr << "file_exists should return true for existing file" << std::endl;
        cleanup_test_files();
        return false;
    }
    
    if (Simple_SBT_file::file_exists("/tmp/non_existent_file.sbt")) {
        std::cerr << "file_exists should return false for non-existent file" << std::endl;
        cleanup_test_files();
        return false;
    }
    
    cleanup_test_files();
    return true;
}

// Test 5: Error handling edge cases
bool test_error_handling() {
    cleanup_test_files();
    
    Simple_SBT_file file;
    
    // Test invalid arguments
    int result = file.create(nullptr);
    if (result == SBT_SUCCESS) {
        std::cerr << "Should have failed with null filename" << std::endl;
        return false;
    }
    
    result = file.open(nullptr);
    if (result == SBT_SUCCESS) {
        std::cerr << "Should have failed with null filename" << std::endl;
        return false;
    }
    
    // Test operations on closed file
    my_off_t size = file.get_file_size();
    if (size != -1) {
        std::cerr << "Should have failed to get size of closed file" << std::endl;
        return false;
    }
    
    return true;
}

// Main test runner
int main() {
    std::cout << "=== SBT File Operations Interface Test (Simple) ===" << std::endl;
    std::cout << "Testing basic file operations functionality..." << std::endl << std::endl;
    
    FileOperationsTest test_runner;
    
    // Run all tests
    test_runner.run_test("File Creation and Deletion", test_file_creation_deletion);
    test_runner.run_test("File Opening and Validation", test_file_opening_validation);
    test_runner.run_test("File Corruption Detection", test_file_corruption_detection);
    test_runner.run_test("File Metadata Verification", test_file_metadata);
    test_runner.run_test("Error Handling Edge Cases", test_error_handling);
    
    // Print summary
    test_runner.print_summary();
    
    // Clean up any remaining test files
    cleanup_test_files();
    
    return test_runner.all_passed() ? 0 : 1;
}