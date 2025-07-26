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

/** @file sbt_common.cc
 SBT Common Utilities Implementation

 Created 2025-01-25
 *******************************************************/

#include "my_config.h"
#include "../include/sbt_common.h"
#include "sql/log.h"
#include "sql/mysqld.h"
#include "my_sys.h"
#include "my_thread.h"
#include <sys/time.h>
#include <cstdarg>
#include <cstring>

/** Convert SBT error to MySQL error code */
int sbt_error_to_mysql_error(sbt_error_t sbt_error) {
  switch (sbt_error) {
    case SBT_SUCCESS:
      return 0;
      
    // Memory related errors
    case SBT_ERR_OUT_OF_MEMORY:
    case SBT_ERR_MEMORY_CORRUPTION:
      return HA_ERR_OUT_OF_MEM;
      
    // File system errors
    case SBT_ERR_FILE_NOT_FOUND:
      return HA_ERR_NO_SUCH_TABLE;
    case SBT_ERR_FILE_EXISTS:
      return HA_ERR_TABLE_EXIST;
    case SBT_ERR_FILE_PERMISSION:
      return HA_ERR_CRASHED_ON_USAGE;
    case SBT_ERR_FILE_CORRUPTED:
    case SBT_ERR_IO_ERROR:
      return HA_ERR_CRASHED_ON_USAGE;
    case SBT_ERR_DISK_FULL:
      return HA_ERR_RECORD_FILE_FULL;
      
    // Data integrity errors
    case SBT_ERR_CORRUPTED_DATA:
    case SBT_ERR_INVALID_HEADER:
    case SBT_ERR_CHECKSUM_MISMATCH:
    case SBT_ERR_VERSION_MISMATCH:
      return HA_ERR_CRASHED_ON_USAGE;
      
    // Tree operation errors
    case SBT_ERR_DUPLICATE_KEY:
      return HA_ERR_FOUND_DUPP_KEY;
    case SBT_ERR_KEY_NOT_FOUND:
      return HA_ERR_KEY_NOT_FOUND;
    case SBT_ERR_TREE_CORRUPTED:
    case SBT_ERR_NODE_INVALID:
      return HA_ERR_CRASHED_ON_USAGE;
      
    // Parameter and state errors
    case SBT_ERR_INVALID_ARGUMENT:
    case SBT_ERR_NULL_POINTER:
    case SBT_ERR_BUFFER_TOO_SMALL:
      return HA_ERR_WRONG_COMMAND;
    case SBT_ERR_INVALID_STATE:
      return HA_ERR_CRASHED_ON_USAGE;
      
    // Resource errors
    case SBT_ERR_RESOURCE_BUSY:
      return HA_ERR_LOCK_WAIT_TIMEOUT;
    case SBT_ERR_RESOURCE_EXHAUSTED:
      return HA_ERR_OUT_OF_MEM;
    case SBT_ERR_TIMEOUT:
      return HA_ERR_LOCK_WAIT_TIMEOUT;
      
    // Generic and unknown errors
    case SBT_ERR_NOT_IMPLEMENTED:
      return HA_ERR_WRONG_COMMAND;
    case SBT_ERR_GENERIC:
    case SBT_ERR_UNKNOWN:
    default:
      return HA_ERR_GENERIC;
  }
}

/** Convert SBT error code to human-readable string */
const char *sbt_error_to_string(sbt_error_t error_code) {
  switch (error_code) {
    case SBT_SUCCESS:
      return "Success";
      
    // Memory related errors
    case SBT_ERR_OUT_OF_MEMORY:
      return "Out of memory";
    case SBT_ERR_MEMORY_CORRUPTION:
      return "Memory corruption detected";
      
    // File system errors
    case SBT_ERR_FILE_NOT_FOUND:
      return "File not found";
    case SBT_ERR_FILE_EXISTS:
      return "File already exists";
    case SBT_ERR_FILE_PERMISSION:
      return "File permission denied";
    case SBT_ERR_FILE_CORRUPTED:
      return "File is corrupted";
    case SBT_ERR_IO_ERROR:
      return "I/O error";
    case SBT_ERR_DISK_FULL:
      return "Disk full";
      
    // Data integrity errors
    case SBT_ERR_CORRUPTED_DATA:
      return "Data corruption detected";
    case SBT_ERR_INVALID_HEADER:
      return "Invalid file header";
    case SBT_ERR_CHECKSUM_MISMATCH:
      return "Checksum mismatch";
    case SBT_ERR_VERSION_MISMATCH:
      return "Version mismatch";
      
    // Tree operation errors
    case SBT_ERR_DUPLICATE_KEY:
      return "Duplicate key";
    case SBT_ERR_KEY_NOT_FOUND:
      return "Key not found";
    case SBT_ERR_TREE_CORRUPTED:
      return "Tree structure corrupted";
    case SBT_ERR_NODE_INVALID:
      return "Invalid tree node";
      
    // Parameter and state errors
    case SBT_ERR_INVALID_ARGUMENT:
      return "Invalid argument";
    case SBT_ERR_NULL_POINTER:
      return "Null pointer";
    case SBT_ERR_BUFFER_TOO_SMALL:
      return "Buffer too small";
    case SBT_ERR_INVALID_STATE:
      return "Invalid state";
      
    // Resource errors
    case SBT_ERR_RESOURCE_BUSY:
      return "Resource busy";
    case SBT_ERR_RESOURCE_EXHAUSTED:
      return "Resource exhausted";
    case SBT_ERR_TIMEOUT:
      return "Operation timeout";
      
    // Generic and unknown errors
    case SBT_ERR_NOT_IMPLEMENTED:
      return "Not implemented";
    case SBT_ERR_GENERIC:
      return "Generic error";
    case SBT_ERR_UNKNOWN:
    default:
      return "Unknown error";
  }
}

