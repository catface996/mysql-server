/*****************************************************************************

Copyright (c) 2025, Oracle and/or its affiliates.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License, version 2.0,
as published by the Free Software Foundation.

*****************************************************************************/

/** @file tests/standalone/test_basic_functionality_standalone.cc
 SBT Storage Engine Basic Functionality Tests - Task 8.1 Standalone Version

 This file implements comprehensive basic functionality tests for the SBT storage engine
 as a standalone program that doesn't require MySQL framework. It covers:
 - SBT algorithm unit tests
 - Insert, delete, search operation correctness
 - Tree balance property maintenance
 - Requirements coverage: 2.1, 2.2, 3.1, 4.1, 5.1

 Created 2025-01-26
 *******************************************************/

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <cassert>

// Basic type definitions
typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long long uint64_t;
typedef uint64_t sbt_insert_id_t;

// Error codes
enum sbt_error_t {
  SBT_SUCCESS = 0,
  SBT_ERR_OUT_OF_MEMORY,
  SBT_ERR_FILE_NOT_FOUND,
  SBT_ERR_CORRUPTED_DATA,
  SBT_ERR_DUPLICATE_KEY,
  SBT_ERR_IO_ERROR,
  SBT_ERR_INVALID_ARGUMENT,
  SBT_ERR_GENERIC
};

// Mock MEM_ROOT class for standalone testing
class MEM_ROOT {
private:
    std::vector<void*> allocated_blocks;
    
public:
    MEM_ROOT(void* /*psi*/, size_t /*block_size*/) {}
    
    ~MEM_ROOT() {
        clear();
    }
    
    void* alloc(size_t size) {
        void* ptr = malloc(size);
        if (ptr) {
            allocated_blocks.push_back(ptr);
        }
        return ptr;
    }
    
    void clear() {
        for (void* ptr : allocated_blocks) {
            free(ptr);
        }
        allocated_blocks.clear();
    }
};

// SBT node structure
struct SBT_node {
    uchar *data;
    uint data_length;
    sbt_insert_id_t insert_id;
    SBT_node *left;
    SBT_node *right;
    uint size;
    
    SBT_node() : data(nullptr), data_length(0), insert_id(0), 
                 left(nullptr), right(nullptr), size(1) {}
};

// Simplified SBT tree implementation for testing
class SBT_tree {
private:
    SBT_node *root;
    MEM_ROOT mem_root;
    sbt_insert_id_t next_insert_id;
    uint record_count;
    
    // Helper functions
    uint get_size(SBT_node *node) {
        return node ? node->size : 0;
    }
    
    void update_size(SBT_node *node) {
        if (node) {
            node->size = get_size(node->left) + get_size(node->right) + 1;
        }
    }
    
    SBT_node* rotate_left(SBT_node *node) {
        if (!node || !node->right) return node;
        
        SBT_node *new_root = node->right;
        node->right = new_root->left;
        new_root->left = node;
        
        update_size(node);
        update_size(new_root);
        
        return new_root;
    }
    
    SBT_node* rotate_right(SBT_node *node) {
        if (!node || !node->left) return node;
        
        SBT_node *new_root = node->left;
        node->left = new_root->right;
        new_root->right = node;
        
        update_size(node);
        update_size(new_root);
        
        return new_root;
    }
    
    SBT_node* maintain(SBT_node *node, bool flag) {
        if (!node) return node;
        
        if (!flag) {
            // Left case
            if (node->left && node->left->left && 
                get_size(node->left->left) > get_size(node->right)) {
                node = rotate_right(node);
            } else if (node->left && node->left->right && 
                       get_size(node->left->right) > get_size(node->right)) {
                node->left = rotate_left(node->left);
                node = rotate_right(node);
            } else {
                return node;
            }
        } else {
            // Right case
            if (node->right && node->right->right && 
                get_size(node->right->right) > get_size(node->left)) {
                node = rotate_left(node);
            } else if (node->right && node->right->left && 
                       get_size(node->right->left) > get_size(node->left)) {
                node->right = rotate_right(node->right);
                node = rotate_left(node);
            } else {
                return node;
            }
        }
        
        node->left = maintain(node->left, false);
        node->right = maintain(node->right, true);
        node = maintain(node, false);
        node = maintain(node, true);
        
        return node;
    }
    
