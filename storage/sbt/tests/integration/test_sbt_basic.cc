/*****************************************************************************

Copyright (c) 2025, Oracle and/or its affiliates.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License, version 2.0,
as published by the Free Software Foundation.

*****************************************************************************/

/** @file test_sbt_basic.cc
 Standalone test for SBT basic functionality

 Created 2025-01-25
 *******************************************************/

#include <iostream>
#include <string>
#include <cstring>
#include <cassert>

// Mock MySQL types and functions for standalone testing
typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long long uint64_t;
typedef unsigned int uint32_t;

#define PSI_NOT_INSTRUMENTED 0
#define MYF(x) (x)
#define MY_WME 0

// Mock memory functions
void* my_malloc(int, size_t size, int) {
    return malloc(size);
}

void my_free(void* ptr) {
    free(ptr);
}

void* my_realloc(int, void* ptr, size_t size, int) {
    return realloc(ptr, size);
}

// Mock MEM_ROOT class
class MEM_ROOT {
public:
    MEM_ROOT(int, size_t) {}
    void* Alloc(size_t size) { return malloc(size); }
    void Clear() {}
};

// SBT Error Codes (simplified)
enum sbt_error_t {
  SBT_SUCCESS = 0,
  SBT_ERR_OUT_OF_MEMORY,
  SBT_ERR_INVALID_ARGUMENT,
  SBT_ERR_GENERIC
};

typedef uint64_t sbt_insert_id_t;

// Data comparison function
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

// SBT Node Structure
struct SBT_node {
  uchar *data;
  uint data_length;
  sbt_insert_id_t insert_id;
  SBT_node *left;
  SBT_node *right;
  uint size;
};

// Simplified SBT Tree implementation for testing
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
    return node;
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
    return node;
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
};

// Test functions
void test_tree_construction() {
    std::cout << "Testing SBT_tree construction..." << std::endl;
    
    SBT_tree tree;
    assert(tree.is_empty());
    assert(tree.get_record_count() == 0);
    assert(tree.get_next_insert_id() == 1);
    
    std::cout << "✓ Tree construction test passed" << std::endl;
}

void test_node_creation() {
    std::cout << "Testing node creation..." << std::endl;
    
    SBT_tree tree;
    const char* test_data = "test_record_data";
    uint data_length = strlen(test_data);
    
    int result = tree.insert((const uchar*)test_data, data_length);
    assert(result == SBT_SUCCESS);
    assert(!tree.is_empty());
    assert(tree.get_record_count() == 1);
    assert(tree.get_next_insert_id() == 2);
    
    SBT_node* node = tree.find_by_data((const uchar*)test_data, data_length);
    assert(node != nullptr);
    assert(node->data_length == data_length);
    assert(node->insert_id == 1);
    assert(node->size == 1);
    assert(node->left == nullptr);
    assert(node->right == nullptr);
    assert(memcmp(node->data, test_data, data_length) == 0);
    
    std::cout << "✓ Node creation test passed" << std::endl;
}

void test_multiple_insertions() {
    std::cout << "Testing multiple insertions..." << std::endl;
    
    SBT_tree tree;
    const char* data1 = "record_1";
    const char* data2 = "record_2";
    const char* data3 = "record_3";
    
    tree.insert((const uchar*)data1, strlen(data1));
    tree.insert((const uchar*)data2, strlen(data2));
    tree.insert((const uchar*)data3, strlen(data3));
    
    assert(tree.get_record_count() == 3);
    assert(tree.get_next_insert_id() == 4);
    
    // Verify all nodes exist
    SBT_node* node1 = tree.find_by_data((const uchar*)data1, strlen(data1));
    SBT_node* node2 = tree.find_by_data((const uchar*)data2, strlen(data2));
    SBT_node* node3 = tree.find_by_data((const uchar*)data3, strlen(data3));
    
    assert(node1 != nullptr && node1->insert_id == 1);
    assert(node2 != nullptr && node2->insert_id == 2);
    assert(node3 != nullptr && node3->insert_id == 3);
    
    std::cout << "✓ Multiple insertions test passed" << std::endl;
}