/** Convert severity level to string */
const char *sbt_severity_to_string(sbt_error_severity_t severity) {
  switch (severity) {
    case SBT_SEVERITY_INFO:
      return "INFO";
    case SBT_SEVERITY_WARNING:
      return "WARNING";
    case SBT_SEVERITY_ERROR:
      return "ERROR";
    case SBT_SEVERITY_FATAL:
      return "FATAL";
    default:
      return "UNKNOWN";
  }
}

/** Log error message */
void sbt_log_error(const char *format, ...) {
  va_list args;
  va_start(args, format);
  
  char buffer[1024];
  vsnprintf(buffer, sizeof(buffer), format, args);
  
  // Use stderr for logging
  fprintf(stderr, "[ERROR] SBT: %s\n", buffer);
  
  va_end(args);
}

/** Log warning message */
void sbt_log_warning(const char *format, ...) {
  va_list args;
  va_start(args, format);
  
  char buffer[1024];
  vsnprintf(buffer, sizeof(buffer), format, args);
  
  // Use stderr for logging
  fprintf(stderr, "[WARNING] SBT: %s\n", buffer);
  
  va_end(args);
}

/** Log info message */
void sbt_log_info(const char *format, ...) {
  va_list args;
  va_start(args, format);
  
  char buffer[1024];
  vsnprintf(buffer, sizeof(buffer), format, args);
  
  // Use stdout for logging
  printf("[INFO] SBT: %s\n", buffer);
  
  va_end(args);
}

/** Log debug message */
void sbt_log_debug(const char *format, ...) {
  va_list args;
  va_start(args, format);
  
  char buffer[1024];
  vsnprintf(buffer, sizeof(buffer), format, args);
  
  #ifdef MYSQL_SERVER
    #ifdef UNIV_DEBUG
      LogErr(INFORMATION_LEVEL, ER_IB_MSG_GENERIC, "SBT-DEBUG", buffer);
    #endif
  #else
    #ifdef DEBUG
      printf("[DEBUG] SBT: %s\n", buffer);
    #endif
  #endif
  
  va_end(args);
}

/** Allocate memory */
void *sbt_malloc(size_t size) {
  return my_malloc(PSI_NOT_INSTRUMENTED, size, MYF(MY_WME));
}

/** Free memory */
void sbt_free(void *ptr) {
  if (ptr) {
    my_free(ptr);
  }
}

/** Reallocate memory */
void *sbt_realloc(void *ptr, size_t size) {
  return my_realloc(PSI_NOT_INSTRUMENTED, ptr, size, MYF(MY_WME));
}

/** Compare record data */
int sbt_data_compare(const uchar *data1, uint length1, 
                     const uchar *data2, uint length2) {
  if (!data1 || !data2) {
    if (data1 == data2) return 0;
    return data1 ? 1 : -1;
  }
  
  // Compare lengths first
  if (length1 != length2) {
    return (length1 < length2) ? -1 : 1;
  }
  
  // Compare data content
  return memcmp(data1, data2, length1);
}

/** CRC32 lookup table for polynomial 0xEDB88320 */
static uint32_t crc32_table[256];
static bool crc32_table_initialized = false;

