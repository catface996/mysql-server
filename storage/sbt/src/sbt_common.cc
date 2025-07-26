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
#include <sys/time.h>
#include <cstdarg>

/** Convert SBT error to MySQL error code */
int sbt_error_to_mysql_error(int sbt_error) {
  switch (sbt_error) {
    case SBT_SUCCESS:
      return 0;
    case SBT_ERR_OUT_OF_MEMORY:
      return HA_ERR_OUT_OF_MEM;
    case SBT_ERR_FILE_NOT_FOUND:
      return HA_ERR_NO_SUCH_TABLE;
    case SBT_ERR_CORRUPTED_DATA:
      return HA_ERR_CRASHED_ON_USAGE;
    case SBT_ERR_DUPLICATE_KEY:
      return HA_ERR_FOUND_DUPP_KEY;
    case SBT_ERR_IO_ERROR:
      return HA_ERR_CRASHED_ON_USAGE;
    case SBT_ERR_INVALID_ARGUMENT:
      return HA_ERR_WRONG_COMMAND;
    case SBT_ERR_GENERIC:
    default:
      return HA_ERR_GENERIC;
  }
}

/** Log error message */
void sbt_log_error(const char *format, ...) {
  va_list args;
  va_start(args, format);
  
  char buffer[1024];
  vsnprintf(buffer, sizeof(buffer), format, args);
  
  // TODO: Implement proper logging
  fprintf(stderr, "SBT Error: %s\n", buffer);
  
  va_end(args);
}

/** Log info message */
void sbt_log_info(const char *format, ...) {
  va_list args;
  va_start(args, format);
  
  char buffer[1024];
  vsnprintf(buffer, sizeof(buffer), format, args);
  
  // TODO: Implement proper logging
  printf("SBT Info: %s\n", buffer);
  
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