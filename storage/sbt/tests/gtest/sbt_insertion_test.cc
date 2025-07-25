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

/** @file unittest/sbt_insertion_test.cc
 SBT Tree Insertion Unit Tests - Comprehensive

 Created 2025-01-25
 *******************************************************/

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <random>
#include <algorithm>
#include "my_config.h"
#include "../include/sbt_tree.h"
#include "../include/sbt_common.h"

class SBTInsertionTest : public ::testing::Test {
protected:
  void SetUp() override {
    tree = new SBT_tree();
  }

  void TearDown() override {
    delete tree;
  }

  // Helper function to verify SBT property
  bool verify_sbt_property(SBT_node* node) {
    if (!node) return true;
    
    uint left_size = node->left ? node->left->size : 0;
    uint right_size = node->right ? node->right->size : 0;
    
    // Check size consistency
    if (node->size != 1 + left_size + right_size) {
      return false;
    }
    
    // Check SBT balance property
    if (node->left) {
      uint ll_size = node->left->left ? node->left->left->size : 0;
      uint lr_size = node->left->right ? node->left->right->size : 0;
      
      // Left subtree should not violate SBT property
      if (ll_size > right_size || lr_size > right_size) {
        return false;
      }
    }
    
    if (node->right) {
      uint rl_size = node->right->left ? node->right->left->size : 0;
      uint rr_size = node->right->right ? node->right->right->size : 0;
      
      // Right subtree should not violate SBT property
      if (rr_size > left_size || rl_size > left_size) {
        return false;
      }
    }
    
    // Recursively check subtrees
    return verify_sbt_property(node->left) && verify_sbt_property(node->right);
  }

  // Helper function to get tree height
  int get_tree_height(SBT_node* node) {
    if (!node) return 0;
    return 1 + std::max(get_tree_height(node->left), get_tree_height(node->right));
  }

  // Helper function to verify insert_id ordering
  bool verify_insert_id_ordering(SBT_node* node) {
    if (!node) return true;
    
    if (node->left && node->left->insert_id >= node->insert_id) {
      return false;
    }
    
    if (node->right && node->right->insert_id <= node->insert_id) {
      return false;
    }
    
    return verify_insert_id_ordering(node->left) && verify_insert_id_ordering(node->right);
  }

  SBT_tree *tree;
};

/** Test basic insertion functionality */
TEST_F(SBTInsertionTest, BasicInsertion) {
  const char *test_data = "test_record";
  uint data_length = strlen(test_data);
  
  // Test successful insertion
  int result = tree->insert((const uchar *)test_data, data_length);
  EXPECT_EQ(result, SBT_SUCCESS);
  EXPECT_FALSE(tree->is_empty());
  EXPECT_EQ(tree->get_record_count(), 1);
  
  // Verify the record can be found
  SBT_node *found = tree->find_by_data((const uchar *)test_data, data_length);
  EXPECT_TRUE(found != nullptr);
  EXPECT_EQ(found->data_length, data_length);
  EXPECT_EQ(memcmp(found->data, test_data, data_length), 0);
  EXPECT_EQ(found->insert_id, 1); // First record should have insert_id = 1
  EXPECT_EQ(found->size, 1); // Single node should have size = 1
}

/** Test insertion with invalid arguments */
TEST_F(SBTInsertionTest, InvalidArguments) {
  // Test null data
  int result = tree->insert(nullptr, 10);
  EXPECT_EQ(result, SBT_ERR_INVALID_ARGUMENT);
  
  // Test zero length
  const char *test_data = "test";
  result = tree->insert((const uchar *)test_data, 0);
  EXPECT_EQ(result, SBT_ERR_INVALID_ARGUMENT);
  
  // Tree should remain empty
  EXPECT_TRUE(tree->is_empty());
  EXPECT_EQ(tree->get_record_count(), 0);
}

/** Test multiple insertions and tree structure */
TEST_F(SBTInsertionTest, MultipleInsertions) {
  std::vector<std::string> test_data = {
    "record1", "record2", "record3", "record4", "record5",
    "record6", "record7", "record8", "record9", "record10"
  };
  
  // Insert all records
  for (size_t i = 0; i < test_data.size(); i++) {
    int result = tree->insert((const uchar *)test_data[i].c_str(), test_data[i].length());
    EXPECT_EQ(result, SBT_SUCCESS);
    EXPECT_EQ(tree->get_record_count(), i + 1);
    
    // Verify tree balance after each insertion
    EXPECT_TRUE(verify_sbt_property(tree->root));
    
    // Verify insert_id ordering
    EXPECT_TRUE(verify_insert_id_ordering(tree->root));
  }
  
  // Verify all records can be found
  for (const auto& data : test_data) {
    SBT_node *found = tree->find_by_data((const uchar *)data.c_str(), data.length());
    EXPECT_TRUE(found != nullptr);
  }
}

