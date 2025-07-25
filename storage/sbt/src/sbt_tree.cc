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

/** @file src/sbt_tree.cc
 SBT Tree Implementation - Skeleton

 Created 2025-01-25
 *******************************************************/

#include "my_config.h"
#include "../include/sbt_tree.h"
#include "../include/sbt_common.h"
#include "my_alloc.h"
#include "mysql/psi/mysql_memory.h"

/** Constructor */
SBT_tree::SBT_tree() 
    : root(nullptr), 
      mem_root(PSI_NOT_INSTRUMENTED, 8192),
      next_insert_id(1), 
      record_count(0) {
}

/** Destructor */
SBT_tree::~SBT_tree() {
  clear();
}

/** Insert record */
int SBT_tree::insert(const uchar *data, uint length) {
  if (!data || length == 0) {
    return SBT_ERR_INVALID_ARGUMENT;
  }

  sbt_insert_id_t insert_id = next_insert_id++;
  root = insert_node(root, data, length, insert_id);
  
  if (root) {
    record_count++;
    return SBT_SUCCESS;
  } else {
    next_insert_id--; // Rollback on failure
    return SBT_ERR_OUT_OF_MEMORY;
  }
}

/** Remove record */
int SBT_tree::remove(const uchar *data, uint length) {
  if (!data || length == 0) {
    return SBT_ERR_INVALID_ARGUMENT;
  }

  // Find the node to remove first
  SBT_node *node_to_remove = find_by_data(data, length);
  if (!node_to_remove) {
    return SBT_ERR_INVALID_ARGUMENT; // Record not found
  }

  root = remove_node(root, data, length);
  if (record_count > 0) {
    record_count--;
  }
  return SBT_SUCCESS;
}

/** Update record - skeleton implementation */
int SBT_tree::update(const uchar *old_data, uint old_length,
                     const uchar *new_data, uint new_length) {
  if (!old_data || !new_data || old_length == 0 || new_length == 0) {
    return SBT_ERR_INVALID_ARGUMENT;
  }

  // Find the node with old data
  SBT_node *node = find_by_data(old_data, old_length);
  if (!node) {
    return SBT_ERR_INVALID_ARGUMENT; // Record not found
  }

  // For now, just update the data in place if lengths match
  if (old_length == new_length) {
    memcpy(node->data, new_data, new_length);
    return SBT_SUCCESS;
  }

  // If lengths don't match, we need to remove and re-insert
  // This is a simplified implementation for task 2.1
  int remove_result = remove(old_data, old_length);
  if (remove_result != SBT_SUCCESS) {
    return remove_result;
  }

  return insert(new_data, new_length);
}

/** Find record by data */
SBT_node *SBT_tree::find_by_data(const uchar *data, uint length) {
  if (!data || length == 0) {
    return nullptr;
  }

  return find_by_data_recursive(root, data, length);
}

/** Get first record */
SBT_node *SBT_tree::get_first() {
  if (!root) {
    return nullptr;
  }

  return find_min(root);
}

/** Get next record */
SBT_node *SBT_tree::get_next(SBT_node *current) {
  if (!current) {
    return nullptr;
  }

  // If right subtree exists, find minimum in right subtree
  if (current->right) {
    return find_min(current->right);
  }

  // Otherwise, find the first ancestor where current is in left subtree
  // This requires a parent pointer or stack-based traversal
  // For now, we'll implement a simple approach by finding the next node
  // with insert_id greater than current
  return find_next_by_insert_id(root, current->insert_id);
}

/** Clear all records */
void SBT_tree::clear() {
  root = nullptr;
  record_count = 0;
  next_insert_id = 1;
  mem_root.Clear();
}

/** Private helper methods - skeleton implementations */

SBT_node *SBT_tree::insert_node(SBT_node *node, const uchar *data, uint length,
                                 sbt_insert_id_t insert_id) {
  // Base case: create new node
  if (!node) {
    return create_node(data, length, insert_id);
  }

  // Insert based on insert_id for SBT ordering
  if (insert_id < node->insert_id) {
    node->left = insert_node(node->left, data, length, insert_id);
  } else {
    node->right = insert_node(node->right, data, length, insert_id);
  }

  // Update size and maintain SBT property
  update_size(node);
  return maintain(node, insert_id >= node->insert_id);
}

