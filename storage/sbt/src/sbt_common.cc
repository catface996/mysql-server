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