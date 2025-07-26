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

/** @file sbt_file.cc
 SBT File Management Implementation - Skeleton

 Created 2025-01-25
 *******************************************************/

#include "my_config.h"
#include "../include/sbt_file.h"
#include "../include/sbt_common.h"
#include "../include/sbt_raii.h"
#include "my_sys.h"
#include "my_io.h"

/** Constructor */
SBT_file::SBT_file() 
    : fd(-1), 
      file_name(nullptr), 
      is_open(false) {
}

/** Destructor */
SBT_file::~SBT_file() {
  if (is_open) {
    close();
  }
  if (file_name) {
    sbt_free(file_name);
  }
}

/** Create new file with exception safety */
int SBT_file::create(const char *name) {
  if (!name) {
    return SBT_ERR_INVALID_ARGUMENT;
  }

  // Use RAII file guard for automatic cleanup
  SBT_file_guard file_guard(my_create(name, 0, O_RDWR | O_TRUNC, MYF(MY_WME)));
  if (!file_guard.is_valid()) {
    return SBT_ERR_IO_ERROR;
  }

  // Use RAII memory guard for file name
  size_t name_len = strlen(name);
  SBT_memory_guard name_guard = sbt_make_memory_guard(name_len + 1);
  if (!name_guard.is_valid()) {
    return SBT_ERR_OUT_OF_MEMORY;
  }
  strcpy(name_guard.get_as<char>(), name);

  // Initialize file header with proper values
  SBT_header header;
  memset(&header, 0, sizeof(header));
  
  // Set magic number
  memcpy(header.magic, SBT_FILE_MAGIC, SBT_FILE_MAGIC_SIZE);
  
  // Set version and basic info
  header.version = SBT_FILE_VERSION;
  header.record_count = 0;
  header.next_insert_id = 1;
  header.tree_root_offset = SBT_HEADER_SIZE;  // Tree data starts after header
  header.tree_data_size = 0;                 // No tree data initially
  header.header_size = SBT_HEADER_SIZE;
  
  // Set timestamps
  uint64_t current_time = sbt_get_current_time();
  header.created_time = current_time;
  header.modified_time = current_time;
  
  // Temporarily set fd for header writing
  fd = file_guard.get();
  is_open = true;
  
  // Write header to file
  int error = write_header(&header);
  if (error != SBT_SUCCESS) {
    fd = -1;
    is_open = false;
    return error;
  }

  // Flush to ensure data is written
  error = flush();
  if (error != SBT_SUCCESS) {
    fd = -1;
    is_open = false;
    return error;
  }

  // Success - transfer ownership to this object
  fd = file_guard.release();
  file_name = static_cast<char*>(name_guard.release());
  is_open = true;

  return SBT_SUCCESS;
}

/** Open existing file with exception safety */
int SBT_file::open(const char *name) {
  if (!name) {
    return SBT_ERR_INVALID_ARGUMENT;
  }

  // Use RAII file guard for automatic cleanup
  SBT_file_guard file_guard(my_open(name, O_RDWR, MYF(MY_WME)));
  if (!file_guard.is_valid()) {
    return SBT_ERR_FILE_NOT_FOUND;
  }

  // Use RAII memory guard for file name
  size_t name_len = strlen(name);
  SBT_memory_guard name_guard = sbt_make_memory_guard(name_len + 1);
  if (!name_guard.is_valid()) {
    return SBT_ERR_OUT_OF_MEMORY;
  }
  strcpy(name_guard.get_as<char>(), name);

  // Temporarily set fd for header reading
  fd = file_guard.get();
  is_open = true;

  // Read and validate header
  SBT_header header;
  int error = read_header(&header);
  if (error != SBT_SUCCESS) {
    fd = -1;
    is_open = false;
    return error;
  }

  // Validate file format version
  if (header.version != SBT_FILE_VERSION) {
    fd = -1;
    is_open = false;
    return SBT_ERR_CORRUPTED_DATA;
  }

  // Validate header size
  if (header.header_size != SBT_HEADER_SIZE) {
    fd = -1;
    is_open = false;
    return SBT_ERR_CORRUPTED_DATA;
  }

  // Success - transfer ownership to this object
  fd = file_guard.release();
  file_name = static_cast<char*>(name_guard.release());
  is_open = true;

  return SBT_SUCCESS;
}

/** Close file */
int SBT_file::close() {
  if (is_open && fd >= 0) {
    my_close(fd, MYF(0));
    fd = -1;
    is_open = false;
  }
  return SBT_SUCCESS;
}

