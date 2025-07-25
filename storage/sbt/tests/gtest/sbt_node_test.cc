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

/** @file unittest/sbt_node_test.cc
 SBT Node and Basic Data Structure Tests

 Created 2025-01-25
 *******************************************************/

#include <gtest/gtest.h>
#include <string>
#include "my_config.h"
#include "../include/sbt_tree.h"
#include "../include/sbt_common.h"

/** Test SBT_tree construction and destruction */
class SBTNodeTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Test will create trees as needed
  }

  void TearDown() override {
    // Cleanup handled by individual tests
  }
};

/** Test SBT_tree constructor */
TEST_F(SBTNodeTest, TreeConstruction) {
  SBT_tree *tree = new SBT_tree();
  
  // Verify initial state
  EXPECT_TRUE(tree != nullptr);
  EXPECT_TRUE(tree->is_empty());
  EXPECT_EQ(tree->get_record_count(), 0);
  EXPECT_EQ(tree->get_next_insert_id(), 1);
  
  delete tree;
}

/** Test SBT_tree destructor and memory cleanup */
TEST_F(SBTNodeTest, TreeDestruction) {
  SBT_tree *tree = new SBT_tree();
  
  // Insert some data to test cleanup
  const char *data1 = "test_data_1";
  const char *data2 = "test_data_2";
  const char *data3 = "test_data_3";
  
  tree->insert((const uchar *)data1, strlen(data1));
  tree->insert((const uchar *)data2, strlen(data2));
  tree->insert((const uchar *)data3, strlen(data3));
  
  EXPECT_EQ(tree->get_record_count(), 3);
  EXPECT_FALSE(tree->is_empty());
  
  // Destructor should clean up all memory
  delete tree;
  
  // Test passes if no memory leaks or crashes occur
  SUCCEED();
}/** Test n
ode creation and basic properties */
TEST_F(SBTNodeTest, NodeCreation) {
  SBT_tree tree;
  
  const char *test_data = "node_test_data";
  uint data_length = strlen(test_data);
  
  // Insert a record to create a node
  int result = tree.insert((const uchar *)test_data, data_length);
  EXPECT_EQ(result, SBT_SUCCESS);
  
  // Find the created node
  SBT_node *node = tree.find_by_data((const uchar *)test_data, data_length);
  EXPECT_TRUE(node != nullptr);
  
  // Verify node properties
  EXPECT_TRUE(node->data != nullptr);
  EXPECT_EQ(node->data_length, data_length);
  EXPECT_EQ(node->insert_id, 1); // First record should have insert_id = 1
  EXPECT_EQ(node->size, 1); // Single node has size 1
  EXPECT_TRUE(node->left == nullptr);
  EXPECT_TRUE(node->right == nullptr);
  
  // Verify data content
  EXPECT_EQ(memcmp(node->data, test_data, data_length), 0);
}

/** Test multiple node creation and tree structure */
TEST_F(SBTNodeTest, MultipleNodeCreation) {
  SBT_tree tree;
  
  const char *data1 = "first";
  const char *data2 = "second";
  const char *data3 = "third";
  
  // Insert multiple records
  tree.insert((const uchar *)data1, strlen(data1));
  tree.insert((const uchar *)data2, strlen(data2));
  tree.insert((const uchar *)data3, strlen(data3));
  
  // Verify all nodes exist
  SBT_node *node1 = tree.find_by_data((const uchar *)data1, strlen(data1));
  SBT_node *node2 = tree.find_by_data((const uchar *)data2, strlen(data2));
  SBT_node *node3 = tree.find_by_data((const uchar *)data3, strlen(data3));
  
  EXPECT_TRUE(node1 != nullptr);
  EXPECT_TRUE(node2 != nullptr);
  EXPECT_TRUE(node3 != nullptr);
  
  // Verify insert IDs are sequential
  EXPECT_EQ(node1->insert_id, 1);
  EXPECT_EQ(node2->insert_id, 2);
  EXPECT_EQ(node3->insert_id, 3);
  
  // Verify tree structure (root should have appropriate size)
  SBT_node *first_node = tree.get_first();
  EXPECT_TRUE(first_node != nullptr);
  
  // The tree should maintain SBT properties
  EXPECT_EQ(tree.get_record_count(), 3);
}

/** Test memory management with large data */
TEST_F(SBTNodeTest, LargeDataHandling) {
  SBT_tree tree;
  
  // Create a large data record
  std::string large_data(1024, 'A'); // 1KB of 'A' characters
  
  int result = tree.insert((const uchar *)large_data.c_str(), large_data.length());
  EXPECT_EQ(result, SBT_SUCCESS);
  
  // Verify the large data was stored correctly
  SBT_node *node = tree.find_by_data((const uchar *)large_data.c_str(), large_data.length());
  EXPECT_TRUE(node != nullptr);
  EXPECT_EQ(node->data_length, large_data.length());
  EXPECT_EQ(memcmp(node->data, large_data.c_str(), large_data.length()), 0);
}

/** Test tree clear functionality */
TEST_F(SBTNodeTest, TreeClear) {
  SBT_tree tree;
  
  // Insert multiple records
  for (int i = 0; i < 5; i++) {
    std::string data = "record_" + std::to_string(i);
    tree.insert((const uchar *)data.c_str(), data.length());
  }
  
  EXPECT_EQ(tree.get_record_count(), 5);
  EXPECT_FALSE(tree.is_empty());
  
  // Clear the tree
  tree.clear();
  
  // Verify tree is empty
  EXPECT_TRUE(tree.is_empty());
  EXPECT_EQ(tree.get_record_count(), 0);
  EXPECT_EQ(tree.get_next_insert_id(), 1);
  
  // Verify we can insert after clear
  const char *new_data = "after_clear";
  int result = tree.insert((const uchar *)new_data, strlen(new_data));
  EXPECT_EQ(result, SBT_SUCCESS);
  EXPECT_EQ(tree.get_record_count(), 1);
}

/** Test insert ID management */
TEST_F(SBTNodeTest, InsertIdManagement) {
  SBT_tree tree;
  
  // Insert several records
  const char *data1 = "first";
  const char *data2 = "second";
  const char *data3 = "third";
  
  tree.insert((const uchar *)data1, strlen(data1));
  tree.insert((const uchar *)data2, strlen(data2));
  tree.insert((const uchar *)data3, strlen(data3));
  
  // Verify next insert ID is correct
  EXPECT_EQ(tree.get_next_insert_id(), 4);
  
  // Test setting next insert ID (for deserialization)
  tree.set_next_insert_id(100);
  EXPECT_EQ(tree.get_next_insert_id(), 100);
  
  // Insert another record
  const char *data4 = "fourth";
  tree.insert((const uchar *)data4, strlen(data4));
  
  SBT_node *node4 = tree.find_by_data((const uchar *)data4, strlen(data4));
  EXPECT_TRUE(node4 != nullptr);
  EXPECT_EQ(node4->insert_id, 100);
  EXPECT_EQ(tree.get_next_insert_id(), 101);
}