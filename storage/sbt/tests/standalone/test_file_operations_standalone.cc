/*****************************************************************************

Copyright (c) 2025, Oracle and/or its affiliates.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License, version 2.0,
as published by the Free Software Foundation.

*****************************************************************************/

/** @file test_file_operations_standalone.cc
 Comprehensive Test for SBT File Operations Interface

 This test verifies all file operations including:
 - File creation and deletion
 - File opening and closing
 - Tree saving and loading
 - Error handling and data integrity
 - File corruption detection
 
 Created 2025-01-26
 *******************************************************/

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <cassert>
#include <unistd.h>
#include <sys/stat.h>

// Include SBT headers
#include "../../include/sbt_common.h"
#include "../../include/sbt_tree.h"
#include "../../include/sbt_file.h"

// Test configuration
const char* TEST_FILE_PATH = "/tmp/sbt_file_test.sbt";
const char* TEST_FILE_PATH_2 = "/tmp/sbt_file_test_2.sbt";
const char* CORRUPTED_FILE_PATH = "/tmp/sbt_corrupted_test.sbt";

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

// Helper function to clean up test files
void cleanup_test_files() {
    unlink(TEST_FILE_PATH);
    unlink(TEST_FILE_PATH_2);
    unlink(CORRUPTED_FILE_PATH);
}

// Helper function to check if file exists
bool file_exists(const char* path) {
    struct stat buffer;
    return (stat(path, &buffer) == 0);
}

