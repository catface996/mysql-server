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

/** @file tests/gtest/sbt_serialization_test.cc
 SBT Tree Serialization and Deserialization Unit Tests

 Created 2025-01-25
 *******************************************************/

#include <gtest/gtest.h>
#include "my_config.h"
#include "../../include/sbt_tree.h"
#include "../../include/sbt_file.h"
#include "../../include/sbt_common.h"

class SBTSerializationTest : public ::testing::Test {
protected:
  void SetUp() override {
    tree = new SBT_tree();
    file = new SBT_file();
    test_file_name = "/tmp/sbt_serialization_test.sbt";
  }

  void TearDown() override {
    // Clean up test file
    SBT_file::delete_file(test_file_name);
    delete file;
    delete tree;
  }

  SBT_tree *tree;
  SBT_file *file;
  const char *test_file_name;
  
  // Helper function to compare tree structure and data
  bool compare_trees_recursive(SBT_node* node1, SBT_node* node2) {
    // Both null
    if (node1 == nullptr && node2 == nullptr) {
      return true;
    }
    
    // One null, one not
    if (node1 == nullptr || node2 == nullptr) {
      return false;
    }
    
    // Compare node properties
    if (node1->insert_id != node2->insert_id ||
        node1->data_length != node2->data_length ||
        node1->size != node2->size) {
      return false;
    }
    
    // Compare data content
    if (node1->data_length > 0) {
      if (memcmp(node1->data, node2->data, node1->data_length) != 0) {
        return false;
      }
    }
    
    // Recursively compare subtrees
    return compare_trees_recursive(node1->left, node2->left) && 
           compare_trees_recursive(node1->right, node2->right);
  }
  
  bool compare_trees(SBT_tree* tree1, SBT_tree* tree2) {
    // Compare basic properties
    if (tree1->get_record_count() != tree2->get_record_count() ||
        tree1->get_next_insert_id() != tree2->get_next_insert_id()) {
      return false;
    }
    
    // Compare tree structure
    return compare_trees_recursive(tree1->get_root(), tree2->get_root());
  }
};

/** Test empty tree serialization and deserialization */
TEST_F(SBTSerializationTest, EmptyTreeSerialization) {
  // Create file
  int result = file->create(test_file_name);
  EXPECT_EQ(result, SBT_SUCCESS);
  
  // Save empty tree
  result = file->save_tree(tree);
  EXPECT_EQ(result, SBT_SUCCESS);
  
  // Load into new tree
  SBT_tree loaded_tree;
  result = file->load_tree(&loaded_tree);
  EXPECT_EQ(result, SBT_SUCCESS);
  
  // Verify trees match
  EXPECT_TRUE(compare_trees(tree, &loaded_tree));
  EXPECT_EQ(loaded_tree.get_record_count(), 0);
  EXPECT_TRUE(loaded_tree.is_empty());
}

/** Test single record serialization */
TEST_F(SBTSerializationTest, SingleRecordSerialization) {
  // Create file
  int result = file->create(test_file_name);
  EXPECT_EQ(result, SBT_SUCCESS);
  
  // Insert single record
  const char *test_data = "Single test record";
  result = tree->insert((const uchar *)test_data, strlen(test_data));
  EXPECT_EQ(result, SBT_SUCCESS);
  
  // Save tree
  result = file->save_tree(tree);
  EXPECT_EQ(result, SBT_SUCCESS);
  
  // Load into new tree
  SBT_tree loaded_tree;
  result = file->load_tree(&loaded_tree);
  EXPECT_EQ(result, SBT_SUCCESS);
  
  // Verify trees match
  EXPECT_TRUE(compare_trees(tree, &loaded_tree));
  EXPECT_EQ(loaded_tree.get_record_count(), 1);
  
  // Verify we can find the record
  SBT_node *found = loaded_tree.find_by_data((const uchar *)test_data, strlen(test_data));
  EXPECT_NE(found, nullptr);
}