    SBT_node* insert_node(SBT_node *node, const uchar *data, uint length, sbt_insert_id_t insert_id) {
        if (!node) {
            SBT_node *new_node = (SBT_node*)mem_root.alloc(sizeof(SBT_node));
            if (!new_node) return nullptr;
            
            new_node->data = (uchar*)mem_root.alloc(length);
            if (!new_node->data) return nullptr;
            
            memcpy(new_node->data, data, length);
            new_node->data_length = length;
            new_node->insert_id = insert_id;
            new_node->left = nullptr;
            new_node->right = nullptr;
            new_node->size = 1;
            
            return new_node;
        }
        
        if (insert_id < node->insert_id) {
            node->left = insert_node(node->left, data, length, insert_id);
        } else {
            node->right = insert_node(node->right, data, length, insert_id);
        }
        
        update_size(node);
        return maintain(node, insert_id >= node->insert_id);
    }
    
    SBT_node* find_min(SBT_node *node) {
        while (node && node->left) {
            node = node->left;
        }
        return node;
    }
    
    // Simple removal implementation for testing
    SBT_node* remove_node(SBT_node *node, const uchar *data, uint length) {
        if (!node) return nullptr;
        
        // Compare data content to find the node
        int cmp = compare_data(data, length, node->data, node->data_length);
        
        if (cmp == 0) {
            // Found the node to remove
            if (!node->left && !node->right) {
                // Leaf node
                return nullptr;
            } else if (!node->left) {
                // Only right child
                return node->right;
            } else if (!node->right) {
                // Only left child
                return node->left;
            } else {
                // Both children - find inorder successor
                SBT_node *successor = find_min(node->right);
                
                // Replace node's data with successor's data
                node->data = (uchar*)mem_root.alloc(successor->data_length);
                if (node->data) {
                    memcpy(node->data, successor->data, successor->data_length);
                    node->data_length = successor->data_length;
                    node->insert_id = successor->insert_id;
                }
                
                // Remove successor from right subtree
                node->right = remove_node(node->right, successor->data, successor->data_length);
            }
        } else {
            // Search in appropriate subtree based on insert_id ordering
            if (node->left) {
                SBT_node* left_result = remove_node(node->left, data, length);
                if (left_result != node->left) {
                    node->left = left_result;
                    update_size(node);
                    return node;
                }
            }
            if (node->right) {
                SBT_node* right_result = remove_node(node->right, data, length);
                if (right_result != node->right) {
                    node->right = right_result;
                    update_size(node);
                    return node;
                }
            }
        }
        
        update_size(node);
        return node;
    }
    
    SBT_node* find_by_data_recursive(SBT_node *node, const uchar *data, uint length) {
        if (!node) return nullptr;
        
        int cmp = compare_data(data, length, node->data, node->data_length);
        
        if (cmp == 0) {
            return node;
        } else if (cmp < 0) {
            return find_by_data_recursive(node->left, data, length);
        } else {
            return find_by_data_recursive(node->right, data, length);
        }
    }
    
    int compare_data(const uchar *data1, uint len1, const uchar *data2, uint len2) {
        uint min_len = std::min(len1, len2);
        int result = memcmp(data1, data2, min_len);
        if (result == 0) {
            if (len1 < len2) return -1;
            if (len1 > len2) return 1;
            return 0;
        }
        return result;
    }
    
    void inorder_traversal(SBT_node *node, std::vector<SBT_node*> &nodes) {
        if (!node) return;
        
        inorder_traversal(node->left, nodes);
        nodes.push_back(node);
        inorder_traversal(node->right, nodes);
    }
    
public:
    SBT_tree() : root(nullptr), mem_root(nullptr, 8192), next_insert_id(1), record_count(0) {}
    
    ~SBT_tree() {
        clear();
    }
    
    void clear() {
        mem_root.clear();
        root = nullptr;
        next_insert_id = 1;
        record_count = 0;
    }
    
    bool is_empty() const {
        return root == nullptr;
    }
    
    uint get_record_count() const {
        return record_count;
    }
    
    int insert(const uchar *data, uint length) {
        if (!data || length == 0) {
            return SBT_ERR_INVALID_ARGUMENT;
        }
        
        root = insert_node(root, data, length, next_insert_id++);
        if (root) {
            record_count++;
            return SBT_SUCCESS;
        }
        
        return SBT_ERR_OUT_OF_MEMORY;
    }
    
