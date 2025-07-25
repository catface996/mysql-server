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

/** @file include/ha_sbt.h
 SBT Storage Engine Handler Interface

 Created 2025-01-25
 *******************************************************/

#ifndef ha_sbt_h
#define ha_sbt_h

#include "sbt_common.h"
#include "sbt_share.h"
#include "sbt_tree.h"
#include "sql/handler.h"
#include "thr_lock.h"

/** SBT Storage Engine Handler Class
 * 
 * Implements MySQL's handler interface for the SBT storage engine.
 * Provides basic CRUD operations without primary key or index support.
 * All record operations use full table scan for simplicity.
 */
class ha_sbt : public handler {
private:
  SBT_share *share;               // Shared table information
  THR_LOCK_DATA lock;             // Lock data for this handler
  SBT_node *current_node;         // Current position for table scan
  bool scan_initialized;          // Whether scan is initialized

public:
  /** Constructor
   * @param[in] hton Handlerton for this storage engine
   * @param[in] table_arg Table structure
   */
  ha_sbt(handlerton *hton, TABLE_SHARE *table_arg);

  /** Destructor
   * Clean up resources
   */
  ~ha_sbt() override;

  /** Get storage engine name
   * @return Storage engine name string
   */
  const char *table_type() const override { return "SBT"; }

  /** Get table flags
   * @return Bitmask of table capabilities
   */
  ulonglong table_flags() const override;

  /** Get index flags (not supported)
   * @param[in] inx Index number
   * @param[in] part Part number
   * @param[in] all_parts Whether all parts are requested
   * @return Index flags (always 0 for SBT)
   */
  ulong index_flags(uint inx, uint part, bool all_parts) const override {
    return 0;  // No index support
  }

  /** Get maximum supported keys (not supported)
   * @return Maximum number of keys (always 0 for SBT)
   */
  uint max_supported_keys() const override { return 0; }

  /** Get maximum supported key length (not supported)
   * @return Maximum key length (always 0 for SBT)
   */
  uint max_supported_key_length() const override { return 0; }

  /** Get maximum supported key parts (not supported)
   * @return Maximum key parts (always 0 for SBT)
   */
  uint max_supported_key_parts() const override { return 0; }

  /** Get maximum supported key part length (not supported)
   * @return Maximum key part length (always 0 for SBT)
   */
  uint max_supported_key_part_length(HA_CREATE_INFO *create_info) const override { return 0; }

  /** Open table
   * @param[in] name Table name
   * @param[in] mode Open mode
   * @param[in] test_if_locked Test if locked
   * @param[in] table_def Table definition
   * @return 0 on success, error code on failure
   */
  int open(const char *name, int mode, uint test_if_locked,
           const dd::Table *table_def) override;

  /** Close table
   * @return 0 on success, error code on failure
   */
  int close() override;

  /** Write a row to the table
   * @param[in] buf Row data buffer
   * @return 0 on success, error code on failure
   */
  int write_row(uchar *buf) override;

  /** Update a row in the table
   * @param[in] old_data Old row data
   * @param[in] new_data New row data
   * @return 0 on success, error code on failure
   */
  int update_row(const uchar *old_data, uchar *new_data) override;

  /** Delete a row from the table
   * @param[in] buf Row data buffer
   * @return 0 on success, error code on failure
   */
  int delete_row(const uchar *buf) override;

  /** Initialize random (full table) scan
   * @param[in] scan Whether this is a scan operation
   * @return 0 on success, error code on failure
   */
  int rnd_init(bool scan) override;

  /** Get next row in random scan
   * @param[out] buf Buffer to store row data
   * @return 0 on success, HA_ERR_END_OF_FILE at end, error code on failure
   */
  int rnd_next(uchar *buf) override;

  /** End random scan
   * @return 0 on success, error code on failure
   */
  int rnd_end() override;

  /** Get current row position (not supported)
   * @param[in] record Record data
   */
  void position(const uchar *record) override;

  /** Read row by position (not supported)
   * @param[out] buf Buffer to store row data
   * @param[in] pos Position reference
   * @return Error code (always HA_ERR_WRONG_COMMAND for SBT)
   */
  int rnd_pos(uchar *buf, uchar *pos) override;

  /** Get information about the table
   * @param[in] flag Information type flag
   * @return 0 on success, error code on failure
   */
  int info(uint flag) override;

  /** Create a new table
   * @param[in] name Table name
   * @param[in] table_arg Table structure
   * @param[in] create_info Creation information
   * @param[in] table_def Table definition
   * @return 0 on success, error code on failure
   */
  int create(const char *name, TABLE *table_arg, HA_CREATE_INFO *create_info,
             dd::Table *table_def) override;

  /** Delete a table
   * @param[in] name Table name
   * @param[in] table_def Table definition
   * @return 0 on success, error code on failure
   */
  int delete_table(const char *name, const dd::Table *table_def) override;

  /** External lock (for transaction coordination)
   * @param[in] thd Thread handle
   * @param[in] lock_type Lock type
   * @return 0 on success, error code on failure
   */
  int external_lock(THD *thd, int lock_type) override;

  /** Get lock data for this handler
   * @return Pointer to lock data
   */
  THR_LOCK_DATA **store_lock(THD *thd, THR_LOCK_DATA **to,
                             enum thr_lock_type lock_type) override;

private:
  /** Pack row data from MySQL format to SBT format
   * @param[in] record MySQL record buffer
   * @param[out] packed_data Buffer for packed data
   * @param[out] packed_length Length of packed data
   * @return 0 on success, error code on failure
   */
  int pack_row(const uchar *record, uchar **packed_data, uint *packed_length);

  /** Unpack row data from SBT format to MySQL format
   * @param[in] packed_data Packed data buffer
   * @param[in] packed_length Length of packed data
   * @param[out] record MySQL record buffer
   * @return 0 on success, error code on failure
   */
  int unpack_row(const uchar *packed_data, uint packed_length, uchar *record);

  /** Get full table file path
   * @param[in] name Table name
   * @param[out] path Buffer for full path
   * @param[in] path_size Size of path buffer
   */
  void get_table_file_path(const char *name, char *path, size_t path_size);
};

// Storage engine initialization functions
int sbt_init_func(void *p);
int sbt_done_func(void *p);

// Handlerton for SBT storage engine
extern handlerton *sbt_hton;

#endif /* ha_sbt_h */