/** Test multiple records serialization */
TEST_F(SBTSerializationTest, MultipleRecordsSerialization) {
  // Create file
  int result = file->create(test_file_name);
  EXPECT_EQ(result, SBT_SUCCESS);
  
  // Insert multiple records
  const char *test_records[] = {
    "Record 1: First test record",
    "Record 2: Second test record with more data",
    "Record 3: Third record",
    "Record 4: Fourth record with special chars: !@#$%^&*()",
    "Record 5: Fifth record with unicode: 你好世界"
  };
  const int num_records = sizeof(test_records) / sizeof(test_records[0]);
  
  for (int i = 0; i < num_records; i++) {
    result = tree->insert((const uchar *)test_records[i], strlen(test_records[i]));
    EXPECT_EQ(result, SBT_SUCCESS);
  }
  
  EXPECT_EQ(tree->get_record_count(), num_records);
  
  // Save tree
  result = file->save_tree(tree);
  EXPECT_EQ(result, SBT_SUCCESS);
  
  // Load into new tree
  SBT_tree loaded_tree;
  result = file->load_tree(&loaded_tree);
  EXPECT_EQ(result, SBT_SUCCESS);
  
  // Verify trees match
  EXPECT_TRUE(compare_trees(tree, &loaded_tree));
  EXPECT_EQ(loaded_tree.get_record_count(), num_records);
  
  // Verify all records can be found
  for (int i = 0; i < num_records; i++) {
    SBT_node *found = loaded_tree.find_by_data((const uchar *)test_records[i], strlen(test_records[i]));
    EXPECT_NE(found, nullptr) << "Could not find record " << i;
  }
}

/** Test serialization preserves tree structure */
TEST_F(SBTSerializationTest, TreeStructurePreservation) {
  // Create file
  int result = file->create(test_file_name);
  EXPECT_EQ(result, SBT_SUCCESS);
  
  // Insert records in specific order to create known tree structure
  const char *records[] = {
    "Record_05",  // Will be root (insert_id = 1)
    "Record_03",  // Left child (insert_id = 2)
    "Record_07",  // Right child (insert_id = 3)
    "Record_01",  // Left-left grandchild (insert_id = 4)
    "Record_04",  // Left-right grandchild (insert_id = 5)
    "Record_06",  // Right-left grandchild (insert_id = 6)
    "Record_09"   // Right-right grandchild (insert_id = 7)
  };
  const int num_records = sizeof(records) / sizeof(records[0]);
  
  for (int i = 0; i < num_records; i++) {
    result = tree->insert((const uchar *)records[i], strlen(records[i]));
    EXPECT_EQ(result, SBT_SUCCESS);
  }
  
  // Save original tree structure
  SBT_node *original_root = tree->get_root();
  EXPECT_NE(original_root, nullptr);
  
  // Save tree
  result = file->save_tree(tree);
  EXPECT_EQ(result, SBT_SUCCESS);
  
  // Load into new tree
  SBT_tree loaded_tree;
  result = file->load_tree(&loaded_tree);
  EXPECT_EQ(result, SBT_SUCCESS);
  
  // Verify complete tree structure matches
  EXPECT_TRUE(compare_trees(tree, &loaded_tree));
  
  // Verify in-order traversal produces same sequence
  std::vector<sbt_insert_id_t> original_sequence;
  std::vector<sbt_insert_id_t> loaded_sequence;
  
  // Get original sequence
  SBT_node *current = tree->get_first();
  while (current) {
    original_sequence.push_back(current->insert_id);
    current = tree->get_next(current);
  }
  
  // Get loaded sequence
  current = loaded_tree.get_first();
  while (current) {
    loaded_sequence.push_back(current->insert_id);
    current = loaded_tree.get_next(current);
  }
  
  // Sequences should match
  EXPECT_EQ(original_sequence.size(), loaded_sequence.size());
  for (size_t i = 0; i < original_sequence.size(); i++) {
    EXPECT_EQ(original_sequence[i], loaded_sequence[i]) 
      << "Sequence mismatch at position " << i;
  }
}

/** Test serialization with various data sizes */
TEST_F(SBTSerializationTest, VariousDataSizes) {
  // Create file
  int result = file->create(test_file_name);
  EXPECT_EQ(result, SBT_SUCCESS);
  
  // Test with different data sizes
  std::vector<std::string> test_data;
  
  // Empty data
  test_data.push_back("");
  
  // Small data
  test_data.push_back("A");
  
  // Medium data
  test_data.push_back("This is a medium-sized test record with some content");
  
  // Large data
  test_data.push_back(std::string(1000, 'X'));
  
  // Very large data (but within limits)
  test_data.push_back(std::string(10000, 'Y'));
  
  // Insert all test data
  for (size_t i = 0; i < test_data.size(); i++) {
    result = tree->insert((const uchar *)test_data[i].c_str(), test_data[i].length());
    EXPECT_EQ(result, SBT_SUCCESS) << "Failed to insert record " << i;
  }
  
  // Save tree
  result = file->save_tree(tree);
  EXPECT_EQ(result, SBT_SUCCESS);
  
  // Load into new tree
  SBT_tree loaded_tree;
  result = file->load_tree(&loaded_tree);
  EXPECT_EQ(result, SBT_SUCCESS);
  
  // Verify trees match
  EXPECT_TRUE(compare_trees(tree, &loaded_tree));
  
  // Verify all data can be found with correct content
  for (size_t i = 0; i < test_data.size(); i++) {
    SBT_node *found = loaded_tree.find_by_data(
      (const uchar *)test_data[i].c_str(), test_data[i].length());
    EXPECT_NE(found, nullptr) << "Could not find record " << i;
    
    if (found) {
      EXPECT_EQ(found->data_length, test_data[i].length()) 
        << "Data length mismatch for record " << i;
      
      if (found->data_length > 0) {
        EXPECT_EQ(memcmp(found->data, test_data[i].c_str(), found->data_length), 0)
          << "Data content mismatch for record " << i;
      }
    }
  }
}

