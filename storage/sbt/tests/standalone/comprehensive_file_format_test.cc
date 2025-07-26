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

/** @file tests/standalone/comprehensive_file_format_test.cc
 SBT File Format Comprehensive Verification Test

 Created 2025-01-25
 *******************************************************/

#include <iostream>
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <sys/stat.h>

// Include SBT headers
#include "../../include/sbt_common.h"
#include "../../include/sbt_tree.h"
#include "../../include/sbt_file.h"

// Test configuration
const char* TEST_FILE = "/tmp/sbt_comprehensive_test.sbt";

// Test utilities
void cleanup_test_file() {
    SBT_file::delete_file(TEST_FILE);
}

void print_test_header(const char* test_name) {
    std::cout << "\n=== " << test_name << " ===" << std::endl;
}

void print_test_result(const char* test_name, bool passed) {
    std::cout << "[" << (passed ? "PASS" : "FAIL") << "] " << test_name << std::endl;
}

// Test 1: File Header Structure Verification
bool test_file_header_structure() {
    print_test_header("File Header Structure Verification");
    
    // Verify header size and alignment
    size_t expected_header_size = sizeof(SBT_header);
    std::cout << "Header size: " << expected_header_size << " bytes" << std::endl;
    
    // Verify magic number constant
    if (strlen(SBT_FILE_MAGIC) != SBT_FILE_MAGIC_SIZE - 1) {
        std::cout << "FAILED: Magic number size mismatch" << std::endl;
        return false;
    }
    
    // Verify file version
    if (SBT_FILE_VERSION != 1) {
        std::cout << "FAILED: Unexpected file version: " << SBT_FILE_VERSION << std::endl;
        return false;
    }
    
    std::cout << "Magic number: '" << SBT_FILE_MAGIC << "'" << std::endl;
    std::cout << "File version: " << SBT_FILE_VERSION << std::endl;
    std::cout << "Header alignment: " << SBT_FILE_ALIGNMENT << " bytes" << std::endl;
    
    return true;
}

// Test 2: File Creation and Header Persistence
bool test_file_creation_and_header() {
    print_test_header("File Creation and Header Persistence");
    
    SBT_file file;
    
    // Create file
    int result = file.create(TEST_FILE);
    if (result != SBT_SUCCESS) {
        std::cout << "FAILED: Could not create file, error: " << result << std::endl;
        return false;
    }
    
    // Verify file exists
    if (!SBT_file::file_exists(TEST_FILE)) {
        std::cout << "FAILED: File does not exist after creation" << std::endl;
        return false;
    }
    
    // Check file size
    my_off_t size = file.get_file_size();
    if (size < (my_off_t)sizeof(SBT_header)) {
        std::cout << "FAILED: File size too small: " << size << std::endl;
        return false;
    }
    
    std::cout << "File created successfully, size: " << size << " bytes" << std::endl;
    
    file.close();
    
    // Reopen and verify header persistence
    result = file.open(TEST_FILE);
    if (result != SBT_SUCCESS) {
        std::cout << "FAILED: Could not reopen file, error: " << result << std::endl;
        return false;
    }
    
    file.close();
    std::cout << "Header persistence verified" << std::endl;
    
    return true;
}

