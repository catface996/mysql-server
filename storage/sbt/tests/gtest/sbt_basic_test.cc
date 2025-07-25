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

/** @file unittest/sbt_basic_test.cc
 SBT Basic Data Structure Tests

 Created 2025-01-25
 *******************************************************/

#include <gtest/gtest.h>
#include "my_config.h"
#include "../include/sbt_tree.h"
#include "../include/sbt_common.h"

class SBTBasicTest : public ::testing::Test {
protected:
  void SetUp() override {
    tree = new SBT_tree();
  }

  void TearDown() override {
    delete tree;
  }

  SBT_tree *tree;
};

/** Test SBT_tree construction and destruction */
TEST_F(SBTBasicTest, TreeConstructorDestructor) {
  EXPECT_TRUE(tree != nullptr);
  EXPECT_TRUE(tree->is_empty());
  EXPECT_EQ(tree->get_record_count(), 0);
  EXPECT_EQ(tree->get_next_insert_id(), 1);
}

/** Test memory management with node creation */
TEST_F(SBTBasicTest, NodeCreationAndMemoryManagement) {
  const char *test_data = "test_record_data";
  uint data_length = strlen(test_data);
  
  // Insert a record
  int result = tree->insert((const uchar *)test_data, data_length);
  EXPECT_EQ(result, SBT_SUCCESS);
  EXPECT_FALSE(tree->is_empty());
  EXPECT_EQ(tree->get_record_count(), 1);
  EXPECT_EQ(tree->get_next_insert_id(), 2);
  
  // Clear tree and verify memory cleanup
  tree->clear();
  EXPECT_TRUE(tree->is_empty());
  EXPECT_EQ(tree->get_record_count(), 0);
  EXPECT_EQ(tree->get_next_insert_id(), 1);
}

/** Test basic insertion functionality */
TEST_F(SBTBasicTest, BasicInsertion) {
  const char *records[] = {"record1", "record2", "record3"};
  const int num_records = 3;
  
  // Insert multiple records
  for (int i = 0; i < num_records; i++) {
    int result = tree->insert((const uchar *)records[i], strlen(records[i]));
    EXPECT_EQ(result, SBT_SUCCESS);
    EXPECT_EQ(tree->get_record_count(), i + 1);
  }
  
  EXPECT_FALSE(tree->is_empty());
  EXPECT_EQ(tree->get_record_count(), num_records);
}

/** Test record finding by data content */
TEST_F(SBTBasicTest, FindByDataContent) {
  const char *test_data = "findable_record";
  uint data_length = strlen(test_data);
  
  // Initially should not find anything
  SBT_node *found = tree->find_by_data((const uchar *)test_data, data_length);
  EXPECT_EQ(found, nullptr);
  
  // Insert record
  int result = tree->insert((const uchar *)test_data, data_length);
  EXPECT_EQ(result, SBT_SUCCESS);
  
  // Now should find the record
  found = tree->find_by_data((const uchar *)test_data, data_length);
  EXPECT_NE(found, nullptr);
  EXPECT_EQ(found->data_length, data_length);
  EXPECT_EQ(memcmp(found->data, test_data, data_length), 0);
  EXPECT_EQ(found->insert_id, 1);
}

/** Test in-order traversal */
TEST_F(SBTBasicTest, InOrderTraversal) {
  const char *records[] = {"record1", "record2", "record3"};
  const int num_records = 3;
  
  // Insert records
  for (int i = 0; i < num_records; i++) {
    int result = tree->insert((const uchar *)records[i], strlen(records[i]));
    EXPECT_EQ(result, SBT_SUCCESS);
  }
  
  // Test traversal
  SBT_node *current = tree->get_first();
  EXPECT_NE(current, nullptr);
  
  int count = 0;
  sbt_insert_id_t prev_id = 0;
  
  while (current) {
    count++;
    // Verify in-order property (insert_id should be increasing)
    EXPECT_GT(current->insert_id, prev_id);
    prev_id = current->insert_id;
    
    current = tree->get_next(current);
  }
  
  EXPECT_EQ(count, num_records);
}

/** Test invalid arguments */
TEST_F(SBTBasicTest, InvalidArguments) {
  // Test null data
  int result = tree->insert(nullptr, 10);
  EXPECT_EQ(result, SBT_ERR_INVALID_ARGUMENT);
  
  // Test zero length
  const char *test_data = "test";
  result = tree->insert((const uchar *)test_data, 0);
  EXPECT_EQ(result, SBT_ERR_INVALID_ARGUMENT);
  
  // Test find with null data
  SBT_node *found = tree->find_by_data(nullptr, 10);
  EXPECT_EQ(found, nullptr);
  
  // Test find with zero length
  found = tree->find_by_data((const uchar *)test_data, 0);
  EXPECT_EQ(found, nullptr);
}

/** Test tree properties after multiple insertions */
TEST_F(SBTBasicTest, TreePropertiesAfterInsertions) {
  const int num_records = 10;
  char record_data[32];
  
  // Insert multiple records
  for (int i = 0; i < num_records; i++) {
    snprintf(record_data, sizeof(record_data), "record_%d", i);
    int result = tree->insert((const uchar *)record_data, strlen(record_data));
    EXPECT_EQ(result, SBT_SUCCESS);
  }
  
  EXPECT_EQ(tree->get_record_count(), num_records);
  EXPECT_EQ(tree->get_next_insert_id(), num_records + 1);
  
  // Verify all records can be found
  for (int i = 0; i < num_records; i++) {
    snprintf(record_data, sizeof(record_data), "record_%d", i);
    SBT_node *found = tree->find_by_data((const uchar *)record_data, strlen(record_data));
    EXPECT_NE(found, nullptr);
    EXPECT_EQ(found->insert_id, i + 1);
  }
}