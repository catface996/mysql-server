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

/** @file include/sbt_tree.h
 SBT Tree Data Structure Implementation

 Created 2025-01-25
 *******************************************************/

#ifndef sbt_tree_h
#define sbt_tree_h

#include "sbt_common.h"
#include "sbt_raii.h"
#include "my_alloc.h"

// SBT Node Structure
struct SBT_node {
  uchar *data;                    // Record data
  uint data_length;               // Data length in bytes
  sbt_insert_id_t insert_id;      // Insert order ID (for sorting only)
  SBT_node *left;                 // Left child
  SBT_node *right;                // Right child
  uint size;                      // Size of subtree (including self)
};

/** SBT Tree Class
 * 
 * Implements Size Balanced Tree data structure for storing table records.
 * Records are ordered by insert_id for tree balancing, but this is not
 * a primary key - all record operations use full table scan.
 */
class SBT_tree {
private:
  SBT_node *root;                 // Root node of the tree
  MEM_ROOT mem_root;              // Memory allocator for nodes
  sbt_insert_id_t next_insert_id; // Next insert ID to assign
  uint64_t record_count;          // Total number of records

public:
  /** Constructor
   * Initialize empty SBT tree
   */
  SBT_tree();

  /** Destructor
   * Clean up all allocated memory
   */
  ~SBT_tree();

  /** Insert a new record
   * @param[in] data Record data to insert
   * @param[in] length Length of record data
   * @return SBT_SUCCESS on success, error code on failure
   */
  int insert(const uchar *data, uint length);

  /** Remove a record by data content
   * @param[in] data Record data to remove
   * @param[in] length Length of record data
   * @return SBT_SUCCESS on success, error code on failure
   */
  int remove(const uchar *data, uint length);

  /** Update a record by replacing old data with new data
   * @param[in] old_data Old record data to find
   * @param[in] old_length Length of old record data
   * @param[in] new_data New record data to replace with
   * @param[in] new_length Length of new record data
   * @return SBT_SUCCESS on success, error code on failure
   */
  int update(const uchar *old_data, uint old_length,
             const uchar *new_data, uint new_length);

  /** Find a record by data content
   * @param[in] data Record data to find
   * @param[in] length Length of record data
   * @return Pointer to node if found, nullptr if not found
   */
  SBT_node *find_by_data(const uchar *data, uint length);

  /** Get the first record in in-order traversal
   * @return Pointer to first node, nullptr if tree is empty
   */
  SBT_node *get_first();

  /** Get the next record in in-order traversal
   * @param[in] current Current node
   * @return Pointer to next node, nullptr if no more records
   */
  SBT_node *get_next(SBT_node *current);

  /** Get total number of records
   * @return Number of records in the tree
   */
  uint64_t get_record_count() const { return record_count; }

  /** Get next insert ID (for serialization)
   * @return Next insert ID that would be assigned
   */
  sbt_insert_id_t get_next_insert_id() const { return next_insert_id; }

  /** Set next insert ID (for deserialization)
   * @param[in] id Next insert ID to set
   */
  void set_next_insert_id(sbt_insert_id_t id) { next_insert_id = id; }

  /** Clear all records from the tree
   */
  void clear();

  /** Check if tree is empty
   * @return true if empty, false otherwise
   */
  bool is_empty() const { return root == nullptr; }

  /** Allocate a new node (for deserialization)
   * @return Pointer to allocated node, nullptr on failure
   */
  SBT_node *allocate_node();

  /** Allocate memory from tree's memory pool
   * @param[in] size Size of memory to allocate
   * @return Pointer to allocated memory, nullptr on failure
   */
  void *allocate_memory(size_t size);

  /** Set root node (for deserialization)
   * @param[in] new_root New root node
   */
  void set_root(SBT_node *new_root);

  /** Get root node (for serialization)
   * @return Pointer to root node
   */
  SBT_node *get_root() const { return root; }

private:
  /** Insert a node into the tree (recursive)
   * @param[in] node Current node (may be nullptr)
   * @param[in] data Record data to insert
   * @param[in] length Length of record data
   * @param[in] insert_id Insert ID for the new record
   * @return Pointer to the (possibly new) root of this subtree
   */
  SBT_node *insert_node(SBT_node *node, const uchar *data, uint length,
                         sbt_insert_id_t insert_id);

  /** Remove a node from the tree (recursive)
   * @param[in] node Current node (may be nullptr)
   * @param[in] data Record data to remove
   * @param[in] length Length of record data
   * @return Pointer to the (possibly new) root of this subtree
   */
  SBT_node *remove_node(SBT_node *node, const uchar *data, uint length);

  /** Maintain SBT balance property
   * @param[in] node Node to maintain
   * @param[in] flag Direction flag for maintain operation
   * @return Pointer to the (possibly new) root of this subtree
   */
  SBT_node *maintain(SBT_node *node, bool flag);

  /** Perform left rotation
   * @param[in] node Node to rotate
   * @return Pointer to new root of this subtree
   */
  SBT_node *rotate_left(SBT_node *node);

  /** Perform right rotation
   * @param[in] node Node to rotate
   * @return Pointer to new root of this subtree
   */
  SBT_node *rotate_right(SBT_node *node);

  /** Update size of a node based on its children
   * @param[in] node Node to update
   */
  void update_size(SBT_node *node);

  /** Get size of a node (0 if nullptr)
   * @param[in] node Node to get size of
   * @return Size of the node's subtree
   */
  uint get_size(SBT_node *node) const;

  /** Create a new node
   * @param[in] data Record data
   * @param[in] length Length of record data
   * @param[in] insert_id Insert ID for the record
   * @return Pointer to new node, nullptr on failure
   */
  SBT_node *create_node(const uchar *data, uint length, 
                        sbt_insert_id_t insert_id);

  /** Find minimum node in subtree
   * @param[in] node Root of subtree
   * @return Pointer to minimum node
   */
  SBT_node *find_min(SBT_node *node);

  /** Find a node by data content (recursive)
   * @param[in] node Current node
   * @param[in] data Record data to find
   * @param[in] length Length of record data
   * @return Pointer to node if found, nullptr if not found
   */
  SBT_node *find_by_data_recursive(SBT_node *node, const uchar *data, uint length);

  /** Find next node by insert_id (for in-order traversal)
   * @param[in] node Current node
   * @param[in] current_id Current insert_id
   * @return Pointer to next node, nullptr if not found
   */
  SBT_node *find_next_by_insert_id(SBT_node *node, sbt_insert_id_t current_id);
};

#endif /* sbt_tree_h */