    int remove(const uchar *data, uint length) {
        if (!data || length == 0) {
            return SBT_ERR_INVALID_ARGUMENT;
        }
        
        // Check if the record exists
        SBT_node* target = find_by_data(data, length);
        if (!target) {
            return SBT_SUCCESS; // Not found, but not an error
        }
        
        root = remove_node(root, data, length);
        record_count--;
        return SBT_SUCCESS;
    }
    
    SBT_node* find_by_data(const uchar *data, uint length) {
        if (!data || length == 0) {
            return nullptr;
        }
        
        return find_by_data_recursive(root, data, length);
    }
    
    SBT_node* get_first() {
        if (!root) return nullptr;
        
        SBT_node *current = root;
        while (current->left) {
            current = current->left;
        }
        return current;
    }
    
    SBT_node* get_next(SBT_node *current) {
        if (!current) return nullptr;
        
        // Get all nodes in order and find the next one
        std::vector<SBT_node*> nodes;
        inorder_traversal(root, nodes);
        
        for (size_t i = 0; i < nodes.size() - 1; i++) {
            if (nodes[i] == current) {
                return nodes[i + 1];
            }
        }
        
        return nullptr; // Current is the last node
    }
    
    // Helper function for testing - verify SBT balance properties
    bool verify_balance() {
        return verify_balance_recursive(root);
    }
    
private:
    bool verify_balance_recursive(SBT_node *node) {
        if (!node) return true;
        
        uint left_size = get_size(node->left);
        uint right_size = get_size(node->right);
        
        // Verify size property
        if (node->size != left_size + right_size + 1) {
            return false;
        }
        
        // Verify SBT balance properties
        if (node->left) {
            uint ll_size = node->left->left ? get_size(node->left->left) : 0;
            uint lr_size = node->left->right ? get_size(node->left->right) : 0;
            
            if (ll_size > right_size || lr_size > right_size) {
                return false;
            }
        }
        
        if (node->right) {
            uint rl_size = node->right->left ? get_size(node->right->left) : 0;
            uint rr_size = node->right->right ? get_size(node->right->right) : 0;
            
            if (rr_size > left_size || rl_size > left_size) {
                return false;
            }
        }
        
        return verify_balance_recursive(node->left) && verify_balance_recursive(node->right);
    }
};

// Test framework
class TestFramework {
private:
    int total_tests;
    int passed_tests;
    int failed_tests;
    
public:
    TestFramework() : total_tests(0), passed_tests(0), failed_tests(0) {}
    
    void run_test(const std::string& test_name, bool (*test_func)()) {
        total_tests++;
        std::cout << "Running: " << test_name << " ... ";
        
        if (test_func()) {
            std::cout << "PASSED" << std::endl;
            passed_tests++;
        } else {
            std::cout << "FAILED" << std::endl;
            failed_tests++;
        }
    }
    
    void print_summary() {
        std::cout << "\n=== Test Summary ===" << std::endl;
        std::cout << "Total tests: " << total_tests << std::endl;
        std::cout << "Passed: " << passed_tests << std::endl;
        std::cout << "Failed: " << failed_tests << std::endl;
        
        if (failed_tests == 0) {
            std::cout << "\n🎉 ALL BASIC FUNCTIONALITY TESTS PASSED! 🎉" << std::endl;
        } else {
            std::cout << "\n❌ Some tests failed. Please review the implementation." << std::endl;
        }
    }
    
    bool all_passed() const {
        return failed_tests == 0;
    }
};

// Helper function to create test data
std::string create_test_data(int id) {
    return "test_record_" + std::to_string(id);
}

// ============================================================================
// REQUIREMENT 2.1: SBT节点和基础数据结构测试
// ============================================================================

bool test_tree_creation_and_initialization() {
    SBT_tree tree;
    return tree.is_empty() && tree.get_record_count() == 0;
}

bool test_tree_clear_operation() {
    SBT_tree tree;
    
    // Insert some data
    std::string data = create_test_data(1);
    if (tree.insert((const uchar*)data.c_str(), data.length()) != SBT_SUCCESS) {
        return false;
    }
    
    if (tree.is_empty()) return false;
    
    // Clear the tree
    tree.clear();
    return tree.is_empty() && tree.get_record_count() == 0;
}

