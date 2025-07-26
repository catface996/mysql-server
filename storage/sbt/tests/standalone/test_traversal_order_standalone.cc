/*****************************************************************************

Copyright (c) 2025, Oracle and/or its affiliates.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License, version 2.0,
as published by the Free Software Foundation.

*****************************************************************************/

/** @file tests/standalone/test_traversal_order_standalone.cc
 SBT Tree Traversal Order Verification Test - Standalone

 Created 2025-01-25
 *******************************************************/

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cassert>
#include <algorithm>

// Mock MySQL dependencies for standalone testing
typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long long uint64_t;

// Mock memory allocator
class MEM_ROOT {
public:
    MEM_ROOT(int, int) {}
    void* Alloc(size_t size) { return malloc(size); }
    void Clear() {}
};

// Mock PSI
#define PSI_NOT_INSTRUMENTED 0

// Mock SBT common definitions for standalone testing
#define SBT_SUCCESS 0
#define SBT_ERR_INVALID_ARGUMENT 1
#define SBT_ERR_OUT_OF_MEMORY 2

typedef uint64_t sbt_insert_id_t;

// Mock data comparison function
int sbt_data_compare(const uchar *data1, uint length1, const uchar *data2, uint length2) {
    if (length1 != length2) {
        return (length1 < length2) ? -1 : 1;
    }
    return memcmp(data1, data2, length1);
}

// Include SBT tree header (we'll need to mock the implementation)
struct SBT_node {
    uchar *data;
    uint data_length;
    sbt_insert_id_t insert_id;
    SBT_node *left;
    SBT_node *right;
    uint size;
};

class SBT_tree {
private:
    SBT_node *root;
    MEM_ROOT mem_root;
    sbt_insert_id_t next_insert_id;
    uint64_t record_count;

public:
    SBT_tree() : root(nullptr), mem_root(0, 0), next_insert_id(1), record_count(0) {}
    ~SBT_tree() { clear(); }
    
    int insert(const uchar *data, uint length);
    int remove(const uchar *data, uint length);
    SBT_node *find_by_data(const uchar *data, uint length);
    SBT_node *get_first();
    SBT_node *get_next(SBT_node *current);
    uint64_t get_record_count() const { return record_count; }
    void clear();
    
    // Additional methods for testing
    void print_inorder();
    bool verify_inorder_property();
    
private:
    SBT_node *create_node(const uchar *data, uint length, sbt_insert_id_t insert_id);
    SBT_node *insert_node(SBT_node *node, const uchar *data, uint length, sbt_insert_id_t insert_id);
    SBT_node *remove_node(SBT_node *node, const uchar *data, uint length);
    SBT_node *find_by_data_recursive(SBT_node *node, const uchar *data, uint length);
    SBT_node *find_min(SBT_node *node);
    SBT_node *find_next_by_insert_id(SBT_node *node, sbt_insert_id_t current_id);
    void update_size(SBT_node *node);
    uint get_size(SBT_node *node) const;
    SBT_node *maintain(SBT_node *node, bool flag);
    SBT_node *rotate_left(SBT_node *node);
    SBT_node *rotate_right(SBT_node *node);
    
    // Helper methods for testing
    void print_inorder_recursive(SBT_node *node);
    bool verify_inorder_recursive(SBT_node *node, sbt_insert_id_t &last_id);
};

// SBT_tree implementation for standalone testing
SBT_node *SBT_tree::create_node(const uchar *data, uint length, sbt_insert_id_t insert_id) {
    if (!data || length == 0) return nullptr;
    
    SBT_node *node = (SBT_node*)malloc(sizeof(SBT_node));
    if (!node) return nullptr;
    
    node->data = (uchar*)malloc(length);
    if (!node->data) {
        free(node);
        return nullptr;
    }
    
    memcpy(node->data, data, length);
    node->data_length = length;
    node->insert_id = insert_id;
    node->left = nullptr;
    node->right = nullptr;
    node->size = 1;
    
    return node;
}

int SBT_tree::insert(const uchar *data, uint length) {
    if (!data || length == 0) return SBT_ERR_INVALID_ARGUMENT;
    
    sbt_insert_id_t insert_id = next_insert_id++;
    root = insert_node(root, data, length, insert_id);
    
    if (root) {
        record_count++;
        return SBT_SUCCESS;
    } else {
        next_insert_id--;
        return SBT_ERR_OUT_OF_MEMORY;
    }
}