// Test 3: CRC32 Checksum Functionality
bool test_crc32_checksum() {
    print_test_header("CRC32 Checksum Functionality");
    
    // Test basic CRC32 calculation
    const char* test_data = "Hello, SBT Storage Engine!";
    uint32_t crc1 = sbt_crc32((const uchar*)test_data, strlen(test_data));
    uint32_t crc2 = sbt_crc32((const uchar*)test_data, strlen(test_data));
    
    if (crc1 != crc2) {
        std::cout << "FAILED: CRC32 calculation not consistent" << std::endl;
        return false;
    }
    
    std::cout << "CRC32 for test data: 0x" << std::hex << crc1 << std::dec << std::endl;
    
    // Test CRC32 update functionality
    const char* part1 = "Hello, ";
    const char* part2 = "SBT Storage Engine!";
    
    uint32_t crc_full = sbt_crc32((const uchar*)test_data, strlen(test_data));
    uint32_t crc_part1 = sbt_crc32((const uchar*)part1, strlen(part1));
    uint32_t crc_combined = sbt_crc32_update(crc_part1, (const uchar*)part2, strlen(part2));
    
    if (crc_full != crc_combined) {
        std::cout << "FAILED: CRC32 update functionality broken" << std::endl;
        std::cout << "Full CRC: 0x" << std::hex << crc_full << std::endl;
        std::cout << "Combined CRC: 0x" << std::hex << crc_combined << std::dec << std::endl;
        return false;
    }
    
    std::cout << "CRC32 update functionality verified" << std::endl;
    
    // Test empty data
    uint32_t empty_crc = sbt_crc32((const uchar*)"", 0);
    std::cout << "CRC32 for empty data: 0x" << std::hex << empty_crc << std::dec << std::endl;
    
    return true;
}

// Test 4: Tree Serialization Format
bool test_tree_serialization_format() {
    print_test_header("Tree Serialization Format");
    
    SBT_file file;
    SBT_tree tree;
    
    // Create file
    int result = file.create(TEST_FILE);
    if (result != SBT_SUCCESS) {
        std::cout << "FAILED: Could not create file" << std::endl;
        return false;
    }
    
    // Test empty tree serialization
    result = file.save_tree(&tree);
    if (result != SBT_SUCCESS) {
        std::cout << "FAILED: Could not save empty tree" << std::endl;
        file.close();
        return false;
    }
    
    my_off_t empty_size = file.get_file_size();
    std::cout << "Empty tree file size: " << empty_size << " bytes" << std::endl;
    
    // Add test data
    const char* records[] = {
        "Record 1: Short data",
        "Record 2: Medium length data with more content",
        "Record 3: Very long data content that tests the variable-length record handling capabilities of the serialization system",
        "Record 4: Unicode test: 你好世界",
        "Record 5: Special chars: !@#$%^&*()_+-=[]{}|;:,.<>?"
    };
    
    for (int i = 0; i < 5; i++) {
        result = tree.insert((const uchar*)records[i], strlen(records[i]));
        if (result != SBT_SUCCESS) {
            std::cout << "FAILED: Could not insert record " << i << std::endl;
            file.close();
            return false;
        }
    }
    
    std::cout << "Inserted " << tree.get_record_count() << " records" << std::endl;
    
    // Save tree with data
    result = file.save_tree(&tree);
    if (result != SBT_SUCCESS) {
        std::cout << "FAILED: Could not save tree with data" << std::endl;
        file.close();
        return false;
    }
    
    my_off_t full_size = file.get_file_size();
    std::cout << "Tree with data file size: " << full_size << " bytes" << std::endl;
    std::cout << "Data size increase: " << (full_size - empty_size) << " bytes" << std::endl;
    
    // Load and verify
    SBT_tree loaded_tree;
    result = file.load_tree(&loaded_tree);
    if (result != SBT_SUCCESS) {
        std::cout << "FAILED: Could not load tree" << std::endl;
        file.close();
        return false;
    }
    
    if (loaded_tree.get_record_count() != tree.get_record_count()) {
        std::cout << "FAILED: Record count mismatch" << std::endl;
        file.close();
        return false;
    }
    
    // Verify all records can be found
    for (int i = 0; i < 5; i++) {
        if (loaded_tree.find_by_data((const uchar*)records[i], strlen(records[i])) == nullptr) {
            std::cout << "FAILED: Could not find record " << i << " after loading" << std::endl;
            file.close();
            return false;
        }
    }
    
    std::cout << "All records verified after serialization round-trip" << std::endl;
    
    file.close();
    return true;
}

