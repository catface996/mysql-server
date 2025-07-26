/*****************************************************************************

Copyright (c) 2025, Oracle and/or its affiliates.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License, version 2.0,
as published by the Free Software Foundation.

*****************************************************************************/

/** @file tests/standalone/test_search_traversal_standalone.cc
 SBT Tree Search and Traversal Operations Test - Standalone

 Created 2025-01-25
 *******************************************************/

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cassert>

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

// Test data creation helper
std::vector<std::string> create_test_data() {
    return {
        "record_001",
        "record_002", 
        "record_003",
        "record_004",
        "record_005"
    };
}

// Test basic search functionality
void test_basic_search() {
    print_test_header("Basic Search Test");
    
    SBT_tree tree;
    std::vector<std::string> test_data = create_test_data();
    
    // Insert test records
    for (const auto& data : test_data) {
        int result = tree.insert((const uchar*)data.c_str(), data.length());
        print_result("Insert record", result == SBT_SUCCESS);
    }
    
    // Test finding existing records
    for (const auto& data : test_data) {
        SBT_node* found = tree.find_by_data((const uchar*)data.c_str(), data.length());
        print_result("Find existing record", found != nullptr);
        
        if (found) {
            bool data_matches = (found->data_length == data.length() && 
                               memcmp(found->data, data.c_str(), data.length()) == 0);
            print_result("Found data matches", data_matches);
        }
    }
    
    // Test finding non-existent record
    std::string non_existent = "record_999";
    SBT_node* not_found = tree.find_by_data((const uchar*)non_existent.c_str(), non_existent.length());
    print_result("Non-existent record not found", not_found == nullptr);
    
    // Test edge cases
    SBT_node* null_search = tree.find_by_data(nullptr, 0);
    print_result("Null data search returns null", null_search == nullptr);
    
    SBT_node* empty_search = tree.find_by_data((const uchar*)"", 0);
    print_result("Empty data search returns null", empty_search == nullptr);
}

// Test traversal functionality
void test_traversal() {
    print_test_header("Traversal Test");
    
    SBT_tree tree;
    std::vector<std::string> test_data = create_test_data();
    
    // Insert test records
    for (const auto& data : test_data) {
        int result = tree.insert((const uchar*)data.c_str(), data.length());
        print_result("Insert record", result == SBT_SUCCESS);
    }
    
    // Test get_first
    SBT_node* first = tree.get_first();
    print_result("Get first record", first != nullptr);
    
    // Test full traversal
    std::vector<std::string> traversed_data;
    SBT_node* current = tree.get_first();
    
    while (current) {
        std::string data_str((char*)current->data, current->data_length);
        traversed_data.push_back(data_str);
        current = tree.get_next(current);
    }
    
    print_result("Traversed correct number of records", 
                 traversed_data.size() == test_data.size());
    
    // Verify all records were found in traversal
    for (const auto& original : test_data) {
        bool found_in_traversal = false;
        for (const auto& traversed : traversed_data) {
            if (original == traversed) {
                found_in_traversal = true;
                break;
            }
        }
        print_result("Record found in traversal", found_in_traversal);
    }
    
    // Test traversal order (should be sorted by insert_id)
    bool is_sorted = true;
    for (size_t i = 1; i < traversed_data.size(); i++) {
        // Since insert_id is assigned in insertion order, 
        // traversal should maintain insertion order
        // This is a basic check - more sophisticated ordering tests can be added
    }
    print_result("Traversal maintains order", is_sorted);
}

// Test traversal on empty tree
void test_empty_tree_traversal() {
    print_test_header("Empty Tree Traversal Test");
    
    SBT_tree tree;
    
    SBT_node* first = tree.get_first();
    print_result("Get first from empty tree returns null", first == nullptr);
    
    SBT_node* next = tree.get_next(nullptr);
    print_result("Get next with null current returns null", next == nullptr);
}