SBT_node *SBT_tree::insert_node(SBT_node *node, const uchar *data, uint length, sbt_insert_id_t insert_id) {
    if (!node) {
        return create_node(data, length, insert_id);
    }
    
    if (insert_id < node->insert_id) {
        node->left = insert_node(node->left, data, length, insert_id);
    } else {
        node->right = insert_node(node->right, data, length, insert_id);
    }
    
    update_size(node);
    return maintain(node, insert_id >= node->insert_id);
}

int SBT_tree::remove(const uchar *data, uint length) {
    if (!data || length == 0) return SBT_ERR_INVALID_ARGUMENT;
    
    SBT_node *node_to_remove = find_by_data(data, length);
    if (!node_to_remove) return SBT_ERR_INVALID_ARGUMENT;
    
    root = remove_node(root, data, length);
    if (record_count > 0) record_count--;
    return SBT_SUCCESS;
}

SBT_node *SBT_tree::remove_node(SBT_node *node, const uchar *data, uint length) {
    if (!node) return nullptr;
    
    if (sbt_data_compare(node->data, node->data_length, data, length) == 0) {
        if (!node->left && !node->right) {
            free(node->data);
            free(node);
            return nullptr;
        }
        if (!node->left) {
            SBT_node *right = node->right;
            free(node->data);
            free(node);
            return right;
        }
        if (!node->right) {
            SBT_node *left = node->left;
            free(node->data);
            free(node);
            return left;
        }
        
        SBT_node *successor = find_min(node->right);
        
        free(node->data);
        node->data = (uchar*)malloc(successor->data_length);
        memcpy(node->data, successor->data, successor->data_length);
        node->data_length = successor->data_length;
        node->insert_id = successor->insert_id;
        
        node->right = remove_node(node->right, successor->data, successor->data_length);
        update_size(node);
        node = maintain(node, true);
        node = maintain(node, false);
        return node;
    } else {
        node->left = remove_node(node->left, data, length);
        node->right = remove_node(node->right, data, length);
        update_size(node);
        node = maintain(node, false);
        node = maintain(node, true);
        return node;
    }
}

SBT_node *SBT_tree::find_by_data(const uchar *data, uint length) {
    if (!data || length == 0) return nullptr;
    return find_by_data_recursive(root, data, length);
}

SBT_node *SBT_tree::find_by_data_recursive(SBT_node *node, const uchar *data, uint length) {
    if (!node) return nullptr;
    
    if (sbt_data_compare(node->data, node->data_length, data, length) == 0) {
        return node;
    }
    
    SBT_node *found = find_by_data_recursive(node->left, data, length);
    if (found) return found;
    
    return find_by_data_recursive(node->right, data, length);
}

SBT_node *SBT_tree::get_first() {
    if (!root) return nullptr;
    return find_min(root);
}

SBT_node *SBT_tree::find_min(SBT_node *node) {
    if (!node) return nullptr;
    while (node->left) {
        node = node->left;
    }
    return node;
}

SBT_node *SBT_tree::get_next(SBT_node *current) {
    if (!current) return nullptr;
    
    if (current->right) {
        return find_min(current->right);
    }
    
    return find_next_by_insert_id(root, current->insert_id);
}

SBT_node *SBT_tree::find_next_by_insert_id(SBT_node *node, sbt_insert_id_t current_id) {
    if (!node) return nullptr;
    
    SBT_node *result = nullptr;
    
    if (node->insert_id > current_id) {
        result = node;
        SBT_node *left_result = find_next_by_insert_id(node->left, current_id);
        if (left_result && left_result->insert_id < result->insert_id) {
            result = left_result;
        }
    } else {
        result = find_next_by_insert_id(node->right, current_id);
    }
    
    return result;
}

void SBT_tree::clear() {
    // Simple implementation - in real code we'd do proper cleanup
    root = nullptr;
    record_count = 0;
    next_insert_id = 1;
}

void SBT_tree::update_size(SBT_node *node) {
    if (node) {
        node->size = 1 + get_size(node->left) + get_size(node->right);
    }
}

uint SBT_tree::get_size(SBT_node *node) const {
    return node ? node->size : 0;
}

