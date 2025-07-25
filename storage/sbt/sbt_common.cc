/* Copyright (c) 2024, Oracle and/or its affiliates.

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
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

/**
 * @file sbt_common.cc
 * @brief SBT Common Utilities Implementation
 * 
 * This file contains the implementation of common utility functions
 * and error handling for the SBT storage engine.
 */

#include "sbt_common.h"
#include "sql/log.h"
#include <cstdarg>
#include <cstring>
#include <cerrno>

// SBT_error_handler implementation

int SBT_error_handler::handle_io_error(int error_code, const char *operation, const char *file_name) {
    SBT_DBUG_ENTER("SBT_error_handler::handle_io_error");
    
    const char *error_msg = strerror(error_code);
    
    if (file_name) {
        log_error(ERROR_LEVEL, "SBT: I/O error during %s operation on file %s: %s", 
                 operation, file_name, error_msg);
    } else {
        log_error(ERROR_LEVEL, "SBT: I/O error during %s operation: %s", operation, error_msg);
    }
    
    int mysql_error = mysql_error_from_errno(error_code);
    SBT_DBUG_RETURN(mysql_error);
}

int SBT_error_handler::handle_memory_error(const char *operation) {
    SBT_DBUG_ENTER("SBT_error_handler::handle_memory_error");
    
    if (operation) {
        log_error(ERROR_LEVEL, "SBT: Memory allocation failed during %s", operation);
    } else {
        log_error(ERROR_LEVEL, "SBT: Memory allocation failed");
    }
    
    SBT_DBUG_RETURN(HA_ERR_OUT_OF_MEM);
}

int SBT_error_handler::handle_corruption_error(const char *table_name, const char *details) {
    SBT_DBUG_ENTER("SBT_error_handler::handle_corruption_error");
    
    if (details) {
        log_error(ERROR_LEVEL, "SBT: Data corruption detected in table %s: %s", table_name, details);
    } else {
        log_error(ERROR_LEVEL, "SBT: Data corruption detected in table %s", table_name);
    }
    
    SBT_DBUG_RETURN(HA_ERR_SBT_CORRUPTED_DATA);
}

void SBT_error_handler::log_error(int level, const char *format, ...) {
    va_list args;
    va_start(args, format);
    
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    
    // Use simple printf for now - can be enhanced later
    fprintf(stderr, "[SBT ERROR] %s\n", buffer);
    
    va_end(args);
}

void SBT_error_handler::log_warning(const char *format, ...) {
    va_list args;
    va_start(args, format);
    
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    
    fprintf(stderr, "[SBT WARNING] %s\n", buffer);
    
    va_end(args);
}

void SBT_error_handler::log_info(const char *format, ...) {
    va_list args;
    va_start(args, format);
    
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    
    fprintf(stderr, "[SBT INFO] %s\n", buffer);
    
    va_end(args);
}

int SBT_error_handler::mysql_error_from_errno(int sys_errno) {
    switch (sys_errno) {
        case ENOENT:
            return ENOENT;
        case EACCES:
            return HA_ERR_GENERIC;
        case ENOSPC:
            return HA_ERR_RECORD_FILE_FULL;
        case ENOMEM:
            return HA_ERR_OUT_OF_MEM;
        case EIO:
            return HA_ERR_CRASHED_ON_USAGE;
        default:
            return HA_ERR_GENERIC;
    }
}

const char* SBT_error_handler::get_error_message(int error_code) {
    switch (error_code) {
        case HA_ERR_SBT_CORRUPTED_DATA:
            return "SBT data corruption detected";
        case HA_ERR_SBT_INVALID_TREE:
            return "SBT tree structure is invalid";
        case HA_ERR_SBT_FILE_FORMAT:
            return "SBT file format is invalid";
        case HA_ERR_SBT_TREE_UNBALANCED:
            return "SBT tree is unbalanced";
        default:
            return "Unknown SBT error";
    }
}

// SBT_utils implementation

