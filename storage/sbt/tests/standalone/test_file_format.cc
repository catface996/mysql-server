/*****************************************************************************

Copyright (c) 2025, Oracle and/or its affiliates.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License, version 2.0,
as published by the Free Software Foundation.

This program is also distributed with certain software (including
but not limited to OpenSSL) that is licensed under separate terms,
as designated in a particular file or component or in included license
documentation.  The authors of MySQL hereby grant you an additional
permission to link the program and your derivative works with the
separately licensed software that they have included with MySQL.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License, version 2.0, for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA

*****************************************************************************/

/** @file tests/standalone/test_file_format.cc
 SBT File Format Standalone Test

 Created 2025-01-25
 *******************************************************/

#include <iostream>
#include <cstring>
#include <cstdio>
#include <unistd.h>

// Include SBT headers
#include "../../include/sbt_common.h"
#include "../../include/sbt_tree.h"
#include "../../include/sbt_file.h"

// Test configuration
const char* TEST_FILE = "/tmp/sbt_file_format_test.sbt";

// Test utilities
void cleanup_test_file() {
    SBT_file::delete_file(TEST_FILE);
}

bool test_file_creation() {
    std::cout << "Testing file creation..." << std::endl;
    
    SBT_file file;
    int result = file.create(TEST_FILE);
    
    if (result != SBT_SUCCESS) {
        std::cout << "FAILED: Could not create file, error: " << result << std::endl;
        return false;
    }
    
    // Check if file exists
    if (!SBT_file::file_exists(TEST_FILE)) {
        std::cout << "FAILED: File does not exist after creation" << std::endl;
        return false;
    }
    
    // Check file size (should be at least header size)
    my_off_t size = file.get_file_size();
    if (size < (my_off_t)sizeof(SBT_header)) {
        std::cout << "FAILED: File size too small: " << size << std::endl;
        return false;
    }
    
    file.close();
    std::cout << "PASSED: File creation test" << std::endl;
    return true;
}

bool test_file_header() {
    std::cout << "Testing file header format..." << std::endl;
    
    SBT_file file;
    
    // Create file
    int result = file.create(TEST_FILE);
    if (result != SBT_SUCCESS) {
        std::cout << "FAILED: Could not create file" << std::endl;
        return false;
    }
    file.close();
    
    // Reopen file to test header persistence
    result = file.open(TEST_FILE);
    if (result != SBT_SUCCESS) {
        std::cout << "FAILED: Could not reopen file, error: " << result << std::endl;
        return false;
    }
    
    file.close();
    std::cout << "PASSED: File header test" << std::endl;
    return true;
}

bool test_empty_tree_serialization() {
    std::cout << "Testing empty tree serialization..." << std::endl;
    
    SBT_file file;
    SBT_tree tree;
    
    // Create file
    int result = file.create(TEST_FILE);
    if (result != SBT_SUCCESS) {
        std::cout << "FAILED: Could not create file" << std::endl;
        return false;
    }
    
    // Save empty tree
    result = file.save_tree(&tree);
    if (result != SBT_SUCCESS) {
        std::cout << "FAILED: Could not save empty tree, error: " << result << std::endl;
        file.close();
        return false;
    }
    
    // Load tree back
    SBT_tree loaded_tree;
    result = file.load_tree(&loaded_tree);
    if (result != SBT_SUCCESS) {
        std::cout << "FAILED: Could not load tree, error: " << result << std::endl;
        file.close();
        return false;
    }
    
    // Verify loaded tree is empty
    if (!loaded_tree.is_empty()) {
        std::cout << "FAILED: Loaded tree is not empty" << std::endl;
        file.close();
        return false;
    }
    
    if (loaded_tree.get_record_count() != 0) {
        std::cout << "FAILED: Loaded tree record count is not 0" << std::endl;
        file.close();
        return false;
    }
    
    file.close();
    std::cout << "PASSED: Empty tree serialization test" << std::endl;
    return true;
}

