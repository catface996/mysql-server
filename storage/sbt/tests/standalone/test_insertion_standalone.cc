/*****************************************************************************

Copyright (c) 2025, Oracle and/or its affiliates.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License, version 2.0,
as published by the Free Software Foundation.

*****************************************************************************/

/** @file test_insertion_standalone.cc
 SBT Tree Insertion Test Program - Standalone Version

 Created 2025-01-25
 *******************************************************/

#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <ctime>

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

// Mock MEM_ROOT class
class MEM_ROOT {
private:
    std::vector<void*> allocated_blocks;
    
public:
    MEM_ROOT(void* /*psi*/, size_t /*block_size*/) {}
    
    void* Alloc(size_t size) {
        void* ptr = malloc(size);
        if (ptr) {
            allocated_blocks.push_back(ptr);
        }
        return ptr;
    }
    
    void Clear() {
        for (void* ptr : allocated_blocks) {
            free(ptr);
        }
        allocated_blocks.clear();
    }
    
    ~MEM_ROOT() {
        Clear();
    }
};

// SBT Node Structure
struct SBT_node {
  uchar *data;                    // Record data
  uint data_length;               // Data length in bytes
  sbt_insert_id_t insert_id;      // Insert order ID (for sorting only)
  SBT_node *left;                 // Left child
  SBT_node *right;                // Right child
  uint size;                      // Size of subtree (including self)
};

// Utility function
int sbt_data_compare(const uchar *data1, uint length1, 
                     const uchar *data2, uint length2) {
    if (!data1 || !data2) {
        if (data1 == data2) return 0;
        return data1 ? 1 : -1;
    }
    
    if (length1 != length2) {
        return (length1 < length2) ? -1 : 1;
    }
    
    return memcmp(data1, data2, length1);
}

/** SBT Tree Class - Simplified for testing */
class SBT_tree {
private:
  SBT_node *root;                 // Root node of the tree
  MEM_ROOT mem_root;              // Memory allocator for nodes
  sbt_insert_id_t next_insert_id; // Next insert ID to assign
  uint64_t record_count;          // Total number of records

public:
  /** Constructor */
  SBT_tree() : root(nullptr), mem_root(nullptr, 8192), next_insert_id(1), record_count(0) {}

  /** Destructor */
  ~SBT_tree() {
    clear();
  }

  /** Insert a new record */
  int insert(const uchar *data, uint length) {
    if (!data || length == 0) {
      return SBT_ERR_INVALID_ARGUMENT;
    }

    sbt_insert_id_t insert_id = next_insert_id++;
    root = insert_node(root, data, length, insert_id);
    
    if (root) {
      record_count++;
      return SBT_SUCCESS;
    } else {
      next_insert_id--; // Rollback on failure
      return SBT_ERR_OUT_OF_MEMORY;
    }
  }

  /** Find a record by data content */
  SBT_node *find_by_data(const uchar *data, uint length) {
    if (!data || length == 0) {
      return nullptr;
    }
    return find_by_data_recursive(root, data, length);
  }

  /** Get the first record in in-order traversal */
  SBT_node *get_first() {
    if (!root) {
      return nullptr;
    }
    return find_min(root);
  }

  /** Get the next record in in-order traversal */
  SBT_node *get_next(SBT_node *current) {
    if (!current) {
      return nullptr;
    }

    // If right subtree exists, find minimum in right subtree
    if (current->right) {
      return find_min(current->right);
    }

    // Otherwise, find the next node with insert_id greater than current
    return find_next_by_insert_id(root, current->insert_id);
  }

  /** Get total number of records */
  uint64_t get_record_count() const { return record_count; }

  /** Check if tree is empty */
  bool is_empty() const { return root == nullptr; }

  /** Clear all records from the tree */
  void clear() {
    root = nullptr;
    record_count = 0;
    next_insert_id = 1;
    mem_root.Clear();
  }

  // Public access to root for testing
  SBT_node* get_root() const { return root; }

private:
  /** Insert a node into the tree (recursive) */
  SBT_node *insert_node(SBT_node *node, const uchar *data, uint length,
                         sbt_insert_id_t insert_id) {
    // Base case: create new node
    if (!node) {
      return create_node(data, length, insert_id);
    }

    // Insert based on insert_id for SBT ordering
    if (insert_id < node->insert_id) {
      node->left = insert_node(node->left, data, length, insert_id);
      // Update size after insertion
      update_size(node);
      // Maintain SBT property - left subtree was modified
      return maintain(node, false);
    } else {
      node->right = insert_node(node->right, data, length, insert_id);
      // Update size after insertion
      update_size(node);
      // Maintain SBT property - right subtree was modified
      return maintain(node, true);
    }
  }

