/*****************************************************************************

Copyright (c) 2025, Oracle and/or its affiliates.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License, version 2.0,
as published by the Free Software Foundation.

*****************************************************************************/

/** @file tests/gtest/sbt_search_traversal_test.cc
 SBT Tree Search and Traversal Google Test Unit Tests

 Created 2025-01-25
 *******************************************************/

#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <cstring>

// Mock MySQL dependencies for Google Test
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

// Minimal SBT_tree implementation for Google Test
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

// Google Test fixture for SBT search and traversal tests
class SBTSearchTraversalTest : public ::testing::Test {
protected:
    void SetUp() override {
        tree = new SBT_tree();
    }
    
    void TearDown() override {
        delete tree;
    }
    
    SBT_tree* tree;
};

// Test basic search functionality
TEST_F(SBTSearchTraversalTest, BasicSearch) {
    std::vector<std::string> test_data = {
        "record_001", "record_002", "record_003", "record_004", "record_005"
    };
    
    // Insert test records
    for (const auto& data : test_data) {
        EXPECT_EQ(SBT_SUCCESS, tree->insert((const uchar*)data.c_str(), data.length()));
    }
    
    // Test finding existing records
    for (const auto& data : test_data) {
        SBT_node* found = tree->find_by_data((const uchar*)data.c_str(), data.length());
        EXPECT_NE(nullptr, found);
        
        if (found) {
            EXPECT_EQ(data.length(), found->data_length);
            EXPECT_EQ(0, memcmp(found->data, data.c_str(), data.length()));
        }
    }
    
    // Test finding non-existent record
    std::string non_existent = "record_999";
    SBT_node* not_found = tree->find_by_data((const uchar*)non_existent.c_str(), non_existent.length());
    EXPECT_EQ(nullptr, not_found);
}

// Test search edge cases
TEST_F(SBTSearchTraversalTest, SearchEdgeCases) {
    // Test null data search
    EXPECT_EQ(nullptr, tree->find_by_data(nullptr, 0));
    
    // Test empty data search
    EXPECT_EQ(nullptr, tree->find_by_data((const uchar*)"", 0));
    
    // Test search in empty tree
    std::string test_data = "test_record";
    EXPECT_EQ(nullptr, tree->find_by_data((const uchar*)test_data.c_str(), test_data.length()));
}

// Test basic traversal functionality
TEST_F(SBTSearchTraversalTest, BasicTraversal) {
    std::vector<std::string> test_data = {
        "record_001", "record_002", "record_003", "record_004", "record_005"
    };
    
    // Insert test records
    for (const auto& data : test_data) {
        EXPECT_EQ(SBT_SUCCESS, tree->insert((const uchar*)data.c_str(), data.length()));
    }
    
    // Test get_first
    SBT_node* first = tree->get_first();
    EXPECT_NE(nullptr, first);
    
    // Test full traversal
    std::vector<std::string> traversed_data;
    SBT_node* current = tree->get_first();
    
    while (current) {
        std::string data_str((char*)current->data, current->data_length);
        traversed_data.push_back(data_str);
        current = tree->get_next(current);
    }
    
    EXPECT_EQ(test_data.size(), traversed_data.size());
    
    // Verify all records were found in traversal
    for (const auto& original : test_data) {
        bool found_in_traversal = false;
        for (const auto& traversed : traversed_data) {
            if (original == traversed) {
                found_in_traversal = true;
                break;
            }
        }
        EXPECT_TRUE(found_in_traversal);
    }
}

// Test traversal on empty tree
TEST_F(SBTSearchTraversalTest, EmptyTreeTraversal) {
    EXPECT_EQ(nullptr, tree->get_first());
    EXPECT_EQ(nullptr, tree->get_next(nullptr));
}

