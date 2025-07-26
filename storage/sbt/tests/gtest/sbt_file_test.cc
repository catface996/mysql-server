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

/** @file unittest/sbt_file_test.cc
 SBT File Unit Tests - Skeleton

 Created 2025-01-25
 *******************************************************/

#include <gtest/gtest.h>
#include "my_config.h"
#include "../include/sbt_file.h"
#include "../include/sbt_common.h"

class SBTFileTest : public ::testing::Test {
protected:
  void SetUp() override {
    file = new SBT_file();
    test_file_name = "/tmp/sbt_test.sbt";
  }

  void TearDown() override {
    // Clean up test file
    SBT_file::delete_file(test_file_name);
    delete file;
  }

  SBT_file *file;
  const char *test_file_name;
};

/** Test file creation */
TEST_F(SBTFileTest, CreateFile) {
  int result = file->create(test_file_name);
  EXPECT_EQ(result, SBT_SUCCESS);
  
  // Verify file exists
  EXPECT_TRUE(SBT_file::file_exists(test_file_name));
}

/** Test file deletion */
TEST_F(SBTFileTest, DeleteFile) {
  // Create file first
  file->create(test_file_name);
  EXPECT_TRUE(SBT_file::file_exists(test_file_name));
  
  // Delete file
  int result = SBT_file::delete_file(test_file_name);
  EXPECT_EQ(result, SBT_SUCCESS);
  EXPECT_FALSE(SBT_file::file_exists(test_file_name));
}

/** Test invalid arguments */
TEST_F(SBTFileTest, InvalidArguments) {
  // Test null file name
  int result = file->create(nullptr);
  EXPECT_EQ(result, SBT_ERR_INVALID_ARGUMENT);
}

/** Test file header format */
TEST_F(SBTFileTest, FileHeaderFormat) {
  // Create file
  int result = file->create(test_file_name);
  EXPECT_EQ(result, SBT_SUCCESS);
  
  // Close and reopen to test header persistence
  file->close();
  result = file->open(test_file_name);
  EXPECT_EQ(result, SBT_SUCCESS);
  
  // File should open successfully, indicating valid header
}

/** Test tree serialization and deserialization */
TEST_F(SBTFileTest, TreeSerialization) {
  // Create file
  int result = file->create(test_file_name);
  EXPECT_EQ(result, SBT_SUCCESS);
  
  // Create a tree with some data
  SBT_tree tree;
  const char *test_data1 = "test_record_1";
  const char *test_data2 = "test_record_2";
  const char *test_data3 = "test_record_3";
  
  tree.insert((const uchar *)test_data1, strlen(test_data1));
  tree.insert((const uchar *)test_data2, strlen(test_data2));
  tree.insert((const uchar *)test_data3, strlen(test_data3));
  
  // Save tree to file
  result = file->save_tree(&tree);
  EXPECT_EQ(result, SBT_SUCCESS);
  
  // Create new tree and load from file
  SBT_tree loaded_tree;
  result = file->load_tree(&loaded_tree);
  EXPECT_EQ(result, SBT_SUCCESS);
  
  // Verify loaded tree has same record count
  EXPECT_EQ(loaded_tree.get_record_count(), tree.get_record_count());
  
  // Verify we can find the same records
  EXPECT_NE(loaded_tree.find_by_data((const uchar *)test_data1, strlen(test_data1)), nullptr);
  EXPECT_NE(loaded_tree.find_by_data((const uchar *)test_data2, strlen(test_data2)), nullptr);
  EXPECT_NE(loaded_tree.find_by_data((const uchar *)test_data3, strlen(test_data3)), nullptr);
}

/** Test empty tree serialization */
TEST_F(SBTFileTest, EmptyTreeSerialization) {
  // Create file
  int result = file->create(test_file_name);
  EXPECT_EQ(result, SBT_SUCCESS);
  
  // Create empty tree
  SBT_tree empty_tree;
  
  // Save empty tree
  result = file->save_tree(&empty_tree);
  EXPECT_EQ(result, SBT_SUCCESS);
  
  // Load tree
  SBT_tree loaded_tree;
  result = file->load_tree(&loaded_tree);
  EXPECT_EQ(result, SBT_SUCCESS);
  
  // Verify loaded tree is empty
  EXPECT_EQ(loaded_tree.get_record_count(), 0);
  EXPECT_TRUE(loaded_tree.is_empty());
}

/** Test file corruption detection */
TEST_F(SBTFileTest, CorruptionDetection) {
  // Create file
  int result = file->create(test_file_name);
  EXPECT_EQ(result, SBT_SUCCESS);
  file->close();
  
  // Corrupt the file by writing invalid magic number
  FILE *corrupt_file = fopen(test_file_name, "r+b");
  ASSERT_NE(corrupt_file, nullptr);
  
  // Write invalid magic
  const char invalid_magic[] = "XXXX";
  fwrite(invalid_magic, 1, 4, corrupt_file);
  fclose(corrupt_file);
  
  // Try to open corrupted file
  result = file->open(test_file_name);
  EXPECT_EQ(result, SBT_ERR_CORRUPTED_DATA);
}

/** Test file size calculation */
TEST_F(SBTFileTest, FileSizeCalculation) {
  // Create file
  int result = file->create(test_file_name);
  EXPECT_EQ(result, SBT_SUCCESS);
  
  // Get initial file size (should be header size)
  my_off_t initial_size = file->get_file_size();
  EXPECT_GE(initial_size, (my_off_t)sizeof(SBT_header));
  
  // Add some data
  SBT_tree tree;
  const char *test_data = "test_record_with_some_data";
  tree.insert((const uchar *)test_data, strlen(test_data));
  
  // Save tree
  result = file->save_tree(&tree);
  EXPECT_EQ(result, SBT_SUCCESS);
  
  // File size should have increased
  my_off_t final_size = file->get_file_size();
  EXPECT_GT(final_size, initial_size);
}