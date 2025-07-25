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