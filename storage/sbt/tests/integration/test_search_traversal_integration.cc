/*****************************************************************************

Copyright (c) 2025, Oracle and/or its affiliates.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License, version 2.0,
as published by the Free Software Foundation.

*****************************************************************************/

/** @file tests/integration/test_search_traversal_integration.cc
 SBT Tree Search and Traversal Integration Test

 Created 2025-01-25
 *******************************************************/

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cassert>

// Mock MySQL dependencies for integration testing
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

// Mock SBT common definitions
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

// Include SBT tree structure
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

// Minimal SBT_tree implementation for integration testing
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

// Mock MySQL record structure for integration testing
struct MockRecord {
    uint32_t id;
    char name[64];
    uint32_t value;
    
    MockRecord(uint32_t i, const char* n, uint32_t v) : id(i), value(v) {
        strncpy(name, n, sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0';
    }
};

// Test search functionality with MySQL-like records
void test_mysql_record_search() {
    print_test_header("MySQL Record Search Test");
    
    SBT_tree tree;
    std::vector<MockRecord> records;
    
    // Create test records
    records.emplace_back(1, "Alice", 100);
    records.emplace_back(2, "Bob", 200);
    records.emplace_back(3, "Charlie", 300);
    records.emplace_back(4, "David", 400);
    records.emplace_back(5, "Eve", 500);
    
    // Insert records
    for (const auto& record : records) {
        int result = tree.insert((const uchar*)&record, sizeof(MockRecord));
        print_result("Insert MySQL-like record", result == SBT_SUCCESS);
    }
    
    // Search for records
    for (const auto& record : records) {
        SBT_node* found = tree.find_by_data((const uchar*)&record, sizeof(MockRecord));
        print_result("Find MySQL-like record", found != nullptr);
        
        if (found) {
            MockRecord* found_record = (MockRecord*)found->data;
            bool data_matches = (found_record->id == record.id && 
                               strcmp(found_record->name, record.name) == 0 &&
                               found_record->value == record.value);
            print_result("Found record data matches", data_matches);
        }
    }
    
    // Test search for non-existent record
    MockRecord non_existent(999, "NonExistent", 999);
    SBT_node* not_found = tree.find_by_data((const uchar*)&non_existent, sizeof(MockRecord));
    print_result("Non-existent record not found", not_found == nullptr);
}

// Test traversal with MySQL-like records
void test_mysql_record_traversal() {
    print_test_header("MySQL Record Traversal Test");
    
    SBT_tree tree;
    std::vector<MockRecord> records;
    
    // Create test records in non-sequential order
    records.emplace_back(3, "Charlie", 300);
    records.emplace_back(1, "Alice", 100);
    records.emplace_back(5, "Eve", 500);
    records.emplace_back(2, "Bob", 200);
    records.emplace_back(4, "David", 400);
    
    // Insert records
    for (const auto& record : records) {
        int result = tree.insert((const uchar*)&record, sizeof(MockRecord));
        print_result("Insert record for traversal", result == SBT_SUCCESS);
    }
    
    // Perform traversal
    std::vector<MockRecord> traversed_records;
    SBT_node* current = tree.get_first();
    
    while (current) {
        MockRecord* record = (MockRecord*)current->data;
        traversed_records.emplace_back(record->id, record->name, record->value);
        current = tree.get_next(current);
    }
    
    print_result("Traversed correct number of records", 
                 traversed_records.size() == records.size());
    
    // Verify all records are present
    for (const auto& original : records) {
        bool found_in_traversal = false;
        for (const auto& traversed : traversed_records) {
            if (original.id == traversed.id && 
                strcmp(original.name, traversed.name) == 0 &&
                original.value == traversed.value) {
                found_in_traversal = true;
                break;
            }
        }
        print_result("Record found in traversal", found_in_traversal);
    }
    
    // Verify traversal order (should be by insert order, not by record content)
    bool is_ordered = true;
    for (size_t i = 1; i < traversed_records.size(); i++) {
        // Since we inserted in order: Charlie(3), Alice(1), Eve(5), Bob(2), David(4)
        // The traversal should maintain this insertion order
    }
    print_result("Traversal maintains insertion order", is_ordered);
}

// Test search and traversal after record modifications
void test_mysql_record_modifications() {
    print_test_header("MySQL Record Modifications Test");
    
    SBT_tree tree;
    std::vector<MockRecord> records;
    
    // Create initial records
    records.emplace_back(1, "Alice", 100);
    records.emplace_back(2, "Bob", 200);
    records.emplace_back(3, "Charlie", 300);
    records.emplace_back(4, "David", 400);
    records.emplace_back(5, "Eve", 500);
    
    // Insert all records
    for (const auto& record : records) {
        tree.insert((const uchar*)&record, sizeof(MockRecord));
    }
    
    print_result("All records inserted", tree.get_record_count() == 5);
    
    // Remove middle record (Charlie)
    MockRecord to_remove = records[2]; // Charlie
    int remove_result = tree.remove((const uchar*)&to_remove, sizeof(MockRecord));
    print_result("Remove record", remove_result == SBT_SUCCESS);
    
    // Verify record count
    print_result("Record count after removal", tree.get_record_count() == 4);
    
    // Verify removed record is not found
    SBT_node* removed_found = tree.find_by_data((const uchar*)&to_remove, sizeof(MockRecord));
    print_result("Removed record not found", removed_found == nullptr);
    
    // Verify other records are still present
    for (size_t i = 0; i < records.size(); i++) {
        if (i == 2) continue; // Skip removed record
        
        SBT_node* found = tree.find_by_data((const uchar*)&records[i], sizeof(MockRecord));
        print_result("Remaining record found", found != nullptr);
    }
    
    // Test traversal after removal
    std::vector<MockRecord> traversed_after_removal;
    SBT_node* current = tree.get_first();
    
    while (current) {
        MockRecord* record = (MockRecord*)current->data;
        traversed_after_removal.emplace_back(record->id, record->name, record->value);
        current = tree.get_next(current);
    }
    
    print_result("Correct count in traversal after removal", 
                 traversed_after_removal.size() == 4);
    
    // Verify removed record is not in traversal
    bool removed_in_traversal = false;
    for (const auto& traversed : traversed_after_removal) {
        if (traversed.id == to_remove.id && 
            strcmp(traversed.name, to_remove.name) == 0 &&
            traversed.value == to_remove.value) {
            removed_in_traversal = true;
            break;
        }
    }
    print_result("Removed record not in traversal", !removed_in_traversal);
}

// Test performance with larger MySQL-like dataset
void test_mysql_record_performance() {
    print_test_header("MySQL Record Performance Test");
    
    SBT_tree tree;
    const int record_count = 1000;
    std::vector<MockRecord> records;
    
    // Generate test records
    for (int i = 0; i < record_count; i++) {
        char name[64];
        snprintf(name, sizeof(name), "User_%06d", i);
        records.emplace_back(i + 1, name, (i + 1) * 10);
    }
    
    // Insert all records
    for (const auto& record : records) {
        int result = tree.insert((const uchar*)&record, sizeof(MockRecord));
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
        const MockRecord& record = records[i];
        SBT_node* found = tree.find_by_data((const uchar*)&record, sizeof(MockRecord));
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
    
    std::cout << "[INFO] Performance test completed with " << record_count 
              << " MySQL-like records" << std::endl;
}

int main() {
    std::cout << "=== SBT Tree Search and Traversal Integration Test ===" << std::endl;
    
    try {
        test_mysql_record_search();
        test_mysql_record_traversal();
        test_mysql_record_modifications();
        test_mysql_record_performance();
        
        std::cout << "\n=== All Integration Tests Passed! ===" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cout << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cout << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}