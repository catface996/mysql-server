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

/** @file include/sbt_file.h
 SBT File Management for Data Persistence

 Created 2025-01-25
 *******************************************************/

#ifndef sbt_file_h
#define sbt_file_h

#include "sbt_common.h"
#include "sbt_tree.h"
#include "my_io.h"

/** SBT File Manager Class
 * 
 * Handles persistence of SBT tree data to disk files.
 * Implements serialization and deserialization of tree structures.
 */
class SBT_file {
private:
  File fd;                        // File descriptor
  char *file_name;                // Full path to the data file
  bool is_open;                   // File open status

public:
  /** Constructor
   * Initialize file manager
   */
  SBT_file();

  /** Destructor
   * Clean up resources and close file if open
   */
  ~SBT_file();

  /** Create a new SBT data file
   * @param[in] name Full path to the file to create
   * @return SBT_SUCCESS on success, error code on failure
   */
  int create(const char *name);

  /** Open an existing SBT data file
   * @param[in] name Full path to the file to open
   * @return SBT_SUCCESS on success, error code on failure
   */
  int open(const char *name);

  /** Close the currently open file
   * @return SBT_SUCCESS on success, error code on failure
   */
  int close();

  /** Load tree data from file into memory
   * @param[in,out] tree Tree object to load data into
   * @return SBT_SUCCESS on success, error code on failure
   */
  int load_tree(SBT_tree *tree);

  /** Save tree data from memory to file
   * @param[in] tree Tree object to save data from
   * @return SBT_SUCCESS on success, error code on failure
   */
  int save_tree(SBT_tree *tree);

  /** Delete the data file from disk
   * @param[in] name Full path to the file to delete
   * @return SBT_SUCCESS on success, error code on failure
   */
  static int delete_file(const char *name);

  /** Check if file exists
   * @param[in] name Full path to check
   * @return true if file exists, false otherwise
   */
  static bool file_exists(const char *name);

  /** Get file size
   * @return File size in bytes, or -1 on error
   */
  my_off_t get_file_size();

private:
  /** Write file header to disk
   * @param[in] header Header structure to write
   * @return SBT_SUCCESS on success, error code on failure
   */
  int write_header(const SBT_header *header);

  /** Read file header from disk
   * @param[out] header Header structure to read into
   * @return SBT_SUCCESS on success, error code on failure
   */
  int read_header(SBT_header *header);

  /** Serialize tree to buffer (pre-order traversal)
   * @param[in] node Current node to serialize
   * @param[out] buffer Buffer to write serialized data
   * @param[in,out] offset Current offset in buffer
   * @param[in] buffer_size Total size of buffer
   * @return SBT_SUCCESS on success, error code on failure
   */
  int serialize_tree(SBT_node *node, uchar *buffer, uint &offset, uint buffer_size);

  /** Deserialize tree from buffer
   * @param[out] node Pointer to store deserialized node
   * @param[in] buffer Buffer containing serialized data
   * @param[in,out] offset Current offset in buffer
   * @param[in] buffer_size Total size of buffer
   * @param[in,out] tree Tree object for memory allocation
   * @return SBT_SUCCESS on success, error code on failure
   */
  int deserialize_tree(SBT_node **node, const uchar *buffer, uint &offset,
                       uint buffer_size, SBT_tree *tree);

  /** Calculate buffer size needed for serialization
   * @param[in] node Root node of tree to serialize
   * @return Required buffer size in bytes
   */
  uint calculate_serialize_size(SBT_node *node);

  /** Calculate checksum for header
   * @param[in] header Header to calculate checksum for
   * @return Calculated checksum
   */
  uint32_t calculate_header_checksum(const SBT_header *header);

  /** Verify header checksum
   * @param[in] header Header to verify
   * @return true if checksum is valid, false otherwise
   */
  bool verify_header_checksum(const SBT_header *header);

  /** Flush file buffers to disk
   * @return SBT_SUCCESS on success, error code on failure
   */
  int flush();
};

#endif /* sbt_file_h */