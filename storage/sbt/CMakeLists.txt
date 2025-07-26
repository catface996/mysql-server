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

# SBT Storage Engine CMake Configuration

# Add compile definitions
ADD_DEFINITIONS(-DMYSQL_SERVER)
ADD_DEFINITIONS(-DLOG_SUBSYSTEM_TAG="SBT")

# Include directories
INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/sql
  ${CMAKE_SOURCE_DIR}/storage/sbt/include
)

# Source files
SET(SBT_SOURCES
  src/ha_sbt.cc
  src/sbt_tree.cc
  src/sbt_file.cc
  src/sbt_share.cc
  src/sbt_common.cc
  src/sbt_raii.cc
)

# Header files
SET(SBT_HEADERS
  include/ha_sbt.h
  include/sbt_tree.h
  include/sbt_file.h
  include/sbt_share.h
  include/sbt_common.h
  include/sbt_raii.h
)

# Create the SBT storage engine library
MYSQL_ADD_PLUGIN(sbt ${SBT_SOURCES} ${SBT_HEADERS}
  STORAGE_ENGINE
  MODULE_ONLY
  LINK_LIBRARIES mysys
)

# Set compiler flags
IF(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  TARGET_COMPILE_OPTIONS(sbt PRIVATE -Wall -Wextra -Wno-unused-parameter)
ENDIF()

# Add unit tests if enabled
IF(WITH_UNIT_TESTS)
  ADD_SUBDIRECTORY(tests/gtest)
ENDIF()