SBT_node *SBT_tree::remove_node(SBT_node *node, const uchar *data, uint length) {
  if (!node) {
    return nullptr;
  }

  // Check if this is the node to remove
  if (sbt_data_compare(node->data, node->data_length, data, length) == 0) {
    // Case 1: Node has no children
    if (!node->left && !node->right) {
      return nullptr;
    }
    
    // Case 2: Node has only right child
    if (!node->left) {
      SBT_node *right_child = node->right;
      update_size(right_child);
      return right_child;
    }
    
    // Case 3: Node has only left child
    if (!node->right) {
      SBT_node *left_child = node->left;
      update_size(left_child);
      return left_child;
    }
    
    // Case 4: Node has both children
    // Find the minimum node in the right subtree (successor)
    SBT_node *successor = find_min(node->right);
    
    // Copy successor's data to current node
    // Allocate new memory for the data
    uchar *new_data = (uchar *)mem_root.Alloc(successor->data_length);
    if (new_data) {
      memcpy(new_data, successor->data, successor->data_length);
      node->data = new_data;
      node->data_length = successor->data_length;
      node->insert_id = successor->insert_id;
    }
    
    // Remove the successor from right subtree
    node->right = remove_node(node->right, successor->data, successor->data_length);
  } else {
    // Recursively search in left and right subtrees
    node->left = remove_node(node->left, data, length);
    node->right = remove_node(node->right, data, length);
  }

  // Update size and maintain SBT property
  update_size(node);
  return maintain(node, false); // Maintain both directions
}

SBT_node *SBT_tree::maintain(SBT_node *node, bool flag) {
  if (!node) return node;

  if (!flag) {
    // Left subtree was modified
    if (node->left && node->left->left && 
        get_size(node->left->left) > get_size(node->right)) {
      node = rotate_right(node);
    } else if (node->left && node->left->right && 
               get_size(node->left->right) > get_size(node->right)) {
      node->left = rotate_left(node->left);
      node = rotate_right(node);
    } else {
      return node;
    }
  } else {
    // Right subtree was modified
    if (node->right && node->right->right && 
        get_size(node->right->right) > get_size(node->left)) {
      node = rotate_left(node);
    } else if (node->right && node->right->left && 
               get_size(node->right->left) > get_size(node->left)) {
      node->right = rotate_right(node->right);
      node = rotate_left(node);
    } else {
      return node;
    }
  }

  // Recursively maintain children
  node->left = maintain(node->left, false);
  node->right = maintain(node->right, true);
  return node;
}

SBT_node *SBT_tree::rotate_left(SBT_node *node) {
  if (!node || !node->right) return node;

  SBT_node *new_root = node->right;
  node->right = new_root->left;
  new_root->left = node;

  // Update sizes
  update_size(node);
  update_size(new_root);

  return new_root;
}

SBT_node *SBT_tree::rotate_right(SBT_node *node) {
  if (!node || !node->left) return node;

  SBT_node *new_root = node->left;
  node->left = new_root->right;
  new_root->right = node;

  // Update sizes
  update_size(node);
  update_size(new_root);

  return new_root;
}

void SBT_tree::update_size(SBT_node *node) {
  if (node) {
    node->size = 1 + get_size(node->left) + get_size(node->right);
  }
}

uint SBT_tree::get_size(SBT_node *node) const {
  return node ? node->size : 0;
}

SBT_node *SBT_tree::create_node(const uchar *data, uint length, 
                                 sbt_insert_id_t insert_id) {
  if (!data || length == 0) {
    return nullptr;
  }

  // Allocate node structure
  SBT_node *node = (SBT_node *)mem_root.Alloc(sizeof(SBT_node));
  if (!node) {
    return nullptr;
  }

  // Allocate and copy data
  node->data = (uchar *)mem_root.Alloc(length);
  if (!node->data) {
    return nullptr;
  }
  memcpy(node->data, data, length);

  // Initialize node fields
  node->data_length = length;
  node->insert_id = insert_id;
  node->left = nullptr;
  node->right = nullptr;
  node->size = 1;

  return node;
}

SBT_node *SBT_tree::find_min(SBT_node *node) {
  if (!node) {
    return nullptr;
  }

  while (node->left) {
    node = node->left;
  }
  
  return node;
}

SBT_node *SBT_tree::find_by_data_recursive(SBT_node *node, const uchar *data, uint length) {
  if (!node) {
    return nullptr;
  }

  // Check current node
  if (sbt_data_compare(node->data, node->data_length, data, length) == 0) {
    return node;
  }

  // Search left subtree
  SBT_node *found = find_by_data_recursive(node->left, data, length);
  if (found) {
    return found;
  }

  // Search right subtree
  return find_by_data_recursive(node->right, data, length);
}

SBT_node *SBT_tree::find_next_by_insert_id(SBT_node *node, sbt_insert_id_t current_id) {
  if (!node) {
    return nullptr;
  }

  SBT_node *result = nullptr;

  // If current node has insert_id greater than current_id, it's a candidate
  if (node->insert_id > current_id) {
    result = node;
    // Check if there's a smaller candidate in left subtree
    SBT_node *left_result = find_next_by_insert_id(node->left, current_id);
    if (left_result && left_result->insert_id < result->insert_id) {
      result = left_result;
    }
  } else {
    // Current node's insert_id <= current_id, search right subtree
    result = find_next_by_insert_id(node->right, current_id);
  }

  return result;
}