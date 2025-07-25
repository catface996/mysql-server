# Copyright (c) 2024, Oracle and/or its affiliates.
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

# SBT (Size Balanced Tree) Storage Engine

SET(SBT_PLUGIN_DYNAMIC "sbt")
SET(SBT_PLUGIN_MANDATORY FALSE)

# Source files for the SBT storage engine
SET(SBT_SOURCES
  ha_sbt.cc
  sbt_tree.cc
  sbt_share.cc
  sbt_storage.cc
  sbt_common.cc
)

# Header files (for IDE support and dependency tracking)
SET(SBT_HEADERS
  ha_sbt.h
  sbt_tree.h
  sbt_share.h
  sbt_storage.h
  sbt_common.h
)

# Include directories
INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/sql
  ${CMAKE_SOURCE_DIR}/storage/sbt
)

# Compiler flags specific to SBT
IF(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-sign-compare")
ENDIF()

# Define the storage engine plugin
MYSQL_ADD_PLUGIN(sbt 
  ${SBT_SOURCES} 
  STORAGE_ENGINE
  MODULE_ONLY
  LINK_LIBRARIES mysys
)

# Add compile definitions
TARGET_COMPILE_DEFINITIONS(sbt PRIVATE
  MYSQL_DYNAMIC_PLUGIN
  SBT_STORAGE_ENGINE
)

# Set target properties
SET_TARGET_PROPERTIES(sbt PROPERTIES
  OUTPUT_NAME "ha_sbt"
  PREFIX ""
)

# Install the plugin
IF(INSTALL_PLUGINDIR)
  INSTALL(TARGETS sbt DESTINATION ${INSTALL_PLUGINDIR})
ENDIF()

# Optional: Add unit tests if enabled
# IF(WITH_UNIT_TESTS)
#   ADD_SUBDIRECTORY(unittest)
# ENDIF()

# Optional: Add debugging symbols in debug builds
IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
  TARGET_COMPILE_DEFINITIONS(sbt PRIVATE SBT_DEBUG_BUILD)
ENDIF()

# Documentation
SET(SBT_DOC_FILES
  README.md
)

IF(SBT_DOC_FILES)
  INSTALL(FILES ${SBT_DOC_FILES} 
          DESTINATION ${INSTALL_DOCDIR}/storage-engines/sbt
          OPTIONAL)
ENDIF()