char* SBT_utils::make_table_name(const char *db_name, const char *table_name) {
    SBT_DBUG_ENTER("SBT_utils::make_table_name");
    
    size_t db_len = strlen(db_name);
    size_t table_len = strlen(table_name);
    size_t total_len = db_len + 1 + table_len + 1; // db.table\0
    
    char *result = static_cast<char*>(safe_malloc(total_len));
    if (result) {
        snprintf(result, total_len, "%s.%s", db_name, table_name);
    }
    
    SBT_DBUG_RETURN(result);
}

char* SBT_utils::make_file_name(const char *table_name, const char *extension) {
    SBT_DBUG_ENTER("SBT_utils::make_file_name");
    
    size_t table_len = strlen(table_name);
    size_t ext_len = strlen(extension);
    size_t total_len = table_len + ext_len + 1;
    
    char *result = static_cast<char*>(safe_malloc(total_len));
    if (result) {
        snprintf(result, total_len, "%s%s", table_name, extension);
    }
    
    SBT_DBUG_RETURN(result);
}

void SBT_utils::free_string(char *str) {
    if (str) {
        safe_free(str);
    }
}

void* SBT_utils::safe_malloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr && size > 0) {
        SBT_error_handler::handle_memory_error("malloc");
    }
    return ptr;
}

void* SBT_utils::safe_realloc(void *ptr, size_t size) {
    void *new_ptr = realloc(ptr, size);
    if (!new_ptr && size > 0) {
        SBT_error_handler::handle_memory_error("realloc");
    }
    return new_ptr;
}

void SBT_utils::safe_free(void *ptr) {
    if (ptr) {
        free(ptr);
    }
}

size_t SBT_utils::get_record_size(const uchar *record, TABLE *table) {
    SBT_DBUG_ENTER("SBT_utils::get_record_size");
    
    // TODO: Calculate actual record size based on table definition
    size_t size = table->s->reclength;
    
    SBT_DBUG_RETURN(size);
}

int SBT_utils::copy_record(uchar *dest, const uchar *src, TABLE *table) {
    SBT_DBUG_ENTER("SBT_utils::copy_record");
    
    size_t record_size = get_record_size(src, table);
    memcpy(dest, src, record_size);
    
    SBT_DBUG_RETURN(0);
}

int SBT_utils::compare_records(const uchar *rec1, const uchar *rec2, TABLE *table) {
    SBT_DBUG_ENTER("SBT_utils::compare_records");
    
    size_t record_size = get_record_size(rec1, table);
    int result = memcmp(rec1, rec2, record_size);
    
    SBT_DBUG_RETURN(result);
}

std::string SBT_utils::get_data_dir_path() {
    // TODO: Get actual MySQL data directory
    return "./";
}

std::string SBT_utils::build_file_path(const char *table_name) {
    SBT_DBUG_ENTER("SBT_utils::build_file_path");
    
    std::string data_dir = get_data_dir_path();
    std::string file_path = data_dir + table_name + SBT_FILE_EXTENSION;
    
    SBT_DBUG_RETURN(file_path);
}

bool SBT_utils::is_valid_table_name(const char *name) {
    if (!name || strlen(name) == 0) {
        return false;
    }
    
    // TODO: Add more comprehensive table name validation
    return true;
}

uint32_t SBT_utils::calculate_record_checksum(const uchar *data, size_t length) {
    uint32_t checksum = 0;
    
    for (size_t i = 0; i < length; i++) {
        checksum = (checksum << 1) ^ data[i];
    }
    
    return checksum;
}

uint32_t SBT_utils::calculate_table_checksum(TABLE *table) {
    // TODO: Calculate checksum based on table structure
    return 0;
}

bool SBT_utils::is_balanced_node(size_t left_size, size_t right_size, size_t total_size) {
    if (total_size == 0) return true;
    
    double left_ratio = static_cast<double>(left_size) / total_size;
    double right_ratio = static_cast<double>(right_size) / total_size;
    
    return (left_ratio <= SBT_BALANCE_ALPHA) && (right_ratio <= SBT_BALANCE_ALPHA);
}

double SBT_utils::get_balance_ratio(size_t left_size, size_t right_size) {
    size_t total = left_size + right_size;
    if (total == 0) return 0.0;
    
    size_t max_size = std::max(left_size, right_size);
    return static_cast<double>(max_size) / total;
}