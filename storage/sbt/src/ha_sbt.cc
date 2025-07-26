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

/** @file src/ha_sbt.cc
 SBT Storage Engine Handler Implementation

 Created 2025-01-25
 *******************************************************/

#include "my_config.h"

#include <mysql/plugin.h>
#include "sql/sql_class.h"
#include "sql/sql_plugin.h"
#include "sql/table.h"
#include "thr_lock.h"

#include "../include/ha_sbt.h"
#include "../include/sbt_common.h"

// Global handlerton for SBT storage engine
handlerton *sbt_hton = nullptr;

// Storage engine plugin declaration
static struct st_mysql_storage_engine sbt_storage_engine = {
  MYSQL_HANDLERTON_INTERFACE_VERSION
};

/** Create SBT handler instance
 * @param[in] hton Handlerton
 * @param[in] table Table share
 * @param[in] mem_root Memory root
 * @return New handler instance
 */
static handler *sbt_create_handler(handlerton *hton, TABLE_SHARE *table,
                                   bool, MEM_ROOT *mem_root) {
  return new (mem_root) ha_sbt(hton, table);
}

/** Initialize SBT storage engine
 * @param[in] p Plugin information
 * @return 0 on success, non-zero on failure
 */
int sbt_init_func(void *p) {
  DBUG_ENTER("sbt_init_func");

  handlerton *sbt_hton_local = (handlerton *)p;
  
  // Initialize share system
  if (SBT_share::init_share_system() != 0) {
    DBUG_RETURN(1);
  }

  // Set up handlerton
  sbt_hton_local->state = SHOW_OPTION_YES;
  sbt_hton_local->create = sbt_create_handler;
  sbt_hton_local->flags = HTON_CAN_RECREATE;
  
  // Store global reference
  sbt_hton = sbt_hton_local;

  DBUG_RETURN(0);
}

/** Cleanup SBT storage engine
 * @param[in] p Plugin information
 * @return 0 on success, non-zero on failure
 */
int sbt_done_func(void *p) {
  DBUG_ENTER("sbt_done_func");

  // Cleanup share system
  SBT_share::cleanup_share_system();

  DBUG_RETURN(0);
}

/** SBT Storage Engine Plugin Descriptor */
mysql_declare_plugin(sbt) {
  MYSQL_STORAGE_ENGINE_PLUGIN,
  &sbt_storage_engine,
  "SBT",
  "Oracle Corporation",
  "Size Balanced Tree Storage Engine",
  PLUGIN_LICENSE_GPL,
  sbt_init_func,    // Plugin init function
  nullptr,          // Plugin check uninstall function  
  sbt_done_func,    // Plugin deinit function
  0x0100,           // Version 1.0
  nullptr,          // Status variables
  nullptr,          // System variables
  nullptr,          // Config options
  0,                // Flags
}
mysql_declare_plugin_end;

/** Constructor */
ha_sbt::ha_sbt(handlerton *hton, TABLE_SHARE *table_arg)
    : handler(hton, table_arg),
      share(nullptr),
      current_node(nullptr),
      scan_initialized(false) {
  // Lock data will be initialized in open() when share is available
}

/** Destructor */
ha_sbt::~ha_sbt() {
  // Cleanup will be done in close()
}

/** Get table flags */
ulonglong ha_sbt::table_flags() const {
  return (HA_FAST_KEY_READ |            // Fast key read (not used)
          HA_NULL_IN_KEY |              // NULL values in keys (not used)
          HA_CAN_SQL_HANDLER |          // Can use HANDLER statements
          HA_BINLOG_STMT_CAPABLE);      // Statement-based replication
}

/** Open table */
int ha_sbt::open(const char *name, int mode, uint test_if_locked,
                 const dd::Table *table_def) {
  DBUG_ENTER("ha_sbt::open");
  
  // Validate input parameters
  if (!name) {
    sbt_log_error("Invalid table name for open operation");
    DBUG_RETURN(HA_ERR_WRONG_COMMAND);
  }
  
  // Check if table is already open
  if (share) {
    sbt_log_error("Table is already open: %s", name);
    DBUG_RETURN(HA_ERR_CRASHED_ON_USAGE);
  }
  
  // Get shared table information
  share = SBT_share::get_share(name);
  if (!share) {
    sbt_log_error("Failed to get share for table: %s", name);
    DBUG_RETURN(HA_ERR_OUT_OF_MEM);
  }

  // Initialize lock data with share's lock
  thr_lock_data_init(share->get_lock(), &lock, nullptr);

  // Open table data
  int error = share->open_table();
  if (error != SBT_SUCCESS) {
    sbt_log_error("Failed to open table data: %s, error: %d", name, error);
    SBT_share::release_share(share);
    share = nullptr;
    DBUG_RETURN(sbt_error_to_mysql_error(error));
  }
  
  // Initialize scan state
  current_node = nullptr;
  scan_initialized = false;
  
  sbt_log_info("Successfully opened table: %s", name);
  DBUG_RETURN(0);
}