/** Load tree from file with exception safety */
int SBT_file::load_tree(SBT_tree *tree) {
  if (!tree || !is_open) {
    return SBT_ERR_INVALID_ARGUMENT;
  }

  // Save original tree state for rollback
  SBT_node *original_root = tree->get_root();
  sbt_insert_id_t original_next_id = tree->get_next_insert_id();
  [[maybe_unused]] uint64_t original_record_count = tree->get_record_count();
  
  SBT_transaction_guard transaction([&]() {
    // Rollback tree state on failure
    tree->set_root(original_root);
    tree->set_next_insert_id(original_next_id);
  });

  // Clear existing tree data
  tree->clear();

  // Read file header
  SBT_header header;
  int error = read_header(&header);
  if (error != SBT_SUCCESS) {
    return error;
  }

  // If no tree data, return success with empty tree
  if (header.tree_data_size == 0 || header.record_count == 0) {
    tree->set_next_insert_id(header.next_insert_id);
    transaction.commit();
    return SBT_SUCCESS;
  }

  // Use RAII memory guard for buffer
  SBT_memory_guard buffer_guard = sbt_make_memory_guard(header.tree_data_size);
  if (!buffer_guard.is_valid()) {
    return SBT_ERR_OUT_OF_MEMORY;
  }

  // Read tree data from file
  if (my_pread(fd, buffer_guard.get_as<uchar>(), header.tree_data_size, 
               header.tree_root_offset, MYF(MY_NABP)) != 0) {
    return SBT_ERR_IO_ERROR;
  }

  // Deserialize tree from buffer
  uint offset = 0;
  SBT_node *root = nullptr;
  error = deserialize_tree(&root, buffer_guard.get_as<uchar>(), offset, 
                          header.tree_data_size, tree);
  
  if (error != SBT_SUCCESS) {
    return error;
  }

  // Set tree properties
  tree->set_root(root);
  tree->set_next_insert_id(header.next_insert_id);

  transaction.commit();
  return SBT_SUCCESS;
}

/** Save tree to file with exception safety */
int SBT_file::save_tree(SBT_tree *tree) {
  if (!tree || !is_open) {
    return SBT_ERR_INVALID_ARGUMENT;
  }

  // Calculate required buffer size for serialization
  uint buffer_size = calculate_serialize_size(tree->get_root());
  
  // Prepare file header
  SBT_header header;
  memset(&header, 0, sizeof(header));
  memcpy(header.magic, SBT_FILE_MAGIC, SBT_FILE_MAGIC_SIZE);
  header.version = SBT_FILE_VERSION;
  header.record_count = tree->get_record_count();
  header.next_insert_id = tree->get_next_insert_id();
  header.tree_root_offset = SBT_HEADER_SIZE;
  header.tree_data_size = buffer_size;
  header.header_size = SBT_HEADER_SIZE;
  header.modified_time = sbt_get_current_time();

  // If tree is empty, just write header
  if (buffer_size == 0 || tree->is_empty()) {
    header.tree_data_size = 0;
    return write_header(&header);
  }

  // Use RAII memory guard for buffer
  SBT_memory_guard buffer_guard = sbt_make_memory_guard(buffer_size);
  if (!buffer_guard.is_valid()) {
    return SBT_ERR_OUT_OF_MEMORY;
  }

  // Serialize tree to buffer
  uint offset = 0;
  int error = serialize_tree(tree->get_root(), buffer_guard.get_as<uchar>(), 
                            offset, buffer_size);
  if (error != SBT_SUCCESS) {
    return error;
  }

  // Create backup of current file position for rollback
  my_off_t original_pos = my_tell(fd, MYF(0));
  
  SBT_transaction_guard transaction([&]() {
    // Attempt to restore file position on failure
    if (original_pos != MY_FILEPOS_ERROR) {
      my_seek(fd, original_pos, MY_SEEK_SET, MYF(0));
    }
  });

  // Write header first
  error = write_header(&header);
  if (error != SBT_SUCCESS) {
    return error;
  }

  // Write tree data
  if (my_pwrite(fd, buffer_guard.get_as<uchar>(), buffer_size, 
                header.tree_root_offset, MYF(MY_NABP)) != 0) {
    return SBT_ERR_IO_ERROR;
  }

  // Flush to ensure data is written
  error = flush();
  if (error != SBT_SUCCESS) {
    return error;
  }

  transaction.commit();
  return SBT_SUCCESS;
}