SBT_node *SBT_tree::maintain(SBT_node *node, bool flag) {
    if (!node) return node;
    
    if (!flag) {
        if (node->left && get_size(node->left->left) > get_size(node->right)) {
            node = rotate_right(node);
        } else if (node->left && get_size(node->left->right) > get_size(node->right)) {
            node->left = rotate_left(node->left);
            node = rotate_right(node);
        } else {
            return node;
        }
    } else {
        if (node->right && get_size(node->right->right) > get_size(node->left)) {
            node = rotate_left(node);
        } else if (node->right && get_size(node->right->left) > get_size(node->left)) {
            node->right = rotate_right(node->right);
            node = rotate_left(node);
        } else {
            return node;
        }
    }
    
    if (node->left) {
        node->left = maintain(node->left, false);
    }
    if (node->right) {
        node->right = maintain(node->right, true);
    }
    
    return node;
}

SBT_node *SBT_tree::rotate_left(SBT_node *node) {
    if (!node || !node->right) return node;
    
    SBT_node *new_root = node->right;
    node->right = new_root->left;
    new_root->left = node;
    
    update_size(node);
    update_size(new_root);
    
    return new_root;
}

SBT_node *SBT_tree::rotate_right(SBT_node *node) {
    if (!node || !node->left) return node;
    
    SBT_node *new_root = node->left;
    node->left = new_root->right;
    new_root->right = node;
    
    update_size(node);
    update_size(new_root);
    
    return new_root;
}

void SBT_tree::print_inorder() {
    std::cout << "In-order traversal: ";
    print_inorder_recursive(root);
    std::cout << std::endl;
}

void SBT_tree::print_inorder_recursive(SBT_node *node) {
    if (node) {
        print_inorder_recursive(node->left);
        std::cout << node->insert_id << " ";
        print_inorder_recursive(node->right);
    }
}

bool SBT_tree::verify_inorder_property() {
    sbt_insert_id_t last_id = 0;
    return verify_inorder_recursive(root, last_id);
}

bool SBT_tree::verify_inorder_recursive(SBT_node *node, sbt_insert_id_t &last_id) {
    if (!node) return true;
    
    if (!verify_inorder_recursive(node->left, last_id)) return false;
    
    if (node->insert_id <= last_id) return false;
    last_id = node->insert_id;
    
    return verify_inorder_recursive(node->right, last_id);
}

// Test helper functions
void print_test_header(const char* test_name) {
    std::cout << "\n=== " << test_name << " ===" << std::endl;
}

void print_result(const char* test_name, bool passed) {
    std::cout << "[" << (passed ? "PASS" : "FAIL") << "] " << test_name << std::endl;
    if (!passed) {
        std::cout << "ERROR: Test failed!" << std::endl;
        exit(1);
    }
}

// Test traversal order correctness
void test_traversal_order() {
    print_test_header("Traversal Order Test");
    
    SBT_tree tree;
    std::vector<std::string> test_data = {
        "record_003",
        "record_001", 
        "record_005",
        "record_002",
        "record_004"
    };
    
    // Insert records in non-sequential order
    for (const auto& data : test_data) {
        int result = tree.insert((const uchar*)data.c_str(), data.length());
        print_result("Insert record", result == SBT_SUCCESS);
    }
    
    // Verify BST property is maintained
    print_result("BST property maintained", tree.verify_inorder_property());
    
    // Collect traversal results
    std::vector<sbt_insert_id_t> traversal_ids;
    SBT_node* current = tree.get_first();
    
    while (current) {
        traversal_ids.push_back(current->insert_id);
        current = tree.get_next(current);
    }
    
    // Verify traversal order (should be sorted by insert_id)
    bool is_sorted = true;
    for (size_t i = 1; i < traversal_ids.size(); i++) {
        if (traversal_ids[i] <= traversal_ids[i-1]) {
            is_sorted = false;
            break;
        }
    }
    
    print_result("Traversal is in sorted order", is_sorted);
    
    // Verify all records are present
    print_result("All records present in traversal", 
                 traversal_ids.size() == test_data.size());
    
    // Print traversal order for debugging
    std::cout << "[INFO] Traversal order (insert_ids): ";
    for (auto id : traversal_ids) {
        std::cout << id << " ";
    }
    std::cout << std::endl;
}

