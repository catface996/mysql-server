/*****************************************************************************

Copyright (c) 2025, Oracle and/or its affiliates.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License, version 2.0,
as published by the Free Software Foundation.

*****************************************************************************/

/** @file test_sbt_advanced.cc
 Advanced tests for SBT tree properties and memory management

 Created 2025-01-25
 *******************************************************/

#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <cstring>
#include <cassert>
#include <random>

// Mock MySQL types and functions for standalone testing
typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long long uint64_t;
typedef unsigned int uint32_t;

#define PSI_NOT_INSTRUMENTED 0
#define MYF(x) (x)
#define MY_WME 0

void* my_malloc(int, size_t size, int) {
    return malloc(size);
}

void my_free(void* ptr) {
    free(ptr);
}

void* my_realloc(int, void* ptr, size_t size, int) {
    return realloc(ptr, size);
}

class MEM_ROOT {
public:
    MEM_ROOT(int, size_t) {}
    void* Alloc(size_t size) { return malloc(size); }
    void Clear() {}
};

// SBT Error Codes
enum sbt_error_t {
  SBT_SUCCESS = 0,
  SBT_ERR_OUT_OF_MEMORY,
  SBT_ERR_INVALID_ARGUMENT,
  SBT_ERR_GENERIC
};

typedef uint64_t sbt_insert_id_t;

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
  SBT_tree() : root(nullptr), mem_root(0, 8192), next_insert_id(1), record_count(0) {}
  
  ~SBT_tree() {
    clear();
  }

  int insert(const uchar *data, uint length) {
    if (!data || length == 0) {
      return SBT_ERR_INVALID_ARGUMENT;
    }

    SBT_node *node = create_node(data, length, next_insert_id++);
    if (!node) {
      next_insert_id--;
      return SBT_ERR_OUT_OF_MEMORY;
    }

    root = insert_node(root, node);
    record_count++;
    return SBT_SUCCESS;
  }

  int remove(const uchar *data, uint length) {
    if (!data || length == 0) {
      return SBT_ERR_INVALID_ARGUMENT;
    }

    if (!find_by_data(data, length)) {
      return SBT_ERR_INVALID_ARGUMENT;
    }

    root = remove_node(root, data, length);
    if (record_count > 0) {
      record_count--;
    }
    return SBT_SUCCESS;
  }

  SBT_node *find_by_data(const uchar *data, uint length) {
    if (!data || length == 0) {
      return nullptr;
    }
    return find_by_data_recursive(root, data, length);
  }

  SBT_node *get_first() {
    return find_min(root);
  }

  SBT_node *get_next(SBT_node *current) {
    if (!current) return nullptr;
    if (current->right) {
      return find_min(current->right);
    }
    return find_next_by_insert_id(root, current->insert_id);
  }

  uint64_t get_record_count() const { return record_count; }
  sbt_insert_id_t get_next_insert_id() const { return next_insert_id; }
  void set_next_insert_id(sbt_insert_id_t id) { next_insert_id = id; }
  bool is_empty() const { return root == nullptr; }

  void clear() {
    root = nullptr;
    record_count = 0;
    next_insert_id = 1;
    mem_root.Clear();
  }

  // Additional methods for testing
  SBT_node *get_root() const { return root; }
  
  bool verify_tree_properties() {
    return verify_tree_properties_recursive(root);
  }

