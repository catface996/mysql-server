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

/** @file include/sbt_share.h
 SBT Shared Table Information Management

 Created 2025-01-25
 *******************************************************/

#ifndef sbt_share_h
#define sbt_share_h

#include "sbt_common.h"
#include "sbt_tree.h"
#include "sbt_file.h"
#include "sql/handler.h"
#include "thr_lock.h"
#include "mysql/psi/mysql_mutex.h"
#include "my_sys.h"
#include "my_sys.h"

/** SBT Share Class
 * 
 * Manages shared information for SBT tables, including reference counting,
 * locking, and shared resources like the tree structure and file manager.
 */
class SBT_share : public Handler_share {
private:
  THR_LOCK lock;                  // Table-level lock
  char *table_name;               // Full table name
  uint table_name_length;         // Length of table name
  uint use_count;                 // Reference count
  SBT_tree *tree;                 // Shared tree structure
  SBT_file *file;                 // File manager
  mysql_mutex_t mutex;            // Mutex for thread safety
  SBT_share *next;                // Next share in list

  // Simple share management (no hash table for now)
  static mysql_mutex_t sbt_mutex;
  static bool sbt_init_done;

public:
  /** Constructor
   * @param[in] table_name_arg Table name
   * @param[in] table_name_length_arg Length of table name
   */
  SBT_share(const char *table_name_arg, uint table_name_length_arg);

  /** Destructor
   * Clean up resources
   */
  ~SBT_share();

  /** Get shared table information
   * @param[in] table_name Full table name
   * @return Pointer to share object, nullptr on failure
   */
  static SBT_share *get_share(const char *table_name);

  /** Release shared table information
   * @param[in] share Share object to release
   */
  static void release_share(SBT_share *share);

  /** Initialize the share system
   * @return 0 on success, non-zero on failure
   */
  static int init_share_system();

  /** Cleanup the share system
   */
  static void cleanup_share_system();

  /** Get the table lock
   * @return Pointer to THR_LOCK structure
   */
  THR_LOCK *get_lock() { return &lock; }

  /** Get the tree structure
   * @return Pointer to SBT_tree object
   */
  SBT_tree *get_tree() { return tree; }

  /** Get the file manager
   * @return Pointer to SBT_file object
   */
  SBT_file *get_file() { return file; }

  /** Get table name
   * @return Table name string
   */
  const char *get_table_name() const { return table_name; }

  /** Get use count
   * @return Current reference count
   */
  uint get_use_count() const { return use_count; }

  /** Lock the share for exclusive access
   */
  void lock_share();

  /** Unlock the share
   */
  void unlock_share();

  /** Initialize table data (create tree and file objects)
   * @param[in] table_name Full path to table file
   * @return SBT_SUCCESS on success, error code on failure
   */
  int init_table_data(const char *table_name);

  /** Open table file and load data
   * @return SBT_SUCCESS on success, error code on failure
   */
  int open_table();

  /** Close table file and save data
   * @return SBT_SUCCESS on success, error code on failure
   */
  int close_table();

  /** Create new table file
   * @param[in] file_name Full path to table file
   * @return SBT_SUCCESS on success, error code on failure
   */
  int create_table(const char *file_name);

  /** Delete table file
   * @param[in] file_name Full path to table file
   * @return SBT_SUCCESS on success, error code on failure
   */
  int delete_table(const char *file_name);

private:
  // Hash functions no longer needed with simplified implementation

  /** Increment reference count
   */
  void increment_use_count() { use_count++; }

  /** Decrement reference count
   */
  void decrement_use_count() { use_count--; }
};

#endif /* sbt_share_h */