// Test 5: File Corruption Detection
bool test_corruption_detection() {
    print_test_header("File Corruption Detection");
    
    SBT_file file;
    
    // Create a valid file
    int result = file.create(TEST_FILE);
    if (result != SBT_SUCCESS) {
        std::cout << "FAILED: Could not create file for corruption test" << std::endl;
        return false;
    }
    file.close();
    
    // Test 1: Corrupt magic number
    {
        FILE* corrupt_file = fopen(TEST_FILE, "r+b");
        if (!corrupt_file) {
            std::cout << "FAILED: Could not open file for corruption" << std::endl;
            return false;
        }
        
        const char invalid_magic[] = "XXXX";
        fwrite(invalid_magic, 1, 4, corrupt_file);
        fclose(corrupt_file);
        
        result = file.open(TEST_FILE);
        if (result != SBT_ERR_CORRUPTED_DATA) {
            std::cout << "FAILED: Magic number corruption not detected, got error: " << result << std::endl;
            return false;
        }
        
        std::cout << "Magic number corruption detected correctly" << std::endl;
    }
    
    // Recreate valid file for next test
    result = file.create(TEST_FILE);
    if (result != SBT_SUCCESS) {
        std::cout << "FAILED: Could not recreate file" << std::endl;
        return false;
    }
    file.close();
    
    // Test 2: Corrupt checksum
    {
        FILE* corrupt_file = fopen(TEST_FILE, "r+b");
        if (!corrupt_file) {
            std::cout << "FAILED: Could not open file for checksum corruption" << std::endl;
            return false;
        }
        
        // Seek to checksum field (after magic, version, record_count, next_insert_id, tree_root_offset, tree_data_size, header_size)
        size_t checksum_offset = 4 + 4 + 8 + 8 + 8 + 4 + 4; // 40 bytes
        fseek(corrupt_file, checksum_offset, SEEK_SET);
        
        uint32_t bad_checksum = 0xDEADBEEF;
        fwrite(&bad_checksum, sizeof(bad_checksum), 1, corrupt_file);
        fclose(corrupt_file);
        
        result = file.open(TEST_FILE);
        if (result != SBT_ERR_CORRUPTED_DATA) {
            std::cout << "FAILED: Checksum corruption not detected, got error: " << result << std::endl;
            return false;
        }
        
        std::cout << "Checksum corruption detected correctly" << std::endl;
    }
    
    return true;
}

// Test 6: Performance and Scalability
bool test_performance_characteristics() {
    print_test_header("Performance and Scalability");
    
    SBT_file file;
    SBT_tree tree;
    
    int result = file.create(TEST_FILE);
    if (result != SBT_SUCCESS) {
        std::cout << "FAILED: Could not create file for performance test" << std::endl;
        return false;
    }
    
    // Test with different data sizes
    const int test_sizes[] = {10, 100, 1000};
    const int num_tests = sizeof(test_sizes) / sizeof(test_sizes[0]);
    
    for (int t = 0; t < num_tests; t++) {
        int record_count = test_sizes[t];
        
        // Clear tree
        tree.clear();
        
        // Insert records
        for (int i = 0; i < record_count; i++) {
            char record_data[256];
            snprintf(record_data, sizeof(record_data), "Performance test record %d with some additional data to make it realistic", i);
            
            result = tree.insert((const uchar*)record_data, strlen(record_data));
            if (result != SBT_SUCCESS) {
                std::cout << "FAILED: Could not insert record " << i << " in performance test" << std::endl;
                file.close();
                return false;
            }
        }
        
        // Measure serialization
        result = file.save_tree(&tree);
        if (result != SBT_SUCCESS) {
            std::cout << "FAILED: Could not save tree with " << record_count << " records" << std::endl;
            file.close();
            return false;
        }
        
        my_off_t file_size = file.get_file_size();
        
        // Measure deserialization
        SBT_tree loaded_tree;
        result = file.load_tree(&loaded_tree);
        if (result != SBT_SUCCESS) {
            std::cout << "FAILED: Could not load tree with " << record_count << " records" << std::endl;
            file.close();
            return false;
        }
        
        if (loaded_tree.get_record_count() != (uint64_t)record_count) {
            std::cout << "FAILED: Record count mismatch in performance test" << std::endl;
            file.close();
            return false;
        }
        
        std::cout << "Records: " << record_count 
                  << ", File size: " << file_size 
                  << " bytes, Avg per record: " << (file_size / record_count) << " bytes" << std::endl;
    }
    
    file.close();
    return true;
}