/** Test serialization after tree modifications */
TEST_F(SBTSerializationTest, SerializationAfterModifications) {
  // Create file
  int result = file->create(test_file_name);
  EXPECT_EQ(result, SBT_SUCCESS);
  
  // Insert initial records
  const char *initial_records[] = {
    "Initial Record 1",
    "Initial Record 2",
    "Initial Record 3",
    "Initial Record 4",
    "Initial Record 5"
  };
  const int num_initial = sizeof(initial_records) / sizeof(initial_records[0]);
  
  for (int i = 0; i < num_initial; i++) {
    result = tree->insert((const uchar *)initial_records[i], strlen(initial_records[i]));
    EXPECT_EQ(result, SBT_SUCCESS);
  }
  
  // Remove some records
  result = tree->remove((const uchar *)initial_records[1], strlen(initial_records[1]));
  EXPECT_EQ(result, SBT_SUCCESS);
  result = tree->remove((const uchar *)initial_records[3], strlen(initial_records[3]));
  EXPECT_EQ(result, SBT_SUCCESS);
  
  // Add new records
  const char *new_records[] = {
    "New Record A",
    "New Record B"
  };
  const int num_new = sizeof(new_records) / sizeof(new_records[0]);
  
  for (int i = 0; i < num_new; i++) {
    result = tree->insert((const uchar *)new_records[i], strlen(new_records[i]));
    EXPECT_EQ(result, SBT_SUCCESS);
  }
  
  // Expected final count: 5 - 2 + 2 = 5
  EXPECT_EQ(tree->get_record_count(), 5);
  
  // Save modified tree
  result = file->save_tree(tree);
  EXPECT_EQ(result, SBT_SUCCESS);
  
  // Load into new tree
  SBT_tree loaded_tree;
  result = file->load_tree(&loaded_tree);
  EXPECT_EQ(result, SBT_SUCCESS);
  
  // Verify trees match
  EXPECT_TRUE(compare_trees(tree, &loaded_tree));
  EXPECT_EQ(loaded_tree.get_record_count(), 5);
  
  // Verify remaining initial records exist
  EXPECT_NE(loaded_tree.find_by_data((const uchar *)initial_records[0], strlen(initial_records[0])), nullptr);
  EXPECT_NE(loaded_tree.find_by_data((const uchar *)initial_records[2], strlen(initial_records[2])), nullptr);
  EXPECT_NE(loaded_tree.find_by_data((const uchar *)initial_records[4], strlen(initial_records[4])), nullptr);
  
  // Verify removed records don't exist
  EXPECT_EQ(loaded_tree.find_by_data((const uchar *)initial_records[1], strlen(initial_records[1])), nullptr);
  EXPECT_EQ(loaded_tree.find_by_data((const uchar *)initial_records[3], strlen(initial_records[3])), nullptr);
  
  // Verify new records exist
  EXPECT_NE(loaded_tree.find_by_data((const uchar *)new_records[0], strlen(new_records[0])), nullptr);
  EXPECT_NE(loaded_tree.find_by_data((const uchar *)new_records[1], strlen(new_records[1])), nullptr);
}

