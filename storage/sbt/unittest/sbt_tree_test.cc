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

/** @file unittest/sbt_tree_test.cc
 SBT Tree Unit Tests - Skeleton

 Created 2025-01-25
 *******************************************************/

#include <gtest/gtest.h>
#include <string>
#include "my_config.h"
#include "../include/sbt_tree.h"
#include "../include/sbt_common.h"

class SBTTreeTest : public ::testing::Test {
protected:
  void SetUp() override {
    tree = new SBT_tree();
  }

  void TearDown() override {
    delete tree;
  }

  SBT_tree *tree;
};

/** Test basic tree creation and destruction */
TEST_F(SBTTreeTest, BasicCreation) {
  EXPECT_TRUE(tree != nullptr);
  EXPECT_TRUE(tree->is_empty());
  EXPECT_EQ(tree->get_record_count(), 0);
}

/** Test tree clearing */
TEST_F(SBTTreeTest, ClearTree) {
  tree->clear();
  EXPECT_TRUE(tree->is_empty());
  EXPECT_EQ(tree->get_record_count(), 0);
}

/** Test insert operation */
TEST_F(SBTTreeTest, InsertRecord) {
  const char *test_data = "test_record";
  uint data_length = strlen(test_data);
  
  int result = tree->insert((const uchar *)test_data, data_length);
  EXPECT_EQ(result, SBT_SUCCESS);
  EXPECT_FALSE(tree->is_empty());
  EXPECT_EQ(tree->get_record_count(), 1);
  
  // Verify the record can be found
  SBT_node *found = tree->find_by_data((const uchar *)test_data, data_length);
  EXPECT_TRUE(found != nullptr);
  EXPECT_EQ(found->data_length, data_length);
  EXPECT_EQ(memcmp(found->data, test_data, data_length), 0);
}

/** Test invalid arguments */
TEST_F(SBTTreeTest, InvalidArguments) {
  // Test null data
  int result = tree->insert(nullptr, 10);
  EXPECT_EQ(result, SBT_ERR_INVALID_ARGUMENT);
  
  // Test zero length
  const char *test_data = "test";
  result = tree->insert((const uchar *)test_data, 0);
  EXPECT_EQ(result, SBT_ERR_INVALID_ARGUMENT);
}

/** Test multiple insertions and tree structure */
TEST_F(SBTTreeTest, MultipleInsertions) {
  const char *data1 = "record1";
  const char *data2 = "record2";
  const char *data3 = "record3";
  
  // Insert multiple records
  EXPECT_EQ(tree->insert((const uchar *)data1, strlen(data1)), SBT_SUCCESS);
  EXPECT_EQ(tree->insert((const uchar *)data2, strlen(data2)), SBT_SUCCESS);
  EXPECT_EQ(tree->insert((const uchar *)data3, strlen(data3)), SBT_SUCCESS);
  
  EXPECT_EQ(tree->get_record_count(), 3);
  EXPECT_FALSE(tree->is_empty());
  
  // Verify all records can be found
  EXPECT_TRUE(tree->find_by_data((const uchar *)data1, strlen(data1)) != nullptr);
  EXPECT_TRUE(tree->find_by_data((const uchar *)data2, strlen(data2)) != nullptr);
  EXPECT_TRUE(tree->find_by_data((const uchar *)data3, strlen(data3)) != nullptr);
}

/** Test tree traversal */
TEST_F(SBTTreeTest, TreeTraversal) {
  const char *data1 = "aaa";
  const char *data2 = "bbb";
  const char *data3 = "ccc";
  
  // Insert records
  tree->insert((const uchar *)data1, strlen(data1));
  tree->insert((const uchar *)data2, strlen(data2));
  tree->insert((const uchar *)data3, strlen(data3));
  
  // Test traversal
  SBT_node *first = tree->get_first();
  EXPECT_TRUE(first != nullptr);
  
  SBT_node *second = tree->get_next(first);
  EXPECT_TRUE(second != nullptr);
  EXPECT_TRUE(first != second);
  
  SBT_node *third = tree->get_next(second);
  EXPECT_TRUE(third != nullptr);
  EXPECT_TRUE(second != third);
  
  // Should be no more records
  SBT_node *fourth = tree->get_next(third);
  EXPECT_TRUE(fourth == nullptr);
}

/** Test record removal */
TEST_F(SBTTreeTest, RecordRemoval) {
  const char *data1 = "record1";
  const char *data2 = "record2";
  
  // Insert records
  tree->insert((const uchar *)data1, strlen(data1));
  tree->insert((const uchar *)data2, strlen(data2));
  EXPECT_EQ(tree->get_record_count(), 2);
  
  // Remove one record
  int result = tree->remove((const uchar *)data1, strlen(data1));
  EXPECT_EQ(result, SBT_SUCCESS);
  EXPECT_EQ(tree->get_record_count(), 1);
  
  // Verify record is gone
  EXPECT_TRUE(tree->find_by_data((const uchar *)data1, strlen(data1)) == nullptr);
  EXPECT_TRUE(tree->find_by_data((const uchar *)data2, strlen(data2)) != nullptr);
  
  // Remove non-existent record
  result = tree->remove((const uchar *)data1, strlen(data1));
  EXPECT_EQ(result, SBT_ERR_INVALID_ARGUMENT);
}

/** Test record update */
TEST_F(SBTTreeTest, RecordUpdate) {
  const char *old_data = "old_record";
  const char *new_data = "new_record";
  
  // Insert record
  tree->insert((const uchar *)old_data, strlen(old_data));
  EXPECT_EQ(tree->get_record_count(), 1);
  
  // Update record
  int result = tree->update((const uchar *)old_data, strlen(old_data),
                           (const uchar *)new_data, strlen(new_data));
  EXPECT_EQ(result, SBT_SUCCESS);
  EXPECT_EQ(tree->get_record_count(), 1);
  
  // Verify old record is gone and new record exists
  EXPECT_TRUE(tree->find_by_data((const uchar *)old_data, strlen(old_data)) == nullptr);
  EXPECT_TRUE(tree->find_by_data((const uchar *)new_data, strlen(new_data)) != nullptr);
}

/** Test memory management with clear */
TEST_F(SBTTreeTest, MemoryManagement) {
  // Insert multiple records
  for (int i = 0; i < 10; i++) {
    std::string data = "record" + std::to_string(i);
    tree->insert((const uchar *)data.c_str(), data.length());
  }
  
  EXPECT_EQ(tree->get_record_count(), 10);
  
  // Clear tree
  tree->clear();
  EXPECT_TRUE(tree->is_empty());
  EXPECT_EQ(tree->get_record_count(), 0);
  EXPECT_EQ(tree->get_next_insert_id(), 1);
  
  // Should be able to insert again after clear
  const char *test_data = "after_clear";
  int result = tree->insert((const uchar *)test_data, strlen(test_data));
  EXPECT_EQ(result, SBT_SUCCESS);
  EXPECT_EQ(tree->get_record_count(), 1);
}