/** Close table */
int ha_sbt::close() {
  DBUG_ENTER("ha_sbt::close");

  if (share) {
    // End any active scan
    if (scan_initialized) {
      rnd_end();
    }
    
    // Save table data
    int error = share->close_table();
    if (error != SBT_SUCCESS) {
      sbt_log_error("Failed to close table data, error: %d", error);
      // Continue with cleanup even if save failed
    }
    
    // Release shared information
    SBT_share::release_share(share);
    share = nullptr;
    
    sbt_log_info("Successfully closed table");
  }
  
  // Reset handler state
  current_node = nullptr;
  scan_initialized = false;

  DBUG_RETURN(0);
}

/** Write row */
int ha_sbt::write_row(uchar *buf) {
  DBUG_ENTER("ha_sbt::write_row");
  
  if (!share || !share->get_tree()) {
    DBUG_RETURN(HA_ERR_CRASHED_ON_USAGE);
  }

  // Pack row data
  uchar *packed_data = nullptr;
  uint packed_length = 0;
  int error = pack_row(buf, &packed_data, &packed_length);
  if (error) {
    DBUG_RETURN(error);
  }

  // Insert into tree
  error = share->get_tree()->insert(packed_data, packed_length);
  
  // Free packed data
  if (packed_data) {
    sbt_free(packed_data);
  }

  DBUG_RETURN(sbt_error_to_mysql_error(error));
}

/** Update row */
int ha_sbt::update_row(const uchar *old_data, uchar *new_data) {
  DBUG_ENTER("ha_sbt::update_row");
  
  if (!share || !share->get_tree()) {
    DBUG_RETURN(HA_ERR_CRASHED_ON_USAGE);
  }

  // Pack old and new row data
  uchar *old_packed = nullptr, *new_packed = nullptr;
  uint old_length = 0, new_length = 0;
  
  int error = pack_row(old_data, &old_packed, &old_length);
  if (error) {
    DBUG_RETURN(error);
  }
  
  error = pack_row(new_data, &new_packed, &new_length);
  if (error) {
    if (old_packed) sbt_free(old_packed);
    DBUG_RETURN(error);
  }

  // Update in tree
  error = share->get_tree()->update(old_packed, old_length, 
                                   new_packed, new_length);
  
  // Free packed data
  if (old_packed) sbt_free(old_packed);
  if (new_packed) sbt_free(new_packed);

  DBUG_RETURN(sbt_error_to_mysql_error(error));
}

/** Delete row */
int ha_sbt::delete_row(const uchar *buf) {
  DBUG_ENTER("ha_sbt::delete_row");
  
  if (!share || !share->get_tree()) {
    DBUG_RETURN(HA_ERR_CRASHED_ON_USAGE);
  }

  // Pack row data
  uchar *packed_data = nullptr;
  uint packed_length = 0;
  int error = pack_row(buf, &packed_data, &packed_length);
  if (error) {
    DBUG_RETURN(error);
  }

  // Remove from tree
  error = share->get_tree()->remove(packed_data, packed_length);
  
  // Free packed data
  if (packed_data) {
    sbt_free(packed_data);
  }

  DBUG_RETURN(sbt_error_to_mysql_error(error));
}

/** Initialize random scan */
int ha_sbt::rnd_init(bool scan) {
  DBUG_ENTER("ha_sbt::rnd_init");
  
  if (!share || !share->get_tree()) {
    DBUG_RETURN(HA_ERR_CRASHED_ON_USAGE);
  }

  // Start from first record
  current_node = share->get_tree()->get_first();
  scan_initialized = true;

  DBUG_RETURN(0);
}

/** Get next row in scan */
int ha_sbt::rnd_next(uchar *buf) {
  DBUG_ENTER("ha_sbt::rnd_next");
  
  if (!scan_initialized || !share || !share->get_tree()) {
    DBUG_RETURN(HA_ERR_CRASHED_ON_USAGE);
  }

  // Check if we have a current record
  if (!current_node) {
    DBUG_RETURN(HA_ERR_END_OF_FILE);
  }

  // Unpack current record
  int error = unpack_row(current_node->data, current_node->data_length, buf);
  if (error) {
    DBUG_RETURN(error);
  }

  // Move to next record
  current_node = share->get_tree()->get_next(current_node);

  DBUG_RETURN(0);
}

/** End random scan */
int ha_sbt::rnd_end() {
  DBUG_ENTER("ha_sbt::rnd_end");
  
  current_node = nullptr;
  scan_initialized = false;

  DBUG_RETURN(0);
}