  /** Maintain SBT balance property */
  SBT_node *maintain(SBT_node *node, bool flag) {
    if (!node) return node;

    if (!flag) {
      // Left subtree was modified - check for violations
      if (node->left && get_size(node->left->left) > get_size(node->right)) {
        // Case 1: Left-Left case
        node = rotate_right(node);
      } else if (node->left && get_size(node->left->right) > get_size(node->right)) {
        // Case 2: Left-Right case
        node->left = rotate_left(node->left);
        node = rotate_right(node);
      } else {
        return node; // No violation, no need to maintain further
      }
    } else {
      // Right subtree was modified - check for violations
      if (node->right && get_size(node->right->right) > get_size(node->left)) {
        // Case 3: Right-Right case
        node = rotate_left(node);
      } else if (node->right && get_size(node->right->left) > get_size(node->left)) {
        // Case 4: Right-Left case
        node->right = rotate_right(node->right);
        node = rotate_left(node);
      } else {
        return node; // No violation, no need to maintain further
      }
    }

    // After rotation, recursively maintain both subtrees
    if (node->left) {
      node->left = maintain(node->left, false);
    }
    if (node->right) {
      node->right = maintain(node->right, true);
    }
    
    return node;
  }

  /** Perform left rotation */
  SBT_node *rotate_left(SBT_node *node) {
    if (!node || !node->right) {
      return node;
    }

    SBT_node *new_root = node->right;
    node->right = new_root->left;
    new_root->left = node;

    // Update sizes - order matters: update child first, then parent
    update_size(node);
    update_size(new_root);

    return new_root;
  }

  /** Perform right rotation */
  SBT_node *rotate_right(SBT_node *node) {
    if (!node || !node->left) {
      return node;
    }

    SBT_node *new_root = node->left;
    node->left = new_root->right;
    new_root->right = node;

    // Update sizes - order matters: update child first, then parent
    update_size(node);
    update_size(new_root);

    return new_root;
  }

  /** Update size of a node based on its children */
  void update_size(SBT_node *node) {
    if (node) {
      node->size = 1 + get_size(node->left) + get_size(node->right);
    }
  }

  /** Get size of a node (0 if nullptr) */
  uint get_size(SBT_node *node) const {
    return node ? node->size : 0;
  }

  /** Create a new node */
  SBT_node *create_node(const uchar *data, uint length, 
                        sbt_insert_id_t insert_id) {
    if (!data || length == 0) {
      return nullptr;
    }

    // Allocate node structure
    SBT_node *node = (SBT_node *)mem_root.Alloc(sizeof(SBT_node));
    if (!node) {
      return nullptr;
    }

    // Allocate and copy data
    node->data = (uchar *)mem_root.Alloc(length);
    if (!node->data) {
      return nullptr;
    }
    memcpy(node->data, data, length);

    // Initialize node fields
    node->data_length = length;
    node->insert_id = insert_id;
    node->left = nullptr;
    node->right = nullptr;
    node->size = 1;

    return node;
  }

  /** Find minimum node in subtree */
  SBT_node *find_min(SBT_node *node) {
    if (!node) {
      return nullptr;
    }

    while (node->left) {
      node = node->left;
    }
    
    return node;
  }

  /** Find a node by data content (recursive) */
  SBT_node *find_by_data_recursive(SBT_node *node, const uchar *data, uint length) {
    if (!node) {
      return nullptr;
    }

    // Check current node
    if (sbt_data_compare(node->data, node->data_length, data, length) == 0) {
      return node;
    }

    // Search left subtree
    SBT_node *found = find_by_data_recursive(node->left, data, length);
    if (found) {
      return found;
    }

    // Search right subtree
    return find_by_data_recursive(node->right, data, length);
  }

  /** Find next node by insert_id (for in-order traversal) */
  SBT_node *find_next_by_insert_id(SBT_node *node, sbt_insert_id_t current_id) {
    if (!node) {
      return nullptr;
    }

    SBT_node *result = nullptr;

    // If current node has insert_id greater than current_id, it's a candidate
    if (node->insert_id > current_id) {
      result = node;
      // Check if there's a smaller candidate in left subtree
      SBT_node *left_result = find_next_by_insert_id(node->left, current_id);
      if (left_result && left_result->insert_id < result->insert_id) {
        result = left_result;
      }
    } else {
      // Current node's insert_id <= current_id, search right subtree
      result = find_next_by_insert_id(node->right, current_id);
    }

    return result;
  }
};

class SBTInsertionTester {
private:
    SBT_tree* tree;
    int test_count;
    int passed_tests;

public:
    SBTInsertionTester() : tree(nullptr), test_count(0), passed_tests(0) {}
    
    ~SBTInsertionTester() {
        if (tree) {
            delete tree;
        }
    }
    