/** Test SBT balancing with sequential insertions */
TEST_F(SBTInsertionTest, SequentialInsertionBalance) {
  const int num_records = 15;
  
  // Insert records sequentially (worst case for unbalanced BST)
  for (int i = 1; i <= num_records; i++) {
    std::string data = "seq_record_" + std::to_string(i);
    int result = tree->insert((const uchar *)data.c_str(), data.length());
    EXPECT_EQ(result, SBT_SUCCESS);
    
    // Verify SBT property is maintained
    EXPECT_TRUE(verify_sbt_property(tree->root));
  }
  
  // Tree should be reasonably balanced (height should be O(log n))
  int height = get_tree_height(tree->root);
  int max_expected_height = static_cast<int>(std::log2(num_records)) + 3; // Allow some slack
  EXPECT_LE(height, max_expected_height);
}

/** Test SBT balancing with random insertions */
TEST_F(SBTInsertionTest, RandomInsertionBalance) {
  const int num_records = 50;
  std::vector<int> indices;
  
  // Create random insertion order
  for (int i = 1; i <= num_records; i++) {
    indices.push_back(i);
  }
  
  std::random_device rd;
  std::mt19937 g(rd());
  std::shuffle(indices.begin(), indices.end(), g);
  
  // Insert records in random order
  for (int i : indices) {
    std::string data = "rand_record_" + std::to_string(i);
    int result = tree->insert((const uchar *)data.c_str(), data.length());
    EXPECT_EQ(result, SBT_SUCCESS);
    
    // Verify SBT property is maintained
    EXPECT_TRUE(verify_sbt_property(tree->root));
  }
  
  EXPECT_EQ(tree->get_record_count(), num_records);
  
  // Tree should be reasonably balanced
  int height = get_tree_height(tree->root);
  int max_expected_height = static_cast<int>(std::log2(num_records)) + 3;
  EXPECT_LE(height, max_expected_height);
}

/** Test insertion with duplicate data */
TEST_F(SBTInsertionTest, DuplicateDataInsertion) {
  const char *data = "duplicate_record";
  
  // Insert first record
  int result1 = tree->insert((const uchar *)data, strlen(data));
  EXPECT_EQ(result1, SBT_SUCCESS);
  EXPECT_EQ(tree->get_record_count(), 1);
  
  // Insert same data again (should succeed as we don't enforce uniqueness)
  int result2 = tree->insert((const uchar *)data, strlen(data));
  EXPECT_EQ(result2, SBT_SUCCESS);
  EXPECT_EQ(tree->get_record_count(), 2);
  
  // Both records should have different insert_ids
  SBT_node *first = tree->get_first();
  SBT_node *second = tree->get_next(first);
  
  EXPECT_TRUE(first != nullptr);
  EXPECT_TRUE(second != nullptr);
  EXPECT_NE(first->insert_id, second->insert_id);
  
  // Tree should still be balanced
  EXPECT_TRUE(verify_sbt_property(tree->root));
}

/** Test insertion with various data sizes */
TEST_F(SBTInsertionTest, VariousDataSizes) {
  std::vector<std::string> test_data = {
    "a",                                    // 1 byte
    "short",                               // 5 bytes
    "medium_length_record",                // 20 bytes
    std::string(100, 'x'),                 // 100 bytes
    std::string(1000, 'y'),                // 1000 bytes
    "",                                    // Empty string (should fail)
  };
  
  int successful_insertions = 0;
  
  for (size_t i = 0; i < test_data.size(); i++) {
    int result = tree->insert((const uchar *)test_data[i].c_str(), test_data[i].length());
    
    if (test_data[i].empty()) {
      // Empty string should fail
      EXPECT_EQ(result, SBT_ERR_INVALID_ARGUMENT);
    } else {
      EXPECT_EQ(result, SBT_SUCCESS);
      successful_insertions++;
      
      // Verify record can be found
      SBT_node *found = tree->find_by_data((const uchar *)test_data[i].c_str(), test_data[i].length());
      EXPECT_TRUE(found != nullptr);
      EXPECT_EQ(found->data_length, test_data[i].length());
    }
    
    // Verify tree balance
    EXPECT_TRUE(verify_sbt_property(tree->root));
  }
  
  EXPECT_EQ(tree->get_record_count(), successful_insertions);
}