/** Get current position (not supported) */
void ha_sbt::position(const uchar *record) {
  // Position-based access not supported in SBT
  // This is a no-op
}

/** Read by position (not supported) */
int ha_sbt::rnd_pos(uchar *buf, uchar *pos) {
  DBUG_ENTER("ha_sbt::rnd_pos");
  // Position-based access not supported
  DBUG_RETURN(HA_ERR_WRONG_COMMAND);
}

/** Get table information */
int ha_sbt::info(uint flag) {
  DBUG_ENTER("ha_sbt::info");
  
  if (share && share->get_tree()) {
    stats.records = share->get_tree()->get_record_count();
    stats.deleted = 0;
    stats.data_file_length = 0;  // Will be updated when file operations are implemented
    stats.index_file_length = 0;
    stats.mean_rec_length = 0;
  }

  DBUG_RETURN(0);
}

/** Create table */
int ha_sbt::create(const char *name, TABLE *table_arg, 
                   HA_CREATE_INFO *create_info, dd::Table *table_def) {
  DBUG_ENTER("ha_sbt::create");
  
  // Validate input parameters
  if (!name || !table_arg) {
    sbt_log_error("Invalid parameters for table creation");
    DBUG_RETURN(HA_ERR_WRONG_COMMAND);
  }
  
  char file_path[FN_REFLEN];
  get_table_file_path(name, file_path, sizeof(file_path));
  
  // Check if file already exists
  if (SBT_file::file_exists(file_path)) {
    sbt_log_error("Table file already exists: %s", file_path);
    DBUG_RETURN(HA_ERR_FOUND_DUPP_KEY);  // Use existing MySQL error code
  }
  
  // Create the table file
  SBT_file file;
  int error = file.create(file_path);
  if (error != SBT_SUCCESS) {
    sbt_log_error("Failed to create table file: %s, error: %d", file_path, error);
    DBUG_RETURN(sbt_error_to_mysql_error(error));
  }
  
  sbt_log_info("Successfully created table file: %s", file_path);
  DBUG_RETURN(0);
}

/** Delete table */
int ha_sbt::delete_table(const char *name, const dd::Table *table_def) {
  DBUG_ENTER("ha_sbt::delete_table");
  
  // Validate input parameters
  if (!name) {
    sbt_log_error("Invalid table name for deletion");
    DBUG_RETURN(HA_ERR_WRONG_COMMAND);
  }
  
  char file_path[FN_REFLEN];
  get_table_file_path(name, file_path, sizeof(file_path));
  
  // Check if file exists before attempting deletion
  if (!SBT_file::file_exists(file_path)) {
    sbt_log_error("Table file does not exist: %s", file_path);
    DBUG_RETURN(HA_ERR_NO_SUCH_TABLE);
  }
  
  // Delete the table file
  int error = SBT_file::delete_file(file_path);
  if (error != SBT_SUCCESS) {
    sbt_log_error("Failed to delete table file: %s, error: %d", file_path, error);
    DBUG_RETURN(sbt_error_to_mysql_error(error));
  }
  
  sbt_log_info("Successfully deleted table file: %s", file_path);
  DBUG_RETURN(0);
}

/** External lock */
int ha_sbt::external_lock(THD *thd, int lock_type) {
  DBUG_ENTER("ha_sbt::external_lock");
  // Basic implementation - no special transaction handling needed
  DBUG_RETURN(0);
}

/** Store lock */
THR_LOCK_DATA **ha_sbt::store_lock(THD *thd, THR_LOCK_DATA **to,
                                   enum thr_lock_type lock_type) {
  if (lock_type != TL_IGNORE && lock.type == TL_UNLOCK) {
    lock.type = lock_type;
  }
  *to++ = &lock;
  return to;
}

/** Pack row data - placeholder implementation */
int ha_sbt::pack_row(const uchar *record, uchar **packed_data, uint *packed_length) {
  // Simple implementation: just copy the record
  // In a real implementation, this would handle field packing, NULL values, etc.
  
  *packed_length = table->s->reclength;
  *packed_data = (uchar *)sbt_malloc(*packed_length);
  if (!*packed_data) {
    return HA_ERR_OUT_OF_MEM;
  }
  
  memcpy(*packed_data, record, *packed_length);
  return 0;
}

/** Unpack row data - placeholder implementation */
int ha_sbt::unpack_row(const uchar *packed_data, uint packed_length, uchar *record) {
  // Simple implementation: just copy the data
  // In a real implementation, this would handle field unpacking, NULL values, etc.
  
  if (packed_length > table->s->reclength) {
    return HA_ERR_CRASHED_ON_USAGE;
  }
  
  memcpy(record, packed_data, packed_length);
  return 0;
}

/** Get table file path */
void ha_sbt::get_table_file_path(const char *name, char *path, size_t path_size) {
  snprintf(path, path_size, "%s.sbt", name);
}