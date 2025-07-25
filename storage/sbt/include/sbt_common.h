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

/** @file include/sbt_common.h
 SBT Storage Engine Common Definitions

 Created 2025-01-25
 *******************************************************/

#ifndef sbt_common_h
#define sbt_common_h

#include "my_config.h"
#include "my_inttypes.h"
#include "my_sys.h"
#include "mysql/plugin.h"
#include "sql/handler.h"
#include "sql/table.h"

// SBT Storage Engine Version
#define SBT_VERSION_MAJOR 1
#define SBT_VERSION_MINOR 0
#define SBT_VERSION_PATCH 0

// SBT File Format Constants
#define SBT_FILE_MAGIC "SBT\0"
#define SBT_FILE_MAGIC_SIZE 4
#define SBT_FILE_VERSION 1

// SBT Error Codes
enum sbt_error_t {
  SBT_SUCCESS = 0,
  SBT_ERR_OUT_OF_MEMORY,
  SBT_ERR_FILE_NOT_FOUND,
  SBT_ERR_CORRUPTED_DATA,
  SBT_ERR_DUPLICATE_KEY,
  SBT_ERR_IO_ERROR,
  SBT_ERR_INVALID_ARGUMENT,
  SBT_ERR_GENERIC
};

// Forward declarations
struct SBT_node;
class SBT_tree;
class SBT_file;
class SBT_share;
class ha_sbt;

// Type definitions
typedef struct SBT_node SBT_node_t;
typedef uint64_t sbt_insert_id_t;

// SBT File Header Structure
struct SBT_header {
  char magic[SBT_FILE_MAGIC_SIZE];  // File magic number
  uint32_t version;                 // File format version
  uint64_t record_count;            // Number of records in the tree
  uint64_t next_insert_id;          // Next insert ID to use
  uint64_t tree_root_offset;        // Offset to serialized tree data
  uint32_t checksum;                // Header checksum
  char reserved[32];                // Reserved for future use
};

// Utility functions
int sbt_error_to_mysql_error(int sbt_error);
void sbt_log_error(const char *format, ...);
void sbt_log_info(const char *format, ...);

// Memory management helpers
void *sbt_malloc(size_t size);
void sbt_free(void *ptr);
void *sbt_realloc(void *ptr, size_t size);

// String comparison for record data
int sbt_data_compare(const uchar *data1, uint length1, 
                     const uchar *data2, uint length2);

#endif /* sbt_common_h */