/** Delete file */
int SBT_file::delete_file(const char *name) {
  if (!name) {
    return SBT_ERR_INVALID_ARGUMENT;
  }

  if (my_delete(name, MYF(0)) != 0) {
    return SBT_ERR_IO_ERROR;
  }

  return SBT_SUCCESS;
}

/** Check if file exists */
bool SBT_file::file_exists(const char *name) {
  if (!name) {
    return false;
  }

  // Try to open file for reading
  File test_fd = my_open(name, O_RDONLY, MYF(0));
  if (test_fd < 0) {
    return false;
  }

  my_close(test_fd, MYF(0));
  return true;
}

/** Get file size */
my_off_t SBT_file::get_file_size() {
  if (!is_open || fd < 0) {
    return -1;
  }

  // Get current position
  my_off_t current_pos = my_tell(fd, MYF(0));
  if (current_pos == MY_FILEPOS_ERROR) {
    return -1;
  }

  // Seek to end to get file size
  my_off_t file_size = my_seek(fd, 0, MY_SEEK_END, MYF(0));
  if (file_size == MY_FILEPOS_ERROR) {
    return -1;
  }

  // Restore original position
  if (my_seek(fd, current_pos, MY_SEEK_SET, MYF(0)) == MY_FILEPOS_ERROR) {
    return -1;
  }

  return file_size;
}

/** Private helper methods - skeleton implementations */

int SBT_file::write_header(const SBT_header *header) {
  if (!header || !is_open) {
    return SBT_ERR_INVALID_ARGUMENT;
  }

  // Create a copy to calculate checksum
  SBT_header temp_header = *header;
  temp_header.checksum = 0;  // Clear checksum field before calculation
  
  // Calculate CRC32 checksum of header (excluding checksum field)
  temp_header.checksum = calculate_header_checksum(&temp_header);

  // Write header to beginning of file
  if (my_pwrite(fd, (uchar *)&temp_header, sizeof(temp_header), 0, MYF(MY_NABP)) != 0) {
    return SBT_ERR_IO_ERROR;
  }

  return SBT_SUCCESS;
}

int SBT_file::read_header(SBT_header *header) {
  if (!header || !is_open) {
    return SBT_ERR_INVALID_ARGUMENT;
  }

  // Read header from beginning of file
  if (my_pread(fd, (uchar *)header, sizeof(*header), 0, MYF(MY_NABP)) != 0) {
    return SBT_ERR_IO_ERROR;
  }

  // Verify magic number
  if (memcmp(header->magic, SBT_FILE_MAGIC, SBT_FILE_MAGIC_SIZE) != 0) {
    return SBT_ERR_CORRUPTED_DATA;
  }

  // Verify checksum
  if (!verify_header_checksum(header)) {
    return SBT_ERR_CORRUPTED_DATA;
  }

  return SBT_SUCCESS;
}

int SBT_file::serialize_tree(SBT_node *node, uchar *buffer, uint &offset, uint buffer_size) {
  // Check buffer bounds
  if (offset + SBT_SERIALIZED_NODE_HEADER_SIZE > buffer_size) {
    return SBT_ERR_OUT_OF_MEMORY;
  }

  SBT_serialized_node *serialized = (SBT_serialized_node *)(buffer + offset);
  
  if (node == nullptr) {
    // Serialize null node
    serialized->has_node = 0;
    serialized->insert_id = 0;
    serialized->data_length = 0;
    serialized->size = 0;
    offset += SBT_SERIALIZED_NODE_HEADER_SIZE;
    return SBT_SUCCESS;
  }

  // Serialize existing node
  serialized->has_node = 1;
  serialized->insert_id = node->insert_id;
  serialized->data_length = node->data_length;
  serialized->size = node->size;
  
  offset += SBT_SERIALIZED_NODE_HEADER_SIZE;

  // Check space for record data
  if (offset + node->data_length > buffer_size) {
    return SBT_ERR_OUT_OF_MEMORY;
  }

  // Copy record data
  if (node->data_length > 0 && node->data) {
    memcpy(buffer + offset, node->data, node->data_length);
    offset += node->data_length;
  }

  // Align offset for next node
  offset = sbt_align_offset(offset, SBT_FILE_ALIGNMENT);

  // Recursively serialize left subtree
  int error = serialize_tree(node->left, buffer, offset, buffer_size);
  if (error != SBT_SUCCESS) {
    return error;
  }

  // Recursively serialize right subtree
  error = serialize_tree(node->right, buffer, offset, buffer_size);
  if (error != SBT_SUCCESS) {
    return error;
  }

  return SBT_SUCCESS;
}