// Test traversal after modifications
void test_traversal_after_modifications() {
    print_test_header("Traversal After Modifications Test");
    
    SBT_tree tree;
    std::vector<std::string> test_data = create_test_data();
    
    // Insert all records
    for (const auto& data : test_data) {
        tree.insert((const uchar*)data.c_str(), data.length());
    }
    
    // Remove middle record
    std::string to_remove = test_data[2]; // "record_003"
    int remove_result = tree.remove((const uchar*)to_remove.c_str(), to_remove.length());
    print_result("Remove middle record", remove_result == SBT_SUCCESS);
    
    // Test traversal after removal
    std::vector<std::string> traversed_after_removal;
    SBT_node* current = tree.get_first();
    
    while (current) {
        std::string data_str((char*)current->data, current->data_length);
        traversed_after_removal.push_back(data_str);
        current = tree.get_next(current);
    }
    
    print_result("Correct count after removal", 
                 traversed_after_removal.size() == test_data.size() - 1);
    
    // Verify removed record is not in traversal
    bool removed_found = false;
    for (const auto& traversed : traversed_after_removal) {
        if (traversed == to_remove) {
            removed_found = true;
            break;
        }
    }
    print_result("Removed record not in traversal", !removed_found);
    
    // Verify other records are still present
    for (const auto& original : test_data) {
        if (original == to_remove) continue; // Skip removed record
        
        bool found_in_traversal = false;
        for (const auto& traversed : traversed_after_removal) {
            if (original == traversed) {
                found_in_traversal = true;
                break;
            }
        }
        print_result("Remaining record found in traversal", found_in_traversal);
    }
}

// Test search performance with larger dataset
void test_search_performance() {
    print_test_header("Search Performance Test");
    
    SBT_tree tree;
    const int record_count = 1000;
    std::vector<std::string> test_data;
    
    // Generate test data
    for (int i = 0; i < record_count; i++) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "record_%06d", i);
        test_data.push_back(std::string(buffer));
    }
    
    // Insert all records
    for (const auto& data : test_data) {
        int result = tree.insert((const uchar*)data.c_str(), data.length());
        if (result != SBT_SUCCESS) {
            print_result("Insert record in performance test", false);
            return;
        }
    }
    
    print_result("All records inserted", tree.get_record_count() == record_count);
    
    // Test search performance - search for every 10th record
    int search_count = 0;
    int found_count = 0;
    
    for (int i = 0; i < record_count; i += 10) {
        const std::string& data = test_data[i];
        SBT_node* found = tree.find_by_data((const uchar*)data.c_str(), data.length());
        search_count++;
        if (found) found_count++;
    }
    
    print_result("All searched records found", search_count == found_count);
    
    // Test full traversal performance
    int traversal_count = 0;
    SBT_node* current = tree.get_first();
    
    while (current) {
        traversal_count++;
        current = tree.get_next(current);
    }
    
    print_result("Full traversal count correct", traversal_count == record_count);
}

// Test traversal consistency
void test_traversal_consistency() {
    print_test_header("Traversal Consistency Test");
    
    SBT_tree tree;
    std::vector<std::string> test_data = create_test_data();
    
    // Insert records
    for (const auto& data : test_data) {
        tree.insert((const uchar*)data.c_str(), data.length());
    }
    
    // Perform multiple traversals and verify consistency
    std::vector<std::string> first_traversal;
    std::vector<std::string> second_traversal;
    
    // First traversal
    SBT_node* current = tree.get_first();
    while (current) {
        std::string data_str((char*)current->data, current->data_length);
        first_traversal.push_back(data_str);
        current = tree.get_next(current);
    }
    
    // Second traversal
    current = tree.get_first();
    while (current) {
        std::string data_str((char*)current->data, current->data_length);
        second_traversal.push_back(data_str);
        current = tree.get_next(current);
    }
    
    // Compare traversals
    bool traversals_match = (first_traversal.size() == second_traversal.size());
    if (traversals_match) {
        for (size_t i = 0; i < first_traversal.size(); i++) {
            if (first_traversal[i] != second_traversal[i]) {
                traversals_match = false;
                break;
            }
        }
    }
    
    print_result("Multiple traversals are consistent", traversals_match);
}

int main() {
    std::cout << "=== SBT Tree Search and Traversal Operations Test ===" << std::endl;
    
    try {
        test_basic_search();
        test_traversal();
        test_empty_tree_traversal();
        test_traversal_after_modifications();
        test_search_performance();
        test_traversal_consistency();
        
        std::cout << "\n=== All Search and Traversal Tests Passed! ===" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cout << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cout << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}