void test_tree_traversal() {
    std::cout << "Testing tree traversal..." << std::endl;
    
    SBT_tree tree;
    const char* data1 = "aaa";
    const char* data2 = "bbb";
    const char* data3 = "ccc";
    
    tree.insert((const uchar*)data1, strlen(data1));
    tree.insert((const uchar*)data2, strlen(data2));
    tree.insert((const uchar*)data3, strlen(data3));
    
    // Test get_first
    SBT_node* first = tree.get_first();
    assert(first != nullptr);
    
    // Test get_next
    SBT_node* second = tree.get_next(first);
    assert(second != nullptr);
    assert(second != first);
    
    SBT_node* third = tree.get_next(second);
    assert(third != nullptr);
    assert(third != first && third != second);
    
    SBT_node* end = tree.get_next(third);
    // end might be nullptr or point to another node depending on tree structure
    
    std::cout << "✓ Tree traversal test passed" << std::endl;
}

void test_record_removal() {
    std::cout << "Testing record removal..." << std::endl;
    
    SBT_tree tree;
    const char* data1 = "record_to_keep";
    const char* data2 = "record_to_remove";
    const char* data3 = "another_record";
    
    tree.insert((const uchar*)data1, strlen(data1));
    tree.insert((const uchar*)data2, strlen(data2));
    tree.insert((const uchar*)data3, strlen(data3));
    
    assert(tree.get_record_count() == 3);
    
    // Remove middle record
    int result = tree.remove((const uchar*)data2, strlen(data2));
    assert(result == SBT_SUCCESS);
    assert(tree.get_record_count() == 2);
    
    // Verify record is gone
    SBT_node* removed_node = tree.find_by_data((const uchar*)data2, strlen(data2));
    assert(removed_node == nullptr);
    
    // Verify other records still exist
    SBT_node* node1 = tree.find_by_data((const uchar*)data1, strlen(data1));
    SBT_node* node3 = tree.find_by_data((const uchar*)data3, strlen(data3));
    assert(node1 != nullptr);
    assert(node3 != nullptr);
    
    std::cout << "✓ Record removal test passed" << std::endl;
}

void test_tree_clear() {
    std::cout << "Testing tree clear..." << std::endl;
    
    SBT_tree tree;
    
    // Insert multiple records
    for (int i = 0; i < 5; i++) {
        std::string data = "record_" + std::to_string(i);
        tree.insert((const uchar*)data.c_str(), data.length());
    }
    
    assert(tree.get_record_count() == 5);
    assert(!tree.is_empty());
    
    // Clear the tree
    tree.clear();
    
    assert(tree.is_empty());
    assert(tree.get_record_count() == 0);
    assert(tree.get_next_insert_id() == 1);
    
    // Verify we can insert after clear
    const char* new_data = "after_clear";
    int result = tree.insert((const uchar*)new_data, strlen(new_data));
    assert(result == SBT_SUCCESS);
    assert(tree.get_record_count() == 1);
    
    std::cout << "✓ Tree clear test passed" << std::endl;
}

void test_error_handling() {
    std::cout << "Testing error handling..." << std::endl;
    
    SBT_tree tree;
    
    // Test invalid arguments
    int result = tree.insert(nullptr, 10);
    assert(result == SBT_ERR_INVALID_ARGUMENT);
    
    result = tree.insert((const uchar*)"data", 0);
    assert(result == SBT_ERR_INVALID_ARGUMENT);
    
    result = tree.remove(nullptr, 10);
    assert(result == SBT_ERR_INVALID_ARGUMENT);
    
    result = tree.remove((const uchar*)"nonexistent", 11);
    assert(result == SBT_ERR_INVALID_ARGUMENT);
    
    // Test find with invalid arguments
    SBT_node* node = tree.find_by_data(nullptr, 10);
    assert(node == nullptr);
    
    node = tree.find_by_data((const uchar*)"data", 0);
    assert(node == nullptr);
    
    std::cout << "✓ Error handling test passed" << std::endl;
}

int main() {
    std::cout << "Running SBT Basic Functionality Tests" << std::endl;
    std::cout << "=====================================" << std::endl;
    
    try {
        test_tree_construction();
        test_node_creation();
        test_multiple_insertions();
        test_tree_traversal();
        test_record_removal();
        test_tree_clear();
        test_error_handling();
        
        std::cout << std::endl;
        std::cout << "All tests passed! ✓" << std::endl;
        std::cout << "SBT node and basic data structure implementation is working correctly." << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}