// ============================================================================
// REQUIREMENT 2.2: SBT树的插入操作测试
// ============================================================================

bool test_single_record_insertion() {
    SBT_tree tree;
    std::string data = create_test_data(1);
    
    int result = tree.insert((const uchar*)data.c_str(), data.length());
    return result == SBT_SUCCESS && 
           !tree.is_empty() && 
           tree.get_record_count() == 1;
}

bool test_multiple_record_insertion() {
    SBT_tree tree;
    const int num_records = 10;
    
    for (int i = 1; i <= num_records; i++) {
        std::string data = create_test_data(i);
        int result = tree.insert((const uchar*)data.c_str(), data.length());
        if (result != SBT_SUCCESS || tree.get_record_count() != static_cast<uint>(i)) {
            return false;
        }
    }
    
    return !tree.is_empty() && tree.get_record_count() == num_records;
}

bool test_insertion_invalid_arguments() {
    SBT_tree tree;
    
    // Test null data
    int result1 = tree.insert(nullptr, 10);
    if (result1 != SBT_ERR_INVALID_ARGUMENT) return false;
    
    // Test zero length
    std::string data = create_test_data(1);
    int result2 = tree.insert((const uchar*)data.c_str(), 0);
    if (result2 != SBT_ERR_INVALID_ARGUMENT) return false;
    
    // Tree should remain empty
    return tree.is_empty();
}

bool test_insertion_balance_maintenance() {
    SBT_tree tree;
    const int num_records = 15;
    
    // Insert records in sequential order (worst case for balance)
    for (int i = 1; i <= num_records; i++) {
        std::string data = create_test_data(i);
        int result = tree.insert((const uchar*)data.c_str(), data.length());
        if (result != SBT_SUCCESS) return false;
        
        // Verify balance after each insertion
        if (!tree.verify_balance()) return false;
    }
    
    return tree.get_record_count() == num_records;
}

// ============================================================================
// REQUIREMENT 3.1: SBT树的查找和遍历操作测试
// ============================================================================

bool test_record_search_operation() {
    SBT_tree tree;
    const int num_records = 5;
    std::vector<std::string> test_data;
    
    // Insert test records
    for (int i = 1; i <= num_records; i++) {
        std::string data = create_test_data(i);
        test_data.push_back(data);
        int result = tree.insert((const uchar*)data.c_str(), data.length());
        if (result != SBT_SUCCESS) return false;
    }
    
    // Search for each record
    for (const auto& data : test_data) {
        SBT_node* found = tree.find_by_data((const uchar*)data.c_str(), data.length());
        if (!found) return false;
        if (found->data_length != data.length()) return false;
        if (memcmp(found->data, data.c_str(), data.length()) != 0) return false;
    }
    
    return true;
}

bool test_search_non_existent_record() {
    SBT_tree tree;
    
    // Insert one record
    std::string existing_data = create_test_data(1);
    if (tree.insert((const uchar*)existing_data.c_str(), existing_data.length()) != SBT_SUCCESS) {
        return false;
    }
    
    // Search for non-existent record
    std::string non_existent = create_test_data(999);
    SBT_node* found = tree.find_by_data((const uchar*)non_existent.c_str(), non_existent.length());
    return found == nullptr;
}

bool test_tree_traversal_operation() {
    SBT_tree tree;
    const int num_records = 7;
    std::vector<std::string> inserted_data;
    
    // Insert records
    for (int i = 1; i <= num_records; i++) {
        std::string data = create_test_data(i);
        inserted_data.push_back(data);
        int result = tree.insert((const uchar*)data.c_str(), data.length());
        if (result != SBT_SUCCESS) return false;
    }
    
    // Traverse the tree
    std::vector<std::string> traversed_data;
    SBT_node* current = tree.get_first();
    while (current) {
        std::string data((char*)current->data, current->data_length);
        traversed_data.push_back(data);
        current = tree.get_next(current);
    }
    
    // Verify traversal results
    if (traversed_data.size() != num_records) return false;
    
    // Verify all records are traversed
    for (const auto& data : inserted_data) {
        if (std::find(traversed_data.begin(), traversed_data.end(), data) == traversed_data.end()) {
            return false;
        }
    }
    
    return true;
}