bool test_tree_with_data_serialization() {
    std::cout << "Testing tree with data serialization..." << std::endl;
    
    SBT_file file;
    SBT_tree tree;
    
    // Create file
    int result = file.create(TEST_FILE);
    if (result != SBT_SUCCESS) {
        std::cout << "FAILED: Could not create file" << std::endl;
        return false;
    }
    
    // Add some test data
    const char* test_data1 = "test_record_1";
    const char* test_data2 = "test_record_2";
    const char* test_data3 = "test_record_3";
    
    result = tree.insert((const uchar*)test_data1, strlen(test_data1));
    if (result != SBT_SUCCESS) {
        std::cout << "FAILED: Could not insert test_data1" << std::endl;
        file.close();
        return false;
    }
    
    result = tree.insert((const uchar*)test_data2, strlen(test_data2));
    if (result != SBT_SUCCESS) {
        std::cout << "FAILED: Could not insert test_data2" << std::endl;
        file.close();
        return false;
    }
    
    result = tree.insert((const uchar*)test_data3, strlen(test_data3));
    if (result != SBT_SUCCESS) {
        std::cout << "FAILED: Could not insert test_data3" << std::endl;
        file.close();
        return false;
    }
    
    // Save tree
    result = file.save_tree(&tree);
    if (result != SBT_SUCCESS) {
        std::cout << "FAILED: Could not save tree with data, error: " << result << std::endl;
        file.close();
        return false;
    }
    
    // Load tree back
    SBT_tree loaded_tree;
    result = file.load_tree(&loaded_tree);
    if (result != SBT_SUCCESS) {
        std::cout << "FAILED: Could not load tree with data, error: " << result << std::endl;
        file.close();
        return false;
    }
    
    // Verify loaded tree has correct record count
    if (loaded_tree.get_record_count() != tree.get_record_count()) {
        std::cout << "FAILED: Record count mismatch. Expected: " << tree.get_record_count() 
                  << ", Got: " << loaded_tree.get_record_count() << std::endl;
        file.close();
        return false;
    }
    
    // Verify we can find the same records
    if (loaded_tree.find_by_data((const uchar*)test_data1, strlen(test_data1)) == nullptr) {
        std::cout << "FAILED: Could not find test_data1 in loaded tree" << std::endl;
        file.close();
        return false;
    }
    
    if (loaded_tree.find_by_data((const uchar*)test_data2, strlen(test_data2)) == nullptr) {
        std::cout << "FAILED: Could not find test_data2 in loaded tree" << std::endl;
        file.close();
        return false;
    }
    
    if (loaded_tree.find_by_data((const uchar*)test_data3, strlen(test_data3)) == nullptr) {
        std::cout << "FAILED: Could not find test_data3 in loaded tree" << std::endl;
        file.close();
        return false;
    }
    
    file.close();
    std::cout << "PASSED: Tree with data serialization test" << std::endl;
    return true;
}

bool test_file_corruption_detection() {
    std::cout << "Testing file corruption detection..." << std::endl;
    
    SBT_file file;
    
    // Create file
    int result = file.create(TEST_FILE);
    if (result != SBT_SUCCESS) {
        std::cout << "FAILED: Could not create file" << std::endl;
        return false;
    }
    file.close();
    
    // Corrupt the file by writing invalid magic number
    FILE* corrupt_file = fopen(TEST_FILE, "r+b");
    if (corrupt_file == nullptr) {
        std::cout << "FAILED: Could not open file for corruption" << std::endl;
        return false;
    }
    
    // Write invalid magic
    const char invalid_magic[] = "XXXX";
    fwrite(invalid_magic, 1, 4, corrupt_file);
    fclose(corrupt_file);
    
    // Try to open corrupted file
    result = file.open(TEST_FILE);
    if (result != SBT_ERR_CORRUPTED_DATA) {
        std::cout << "FAILED: Expected corruption error, got: " << result << std::endl;
        return false;
    }
    
    std::cout << "PASSED: File corruption detection test" << std::endl;
    return true;
}

bool test_crc32_checksum() {
    std::cout << "Testing CRC32 checksum calculation..." << std::endl;
    
    // Test known CRC32 values
    const char* test_string = "Hello, World!";
    uint32_t crc = sbt_crc32((const uchar*)test_string, strlen(test_string));
    
    // CRC32 should be consistent
    uint32_t crc2 = sbt_crc32((const uchar*)test_string, strlen(test_string));
    if (crc != crc2) {
        std::cout << "FAILED: CRC32 calculation is not consistent" << std::endl;
        return false;
    }
    
    // Test CRC32 update
    const char* part1 = "Hello, ";
    const char* part2 = "World!";
    
    uint32_t crc_full = sbt_crc32((const uchar*)test_string, strlen(test_string));
    uint32_t crc_part1 = sbt_crc32((const uchar*)part1, strlen(part1));
    uint32_t crc_combined = sbt_crc32_update(crc_part1, (const uchar*)part2, strlen(part2));
    
    if (crc_full != crc_combined) {
        std::cout << "FAILED: CRC32 update does not match full calculation" << std::endl;
        return false;
    }
    
    std::cout << "PASSED: CRC32 checksum test" << std::endl;
    return true;
}

int main() {
    std::cout << "=== SBT File Format Test Suite ===" << std::endl;
    
    int passed = 0;
    int total = 0;
    
    // Clean up any existing test file
    cleanup_test_file();
    
    // Run tests
    total++; if (test_crc32_checksum()) passed++;
    cleanup_test_file();
    
    total++; if (test_file_creation()) passed++;
    cleanup_test_file();
    
    total++; if (test_file_header()) passed++;
    cleanup_test_file();
    
    total++; if (test_empty_tree_serialization()) passed++;
    cleanup_test_file();
    
    total++; if (test_tree_with_data_serialization()) passed++;
    cleanup_test_file();
    
    total++; if (test_file_corruption_detection()) passed++;
    cleanup_test_file();
    
    // Final cleanup
    cleanup_test_file();
    
    std::cout << "\n=== Test Results ===" << std::endl;
    std::cout << "Passed: " << passed << "/" << total << std::endl;
    
    if (passed == total) {
        std::cout << "All tests PASSED!" << std::endl;
        return 0;
    } else {
        std::cout << "Some tests FAILED!" << std::endl;
        return 1;
    }
}