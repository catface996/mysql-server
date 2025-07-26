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
#include "my_sys.h"

// Static members for share management
mysql_mutex_t SBT_share::sbt_mutex;
bool SBT_share::sbt_init_done = false;
// Simple list for share management (simplified implementation)
static SBT_share *sbt_share_list = nullptr;

/** Constructor */
SBT_share::SBT_share(const char *table_name_arg, uint table_name_length_arg)
    : table_name_length(table_name_length_arg),
      use_count(0),
      tree(nullptr),
      file(nullptr),
      next(nullptr) {
  
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

/** Get share - simplified implementation with linked list */
SBT_share *SBT_share::get_share(const char *table_name) {
  if (!table_name) {
    return nullptr;
  }

  mysql_mutex_lock(&sbt_mutex);
  
  SBT_share *share = nullptr;
  uint name_length = strlen(table_name);
  
  // Look up existing share in list
  for (SBT_share *current = sbt_share_list; current; current = current->next) {
    if (current->table_name_length == name_length && 
        strcmp(current->table_name, table_name) == 0) {
      share = current;
      break;
    }
  }
  
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
        
        // Add to list
        share->next = sbt_share_list;
        sbt_share_list = share;
      }
    }
  }
  
  mysql_mutex_unlock(&sbt_mutex);
  return share;
}

/** Release share - simplified implementation with linked list */
void SBT_share::release_share(SBT_share *share) {
  if (!share) {
    return;
  }

  mysql_mutex_lock(&sbt_mutex);
  
  share->decrement_use_count();
  
  // If reference count reaches zero, remove from list and delete
  if (share->get_use_count() == 0) {
    // Remove from list
    if (sbt_share_list == share) {
      sbt_share_list = share->next;
    } else {
      for (SBT_share *current = sbt_share_list; current; current = current->next) {
        if (current->next == share) {
          current->next = share->next;
          break;
        }
      }
    }
    
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
  
  // Initialize share list
  sbt_share_list = nullptr;
  
  sbt_init_done = true;
  return 0;
}

/** Cleanup share system */
void SBT_share::cleanup_share_system() {
  if (!sbt_init_done) {
    return;
  }

  mysql_mutex_lock(&sbt_mutex);
  
  // Cleanup share list
  while (sbt_share_list) {
    SBT_share *next = sbt_share_list->next;
    delete sbt_share_list;
    sbt_share_list = next;
  }
  
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

/** Hash functions no longer needed with simplified implementation */