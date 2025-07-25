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
 * @file sbt_storage.cc
 * @brief SBT Storage Management Implementation
 * 
 * This file contains the implementation of disk I/O operations and
 * data persistence for the SBT storage engine.
 */

#include "sbt_storage.h"
#include "sbt_common.h"
#include "my_base.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstring>

// Static constants
const char SBT_storage::MAGIC_NUMBER[4] = {'S', 'B', 'T', '\0'};

// SBT_file_header implementation

SBT_file_header::SBT_file_header() {
    set_defaults();
}

void SBT_file_header::set_defaults() {
    memcpy(magic, SBT_storage::MAGIC_NUMBER, 4);
    version = SBT_storage::FILE_VERSION;
    record_count = 0;
    next_position = 1;
    table_checksum = 0;
    memset(reserved, 0, sizeof(reserved));
}

bool SBT_file_header::is_valid() const {
    return (memcmp(magic, SBT_storage::MAGIC_NUMBER, 4) == 0) &&
           (version == SBT_storage::FILE_VERSION);
}

// SBT_record_header implementation

SBT_record_header::SBT_record_header() 
    : record_length(0), position(0), checksum(0) {
}

SBT_record_header::SBT_record_header(uint32_t len, uint64_t pos)
    : record_length(len), position(pos), checksum(0) {
}

void SBT_record_header::calculate_checksum(const uchar *data) {
    checksum = SBT_utils::calculate_record_checksum(data, record_length);
}

bool SBT_record_header::verify_checksum(const uchar *data) const {
    uint32_t calculated = SBT_utils::calculate_record_checksum(data, record_length);
    return calculated == checksum;
}

// SBT_storage implementation

SBT_storage::SBT_storage(const char *path)
    : data_file(-1), file_path(path), is_open(false) {
    SBT_DBUG_ENTER("SBT_storage::SBT_storage");
    SBT_DBUG_VOID_RETURN;
}

SBT_storage::~SBT_storage() {
    SBT_DBUG_ENTER("SBT_storage::~SBT_storage");
    
    if (is_open) {
        close_file();
    }
    
    SBT_DBUG_VOID_RETURN;
}

bool SBT_storage::file_exists() const {
    SBT_DBUG_ENTER("SBT_storage::file_exists");
    
    struct stat st;
    bool exists = (stat(file_path.c_str(), &st) == 0);
    
    SBT_DBUG_RETURN(exists);
}