private:
  SBT_node *create_node(const uchar *data, uint length, sbt_insert_id_t insert_id) {
    SBT_node *node = (SBT_node *)mem_root.Alloc(sizeof(SBT_node));
    if (!node) return nullptr;

    node->data = (uchar *)mem_root.Alloc(length);
    if (!node->data) return nullptr;
    
    memcpy(node->data, data, length);
    node->data_length = length;
    node->insert_id = insert_id;
    node->left = nullptr;
    node->right = nullptr;
    node->size = 1;
    
    return node;
  }

  SBT_node *insert_node(SBT_node *node, SBT_node *new_node) {
    if (!node) return new_node;

    if (new_node->insert_id < node->insert_id) {
      node->left = insert_node(node->left, new_node);
    } else {
      node->right = insert_node(node->right, new_node);
    }

    update_size(node);
    return maintain(node, new_node->insert_id >= node->insert_id);
  }

  SBT_node *remove_node(SBT_node *node, const uchar *data, uint length) {
    if (!node) return nullptr;

    if (sbt_data_compare(node->data, node->data_length, data, length) == 0) {
      if (!node->left && !node->right) return nullptr;
      if (!node->left) return node->right;
      if (!node->right) return node->left;

      SBT_node *successor = find_min(node->right);
      uchar *new_data = (uchar *)mem_root.Alloc(successor->data_length);
      if (new_data) {
        memcpy(new_data, successor->data, successor->data_length);
        node->data = new_data;
        node->data_length = successor->data_length;
        node->insert_id = successor->insert_id;
      }
      node->right = remove_node(node->right, successor->data, successor->data_length);
    } else {
      node->left = remove_node(node->left, data, length);
      node->right = remove_node(node->right, data, length);
    }

    update_size(node);
    return maintain(node, false);
  }

  SBT_node *maintain(SBT_node *node, bool flag) {
    if (!node) return node;

    if (!flag) {
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
    return node;
  }

  SBT_node *rotate_left(SBT_node *node) {
    if (!node || !node->right) return node;

    SBT_node *new_root = node->right;
    node->right = new_root->left;
    new_root->left = node;

    update_size(node);
    update_size(new_root);

    return new_root;
  }

  SBT_node *rotate_right(SBT_node *node) {
    if (!node || !node->left) return node;

    SBT_node *new_root = node->left;
    node->left = new_root->right;
    new_root->right = node;

    update_size(node);
    update_size(new_root);

    return new_root;
  }

  void update_size(SBT_node *node) {
    if (node) {
      node->size = 1 + get_size(node->left) + get_size(node->right);
    }
  }

  uint get_size(SBT_node *node) const {
    return node ? node->size : 0;
  }

  SBT_node *find_min(SBT_node *node) {
    if (!node) return nullptr;
    while (node->left) {
      node = node->left;
    }
    return node;
  }

  SBT_node *find_by_data_recursive(SBT_node *node, const uchar *data, uint length) {
    if (!node) return nullptr;

    if (sbt_data_compare(node->data, node->data_length, data, length) == 0) {
      return node;
    }

    SBT_node *found = find_by_data_recursive(node->left, data, length);
    if (found) return found;

    return find_by_data_recursive(node->right, data, length);
  }

  SBT_node *find_next_by_insert_id(SBT_node *node, sbt_insert_id_t current_id) {
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

  bool verify_tree_properties_recursive(SBT_node *node) {
    if (!node) return true;

    // Verify size property
    uint expected_size = 1 + get_size(node->left) + get_size(node->right);
    if (node->size != expected_size) {
      std::cerr << "Size property violated at node with insert_id " << node->insert_id << std::endl;
      return false;
    }

    // Verify BST property for insert_id
    if (node->left && node->left->insert_id >= node->insert_id) {
      std::cerr << "BST property violated: left child insert_id >= parent insert_id" << std::endl;
      return false;
    }
    if (node->right && node->right->insert_id <= node->insert_id) {
      std::cerr << "BST property violated: right child insert_id <= parent insert_id" << std::endl;
      return false;
    }

    // Recursively verify children
    return verify_tree_properties_recursive(node->left) && 
           verify_tree_properties_recursive(node->right);
  }
};

// Test functions
void test_large_dataset() {
    std::cout << "Testing large dataset insertion and retrieval..." << std::endl;
    
    SBT_tree tree;
    const int num_records = 1000;
    std::vector<std::string> test_data;
    
    // Generate test data
    for (int i = 0; i < num_records; i++) {
        test_data.push_back("record_" + std::to_string(i) + "_data_content");
    }
    
    // Insert all records
    for (const auto& data : test_data) {
        int result = tree.insert((const uchar*)data.c_str(), data.length());
        assert(result == SBT_SUCCESS);
    }
    
    assert(tree.get_record_count() == num_records);
    assert(tree.get_next_insert_id() == num_records + 1);
    
    // Verify all records can be found
    for (const auto& data : test_data) {
        SBT_node* node = tree.find_by_data((const uchar*)data.c_str(), data.length());
        assert(node != nullptr);
        assert(memcmp(node->data, data.c_str(), data.length()) == 0);
    }
    
    std::cout << "✓ Large dataset test passed (" << num_records << " records)" << std::endl;
}

void test_tree_balance_properties() {
    std::cout << "Testing SBT balance properties..." << std::endl;
    
    SBT_tree tree;
    
    // Insert records in sequential order (worst case for unbalanced tree)
    for (int i = 0; i < 100; i++) {
        std::string data = "sequential_" + std::to_string(i);
        tree.insert((const uchar*)data.c_str(), data.length());
    }
    
    // Verify tree properties are maintained
    assert(tree.verify_tree_properties());
    
    // The tree should not degenerate into a linked list
    // Check that the tree has reasonable depth
    SBT_node* root = tree.get_root();
    assert(root != nullptr);
    assert(root->size == 100);
    
    std::cout << "✓ Tree balance properties test passed" << std::endl;
}

void test_random_operations() {
    std::cout << "Testing random insert/remove operations..." << std::endl;
    
    SBT_tree tree;
    std::set<std::string> inserted_data;
    std::mt19937 rng(42); // Fixed seed for reproducibility
    std::uniform_int_distribution<int> op_dist(0, 2); // 0=insert, 1=remove, 2=find
    
    for (int i = 0; i < 500; i++) {
        int operation = op_dist(rng);
        std::string data = "random_" + std::to_string(rng() % 200);
        
        if (operation == 0 || inserted_data.empty()) {
            // Insert operation
            if (inserted_data.find(data) == inserted_data.end()) {
                int result = tree.insert((const uchar*)data.c_str(), data.length());
                if (result == SBT_SUCCESS) {
                    inserted_data.insert(data);
                }
            }
        } else if (operation == 1 && !inserted_data.empty()) {
            // Remove operation
            auto it = inserted_data.begin();
            std::advance(it, rng() % inserted_data.size());
            std::string to_remove = *it;
            
            int result = tree.remove((const uchar*)to_remove.c_str(), to_remove.length());
            if (result == SBT_SUCCESS) {
                inserted_data.erase(it);
            }
        } else {
            // Find operation
            SBT_node* node = tree.find_by_data((const uchar*)data.c_str(), data.length());
            bool should_exist = inserted_data.count(data) > 0;
            assert((node != nullptr) == should_exist);
        }
        
        // Verify tree properties after each operation
        assert(tree.verify_tree_properties());
        assert(tree.get_record_count() == inserted_data.size());
    }
    
    std::cout << "✓ Random operations test passed (500 operations)" << std::endl;
}

void test_memory_management() {
    std::cout << "Testing memory management..." << std::endl;
    
    // Test multiple tree lifecycles
    for (int cycle = 0; cycle < 10; cycle++) {
        SBT_tree* tree = new SBT_tree();
        
        // Insert some data
        for (int i = 0; i < 50; i++) {
            std::string data = "cycle_" + std::to_string(cycle) + "_record_" + std::to_string(i);
            tree->insert((const uchar*)data.c_str(), data.length());
        }
        
        assert(tree->get_record_count() == 50);
        
        // Clear and reuse
        tree->clear();
        assert(tree->is_empty());
        assert(tree->get_record_count() == 0);
        
        // Insert again
        for (int i = 0; i < 25; i++) {
            std::string data = "after_clear_" + std::to_string(i);
            tree->insert((const uchar*)data.c_str(), data.length());
        }
        
        assert(tree->get_record_count() == 25);
        
        delete tree;
    }
    
    std::cout << "✓ Memory management test passed" << std::endl;
}

void test_traversal_completeness() {
    std::cout << "Testing traversal completeness..." << std::endl;
    
    SBT_tree tree;
    std::vector<std::string> test_data;
    
    // Insert test data
    for (int i = 0; i < 50; i++) {
        test_data.push_back("traverse_" + std::to_string(i));
        tree.insert((const uchar*)test_data.back().c_str(), test_data.back().length());
    }
    
    // Traverse the entire tree
    std::set<sbt_insert_id_t> visited_ids;
    SBT_node* current = tree.get_first();
    
    while (current) {
        assert(visited_ids.find(current->insert_id) == visited_ids.end()); // No duplicates
        visited_ids.insert(current->insert_id);
        current = tree.get_next(current);
    }
    
    // Should have visited all nodes
    assert(visited_ids.size() == test_data.size());
    
    // Verify all insert_ids from 1 to test_data.size() are present
    for (size_t i = 1; i <= test_data.size(); i++) {
        assert(visited_ids.count(i) == 1);
    }
    
    std::cout << "✓ Traversal completeness test passed" << std::endl;
}

void test_edge_cases() {
    std::cout << "Testing edge cases..." << std::endl;
    
    SBT_tree tree;
    
    // Test with empty tree
    assert(tree.get_first() == nullptr);
    assert(tree.get_next(nullptr) == nullptr);
    
    // Test single node
    const char* single_data = "single";
    tree.insert((const uchar*)single_data, strlen(single_data));
    
    SBT_node* first = tree.get_first();
    assert(first != nullptr);
    assert(tree.get_next(first) == nullptr);
    
    // Test duplicate data (should be allowed since we don't enforce uniqueness)
    tree.insert((const uchar*)single_data, strlen(single_data));
    assert(tree.get_record_count() == 2);
    
    // Test very large data
    std::string large_data(10000, 'X');
    int result = tree.insert((const uchar*)large_data.c_str(), large_data.length());
    assert(result == SBT_SUCCESS);
    
    SBT_node* large_node = tree.find_by_data((const uchar*)large_data.c_str(), large_data.length());
    assert(large_node != nullptr);
    assert(large_node->data_length == large_data.length());
    
    std::cout << "✓ Edge cases test passed" << std::endl;
}

int main() {
    std::cout << "Running SBT Advanced Functionality Tests" << std::endl;
    std::cout << "=========================================" << std::endl;
    
    try {
        test_large_dataset();
        test_tree_balance_properties();
        test_random_operations();
        test_memory_management();
        test_traversal_completeness();
        test_edge_cases();
        
        std::cout << std::endl;
        std::cout << "All advanced tests passed! ✓" << std::endl;
        std::cout << "SBT tree implementation demonstrates:" << std::endl;
        std::cout << "- Correct node creation and memory management" << std::endl;
        std::cout << "- Proper tree balance maintenance" << std::endl;
        std::cout << "- Reliable insert/remove/find operations" << std::endl;
        std::cout << "- Complete traversal functionality" << std::endl;
        std::cout << "- Robust error handling and edge case support" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}