/** Test multiple save/load cycles */
TEST_F(SBTSerializationTest, MultipleSaveLoadCycles) {
  // Create file
  int result = file->create(test_file_name);
  EXPECT_EQ(result, SBT_SUCCESS);
  
  // Insert initial data
  const char *base_record = "Cycle test record ";
  for (int i = 0; i < 10; i++) {
    std::string record = base_record + std::to_string(i);
    result = tree->insert((const uchar *)record.c_str(), record.length());
    EXPECT_EQ(result, SBT_SUCCESS);
  }
  
  // Perform multiple save/load cycles
  for (int cycle = 0; cycle < 5; cycle++) {
    // Save current tree
    result = file->save_tree(tree);
    EXPECT_EQ(result, SBT_SUCCESS) << "Save failed in cycle " << cycle;
    
    // Load into new tree
    SBT_tree loaded_tree;
    result = file->load_tree(&loaded_tree);
    EXPECT_EQ(result, SBT_SUCCESS) << "Load failed in cycle " << cycle;
    
    // Verify trees match
    EXPECT_TRUE(compare_trees(tree, &loaded_tree)) << "Trees don't match in cycle " << cycle;
    
    // Add a new record for next cycle
    std::string new_record = "Cycle " + std::to_string(cycle) + " additional record";
    result = loaded_tree.insert((const uchar *)new_record.c_str(), new_record.length());
    EXPECT_EQ(result, SBT_SUCCESS);
    
    // Replace original tree with loaded tree for next cycle
    tree->clear();
    
    // Copy loaded tree data back to original tree
    SBT_node *current = loaded_tree.get_first();
    while (current) {
      result = tree->insert(current->data, current->data_length);
      EXPECT_EQ(result, SBT_SUCCESS);
      current = loaded_tree.get_next(current);
    }
  }
  
  // Final verification
  EXPECT_EQ(tree->get_record_count(), 15); // 10 initial + 5 cycle additions
}

/** Test error handling during serialization */
TEST_F(SBTSerializationTest, SerializationErrorHandling) {
  // Test saving to non-existent file (file not created)
  int result = file->save_tree(tree);
  EXPECT_NE(result, SBT_SUCCESS);
  
  // Test loading from non-existent file
  result = file->load_tree(tree);
  EXPECT_NE(result, SBT_SUCCESS);
  
  // Test with null parameters
  result = file->save_tree(nullptr);
  EXPECT_EQ(result, SBT_ERR_INVALID_ARGUMENT);
  
  result = file->load_tree(nullptr);
  EXPECT_EQ(result, SBT_ERR_INVALID_ARGUMENT);
}

/** Test serialization data integrity with special characters */
TEST_F(SBTSerializationTest, SpecialCharacterHandling) {
  // Create file
  int result = file->create(test_file_name);
  EXPECT_EQ(result, SBT_SUCCESS);
  
  // Test records with special characters
  const char *special_records[] = {
    "\0",                           // Null character
    "\n\r\t",                       // Control characters
    "Binary\x01\x02\x03\xFF",       // Binary data
    "Unicode: 你好世界 🌍 🚀",        // Unicode
    "Quotes: \"Hello\" 'World'",    // Quotes
    "Backslashes: \\ \\n \\t",      // Backslashes
    "Very long string: " + std::string(500, 'A') // Long string
  };
  
  // Note: We need to handle the first record specially since it contains null
  std::vector<std::pair<const char*, size_t>> test_data;
  test_data.push_back({"\0", 1});  // Null character with explicit length
  
  for (size_t i = 1; i < sizeof(special_records) / sizeof(special_records[0]); i++) {
    test_data.push_back({special_records[i], strlen(special_records[i])});
  }
  
  // Insert all special records
  for (size_t i = 0; i < test_data.size(); i++) {
    result = tree->insert((const uchar *)test_data[i].first, test_data[i].second);
    EXPECT_EQ(result, SBT_SUCCESS) << "Failed to insert special record " << i;
  }
  
  // Save tree
  result = file->save_tree(tree);
  EXPECT_EQ(result, SBT_SUCCESS);
  
  // Load into new tree
  SBT_tree loaded_tree;
  result = file->load_tree(&loaded_tree);
  EXPECT_EQ(result, SBT_SUCCESS);
  
  // Verify trees match
  EXPECT_TRUE(compare_trees(tree, &loaded_tree));
  
  // Verify all special records can be found
  for (size_t i = 0; i < test_data.size(); i++) {
    SBT_node *found = loaded_tree.find_by_data(
      (const uchar *)test_data[i].first, test_data[i].second);
    EXPECT_NE(found, nullptr) << "Could not find special record " << i;
    
    if (found) {
      EXPECT_EQ(found->data_length, test_data[i].second) 
        << "Data length mismatch for special record " << i;
      
      if (found->data_length > 0) {
        EXPECT_EQ(memcmp(found->data, test_data[i].first, found->data_length), 0)
          << "Data content mismatch for special record " << i;
      }
    }
  }
}