int SBT_storage::create_file() {
    SBT_DBUG_ENTER("SBT_storage::create_file");
    
    // Create file with read/write permissions
    data_file = open(file_path.c_str(), O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (data_file == -1) {
        SBT_DBUG_RETURN(SBT_error_handler::handle_io_error(errno, "create", file_path.c_str()));
    }
    
    is_open = true;
    
    // Initialize and write header
    header.set_defaults();
    int result = write_header();
    if (result != 0) {
        close_file();
        SBT_DBUG_RETURN(result);
    }
    
    SBT_DBUG_RETURN(0);
}

int SBT_storage::open_file(int mode) {
    SBT_DBUG_ENTER("SBT_storage::open_file");
    
    if (is_open) {
        SBT_DBUG_RETURN(0);
    }
    
    data_file = open(file_path.c_str(), mode);
    if (data_file == -1) {
        SBT_DBUG_RETURN(SBT_error_handler::handle_io_error(errno, "open", file_path.c_str()));
    }
    
    is_open = true;
    
    // Read and validate header
    int result = read_header();
    if (result != 0) {
        close_file();
        SBT_DBUG_RETURN(result);
    }
    
    SBT_DBUG_RETURN(0);
}

int SBT_storage::close_file() {
    SBT_DBUG_ENTER("SBT_storage::close_file");
    
    if (!is_open) {
        SBT_DBUG_RETURN(0);
    }
    
    int result = 0;
    if (close(data_file) == -1) {
        result = SBT_error_handler::handle_io_error(errno, "close", file_path.c_str());
    }
    
    data_file = -1;
    is_open = false;
    
    SBT_DBUG_RETURN(result);
}

int SBT_storage::delete_file() {
    SBT_DBUG_ENTER("SBT_storage::delete_file");
    
    if (is_open) {
        close_file();
    }
    
    if (unlink(file_path.c_str()) == -1 && errno != ENOENT) {
        SBT_DBUG_RETURN(SBT_error_handler::handle_io_error(errno, "delete", file_path.c_str()));
    }
    
    SBT_DBUG_RETURN(0);
}

int SBT_storage::read_header() {
    SBT_DBUG_ENTER("SBT_storage::read_header");
    
    int result = read_data(&header, HEADER_SIZE, 0);
    if (result != 0) {
        SBT_DBUG_RETURN(result);
    }
    
    if (!header.is_valid()) {
        SBT_DBUG_RETURN(HA_ERR_SBT_FILE_FORMAT);
    }
    
    SBT_DBUG_RETURN(0);
}

int SBT_storage::write_header() {
    SBT_DBUG_ENTER("SBT_storage::write_header");
    
    int result = write_data(&header, HEADER_SIZE, 0);
    if (result != 0) {
        SBT_DBUG_RETURN(result);
    }
    
    SBT_DBUG_RETURN(sync_file());
}

int SBT_storage::read_record(uint64_t offset, uchar *buffer, size_t &length, uint64_t &position) {
    SBT_DBUG_ENTER("SBT_storage::read_record");
    
    // TODO: Implement record reading
    
    SBT_DBUG_RETURN(HA_ERR_END_OF_FILE);
}

int SBT_storage::write_record(const uchar *data, size_t length, uint64_t position, uint64_t &offset) {
    SBT_DBUG_ENTER("SBT_storage::write_record");
    
    // TODO: Implement record writing
    offset = 0;
    
    SBT_DBUG_RETURN(0);
}

int SBT_storage::delete_record(uint64_t offset) {
    SBT_DBUG_ENTER("SBT_storage::delete_record");
    
    // TODO: Implement record deletion
    
    SBT_DBUG_RETURN(0);
}

int SBT_storage::update_record(uint64_t offset, const uchar *data, size_t length) {
    SBT_DBUG_ENTER("SBT_storage::update_record");
    
    // TODO: Implement record update
    
    SBT_DBUG_RETURN(0);
}

int SBT_storage::sync_file() {
    SBT_DBUG_ENTER("SBT_storage::sync_file");
    
    if (!is_open) {
        SBT_DBUG_RETURN(HA_ERR_INTERNAL_ERROR);
    }
    
    if (fsync(data_file) == -1) {
        SBT_DBUG_RETURN(SBT_error_handler::handle_io_error(errno, "sync", file_path.c_str()));
    }
    
    SBT_DBUG_RETURN(0);
}

int SBT_storage::truncate_file(off_t size) {
    SBT_DBUG_ENTER("SBT_storage::truncate_file");
    
    if (!is_open) {
        SBT_DBUG_RETURN(HA_ERR_INTERNAL_ERROR);
    }
    
    if (ftruncate(data_file, size) == -1) {
        SBT_DBUG_RETURN(SBT_error_handler::handle_io_error(errno, "truncate", file_path.c_str()));
    }
    
    SBT_DBUG_RETURN(0);
}

off_t SBT_storage::get_file_size() {
    SBT_DBUG_ENTER("SBT_storage::get_file_size");
    
    if (!is_open) {
        SBT_DBUG_RETURN(-1);
    }
    
    struct stat st;
    if (fstat(data_file, &st) == -1) {
        SBT_DBUG_RETURN(-1);
    }
    
    SBT_DBUG_RETURN(st.st_size);
}

int SBT_storage::validate_file() {
    SBT_DBUG_ENTER("SBT_storage::validate_file");
    
    // TODO: Implement file validation
    
    SBT_DBUG_RETURN(0);
}

int SBT_storage::repair_file() {
    SBT_DBUG_ENTER("SBT_storage::repair_file");
    
    // TODO: Implement file repair
    
    SBT_DBUG_RETURN(0);
}

// Private helper methods

int SBT_storage::read_data(void *buffer, size_t size, off_t offset) {
    SBT_DBUG_ENTER("SBT_storage::read_data");
    
    if (!is_open) {
        SBT_DBUG_RETURN(HA_ERR_INTERNAL_ERROR);
    }
    
    if (lseek(data_file, offset, SEEK_SET) == -1) {
        SBT_DBUG_RETURN(SBT_error_handler::handle_io_error(errno, "seek", file_path.c_str()));
    }
    
    ssize_t bytes_read = read(data_file, buffer, size);
    if (bytes_read == -1) {
        SBT_DBUG_RETURN(SBT_error_handler::handle_io_error(errno, "read", file_path.c_str()));
    }
    
    if (static_cast<size_t>(bytes_read) != size) {
        SBT_DBUG_RETURN(HA_ERR_END_OF_FILE);
    }
    
    SBT_DBUG_RETURN(0);
}

int SBT_storage::write_data(const void *buffer, size_t size, off_t offset) {
    SBT_DBUG_ENTER("SBT_storage::write_data");
    
    if (!is_open) {
        SBT_DBUG_RETURN(HA_ERR_INTERNAL_ERROR);
    }
    
    if (lseek(data_file, offset, SEEK_SET) == -1) {
        SBT_DBUG_RETURN(SBT_error_handler::handle_io_error(errno, "seek", file_path.c_str()));
    }
    
    ssize_t bytes_written = write(data_file, buffer, size);
    if (bytes_written == -1) {
        SBT_DBUG_RETURN(SBT_error_handler::handle_io_error(errno, "write", file_path.c_str()));
    }
    
    if (static_cast<size_t>(bytes_written) != size) {
        SBT_DBUG_RETURN(HA_ERR_RECORD_FILE_FULL);
    }
    
    SBT_DBUG_RETURN(0);
}

uint32_t SBT_storage::calculate_checksum(const void *data, size_t size) {
    // Simple checksum implementation
    const uchar *bytes = static_cast<const uchar*>(data);
    uint32_t checksum = 0;
    
    for (size_t i = 0; i < size; i++) {
        checksum = (checksum << 1) ^ bytes[i];
    }
    
    return checksum;
}