/** Initialize CRC32 lookup table */
static void init_crc32_table() {
  if (crc32_table_initialized) {
    return;
  }
  
  const uint32_t polynomial = 0xEDB88320;
  
  for (uint32_t i = 0; i < 256; i++) {
    uint32_t crc = i;
    for (int j = 0; j < 8; j++) {
      if (crc & 1) {
        crc = (crc >> 1) ^ polynomial;
      } else {
        crc >>= 1;
      }
    }
    crc32_table[i] = crc;
  }
  
  crc32_table_initialized = true;
}

/** Calculate CRC32 checksum */
uint32_t sbt_crc32(const uchar *data, size_t length) {
  init_crc32_table();
  
  uint32_t crc = 0xFFFFFFFF;
  
  for (size_t i = 0; i < length; i++) {
    uint8_t table_index = (crc ^ data[i]) & 0xFF;
    crc = (crc >> 8) ^ crc32_table[table_index];
  }
  
  return crc ^ 0xFFFFFFFF;
}

/** Update CRC32 checksum with additional data */
uint32_t sbt_crc32_update(uint32_t crc, const uchar *data, size_t length) {
  init_crc32_table();
  
  // Convert back to working CRC
  crc ^= 0xFFFFFFFF;
  
  for (size_t i = 0; i < length; i++) {
    uint8_t table_index = (crc ^ data[i]) & 0xFF;
    crc = (crc >> 8) ^ crc32_table[table_index];
  }
  
  return crc ^ 0xFFFFFFFF;
}

/** Get current time in microseconds since epoch */
uint64_t sbt_get_current_time() {
  struct timeval tv;
  if (gettimeofday(&tv, nullptr) != 0) {
    return 0;
  }
  
  return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

/** Align offset to specified boundary */
uint64_t sbt_align_offset(uint64_t offset, uint32_t alignment) {
  if (alignment == 0) {
    return offset;
  }
  
  uint64_t remainder = offset % alignment;
  if (remainder == 0) {
    return offset;
  }
  
  return offset + (alignment - remainder);
}

/** Initialize error context */
void sbt_error_context_init(SBT_error_context *ctx) {
  if (!ctx) return;
  
  memset(ctx, 0, sizeof(SBT_error_context));
  ctx->error_code = SBT_SUCCESS;
  ctx->severity = SBT_SEVERITY_INFO;
  ctx->timestamp = sbt_get_current_time();
  
  #ifdef MYSQL_SERVER
    ctx->thread_id = my_thread_id();
  #else
    ctx->thread_id = 0;
  #endif
}

/** Set error context with detailed information */
void sbt_error_context_set(SBT_error_context *ctx, sbt_error_t error_code,
                          sbt_error_severity_t severity, const char *file,
                          int line, const char *function, const char *format, ...) {
  if (!ctx) return;
  
  ctx->error_code = error_code;
  ctx->severity = severity;
  ctx->file = file;
  ctx->line = line;
  ctx->function = function;
  ctx->timestamp = sbt_get_current_time();
  
  #ifdef MYSQL_SERVER
    ctx->thread_id = my_thread_id();
  #else
    ctx->thread_id = 0;
  #endif
  
  // Format the error message
  if (format) {
    va_list args;
    va_start(args, format);
    vsnprintf(ctx->message, sizeof(ctx->message), format, args);
    va_end(args);
  } else {
    strncpy(ctx->message, sbt_error_to_string(error_code), sizeof(ctx->message) - 1);
    ctx->message[sizeof(ctx->message) - 1] = '\0';
  }
}

/** Log error context */
void sbt_error_context_log(const SBT_error_context *ctx) {
  if (!ctx) return;
  
  const char *severity_str = sbt_severity_to_string(ctx->severity);
  const char *error_str = sbt_error_to_string(ctx->error_code);
  
  char log_message[2048];
  snprintf(log_message, sizeof(log_message),
           "[%s] %s (%d) in %s() at %s:%d - %s (Thread: %u, Time: %llu)",
           severity_str, error_str, (int)ctx->error_code,
           ctx->function ? ctx->function : "unknown",
           ctx->file ? ctx->file : "unknown", ctx->line,
           ctx->message, ctx->thread_id, 
           (unsigned long long)ctx->timestamp);
  
  // Log based on severity
  switch (ctx->severity) {
    case SBT_SEVERITY_INFO:
      sbt_log_info("%s", log_message);
      break;
    case SBT_SEVERITY_WARNING:
      sbt_log_warning("%s", log_message);
      break;
    case SBT_SEVERITY_ERROR:
    case SBT_SEVERITY_FATAL:
      sbt_log_error("%s", log_message);
      break;
  }
}

/** Clear error context */
void sbt_error_context_clear(SBT_error_context *ctx) {
  if (!ctx) return;
  
  sbt_error_context_init(ctx);
}