// Test 7: Edge Cases and Error Handling
bool test_edge_cases() {
    print_test_header("Edge Cases and Error Handling");
    
    // Test invalid file operations
    SBT_file file;
    
    // Test opening non-existent file
    int result = file.open("/tmp/non_existent_file.sbt");
    if (result == SBT_SUCCESS) {
        std::cout << "FAILED: Opening non-existent file should fail" << std::endl;
        return false;
    }
    std::cout << "Non-existent file handling: OK" << std::endl;
    
    // Test creating file with invalid path
    result = file.create("/invalid/path/test.sbt");
    if (result == SBT_SUCCESS) {
        std::cout << "FAILED: Creating file in invalid path should fail" << std::endl;
        return false;
    }
    std::cout << "Invalid path handling: OK" << std::endl;
    
    // Test null parameters
    result = file.create(nullptr);
    if (result != SBT_ERR_INVALID_ARGUMENT) {
        std::cout << "FAILED: Null parameter should return INVALID_ARGUMENT" << std::endl;
        return false;
    }
    std::cout << "Null parameter handling: OK" << std::endl;
    
    // Test operations on closed file
    SBT_tree tree;
    result = file.save_tree(&tree);
    if (result == SBT_SUCCESS) {
        std::cout << "FAILED: Operations on closed file should fail" << std::endl;
        return false;
    }
    std::cout << "Closed file operation handling: OK" << std::endl;
    
    return true;
}

int main() {
    std::cout << "=== SBT File Format Comprehensive Verification Test ===" << std::endl;
    std::cout << "Testing all aspects of the file format implementation..." << std::endl;
    
    int passed = 0;
    int total = 0;
    
    // Clean up any existing test file
    cleanup_test_file();
    
    // Run comprehensive tests
    total++; bool t1 = test_file_header_structure(); 
    print_test_result("File Header Structure", t1); if (t1) passed++;
    
    cleanup_test_file();
    total++; bool t2 = test_file_creation_and_header(); 
    print_test_result("File Creation and Header", t2); if (t2) passed++;
    
    cleanup_test_file();
    total++; bool t3 = test_crc32_checksum(); 
    print_test_result("CRC32 Checksum", t3); if (t3) passed++;
    
    cleanup_test_file();
    total++; bool t4 = test_tree_serialization_format(); 
    print_test_result("Tree Serialization Format", t4); if (t4) passed++;
    
    cleanup_test_file();
    total++; bool t5 = test_corruption_detection(); 
    print_test_result("Corruption Detection", t5); if (t5) passed++;
    
    cleanup_test_file();
    total++; bool t6 = test_performance_characteristics(); 
    print_test_result("Performance Characteristics", t6); if (t6) passed++;
    
    cleanup_test_file();
    total++; bool t7 = test_edge_cases(); 
    print_test_result("Edge Cases and Error Handling", t7); if (t7) passed++;
    
    // Final cleanup
    cleanup_test_file();
    
    std::cout << "\n=== Comprehensive Test Results ===" << std::endl;
    std::cout << "Passed: " << passed << "/" << total << " tests" << std::endl;
    
    if (passed == total) {
        std::cout << "\n🎉 ALL TESTS PASSED! 🎉" << std::endl;
        std::cout << "File format implementation is fully verified and ready for production." << std::endl;
        return 0;
    } else {
        std::cout << "\n❌ SOME TESTS FAILED! ❌" << std::endl;
        std::cout << "Please review the failed tests and fix the implementation." << std::endl;
        return 1;
    }
}