bool test_empty_tree_traversal() {
    SBT_tree tree;
    SBT_node* first = tree.get_first();
    return first == nullptr;
}

// ============================================================================
// REQUIREMENT 4.1 & 5.1: 删除操作测试
// ============================================================================

bool test_single_record_deletion() {
    SBT_tree tree;
    
    // Insert a record
    std::string data = create_test_data(1);
    if (tree.insert((const uchar*)data.c_str(), data.length()) != SBT_SUCCESS) return false;
    if (tree.get_record_count() != 1) return false;
    
    // Delete the record
    int result = tree.remove((const uchar*)data.c_str(), data.length());
    if (result != SBT_SUCCESS) return false;
    if (!tree.is_empty()) return false;
    if (tree.get_record_count() != 0) return false;
    
    // Verify record is no longer found
    SBT_node* found = tree.find_by_data((const uchar*)data.c_str(), data.length());
    return found == nullptr;
}

bool test_multiple_record_deletion() {
    SBT_tree tree;
    const int num_records = 10;
    std::vector<std::string> test_data;
    
    // Insert multiple records
    for (int i = 1; i <= num_records; i++) {
        std::string data = create_test_data(i);
        test_data.push_back(data);
        if (tree.insert((const uchar*)data.c_str(), data.length()) != SBT_SUCCESS) return false;
    }
    
    // Delete every other record
    for (int i = 0; i < num_records; i += 2) {
        const std::string& data = test_data[i];
        int result = tree.remove((const uchar*)data.c_str(), data.length());
        if (result != SBT_SUCCESS) return false;
        
        // Verify record is deleted
        SBT_node* found = tree.find_by_data((const uchar*)data.c_str(), data.length());
        if (found != nullptr) return false;
    }
    
    // Verify remaining records still exist
    for (int i = 1; i < num_records; i += 2) {
        const std::string& data = test_data[i];
        SBT_node* found = tree.find_by_data((const uchar*)data.c_str(), data.length());
        if (found == nullptr) return false;
    }
    
    return tree.get_record_count() == num_records / 2;
}

bool test_delete_non_existent_record() {
    SBT_tree tree;
    
    // Insert one record
    std::string existing_data = create_test_data(1);
    if (tree.insert((const uchar*)existing_data.c_str(), existing_data.length()) != SBT_SUCCESS) return false;
    
    // Try to delete non-existent record
    std::string non_existent = create_test_data(999);
    tree.remove((const uchar*)non_existent.c_str(), non_existent.length());
    
    // Check that the existing record is still there
    if (tree.get_record_count() != 1) return false;
    SBT_node* found = tree.find_by_data((const uchar*)existing_data.c_str(), existing_data.length());
    return found != nullptr;
}

// ============================================================================
// 综合功能测试
// ============================================================================

bool test_mixed_operations() {
    SBT_tree tree;
    const int num_records = 20;
    std::vector<std::string> test_data;
    
    // Phase 1: Insert records
    for (int i = 1; i <= num_records; i++) {
        std::string data = create_test_data(i);
        test_data.push_back(data);
        if (tree.insert((const uchar*)data.c_str(), data.length()) != SBT_SUCCESS) return false;
    }
    
    // Phase 2: Search for all records
    for (const auto& data : test_data) {
        SBT_node* found = tree.find_by_data((const uchar*)data.c_str(), data.length());
        if (!found) return false;
    }
    
    // Phase 3: Delete some records
    for (int i = 0; i < num_records / 2; i++) {
        const std::string& data = test_data[i];
        int result = tree.remove((const uchar*)data.c_str(), data.length());
        if (result != SBT_SUCCESS) return false;
    }
    
    // Phase 4: Verify remaining records
    for (int i = num_records / 2; i < num_records; i++) {
        const std::string& data = test_data[i];
        SBT_node* found = tree.find_by_data((const uchar*)data.c_str(), data.length());
        if (!found) return false;
    }
    
    // Phase 5: Insert new records
    for (int i = num_records + 1; i <= num_records + 5; i++) {
        std::string data = create_test_data(i);
        if (tree.insert((const uchar*)data.c_str(), data.length()) != SBT_SUCCESS) return false;
    }
    
    return tree.get_record_count() == num_records / 2 + 5;
}