int SBT_file::deserialize_tree(SBT_node **node, const uchar *buffer, uint &offset,
                                uint buffer_size, SBT_tree *tree) {
  *node = nullptr;

  // Check buffer bounds
  if (offset + SBT_SERIALIZED_NODE_HEADER_SIZE > buffer_size) {
    return SBT_ERR_CORRUPTED_DATA;
  }

  const SBT_serialized_node *serialized = (const SBT_serialized_node *)(buffer + offset);
  
  // Check if this is a null node
  if (serialized->has_node == 0) {
    offset += SBT_SERIALIZED_NODE_HEADER_SIZE;
    return SBT_SUCCESS;
  }

  // Validate data length
  if (serialized->data_length > SBT_MAX_RECORD_SIZE) {
    return SBT_ERR_CORRUPTED_DATA;
  }

  offset += SBT_SERIALIZED_NODE_HEADER_SIZE;

  // Check space for record data
  if (offset + serialized->data_length > buffer_size) {
    return SBT_ERR_CORRUPTED_DATA;
  }

  // Allocate new node using tree's memory management
  *node = tree->allocate_node();
  if (*node == nullptr) {
    return SBT_ERR_OUT_OF_MEMORY;
  }

  // Set node properties
  (*node)->insert_id = serialized->insert_id;
  (*node)->data_length = serialized->data_length;
  (*node)->size = serialized->size;
  (*node)->left = nullptr;
  (*node)->right = nullptr;

  // Copy record data if present
  if (serialized->data_length > 0) {
    (*node)->data = (uchar *)tree->allocate_memory(serialized->data_length);
    if ((*node)->data == nullptr) {
      return SBT_ERR_OUT_OF_MEMORY;
    }
    memcpy((*node)->data, buffer + offset, serialized->data_length);
    offset += serialized->data_length;
  } else {
    (*node)->data = nullptr;
  }

  // Align offset for next node
  offset = sbt_align_offset(offset, SBT_FILE_ALIGNMENT);

  // Recursively deserialize left subtree
  int error = deserialize_tree(&((*node)->left), buffer, offset, buffer_size, tree);
  if (error != SBT_SUCCESS) {
    return error;
  }

  // Recursively deserialize right subtree
  error = deserialize_tree(&((*node)->right), buffer, offset, buffer_size, tree);
  if (error != SBT_SUCCESS) {
    return error;
  }

  return SBT_SUCCESS;
}

uint SBT_file::calculate_serialize_size(SBT_node *node) {
  if (node == nullptr) {
    return SBT_SERIALIZED_NODE_HEADER_SIZE;
  }

  uint size = SBT_SERIALIZED_NODE_HEADER_SIZE;
  
  // Add space for record data
  size += node->data_length;
  
  // Add alignment padding
  size = sbt_align_offset(size, SBT_FILE_ALIGNMENT);
  
  // Recursively calculate size for subtrees
  size += calculate_serialize_size(node->left);
  size += calculate_serialize_size(node->right);
  
  return size;
}

uint32_t SBT_file::calculate_header_checksum(const SBT_header *header) {
  // Calculate CRC32 checksum of header excluding checksum field
  const uchar *data = (const uchar *)header;
  size_t checksum_offset = offsetof(SBT_header, checksum);
  
  // Calculate CRC32 for data before checksum field
  uint32_t crc = sbt_crc32(data, checksum_offset);
  
  // Skip checksum field and continue with remaining data
  size_t remaining_offset = checksum_offset + sizeof(header->checksum);
  size_t remaining_size = sizeof(SBT_header) - remaining_offset;
  
  if (remaining_size > 0) {
    crc = sbt_crc32_update(crc, data + remaining_offset, remaining_size);
  }
  
  return crc;
}

bool SBT_file::verify_header_checksum(const SBT_header *header) {
  // Create a copy and clear checksum field
  SBT_header temp_header = *header;
  uint32_t original_checksum = temp_header.checksum;
  temp_header.checksum = 0;
  
  // Calculate expected checksum
  uint32_t calculated_checksum = calculate_header_checksum(&temp_header);
  
  return calculated_checksum == original_checksum;
}

int SBT_file::flush() {
  if (!is_open || fd < 0) {
    return SBT_ERR_INVALID_ARGUMENT;
  }

  if (my_sync(fd, MYF(MY_WME)) != 0) {
    return SBT_ERR_IO_ERROR;
  }

  return SBT_SUCCESS;
}