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
  
  // Memory related errors
  SBT_ERR_OUT_OF_MEMORY,
  SBT_ERR_MEMORY_CORRUPTION,
  
  // File system errors
  SBT_ERR_FILE_NOT_FOUND,
  SBT_ERR_FILE_EXISTS,
  SBT_ERR_FILE_PERMISSION,
  SBT_ERR_FILE_CORRUPTED,
  SBT_ERR_IO_ERROR,
  SBT_ERR_DISK_FULL,
  
  // Data integrity errors
  SBT_ERR_CORRUPTED_DATA,
  SBT_ERR_INVALID_HEADER,
  SBT_ERR_CHECKSUM_MISMATCH,
  SBT_ERR_VERSION_MISMATCH,
  
  // Tree operation errors
  SBT_ERR_DUPLICATE_KEY,
  SBT_ERR_KEY_NOT_FOUND,
  SBT_ERR_TREE_CORRUPTED,
  SBT_ERR_NODE_INVALID,
  
  // Parameter and state errors
  SBT_ERR_INVALID_ARGUMENT,
  SBT_ERR_NULL_POINTER,
  SBT_ERR_BUFFER_TOO_SMALL,
  SBT_ERR_INVALID_STATE,
  
  // Resource errors
  SBT_ERR_RESOURCE_BUSY,
  SBT_ERR_RESOURCE_EXHAUSTED,
  SBT_ERR_TIMEOUT,
  
  // Generic and unknown errors
  SBT_ERR_NOT_IMPLEMENTED,
  SBT_ERR_GENERIC,
  SBT_ERR_UNKNOWN
};

// Error severity levels
enum sbt_error_severity_t {
  SBT_SEVERITY_INFO = 0,
  SBT_SEVERITY_WARNING,
  SBT_SEVERITY_ERROR,
  SBT_SEVERITY_FATAL
};

// Error context structure for detailed error reporting
struct SBT_error_context {
  sbt_error_t error_code;
  sbt_error_severity_t severity;
  const char *file;
  int line;
  const char *function;
  char message[512];
  uint64_t timestamp;
  uint32_t thread_id;
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
  char magic[SBT_FILE_MAGIC_SIZE];  // File magic number "SBT\0"
  uint32_t version;                 // File format version
  uint64_t record_count;            // Number of records in the tree
  uint64_t next_insert_id;          // Next insert ID to use
  uint64_t tree_root_offset;        // Offset to serialized tree data in file
  uint32_t tree_data_size;          // Size of serialized tree data
  uint32_t header_size;             // Size of this header structure
  uint32_t checksum;                // Header checksum (CRC32)
  uint64_t created_time;            // File creation timestamp
  uint64_t modified_time;           // Last modification timestamp
  char reserved[16];                // Reserved for future use
};

// SBT Serialized Node Structure (on disk format)
struct SBT_serialized_node {
  uint32_t has_node;                // 1 if node exists, 0 for null
  uint64_t insert_id;               // Insert ID for ordering
  uint32_t data_length;             // Length of record data
  uint32_t size;                    // Subtree size
  // Followed by:
  // - uchar data[data_length]      // Record data
  // - SBT_serialized_node left     // Left subtree (recursive)
  // - SBT_serialized_node right    // Right subtree (recursive)
};

// File format constants
#define SBT_HEADER_SIZE sizeof(SBT_header)
#define SBT_SERIALIZED_NODE_HEADER_SIZE sizeof(SBT_serialized_node)
#define SBT_MAX_RECORD_SIZE (64 * 1024)  // 64KB max record size
#define SBT_FILE_ALIGNMENT 8             // File data alignment

// Error handling functions
int sbt_error_to_mysql_error(sbt_error_t sbt_error);
const char *sbt_error_to_string(sbt_error_t error_code);
const char *sbt_severity_to_string(sbt_error_severity_t severity);

// Enhanced logging functions
void sbt_log_error(const char *format, ...);
void sbt_log_warning(const char *format, ...);
void sbt_log_info(const char *format, ...);
void sbt_log_debug(const char *format, ...);

// Error context functions
void sbt_error_context_init(SBT_error_context *ctx);
void sbt_error_context_set(SBT_error_context *ctx, sbt_error_t error_code,
                          sbt_error_severity_t severity, const char *file,
                          int line, const char *function, const char *format, ...);
void sbt_error_context_log(const SBT_error_context *ctx);
void sbt_error_context_clear(SBT_error_context *ctx);

// Error handling macros for convenience
#define SBT_SET_ERROR(ctx, code, severity, fmt, ...) \
  sbt_error_context_set(ctx, code, severity, __FILE__, __LINE__, __func__, fmt, ##__VA_ARGS__)

#define SBT_LOG_ERROR(fmt, ...) \
  sbt_log_error("[%s:%d] " fmt, __func__, __LINE__, ##__VA_ARGS__)

#define SBT_LOG_WARNING(fmt, ...) \
  sbt_log_warning("[%s:%d] " fmt, __func__, __LINE__, ##__VA_ARGS__)

#define SBT_LOG_INFO(fmt, ...) \
  sbt_log_info("[%s:%d] " fmt, __func__, __LINE__, ##__VA_ARGS__)

#define SBT_LOG_DEBUG(fmt, ...) \
  sbt_log_debug("[%s:%d] " fmt, __func__, __LINE__, ##__VA_ARGS__)

// Error checking macros
#define SBT_RETURN_IF_ERROR(expr) \
  do { \
    sbt_error_t _err = (expr); \
    if (_err != SBT_SUCCESS) { \
      SBT_LOG_ERROR("Operation failed with error: %s", sbt_error_to_string(_err)); \
      return _err; \
    } \
  } while (0)

#define SBT_RETURN_IF_NULL(ptr, error_code) \
  do { \
    if ((ptr) == nullptr) { \
      SBT_LOG_ERROR("Null pointer detected"); \
      return error_code; \
    } \
  } while (0)

// Memory management helpers
void *sbt_malloc(size_t size);
void sbt_free(void *ptr);
void *sbt_realloc(void *ptr, size_t size);

// String comparison for record data
int sbt_data_compare(const uchar *data1, uint length1, 
                     const uchar *data2, uint length2);

// CRC32 checksum calculation
uint32_t sbt_crc32(const uchar *data, size_t length);
uint32_t sbt_crc32_update(uint32_t crc, const uchar *data, size_t length);

// Time utilities
uint64_t sbt_get_current_time();

// File alignment utilities
uint64_t sbt_align_offset(uint64_t offset, uint32_t alignment);

#endif /* sbt_common_h */