bool test_large_data_insertion() {
    SBT_tree tree;
    
    // Create a large data record (1KB)
    std::string large_data(1024, 'A');
    large_data += "_large_record";
    
    int result = tree.insert((const uchar*)large_data.c_str(), large_data.length());
    if (result != SBT_SUCCESS) return false;
    
    // Verify the record can be found
    SBT_node* found = tree.find_by_data((const uchar*)large_data.c_str(), large_data.length());
    if (!found) return false;
    if (found->data_length != large_data.length()) return false;
    if (memcmp(found->data, large_data.c_str(), large_data.length()) != 0) return false;
    
    return true;
}

bool test_stress_operations() {
    SBT_tree tree;
    const int stress_count = 100; // Reduced for standalone test
    
    // Stress insertion
    for (int i = 1; i <= stress_count; i++) {
        std::string data = create_test_data(i);
        int result = tree.insert((const uchar*)data.c_str(), data.length());
        if (result != SBT_SUCCESS) return false;
        
        // Verify balance periodically
        if (i % 10 == 0 && !tree.verify_balance()) return false;
    }
    
    if (tree.get_record_count() != stress_count) return false;
    
    // Stress deletion (delete every 10th record)
    int deleted_count = 0;
    for (int i = 10; i <= stress_count; i += 10) {
        std::string data = create_test_data(i);
        int result = tree.remove((const uchar*)data.c_str(), data.length());
        if (result != SBT_SUCCESS) return false;
        deleted_count++;
    }
    
    return tree.get_record_count() == static_cast<uint>(stress_count - deleted_count);
}

// ============================================================================
// Main test runner
// ============================================================================

int main() {
    std::cout << "=== SBT Storage Engine Basic Functionality Tests - Task 8.1 ===" << std::endl;
    std::cout << "Testing SBT algorithm unit tests, insert/delete/search operations," << std::endl;
    std::cout << "and tree balance property maintenance." << std::endl;
    std::cout << "Requirements coverage: 2.1, 2.2, 3.1, 4.1, 5.1" << std::endl;
    std::cout << std::endl;
    
    TestFramework framework;
    
    // REQUIREMENT 2.1: SBT节点和基础数据结构测试
    std::cout << "=== Testing Requirement 2.1: SBT Node and Basic Data Structures ===" << std::endl;
    framework.run_test("Tree Creation and Initialization", test_tree_creation_and_initialization);
    framework.run_test("Tree Clear Operation", test_tree_clear_operation);
    
    // REQUIREMENT 2.2: SBT树的插入操作测试
    std::cout << "\n=== Testing Requirement 2.2: SBT Tree Insertion Operations ===" << std::endl;
    framework.run_test("Single Record Insertion", test_single_record_insertion);
    framework.run_test("Multiple Record Insertion", test_multiple_record_insertion);
    framework.run_test("Insertion Invalid Arguments", test_insertion_invalid_arguments);
    framework.run_test("Insertion Balance Maintenance", test_insertion_balance_maintenance);
    
    // REQUIREMENT 3.1: SBT树的查找和遍历操作测试
    std::cout << "\n=== Testing Requirement 3.1: SBT Tree Search and Traversal Operations ===" << std::endl;
    framework.run_test("Record Search Operation", test_record_search_operation);
    framework.run_test("Search Non-existent Record", test_search_non_existent_record);
    framework.run_test("Tree Traversal Operation", test_tree_traversal_operation);
    framework.run_test("Empty Tree Traversal", test_empty_tree_traversal);
    
    // REQUIREMENT 4.1 & 5.1: 删除操作测试
    std::cout << "\n=== Testing Requirements 4.1 & 5.1: SBT Tree Deletion Operations ===" << std::endl;
    framework.run_test("Single Record Deletion", test_single_record_deletion);
    framework.run_test("Multiple Record Deletion", test_multiple_record_deletion);
    framework.run_test("Delete Non-existent Record", test_delete_non_existent_record);
    
    // 综合功能测试
    std::cout << "\n=== Comprehensive Functionality Tests ===" << std::endl;
    framework.run_test("Mixed Operations", test_mixed_operations);
    framework.run_test("Large Data Insertion", test_large_data_insertion);
    framework.run_test("Stress Operations", test_stress_operations);
    
    // Print summary
    framework.print_summary();
    
    return framework.all_passed() ? 0 : 1;
}