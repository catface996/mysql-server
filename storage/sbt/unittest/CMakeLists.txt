# Copyright (c) 2025, Oracle and/or its affiliates.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2.0,
# as published by the Free Software Foundation.
#
# This program is also distributed with certain software (including
# but not limited to OpenSSL) that is licensed under separate terms,
# as designated in a particular file or component or in included license
# documentation.  The authors of MySQL hereby grant you an additional
# permission to link the program and your derivative works with the
# separately licensed software that they have included with MySQL.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License, version 2.0, for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA

# SBT Storage Engine Unit Tests

# Include directories
INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/sql
  ${CMAKE_SOURCE_DIR}/storage/sbt/include
  ${CMAKE_SOURCE_DIR}/unittest/gunit/include
)

# SBT source files (needed for testing)
SET(SBT_SOURCES
  ../src/sbt_tree.cc
  ../src/sbt_file.cc
  ../src/sbt_share.cc
  ../src/sbt_common.cc
)

# Test sources
SET(SBT_TEST_SOURCES
  sbt_tree_test.cc
  sbt_node_test.cc
  sbt_file_test.cc
  sbt_basic_test.cc
)

# Create test executable
MYSQL_ADD_EXECUTABLE(sbt_unittest ${SBT_TEST_SOURCES} ${SBT_SOURCES}
  LINK_LIBRARIES gunit_large mysys strings
  SKIP_INSTALL
)

# Add test target
ADD_TEST(NAME sbt_unittest COMMAND sbt_unittest)