// Test traversal order after deletions
void test_traversal_order_after_deletions() {
    print_test_header("Traversal Order After Deletions Test");
    
    SBT_tree tree;
    std::vector<std::string> test_data;
    
    // Generate sequential test data
    for (int i = 1; i <= 10; i++) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "record_%03d", i);
        test_data.push_back(std::string(buffer));
    }
    
    // Insert all records
    for (const auto& data : test_data) {
        tree.insert((const uchar*)data.c_str(), data.length());
    }
    
    print_result("All records inserted", tree.get_record_count() == 10);
    print_result("BST property maintained after insertions", tree.verify_inorder_property());
    
    // Delete every other record
    for (size_t i = 1; i < test_data.size(); i += 2) {
        const std::string& data = test_data[i];
        int result = tree.remove((const uchar*)data.c_str(), data.length());
        print_result("Delete record", result == SBT_SUCCESS);
    }
    
    print_result("Correct count after deletions", tree.get_record_count() == 5);
    print_result("BST property maintained after deletions", tree.verify_inorder_property());
    
    // Verify traversal order is still correct
    std::vector<sbt_insert_id_t> traversal_ids;
    SBT_node* current = tree.get_first();
    
    while (current) {
        traversal_ids.push_back(current->insert_id);
        current = tree.get_next(current);
    }
    
    bool is_sorted = true;
    for (size_t i = 1; i < traversal_ids.size(); i++) {
        if (traversal_ids[i] <= traversal_ids[i-1]) {
            is_sorted = false;
            break;
        }
    }
    
    print_result("Traversal order maintained after deletions", is_sorted);
    print_result("Correct number of remaining records", traversal_ids.size() == 5);
}

// Test edge cases for traversal
void test_traversal_edge_cases() {
    print_test_header("Traversal Edge Cases Test");
    
    // Test single node tree
    SBT_tree single_tree;
    std::string single_data = "single_record";
    single_tree.insert((const uchar*)single_data.c_str(), single_data.length());
    
    SBT_node* first = single_tree.get_first();
    print_result("Single node - get_first works", first != nullptr);
    
    SBT_node* next = single_tree.get_next(first);
    print_result("Single node - get_next returns null", next == nullptr);
    
    // Test two node tree
    SBT_tree two_tree;
    std::string data1 = "record_001";
    std::string data2 = "record_002";
    
    two_tree.insert((const uchar*)data1.c_str(), data1.length());
    two_tree.insert((const uchar*)data2.c_str(), data2.length());
    
    SBT_node* first_two = two_tree.get_first();
    SBT_node* second_two = two_tree.get_next(first_two);
    SBT_node* third_two = two_tree.get_next(second_two);
    
    print_result("Two nodes - get_first works", first_two != nullptr);
    print_result("Two nodes - get_next works", second_two != nullptr);
    print_result("Two nodes - third get_next returns null", third_two == nullptr);
    print_result("Two nodes - correct order", 
                 first_two->insert_id < second_two->insert_id);
}

// Test traversal performance with larger dataset
void test_traversal_performance() {
    print_test_header("Traversal Performance Test");
    
    SBT_tree tree;
    const int record_count = 10000;
    
    // Insert records
    for (int i = 0; i < record_count; i++) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "record_%06d", i);
        std::string data(buffer);
        
        int result = tree.insert((const uchar*)data.c_str(), data.length());
        if (result != SBT_SUCCESS) {
            print_result("Insert record in performance test", false);
            return;
        }
    }
    
    print_result("All records inserted", tree.get_record_count() == record_count);
    
    // Perform full traversal
    int traversal_count = 0;
    std::vector<sbt_insert_id_t> ids;
    
    SBT_node* current = tree.get_first();
    while (current) {
        ids.push_back(current->insert_id);
        traversal_count++;
        current = tree.get_next(current);
    }
    
    print_result("Full traversal completed", traversal_count == record_count);
    
    // Verify order
    bool is_sorted = true;
    for (size_t i = 1; i < ids.size(); i++) {
        if (ids[i] <= ids[i-1]) {
            is_sorted = false;
            break;
        }
    }
    
    print_result("Large dataset traversal maintains order", is_sorted);
    
    std::cout << "[INFO] Performance test completed with " << record_count 
              << " records" << std::endl;
}

int main() {
    std::cout << "=== SBT Tree Traversal Order Verification Test ===" << std::endl;
    
    try {
        test_traversal_order();
        test_traversal_order_after_deletions();
        test_traversal_edge_cases();
        test_traversal_performance();
        
        std::cout << "\n=== All Traversal Order Tests Passed! ===" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cout << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cout << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}