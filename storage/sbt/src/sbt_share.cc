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

/** @file sbt_share.cc
 SBT Share Management Implementation

 Created 2025-01-25
 *******************************************************/

#include "my_config.h"
#include "../include/sbt_share.h"
#include "../include/sbt_common.h"
#include "mysql/psi/mysql_mutex.h"
#include "my_sys.h"
#include "my_hash.h"

// Static members for share management
mysql_mutex_t SBT_share::sbt_mutex;
bool SBT_share::sbt_init_done = false;
static HASH sbt_share_hash;  // Hash table for share management

/** Constructor */
SBT_share::SBT_share(const char *table_name_arg, uint table_name_length_arg)
    : table_name_length(table_name_length_arg),
      use_count(0),
      tree(nullptr),
      file(nullptr) {
  
  // Initialize lock
  thr_lock_init(&lock);
  
  // Initialize mutex
  mysql_mutex_init(PSI_NOT_INSTRUMENTED, &mutex, MY_MUTEX_INIT_FAST);
  
  // Copy table name
  table_name = (char *)sbt_malloc(table_name_length + 1);
  if (table_name) {
    memcpy(table_name, table_name_arg, table_name_length);
    table_name[table_name_length] = '\0';
  }
}

/** Destructor */
SBT_share::~SBT_share() {
  // Clean up tree
  if (tree) {
    delete tree;
    tree = nullptr;
  }
  
  // Clean up file
  if (file) {
    delete file;
    file = nullptr;
  }
  
  // Clean up table name
  if (table_name) {
    sbt_free(table_name);
    table_name = nullptr;
  }
  
  // Destroy lock and mutex
  thr_lock_delete(&lock);
  mysql_mutex_destroy(&mutex);
}

/** Get share - full implementation with hash table */
SBT_share *SBT_share::get_share(const char *table_name) {
  if (!table_name) {
    return nullptr;
  }

  mysql_mutex_lock(&sbt_mutex);
  
  SBT_share *share = nullptr;
  uint name_length = strlen(table_name);
  
  // Look up existing share in hash table
  share = (SBT_share *)my_hash_search(&sbt_share_hash, 
                                      (const uchar *)table_name, 
                                      name_length);
  
  if (share) {
    // Found existing share, increment reference count
    share->increment_use_count();
  } else {
    // Create new share
    share = new SBT_share(table_name, name_length);
    if (share) {
      // Initialize table data first
      if (share->init_table_data(table_name) != SBT_SUCCESS) {
        delete share;
        share = nullptr;
      } else {
        // Set initial reference count
        share->increment_use_count();
        
        // Add to hash table
        if (my_hash_insert(&sbt_share_hash, (uchar *)share)) {
          // Hash insertion failed
          delete share;
          share = nullptr;
        }
      }
    }
  }
  
  mysql_mutex_unlock(&sbt_mutex);
  return share;
}

/** Release share - full implementation with hash table */
void SBT_share::release_share(SBT_share *share) {
  if (!share) {
    return;
  }

  mysql_mutex_lock(&sbt_mutex);
  
  share->decrement_use_count();
  
  // If reference count reaches zero, remove from hash and delete
  if (share->get_use_count() == 0) {
    // Remove from hash table
    my_hash_delete(&sbt_share_hash, (uchar *)share);
    
    // Delete the share object
    delete share;
  }
  
  mysql_mutex_unlock(&sbt_mutex);
}

/** Initialize share system */
int SBT_share::init_share_system() {
  if (sbt_init_done) {
    return 0;
  }

  // Initialize mutex
  mysql_mutex_init(PSI_NOT_INSTRUMENTED, &sbt_mutex, MY_MUTEX_INIT_FAST);
  
  // Initialize hash table
  if (my_hash_init(&sbt_share_hash, system_charset_info, 32, 0, 0,
                   (my_hash_get_key)sbt_hash_key, 
                   (my_hash_free_key)sbt_hash_free, 0)) {
    mysql_mutex_destroy(&sbt_mutex);
    return 1;  // Failed to initialize hash table
  }
  
  sbt_init_done = true;
  return 0;
}

/** Cleanup share system */
void SBT_share::cleanup_share_system() {
  if (!sbt_init_done) {
    return;
  }

  mysql_mutex_lock(&sbt_mutex);
  
  // Cleanup hash table (this will call sbt_hash_free for each element)
  my_hash_free(&sbt_share_hash);
  
  mysql_mutex_unlock(&sbt_mutex);
  mysql_mutex_destroy(&sbt_mutex);
  
  sbt_init_done = false;
}

/** Lock share */
void SBT_share::lock_share() {
  mysql_mutex_lock(&mutex);
}

/** Unlock share */
void SBT_share::unlock_share() {
  mysql_mutex_unlock(&mutex);
}

/** Initialize table data */
int SBT_share::init_table_data(const char *table_name) {
  // Create tree
  tree = new SBT_tree();
  if (!tree) {
    return SBT_ERR_OUT_OF_MEMORY;
  }
  
  // Create file manager
  file = new SBT_file();
  if (!file) {
    delete tree;
    tree = nullptr;
    return SBT_ERR_OUT_OF_MEMORY;
  }
  
  return SBT_SUCCESS;
}

/** Open table */
int SBT_share::open_table() {
  if (!file || !tree) {
    return SBT_ERR_INVALID_ARGUMENT;
  }

  lock_share();
  
  int result = SBT_SUCCESS;
  
  // Open the file
  result = file->open(table_name);
  if (result != SBT_SUCCESS) {
    unlock_share();
    return result;
  }
  
  // Load tree data from file
  result = file->load_tree(tree);
  if (result != SBT_SUCCESS) {
    file->close();
    unlock_share();
    return result;
  }
  
  unlock_share();
  return SBT_SUCCESS;
}

/** Close table */
int SBT_share::close_table() {
  if (!file || !tree) {
    return SBT_ERR_INVALID_ARGUMENT;
  }

  lock_share();
  
  int result = SBT_SUCCESS;
  
  // Save tree data to file
  result = file->save_tree(tree);
  if (result != SBT_SUCCESS) {
    unlock_share();
    return result;
  }
  
  // Close the file
  result = file->close();
  
  unlock_share();
  return result;
}

/** Create table */
int SBT_share::create_table(const char *file_name) {
  if (!file || !file_name) {
    return SBT_ERR_INVALID_ARGUMENT;
  }

  return file->create(file_name);
}

/** Delete table */
int SBT_share::delete_table(const char *file_name) {
  if (!file_name) {
    return SBT_ERR_INVALID_ARGUMENT;
  }

  return SBT_file::delete_file(file_name);
}

/** Hash key function for MySQL hash table */
uchar *SBT_share::sbt_hash_key(const uchar *record, size_t *length,
                               my_bool not_used [[maybe_unused]]) {
  SBT_share *share = (SBT_share *)record;
  *length = share->table_name_length;
  return (uchar *)share->table_name;
}

/** Hash free function for MySQL hash table */
void SBT_share::sbt_hash_free(void *element) {
  if (element) {
    SBT_share *share = (SBT_share *)element;
    delete share;
  }
}