    void run_all_tests() {
        std::cout << "=== SBT Tree Insertion Tests ===" << std::endl;
        
        test_basic_insertion();
        test_multiple_insertions();
        test_insertion_order();
        test_tree_balance();
        test_large_insertions();
        test_duplicate_handling();
        test_edge_cases();
        
        std::cout << "\n=== Test Results ===" << std::endl;
        std::cout << "Passed: " << passed_tests << "/" << test_count << std::endl;
        
        if (passed_tests == test_count) {
            std::cout << "All tests PASSED!" << std::endl;
        } else {
            std::cout << "Some tests FAILED!" << std::endl;
        }
    }

private:
    void setup() {
        if (tree) {
            delete tree;
        }
        tree = new SBT_tree();
    }
    
    void teardown() {
        if (tree) {
            delete tree;
            tree = nullptr;
        }
    }
    
    bool assert_equal(uint64_t expected, uint64_t actual, const std::string& test_name) {
        test_count++;
        if (expected == actual) {
            passed_tests++;
            std::cout << "[PASS] " << test_name << std::endl;
            return true;
        } else {
            std::cout << "[FAIL] " << test_name << " - Expected: " << expected 
                      << ", Actual: " << actual << std::endl;
            return false;
        }
    }
    
    bool assert_equal(int expected, int actual, const std::string& test_name) {
        test_count++;
        if (expected == actual) {
            passed_tests++;
            std::cout << "[PASS] " << test_name << std::endl;
            return true;
        } else {
            std::cout << "[FAIL] " << test_name << " - Expected: " << expected 
                      << ", Actual: " << actual << std::endl;
            return false;
        }
    }
    
    bool assert_true(bool condition, const std::string& test_name) {
        test_count++;
        if (condition) {
            passed_tests++;
            std::cout << "[PASS] " << test_name << std::endl;
            return true;
        } else {
            std::cout << "[FAIL] " << test_name << std::endl;
            return false;
        }
    }
    
    bool assert_not_null(void* ptr, const std::string& test_name) {
        return assert_true(ptr != nullptr, test_name);
    }
    
    // Verify SBT property: size[left] <= size[right] and size[right] <= size[left]
    bool verify_sbt_property(SBT_node* node) {
        if (!node) return true;
        
        uint left_size = node->left ? node->left->size : 0;
        uint right_size = node->right ? node->right->size : 0;
        
        // Check size consistency
        if (node->size != 1 + left_size + right_size) {
            std::cout << "Size inconsistency at node with insert_id " << node->insert_id 
                      << ": expected " << (1 + left_size + right_size) 
                      << ", actual " << node->size << std::endl;
            return false;
        }
        
        // Check SBT balance property
        if (node->left) {
            uint ll_size = node->left->left ? node->left->left->size : 0;
            uint lr_size = node->left->right ? node->left->right->size : 0;
            
            // Left subtree should not violate SBT property
            if (ll_size > right_size || lr_size > right_size) {
                std::cout << "SBT violation at node " << node->insert_id 
                          << ": left subtree sizes (" << ll_size << ", " << lr_size 
                          << ") > right size (" << right_size << ")" << std::endl;
                return false;
            }
        }
        
        if (node->right) {
            uint rl_size = node->right->left ? node->right->left->size : 0;
            uint rr_size = node->right->right ? node->right->right->size : 0;
            
            // Right subtree should not violate SBT property
            if (rr_size > left_size || rl_size > left_size) {
                std::cout << "SBT violation at node " << node->insert_id 
                          << ": right subtree sizes (" << rl_size << ", " << rr_size 
                          << ") > left size (" << left_size << ")" << std::endl;
                return false;
            }
        }
        
        // Recursively check subtrees
        return verify_sbt_property(node->left) && verify_sbt_property(node->right);
    }
    
    bool verify_tree_balance() {
        return verify_sbt_property(tree->get_root());
    }
    
    void test_basic_insertion() {
        std::cout << "\n--- Basic Insertion Tests ---" << std::endl;
        setup();
        
        // Test empty tree
        assert_true(tree->is_empty(), "Empty tree check");
        assert_equal((uint64_t)0, tree->get_record_count(), "Empty tree record count");
        
        // Test single insertion
        const char* data = "test_record";
        int result = tree->insert((const uchar*)data, strlen(data));
        assert_equal(SBT_SUCCESS, result, "Single insertion result");
        assert_true(!tree->is_empty(), "Tree not empty after insertion");
        assert_equal((uint64_t)1, tree->get_record_count(), "Record count after insertion");
        
        // Verify record can be found
        SBT_node* found = tree->find_by_data((const uchar*)data, strlen(data));
        assert_not_null(found, "Find inserted record");
        
        if (found) {
            assert_equal((int)strlen(data), (int)found->data_length, "Record data length");
            assert_equal(0, memcmp(found->data, data, strlen(data)), "Record data content");
        }
        
        teardown();
    }
    