/** Test tree traversal after insertions */
TEST_F(SBTInsertionTest, TreeTraversalAfterInsertion) {
  std::vector<std::string> test_data = {
    "zebra", "apple", "mango", "banana", "orange"
  };
  
  // Insert records
  for (const auto& data : test_data) {
    tree->insert((const uchar *)data.c_str(), data.length());
  }
  
  // Traverse tree and collect insert_ids
  std::vector<sbt_insert_id_t> traversal_ids;
  SBT_node *current = tree->get_first();
  
  while (current) {
    traversal_ids.push_back(current->insert_id);
    current = tree->get_next(current);
  }
  
  // Traversal should return records in insert_id order (1, 2, 3, 4, 5)
  EXPECT_EQ(traversal_ids.size(), test_data.size());
  for (size_t i = 0; i < traversal_ids.size(); i++) {
    EXPECT_EQ(traversal_ids[i], i + 1);
  }
}

/** Test SBT specific rotation scenarios */
TEST_F(SBTInsertionTest, SBTRotationScenarios) {
  // This test creates specific insertion patterns that should trigger
  // different SBT rotation scenarios
  
  // Insert records to create a scenario that requires left rotation
  std::vector<std::string> left_rotation_data = {
    "a", "b", "c", "d"
  };
  
  for (const auto& data : left_rotation_data) {
    tree->insert((const uchar *)data.c_str(), data.length());
    EXPECT_TRUE(verify_sbt_property(tree->root));
  }
  
  tree->clear();
  
  // Insert records to create a scenario that requires right rotation
  std::vector<std::string> right_rotation_data = {
    "d", "c", "b", "a"
  };
  
  for (const auto& data : right_rotation_data) {
    tree->insert((const uchar *)data.c_str(), data.length());
    EXPECT_TRUE(verify_sbt_property(tree->root));
  }
  
  tree->clear();
  
  // Insert records to create a scenario that requires double rotation
  std::vector<std::string> double_rotation_data = {
    "a", "c", "b", "e", "d", "f"
  };
  
  for (const auto& data : double_rotation_data) {
    tree->insert((const uchar *)data.c_str(), data.length());
    EXPECT_TRUE(verify_sbt_property(tree->root));
  }
}

/** Test large scale insertion performance and balance */
TEST_F(SBTInsertionTest, LargeScaleInsertion) {
  const int num_records = 1000;
  
  // Insert many records
  for (int i = 1; i <= num_records; i++) {
    std::string data = "large_scale_record_" + std::to_string(i);
    int result = tree->insert((const uchar *)data.c_str(), data.length());
    EXPECT_EQ(result, SBT_SUCCESS);
    
    // Check balance periodically (not every insertion for performance)
    if (i % 100 == 0) {
      EXPECT_TRUE(verify_sbt_property(tree->root));
    }
  }
  
  EXPECT_EQ(tree->get_record_count(), num_records);
  
  // Final balance check
  EXPECT_TRUE(verify_sbt_property(tree->root));
  
  // Tree should be well-balanced
  int height = get_tree_height(tree->root);
  int max_expected_height = static_cast<int>(std::log2(num_records)) + 5; // Allow more slack for large trees
  EXPECT_LE(height, max_expected_height);
  
  // Verify traversal works correctly
  int traversal_count = 0;
  SBT_node *current = tree->get_first();
  
  while (current && traversal_count < num_records + 10) { // Prevent infinite loop
    traversal_count++;
    current = tree->get_next(current);
  }
  
  EXPECT_EQ(traversal_count, num_records);
}

/** Test memory management during insertions */
TEST_F(SBTInsertionTest, MemoryManagement) {
  const int num_records = 100;
  
  // Insert records
  for (int i = 1; i <= num_records; i++) {
    std::string data = "memory_test_record_" + std::to_string(i);
    tree->insert((const uchar *)data.c_str(), data.length());
  }
  
  EXPECT_EQ(tree->get_record_count(), num_records);
  
  // Clear tree
  tree->clear();
  EXPECT_TRUE(tree->is_empty());
  EXPECT_EQ(tree->get_record_count(), 0);
  EXPECT_EQ(tree->get_next_insert_id(), 1); // Should reset to 1
  
  // Should be able to insert again after clear
  const char *test_data = "after_clear_record";
  int result = tree->insert((const uchar *)test_data, strlen(test_data));
  EXPECT_EQ(result, SBT_SUCCESS);
  EXPECT_EQ(tree->get_record_count(), 1);
  
  // New record should have insert_id = 1
  SBT_node *found = tree->find_by_data((const uchar *)test_data, strlen(test_data));
  EXPECT_TRUE(found != nullptr);
  EXPECT_EQ(found->insert_id, 1);
}