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
 * @file sbt_share.cc
 * @brief SBT Share Management Implementation
 * 
 * This file contains the implementation of shared data structures and
 * resource management for the SBT storage engine.
 */

#include "sbt_share.h"
#include "sbt_tree.h"
#include "sbt_storage.h"
#include "sbt_common.h"
#include <unordered_map>
#include <mutex>

// Global share management
static std::unordered_map<std::string, SBT_share*> sbt_open_tables;
static std::mutex sbt_share_mutex;

SBT_share::SBT_share(const char *name)
    : table_name(name), tree(nullptr), storage(nullptr), 
      ref_count(0), is_loaded(false) {
    SBT_DBUG_ENTER("SBT_share::SBT_share");
    
    // Initialize thread lock
    thr_lock_init(&lock);
    
    // Build data file name
    data_file_name = SBT_utils::build_file_path(name);
    
    SBT_DBUG_VOID_RETURN;
}

SBT_share::~SBT_share() {
    SBT_DBUG_ENTER("SBT_share::~SBT_share");
    
    cleanup();
    thr_lock_delete(&lock);
    
    SBT_DBUG_VOID_RETURN;
}

void SBT_share::release_ref() {
    SBT_DBUG_ENTER("SBT_share::release_ref");
    
    uint old_count = ref_count.fetch_sub(1);
    if (old_count == 1) {
        // Last reference, delete this share
        delete this;
    }
    
    SBT_DBUG_VOID_RETURN;
}

int SBT_share::initialize(TABLE *table) {
    SBT_DBUG_ENTER("SBT_share::initialize");
    
    // Create tree instance
    tree = new SBT_tree(table);
    if (!tree) {
        SBT_DBUG_RETURN(SBT_error_handler::handle_memory_error("tree creation"));
    }
    
    // Create storage instance
    storage = new SBT_storage(data_file_name.c_str());
    if (!storage) {
        delete tree;
        tree = nullptr;
        SBT_DBUG_RETURN(SBT_error_handler::handle_memory_error("storage creation"));
    }
    
    SBT_DBUG_RETURN(0);
}

void SBT_share::cleanup() {
    SBT_DBUG_ENTER("SBT_share::cleanup");
    
    if (tree) {
        delete tree;
        tree = nullptr;
    }
    
    if (storage) {
        delete storage;
        storage = nullptr;
    }
    
    is_loaded = false;
    
    SBT_DBUG_VOID_RETURN;
}

int SBT_share::load_from_disk() {
    SBT_DBUG_ENTER("SBT_share::load_from_disk");
    
    if (!storage) {
        SBT_DBUG_RETURN(HA_ERR_INTERNAL_ERROR);
    }
    
    // TODO: Implement data loading from disk
    is_loaded = true;
    
    SBT_DBUG_RETURN(0);
}

int SBT_share::save_to_disk() {
    SBT_DBUG_ENTER("SBT_share::save_to_disk");
    
    if (!storage) {
        SBT_DBUG_RETURN(HA_ERR_INTERNAL_ERROR);
    }
    
    // TODO: Implement data saving to disk
    
    SBT_DBUG_RETURN(0);
}

int SBT_share::create_data_file() {
    SBT_DBUG_ENTER("SBT_share::create_data_file");
    
    if (!storage) {
        SBT_DBUG_RETURN(HA_ERR_INTERNAL_ERROR);
    }
    
    // TODO: Implement data file creation
    
    SBT_DBUG_RETURN(0);
}

int SBT_share::delete_data_file() {
    SBT_DBUG_ENTER("SBT_share::delete_data_file");
    
    if (!storage) {
        SBT_DBUG_RETURN(HA_ERR_INTERNAL_ERROR);
    }
    
    // TODO: Implement data file deletion
    
    SBT_DBUG_RETURN(0);
}

// Global share management functions

SBT_share* get_sbt_share(const char *table_name, TABLE *table) {
    SBT_DBUG_ENTER("get_sbt_share");
    
    std::lock_guard<std::mutex> lock(sbt_share_mutex);
    
    std::string name(table_name);
    auto it = sbt_open_tables.find(name);
    
    SBT_share *share;
    if (it != sbt_open_tables.end()) {
        // Existing share found
        share = it->second;
        share->add_ref();
    } else {
        // Create new share
        share = new SBT_share(table_name);
        if (share) {
            int result = share->initialize(table);
            if (result != 0) {
                delete share;
                SBT_DBUG_RETURN(nullptr);
            }
            
            share->add_ref();
            sbt_open_tables[name] = share;
        }
    }
    
    SBT_DBUG_RETURN(share);
}

void free_sbt_share(SBT_share *share) {
    SBT_DBUG_ENTER("free_sbt_share");
    
    if (!share) {
        SBT_DBUG_VOID_RETURN;
    }
    
    std::lock_guard<std::mutex> lock(sbt_share_mutex);
    
    // Remove from global map if reference count will reach zero
    if (share->get_ref_count() == 1) {
        auto it = sbt_open_tables.find(share->get_table_name());
        if (it != sbt_open_tables.end()) {
            sbt_open_tables.erase(it);
        }
    }
    
    share->release_ref();
    
    SBT_DBUG_VOID_RETURN;
}