    void test_multiple_insertions() {
        std::cout << "\n--- Multiple Insertions Tests ---" << std::endl;
        setup();
        
        std::vector<std::string> test_data = {
            "record1", "record2", "record3", "record4", "record5"
        };
        
        // Insert multiple records
        for (size_t i = 0; i < test_data.size(); i++) {
            int result = tree->insert((const uchar*)test_data[i].c_str(), test_data[i].length());
            assert_equal(SBT_SUCCESS, result, "Insert record " + std::to_string(i + 1));
        }
        
        assert_equal((uint64_t)test_data.size(), tree->get_record_count(), "Total record count");
        
        // Verify all records can be found
        for (size_t i = 0; i < test_data.size(); i++) {
            SBT_node* found = tree->find_by_data((const uchar*)test_data[i].c_str(), test_data[i].length());
            assert_not_null(found, "Find record " + std::to_string(i + 1));
        }
        
        // Verify tree balance
        assert_true(verify_tree_balance(), "Tree balance after multiple insertions");
        
        teardown();
    }
    
    void test_insertion_order() {
        std::cout << "\n--- Insertion Order Tests ---" << std::endl;
        setup();
        
        std::vector<std::string> test_data = {"c", "a", "e", "b", "d"};
        
        // Insert in non-sorted order
        for (const auto& data : test_data) {
            tree->insert((const uchar*)data.c_str(), data.length());
        }
        
        // Verify in-order traversal gives records in insert_id order (not data order)
        SBT_node* current = tree->get_first();
        int traversal_count = 0;
        
        while (current && traversal_count < 10) { // Prevent infinite loop
            traversal_count++;
            current = tree->get_next(current);
        }
        
        assert_equal((int)test_data.size(), traversal_count, "Traversal count matches insertion count");
        
        teardown();
    }
    
    void test_tree_balance() {
        std::cout << "\n--- Tree Balance Tests ---" << std::endl;
        setup();
        
        // Insert records that would create an unbalanced BST
        for (int i = 1; i <= 7; i++) {
            std::string data = "record" + std::to_string(i);
            tree->insert((const uchar*)data.c_str(), data.length());
            
            // Verify balance after each insertion
            assert_true(verify_tree_balance(), "Tree balance after inserting " + std::to_string(i) + " records");
        }
        
        teardown();
    }
    
    void test_large_insertions() {
        std::cout << "\n--- Large Insertions Tests ---" << std::endl;
        setup();
        
        const int num_records = 50; // Reduced for faster testing
        
        // Insert many records
        for (int i = 0; i < num_records; i++) {
            std::string data = "large_record_" + std::to_string(i);
            int result = tree->insert((const uchar*)data.c_str(), data.length());
            assert_equal(SBT_SUCCESS, result, "Large insertion " + std::to_string(i));
            
            if (result != SBT_SUCCESS) {
                break; // Stop on first failure
            }
        }
        
        assert_equal((uint64_t)num_records, tree->get_record_count(), "Large insertion record count");
        assert_true(verify_tree_balance(), "Tree balance after large insertions");
        
        teardown();
    }
    
    void test_duplicate_handling() {
        std::cout << "\n--- Duplicate Handling Tests ---" << std::endl;
        setup();
        
        const char* data = "duplicate_record";
        
        // Insert first record
        int result1 = tree->insert((const uchar*)data, strlen(data));
        assert_equal(SBT_SUCCESS, result1, "First duplicate insertion");
        
        // Insert same record again (should succeed as we don't enforce uniqueness)
        int result2 = tree->insert((const uchar*)data, strlen(data));
        assert_equal(SBT_SUCCESS, result2, "Second duplicate insertion");
        
        assert_equal((uint64_t)2, tree->get_record_count(), "Record count with duplicates");
        
        teardown();
    }
    
    void test_edge_cases() {
        std::cout << "\n--- Edge Cases Tests ---" << std::endl;
        setup();
        
        // Test null data
        int result1 = tree->insert(nullptr, 10);
        assert_equal(SBT_ERR_INVALID_ARGUMENT, result1, "Null data insertion");
        
        // Test zero length
        const char* data = "test";
        int result2 = tree->insert((const uchar*)data, 0);
        assert_equal(SBT_ERR_INVALID_ARGUMENT, result2, "Zero length insertion");
        
        // Test very long data
        std::string long_data(1000, 'x');
        int result4 = tree->insert((const uchar*)long_data.c_str(), long_data.length());
        assert_equal(SBT_SUCCESS, result4, "Long data insertion");
        
        teardown();
    }
};

int main() {
    SBTInsertionTester tester;
    tester.run_all_tests();
    return 0;
}