// Test traversal after modifications
TEST_F(SBTSearchTraversalTest, TraversalAfterModifications) {
    std::vector<std::string> test_data = {
        "record_001", "record_002", "record_003", "record_004", "record_005"
    };
    
    // Insert all records
    for (const auto& data : test_data) {
        EXPECT_EQ(SBT_SUCCESS, tree->insert((const uchar*)data.c_str(), data.length()));
    }
    
    // Remove middle record
    std::string to_remove = test_data[2]; // "record_003"
    EXPECT_EQ(SBT_SUCCESS, tree->remove((const uchar*)to_remove.c_str(), to_remove.length()));
    
    // Test traversal after removal
    std::vector<std::string> traversed_after_removal;
    SBT_node* current = tree->get_first();
    
    while (current) {
        std::string data_str((char*)current->data, current->data_length);
        traversed_after_removal.push_back(data_str);
        current = tree->get_next(current);
    }
    
    EXPECT_EQ(test_data.size() - 1, traversed_after_removal.size());
    
    // Verify removed record is not in traversal
    bool removed_found = false;
    for (const auto& traversed : traversed_after_removal) {
        if (traversed == to_remove) {
            removed_found = true;
            break;
        }
    }
    EXPECT_FALSE(removed_found);
    
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
        EXPECT_TRUE(found_in_traversal);
    }
}

// Test traversal order consistency
TEST_F(SBTSearchTraversalTest, TraversalOrderConsistency) {
    std::vector<std::string> test_data = {
        "record_003", "record_001", "record_005", "record_002", "record_004"
    };
    
    // Insert records in non-sequential order
    for (const auto& data : test_data) {
        EXPECT_EQ(SBT_SUCCESS, tree->insert((const uchar*)data.c_str(), data.length()));
    }
    
    // Perform multiple traversals and verify consistency
    std::vector<std::string> first_traversal;
    std::vector<std::string> second_traversal;
    
    // First traversal
    SBT_node* current = tree->get_first();
    while (current) {
        std::string data_str((char*)current->data, current->data_length);
        first_traversal.push_back(data_str);
        current = tree->get_next(current);
    }
    
    // Second traversal
    current = tree->get_first();
    while (current) {
        std::string data_str((char*)current->data, current->data_length);
        second_traversal.push_back(data_str);
        current = tree->get_next(current);
    }
    
    // Compare traversals
    EXPECT_EQ(first_traversal.size(), second_traversal.size());
    for (size_t i = 0; i < first_traversal.size(); i++) {
        EXPECT_EQ(first_traversal[i], second_traversal[i]);
    }
}

// Test search and traversal performance
TEST_F(SBTSearchTraversalTest, Performance) {
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
        EXPECT_EQ(SBT_SUCCESS, tree->insert((const uchar*)data.c_str(), data.length()));
    }
    
    EXPECT_EQ(record_count, tree->get_record_count());
    
    // Test search performance - search for every 10th record
    int search_count = 0;
    int found_count = 0;
    
    for (int i = 0; i < record_count; i += 10) {
        const std::string& data = test_data[i];
        SBT_node* found = tree->find_by_data((const uchar*)data.c_str(), data.length());
        search_count++;
        if (found) found_count++;
    }
    
    EXPECT_EQ(search_count, found_count);
    
    // Test full traversal performance
    int traversal_count = 0;
    SBT_node* current = tree->get_first();
    
    while (current) {
        traversal_count++;
        current = tree->get_next(current);
    }
    
    EXPECT_EQ(record_count, traversal_count);
}

// Test edge cases for traversal
TEST_F(SBTSearchTraversalTest, TraversalEdgeCases) {
    // Test single node tree
    std::string single_data = "single_record";
    EXPECT_EQ(SBT_SUCCESS, tree->insert((const uchar*)single_data.c_str(), single_data.length()));
    
    SBT_node* first = tree->get_first();
    EXPECT_NE(nullptr, first);
    
    SBT_node* next = tree->get_next(first);
    EXPECT_EQ(nullptr, next);
    
    // Clear and test two node tree
    tree->clear();
    
    std::string data1 = "record_001";
    std::string data2 = "record_002";
    
    EXPECT_EQ(SBT_SUCCESS, tree->insert((const uchar*)data1.c_str(), data1.length()));
    EXPECT_EQ(SBT_SUCCESS, tree->insert((const uchar*)data2.c_str(), data2.length()));
    
    SBT_node* first_two = tree->get_first();
    SBT_node* second_two = tree->get_next(first_two);
    SBT_node* third_two = tree->get_next(second_two);
    
    EXPECT_NE(nullptr, first_two);
    EXPECT_NE(nullptr, second_two);
    EXPECT_EQ(nullptr, third_two);
    EXPECT_LT(first_two->insert_id, second_two->insert_id);
}