// Test 1: Basic file creation and deletion
bool test_file_creation_deletion() {
    cleanup_test_files();
    
    SBT_file file;
    
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
    result = SBT_file::delete_file(TEST_FILE_PATH);
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
    
    SBT_file file;
    
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

// Test 3: Empty tree save and load
bool test_empty_tree_save_load() {
    cleanup_test_files();
    
    SBT_file file;
    SBT_tree tree1, tree2;
    
    // Create file and save empty tree
    int result = file.create(TEST_FILE_PATH);
    if (result != SBT_SUCCESS) {
        return false;
    }
    
    result = file.save_tree(&tree1);
    if (result != SBT_SUCCESS) {
        std::cerr << "Failed to save empty tree: " << result << std::endl;
        file.close();
        cleanup_test_files();
        return false;
    }
    
    file.close();
    
    // Open file and load tree
    result = file.open(TEST_FILE_PATH);
    if (result != SBT_SUCCESS) {
        cleanup_test_files();
        return false;
    }
    
    result = file.load_tree(&tree2);
    if (result != SBT_SUCCESS) {
        std::cerr << "Failed to load empty tree: " << result << std::endl;
        file.close();
        cleanup_test_files();
        return false;
    }
    
    // Verify tree is empty
    if (!tree2.is_empty()) {
        std::cerr << "Loaded tree should be empty" << std::endl;
        file.close();
        cleanup_test_files();
        return false;
    }
    
    if (tree2.get_record_count() != 0) {
        std::cerr << "Loaded tree should have 0 records" << std::endl;
        file.close();
        cleanup_test_files();
        return false;
    }
    
    file.close();
    cleanup_test_files();
    return true;
}

// Test 4: Tree with data save and load
bool test_tree_with_data_save_load() {
    cleanup_test_files();
    
    SBT_file file;
    SBT_tree tree1, tree2;
    
    // Insert test data into tree1
    const char* test_data[] = {
        "Record 1",
        "Record 2", 
        "Record 3",
        "Record 4",
        "Record 5"
    };
    
    for (int i = 0; i < 5; i++) {
        int result = tree1.insert((const uchar*)test_data[i], strlen(test_data[i]));
        if (result != SBT_SUCCESS) {
            std::cerr << "Failed to insert test data: " << result << std::endl;
            return false;
        }
    }
    
    // Create file and save tree
    int result = file.create(TEST_FILE_PATH);
    if (result != SBT_SUCCESS) {
        return false;
    }
    
    result = file.save_tree(&tree1);
    if (result != SBT_SUCCESS) {
        std::cerr << "Failed to save tree with data: " << result << std::endl;
        file.close();
        cleanup_test_files();
        return false;
    }
    
    file.close();
    
    // Open file and load tree
    result = file.open(TEST_FILE_PATH);
    if (result != SBT_SUCCESS) {
        cleanup_test_files();
        return false;
    }
    
    result = file.load_tree(&tree2);
    if (result != SBT_SUCCESS) {
        std::cerr << "Failed to load tree with data: " << result << std::endl;
        file.close();
        cleanup_test_files();
        return false;
    }
    
    // Verify tree data
    if (tree2.get_record_count() != 5) {
        std::cerr << "Loaded tree should have 5 records, got: " << tree2.get_record_count() << std::endl;
        file.close();
        cleanup_test_files();
        return false;
    }
    
    // Verify all records are present
    for (int i = 0; i < 5; i++) {
        SBT_node* node = tree2.find_by_data((const uchar*)test_data[i], strlen(test_data[i]));
        if (!node) {
            std::cerr << "Record not found after loading: " << test_data[i] << std::endl;
            file.close();
            cleanup_test_files();
            return false;
        }
    }
    
    file.close();
    cleanup_test_files();
    return true;
}

// Test 5: Large tree save and load
bool test_large_tree_save_load() {
    cleanup_test_files();
    
    SBT_file file;
    SBT_tree tree1, tree2;
    
    // Insert many records
    const int num_records = 100;
    std::vector<std::string> test_data;
    
    for (int i = 0; i < num_records; i++) {
        std::string record = "Large Record " + std::to_string(i) + " with some additional data to make it longer";
        test_data.push_back(record);
        
        int result = tree1.insert((const uchar*)record.c_str(), record.length());
        if (result != SBT_SUCCESS) {
            std::cerr << "Failed to insert large test data: " << result << std::endl;
            return false;
        }
    }
    
    // Save tree
    int result = file.create(TEST_FILE_PATH);
    if (result != SBT_SUCCESS) {
        return false;
    }
    
    result = file.save_tree(&tree1);
    if (result != SBT_SUCCESS) {
        std::cerr << "Failed to save large tree: " << result << std::endl;
        file.close();
        cleanup_test_files();
        return false;
    }
    
    file.close();
    
    // Load tree
    result = file.open(TEST_FILE_PATH);
    if (result != SBT_SUCCESS) {
        cleanup_test_files();
        return false;
    }
    
    result = file.load_tree(&tree2);
    if (result != SBT_SUCCESS) {
        std::cerr << "Failed to load large tree: " << result << std::endl;
        file.close();
        cleanup_test_files();
        return false;
    }
    
    // Verify record count
    if (tree2.get_record_count() != num_records) {
        std::cerr << "Large tree record count mismatch. Expected: " << num_records 
                  << ", Got: " << tree2.get_record_count() << std::endl;
        file.close();
        cleanup_test_files();
        return false;
    }
    
    // Verify some random records
    for (int i = 0; i < 10; i++) {
        int index = i * 10; // Check every 10th record
        if (index < num_records) {
            SBT_node* node = tree2.find_by_data((const uchar*)test_data[index].c_str(), 
                                                test_data[index].length());
            if (!node) {
                std::cerr << "Large tree record not found: " << test_data[index] << std::endl;
                file.close();
                cleanup_test_files();
                return false;
            }
        }
    }
    
    file.close();
    cleanup_test_files();
    return true;
}

// Test 6: File corruption detection
bool test_file_corruption_detection() {
    cleanup_test_files();
    
    SBT_file file;
    SBT_tree tree;
    
    // Create a valid file first
    int result = file.create(TEST_FILE_PATH);
    if (result != SBT_SUCCESS) {
        return false;
    }
    
    // Insert some data
    const char* test_data = "Test data for corruption";
    tree.insert((const uchar*)test_data, strlen(test_data));
    
    result = file.save_tree(&tree);
    if (result != SBT_SUCCESS) {
        file.close();
        cleanup_test_files();
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

// Test 7: Multiple file operations
bool test_multiple_file_operations() {
    cleanup_test_files();
    
    SBT_file file1, file2;
    SBT_tree tree1, tree2, tree3, tree4;
    
    // Create two different files with different data
    const char* data1[] = {"File1 Record1", "File1 Record2", "File1 Record3"};
    const char* data2[] = {"File2 RecordA", "File2 RecordB"};
    
    // Setup tree1
    for (int i = 0; i < 3; i++) {
        tree1.insert((const uchar*)data1[i], strlen(data1[i]));
    }
    
    // Setup tree2
    for (int i = 0; i < 2; i++) {
        tree2.insert((const uchar*)data2[i], strlen(data2[i]));
    }
    
    // Save to different files
    int result = file1.create(TEST_FILE_PATH);
    if (result != SBT_SUCCESS) return false;
    
    result = file1.save_tree(&tree1);
    if (result != SBT_SUCCESS) {
        file1.close();
        cleanup_test_files();
        return false;
    }
    file1.close();
    
    result = file2.create(TEST_FILE_PATH_2);
    if (result != SBT_SUCCESS) {
        cleanup_test_files();
        return false;
    }
    
    result = file2.save_tree(&tree2);
    if (result != SBT_SUCCESS) {
        file2.close();
        cleanup_test_files();
        return false;
    }
    file2.close();
    
    // Load from different files
    result = file1.open(TEST_FILE_PATH);
    if (result != SBT_SUCCESS) {
        cleanup_test_files();
        return false;
    }
    
    result = file1.load_tree(&tree3);
    if (result != SBT_SUCCESS) {
        file1.close();
        cleanup_test_files();
        return false;
    }
    file1.close();
    
    result = file2.open(TEST_FILE_PATH_2);
    if (result != SBT_SUCCESS) {
        cleanup_test_files();
        return false;
    }
    
    result = file2.load_tree(&tree4);
    if (result != SBT_SUCCESS) {
        file2.close();
        cleanup_test_files();
        return false;
    }
    file2.close();
    
    // Verify data integrity
    if (tree3.get_record_count() != 3 || tree4.get_record_count() != 2) {
        std::cerr << "Record count mismatch in multiple files test" << std::endl;
        cleanup_test_files();
        return false;
    }
    
    // Verify specific records
    for (int i = 0; i < 3; i++) {
        if (!tree3.find_by_data((const uchar*)data1[i], strlen(data1[i]))) {
            std::cerr << "File1 record not found: " << data1[i] << std::endl;
            cleanup_test_files();
            return false;
        }
    }
    
    for (int i = 0; i < 2; i++) {
        if (!tree4.find_by_data((const uchar*)data2[i], strlen(data2[i]))) {
            std::cerr << "File2 record not found: " << data2[i] << std::endl;
            cleanup_test_files();
            return false;
        }
    }
    
    cleanup_test_files();
    return true;
}

// Test 8: Error handling edge cases
bool test_error_handling() {
    cleanup_test_files();
    
    SBT_file file;
    SBT_tree tree;
    
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
    
    result = file.save_tree(nullptr);
    if (result == SBT_SUCCESS) {
        std::cerr << "Should have failed with null tree" << std::endl;
        return false;
    }
    
    result = file.load_tree(nullptr);
    if (result == SBT_SUCCESS) {
        std::cerr << "Should have failed with null tree" << std::endl;
        return false;
    }
    
    // Test operations on closed file
    result = file.save_tree(&tree);
    if (result == SBT_SUCCESS) {
        std::cerr << "Should have failed on closed file" << std::endl;
        return false;
    }
    
    result = file.load_tree(&tree);
    if (result == SBT_SUCCESS) {
        std::cerr << "Should have failed on closed file" << std::endl;
        return false;
    }
    
    return true;
}

// Test 9: File size and metadata verification
bool test_file_metadata() {
    cleanup_test_files();
    
    SBT_file file;
    SBT_tree tree;
    
    // Create file and add some data
    int result = file.create(TEST_FILE_PATH);
    if (result != SBT_SUCCESS) {
        return false;
    }
    
    // Check initial file size
    my_off_t initial_size = file.get_file_size();
    if (initial_size <= 0) {
        std::cerr << "Invalid initial file size: " << initial_size << std::endl;
        file.close();
        cleanup_test_files();
        return false;
    }
    
    // Add data and save
    const char* test_data = "Metadata test record";
    tree.insert((const uchar*)test_data, strlen(test_data));
    
    result = file.save_tree(&tree);
    if (result != SBT_SUCCESS) {
        file.close();
        cleanup_test_files();
        return false;
    }
    
    // Check file size increased
    my_off_t new_size = file.get_file_size();
    if (new_size <= initial_size) {
        std::cerr << "File size should have increased. Initial: " << initial_size 
                  << ", New: " << new_size << std::endl;
        file.close();
        cleanup_test_files();
        return false;
    }
    
    file.close();
    
    // Test file_exists utility
    if (!SBT_file::file_exists(TEST_FILE_PATH)) {
        std::cerr << "file_exists should return true for existing file" << std::endl;
        cleanup_test_files();
        return false;
    }
    
    if (SBT_file::file_exists("/tmp/non_existent_file.sbt")) {
        std::cerr << "file_exists should return false for non-existent file" << std::endl;
        cleanup_test_files();
        return false;
    }
    
    cleanup_test_files();
    return true;
}

// Main test runner
int main() {
    std::cout << "=== SBT File Operations Interface Test ===" << std::endl;
    std::cout << "Testing comprehensive file operations functionality..." << std::endl << std::endl;
    
    FileOperationsTest test_runner;
    
    // Run all tests
    test_runner.run_test("File Creation and Deletion", test_file_creation_deletion);
    test_runner.run_test("File Opening and Validation", test_file_opening_validation);
    test_runner.run_test("Empty Tree Save and Load", test_empty_tree_save_load);
    test_runner.run_test("Tree with Data Save and Load", test_tree_with_data_save_load);
    test_runner.run_test("Large Tree Save and Load", test_large_tree_save_load);
    test_runner.run_test("File Corruption Detection", test_file_corruption_detection);
    test_runner.run_test("Multiple File Operations", test_multiple_file_operations);
    test_runner.run_test("Error Handling Edge Cases", test_error_handling);
    test_runner.run_test("File Metadata Verification", test_file_metadata);
    
    // Print summary
    test_runner.print_summary();
    
    // Clean up any remaining test files
    cleanup_test_files();
    
    return test_runner.all_passed() ? 0 : 1;
}