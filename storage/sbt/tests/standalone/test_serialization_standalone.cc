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

/** @file tests/standalone/test_serialization_standalone.cc
 SBT Tree Serialization and Deserialization Standalone Test

 Created 2025-01-25
 *******************************************************/

#include <iostream>
#include <cstring>
#include <cstdio>
#include <vector>
#include <string>

// Minimal includes to avoid MySQL dependencies
#include <cstdint>
#include <cstdlib>

// Define minimal types needed for testing
typedef unsigned char uchar;
typedef unsigned int uint;
typedef uint64_t sbt_insert_id_t;

// Minimal error codes
enum sbt_error_t {
  SBT_SUCCESS = 0,
  SBT_ERR_OUT_OF_MEMORY,
  SBT_ERR_CORRUPTED_DATA,
  SBT_ERR_INVALID_ARGUMENT
};

// Minimal SBT node structure for testing
struct SBT_node {
  uchar *data;
  uint data_length;
  sbt_insert_id_t insert_id;
  SBT_node *left;
  SBT_node *right;
  uint size;
};

// Serialized node structure
struct SBT_serialized_node {
  uint32_t has_node;
  uint64_t insert_id;
  uint32_t data_length;
  uint32_t size;
};

#define SBT_SERIALIZED_NODE_HEADER_SIZE sizeof(SBT_serialized_node)
#define SBT_FILE_ALIGNMENT 8
#define SBT_MAX_RECORD_SIZE (64 * 1024)

// Utility functions
uint64_t sbt_align_offset(uint64_t offset, uint32_t alignment) {
    return (offset + alignment - 1) & ~(alignment - 1);
}

void* test_malloc(size_t size) {
    return malloc(size);
}

void test_free(void* ptr) {
    free(ptr);
}

// Simple tree serialization functions for testing
class SerializationTester {
private:
    std::vector<SBT_node*> allocated_nodes;
    
public:
    ~SerializationTester() {
        cleanup();
    }
    
    void cleanup() {
        for (auto node : allocated_nodes) {
            if (node->data) {
                test_free(node->data);
            }
            test_free(node);
        }
        allocated_nodes.clear();
    }
    
    SBT_node* create_test_node(const char* data, sbt_insert_id_t insert_id) {
        SBT_node* node = (SBT_node*)test_malloc(sizeof(SBT_node));
        if (!node) return nullptr;
        
        node->data_length = strlen(data);
        node->data = (uchar*)test_malloc(node->data_length);
        if (!node->data) {
            test_free(node);
            return nullptr;
        }
        
        memcpy(node->data, data, node->data_length);
        node->insert_id = insert_id;
        node->left = nullptr;
        node->right = nullptr;
        node->size = 1;
        
        allocated_nodes.push_back(node);
        return node;
    }
    
    // Pre-order serialization
    int serialize_tree(SBT_node *node, uchar *buffer, uint &offset, uint buffer_size) {
        // Check buffer bounds
        if (offset + SBT_SERIALIZED_NODE_HEADER_SIZE > buffer_size) {
            return SBT_ERR_OUT_OF_MEMORY;
        }

        SBT_serialized_node *serialized = (SBT_serialized_node *)(buffer + offset);
        
        if (node == nullptr) {
            // Serialize null node
            serialized->has_node = 0;
            serialized->insert_id = 0;
            serialized->data_length = 0;
            serialized->size = 0;
            offset += SBT_SERIALIZED_NODE_HEADER_SIZE;
            return SBT_SUCCESS;
        }

        // Serialize existing node
        serialized->has_node = 1;
        serialized->insert_id = node->insert_id;
        serialized->data_length = node->data_length;
        serialized->size = node->size;
        
        offset += SBT_SERIALIZED_NODE_HEADER_SIZE;

        // Check space for record data
        if (offset + node->data_length > buffer_size) {
            return SBT_ERR_OUT_OF_MEMORY;
        }

        // Copy record data
        if (node->data_length > 0 && node->data) {
            memcpy(buffer + offset, node->data, node->data_length);
            offset += node->data_length;
        }

        // Align offset for next node
        offset = sbt_align_offset(offset, SBT_FILE_ALIGNMENT);

        // Recursively serialize left subtree
        int error = serialize_tree(node->left, buffer, offset, buffer_size);
        if (error != SBT_SUCCESS) {
            return error;
        }

        // Recursively serialize right subtree
        error = serialize_tree(node->right, buffer, offset, buffer_size);
        if (error != SBT_SUCCESS) {
            return error;
        }

        return SBT_SUCCESS;
    }
    
    // Deserialization
    int deserialize_tree(SBT_node **node, const uchar *buffer, uint &offset, uint buffer_size) {
        *node = nullptr;

        // Check buffer bounds
        if (offset + SBT_SERIALIZED_NODE_HEADER_SIZE > buffer_size) {
            return SBT_ERR_CORRUPTED_DATA;
        }

        const SBT_serialized_node *serialized = (const SBT_serialized_node *)(buffer + offset);
        
        // Check if this is a null node
        if (serialized->has_node == 0) {
            offset += SBT_SERIALIZED_NODE_HEADER_SIZE;
            return SBT_SUCCESS;
        }

        // Validate data length
        if (serialized->data_length > SBT_MAX_RECORD_SIZE) {
            return SBT_ERR_CORRUPTED_DATA;
        }

        offset += SBT_SERIALIZED_NODE_HEADER_SIZE;

        // Check space for record data
        if (offset + serialized->data_length > buffer_size) {
            return SBT_ERR_CORRUPTED_DATA;
        }

        // Allocate new node
        *node = (SBT_node*)test_malloc(sizeof(SBT_node));
        if (*node == nullptr) {
            return SBT_ERR_OUT_OF_MEMORY;
        }

        // Set node properties
        (*node)->insert_id = serialized->insert_id;
        (*node)->data_length = serialized->data_length;
        (*node)->size = serialized->size;
        (*node)->left = nullptr;
        (*node)->right = nullptr;

        // Copy record data if present
        if (serialized->data_length > 0) {
            (*node)->data = (uchar *)test_malloc(serialized->data_length);
            if ((*node)->data == nullptr) {
                test_free(*node);
                *node = nullptr;
                return SBT_ERR_OUT_OF_MEMORY;
            }
            memcpy((*node)->data, buffer + offset, serialized->data_length);
            offset += serialized->data_length;
        } else {
            (*node)->data = nullptr;
        }

        // Align offset for next node
        offset = sbt_align_offset(offset, SBT_FILE_ALIGNMENT);

        // Recursively deserialize left subtree
        int error = deserialize_tree(&((*node)->left), buffer, offset, buffer_size);
        if (error != SBT_SUCCESS) {
            return error;
        }

        // Recursively deserialize right subtree
        error = deserialize_tree(&((*node)->right), buffer, offset, buffer_size);
        if (error != SBT_SUCCESS) {
            return error;
        }

        allocated_nodes.push_back(*node);
        return SBT_SUCCESS;
    }
    
    uint calculate_serialize_size(SBT_node *node) {
        if (node == nullptr) {
            return SBT_SERIALIZED_NODE_HEADER_SIZE;
        }

        uint size = SBT_SERIALIZED_NODE_HEADER_SIZE;
        
        // Add space for record data
        size += node->data_length;
        
        // Add alignment padding
        size = sbt_align_offset(size, SBT_FILE_ALIGNMENT);
        
        // Recursively calculate size for subtrees
        size += calculate_serialize_size(node->left);
        size += calculate_serialize_size(node->right);
        
        return size;
    }
    
    bool compare_trees(SBT_node* tree1, SBT_node* tree2) {
        // Both null
        if (tree1 == nullptr && tree2 == nullptr) {
            return true;
        }
        
        // One null, one not
        if (tree1 == nullptr || tree2 == nullptr) {
            return false;
        }
        
        // Compare node data
        if (tree1->insert_id != tree2->insert_id ||
            tree1->data_length != tree2->data_length ||
            tree1->size != tree2->size) {
            return false;
        }
        
        // Compare data content
        if (tree1->data_length > 0) {
            if (memcmp(tree1->data, tree2->data, tree1->data_length) != 0) {
                return false;
            }
        }
        
        // Recursively compare subtrees
        return compare_trees(tree1->left, tree2->left) && 
               compare_trees(tree1->right, tree2->right);
    }
    
    void print_tree(SBT_node* node, int depth = 0) {
        if (node == nullptr) {
            for (int i = 0; i < depth; i++) std::cout << "  ";
            std::cout << "NULL" << std::endl;
            return;
        }
        
        for (int i = 0; i < depth; i++) std::cout << "  ";
        std::cout << "Node(id=" << node->insert_id 
                  << ", size=" << node->size 
                  << ", data_len=" << node->data_length;
        if (node->data_length > 0 && node->data_length < 50) {
            std::cout << ", data=\"";
            for (uint i = 0; i < node->data_length; i++) {
                std::cout << (char)node->data[i];
            }
            std::cout << "\"";
        }
        std::cout << ")" << std::endl;
        
        if (node->left || node->right) {
            for (int i = 0; i < depth; i++) std::cout << "  ";
            std::cout << "Left:" << std::endl;
            print_tree(node->left, depth + 1);
            
            for (int i = 0; i < depth; i++) std::cout << "  ";
            std::cout << "Right:" << std::endl;
            print_tree(node->right, depth + 1);
        }
    }
};

// Test functions
bool test_empty_tree_serialization() {
    std::cout << "\n=== Test: Empty Tree Serialization ===" << std::endl;
    
    SerializationTester tester;
    
    // Test serializing null tree
    uint buffer_size = 1024;
    uchar* buffer = (uchar*)test_malloc(buffer_size);
    if (!buffer) {
        std::cout << "FAILED: Could not allocate buffer" << std::endl;
        return false;
    }
    
    uint offset = 0;
    int result = tester.serialize_tree(nullptr, buffer, offset, buffer_size);
    if (result != SBT_SUCCESS) {
        std::cout << "FAILED: Could not serialize empty tree, error: " << result << std::endl;
        test_free(buffer);
        return false;
    }
    
    std::cout << "Empty tree serialized, size: " << offset << " bytes" << std::endl;
    
    // Test deserializing empty tree
    SBT_node* deserialized = nullptr;
    offset = 0;
    result = tester.deserialize_tree(&deserialized, buffer, offset, buffer_size);
    if (result != SBT_SUCCESS) {
        std::cout << "FAILED: Could not deserialize empty tree, error: " << result << std::endl;
        test_free(buffer);
        return false;
    }
    
    if (deserialized != nullptr) {
        std::cout << "FAILED: Deserialized tree should be null" << std::endl;
        test_free(buffer);
        return false;
    }
    
    test_free(buffer);
    std::cout << "PASSED: Empty tree serialization/deserialization" << std::endl;
    return true;
}

bool test_single_node_serialization() {
    std::cout << "\n=== Test: Single Node Serialization ===" << std::endl;
    
    SerializationTester tester;
    
    // Create single node
    SBT_node* original = tester.create_test_node("Hello, World!", 42);
    if (!original) {
        std::cout << "FAILED: Could not create test node" << std::endl;
        return false;
    }
    
    std::cout << "Original tree:" << std::endl;
    tester.print_tree(original);
    
    // Calculate buffer size
    uint buffer_size = tester.calculate_serialize_size(original);
    std::cout << "Required buffer size: " << buffer_size << " bytes" << std::endl;
    
    uchar* buffer = (uchar*)test_malloc(buffer_size);
    if (!buffer) {
        std::cout << "FAILED: Could not allocate buffer" << std::endl;
        return false;
    }
    
    // Serialize
    uint offset = 0;
    int result = tester.serialize_tree(original, buffer, offset, buffer_size);
    if (result != SBT_SUCCESS) {
        std::cout << "FAILED: Could not serialize single node, error: " << result << std::endl;
        test_free(buffer);
        return false;
    }
    
    std::cout << "Serialized size: " << offset << " bytes" << std::endl;
    
    // Deserialize
    SBT_node* deserialized = nullptr;
    offset = 0;
    result = tester.deserialize_tree(&deserialized, buffer, offset, buffer_size);
    if (result != SBT_SUCCESS) {
        std::cout << "FAILED: Could not deserialize single node, error: " << result << std::endl;
        test_free(buffer);
        return false;
    }
    
    std::cout << "Deserialized tree:" << std::endl;
    tester.print_tree(deserialized);
    
    // Compare trees
    if (!tester.compare_trees(original, deserialized)) {
        std::cout << "FAILED: Trees do not match after serialization" << std::endl;
        test_free(buffer);
        return false;
    }
    
    test_free(buffer);
    std::cout << "PASSED: Single node serialization/deserialization" << std::endl;
    return true;
}

bool test_complex_tree_serialization() {
    std::cout << "\n=== Test: Complex Tree Serialization ===" << std::endl;
    
    SerializationTester tester;
    
    // Create a more complex tree structure
    SBT_node* root = tester.create_test_node("Root Node", 10);
    SBT_node* left = tester.create_test_node("Left Child", 5);
    SBT_node* right = tester.create_test_node("Right Child", 15);
    SBT_node* left_left = tester.create_test_node("Left-Left Grandchild", 3);
    SBT_node* right_right = tester.create_test_node("Right-Right Grandchild", 20);
    
    if (!root || !left || !right || !left_left || !right_right) {
        std::cout << "FAILED: Could not create test nodes" << std::endl;
        return false;
    }
    
    // Build tree structure
    root->left = left;
    root->right = right;
    root->size = 5;
    
    left->left = left_left;
    left->size = 2;
    
    right->right = right_right;
    right->size = 2;
    
    std::cout << "Original complex tree:" << std::endl;
    tester.print_tree(root);
    
    // Calculate buffer size
    uint buffer_size = tester.calculate_serialize_size(root);
    std::cout << "Required buffer size: " << buffer_size << " bytes" << std::endl;
    
    uchar* buffer = (uchar*)test_malloc(buffer_size);
    if (!buffer) {
        std::cout << "FAILED: Could not allocate buffer" << std::endl;
        return false;
    }
    
    // Serialize
    uint offset = 0;
    int result = tester.serialize_tree(root, buffer, offset, buffer_size);
    if (result != SBT_SUCCESS) {
        std::cout << "FAILED: Could not serialize complex tree, error: " << result << std::endl;
        test_free(buffer);
        return false;
    }
    
    std::cout << "Serialized size: " << offset << " bytes" << std::endl;
    
    // Deserialize
    SBT_node* deserialized = nullptr;
    offset = 0;
    result = tester.deserialize_tree(&deserialized, buffer, offset, buffer_size);
    if (result != SBT_SUCCESS) {
        std::cout << "FAILED: Could not deserialize complex tree, error: " << result << std::endl;
        test_free(buffer);
        return false;
    }
    
    std::cout << "Deserialized complex tree:" << std::endl;
    tester.print_tree(deserialized);
    
    // Compare trees
    if (!tester.compare_trees(root, deserialized)) {
        std::cout << "FAILED: Complex trees do not match after serialization" << std::endl;
        test_free(buffer);
        return false;
    }
    
    test_free(buffer);
    std::cout << "PASSED: Complex tree serialization/deserialization" << std::endl;
    return true;
}

bool test_error_conditions() {
    std::cout << "\n=== Test: Error Conditions ===" << std::endl;
    
    SerializationTester tester;
    
    // Test buffer too small
    SBT_node* node = tester.create_test_node("Test data", 1);
    if (!node) {
        std::cout << "FAILED: Could not create test node" << std::endl;
        return false;
    }
    
    // Try to serialize with insufficient buffer
    uint small_buffer_size = 10; // Too small
    uchar* small_buffer = (uchar*)test_malloc(small_buffer_size);
    if (!small_buffer) {
        std::cout << "FAILED: Could not allocate small buffer" << std::endl;
        return false;
    }
    
    uint offset = 0;
    int result = tester.serialize_tree(node, small_buffer, offset, small_buffer_size);
    if (result != SBT_ERR_OUT_OF_MEMORY) {
        std::cout << "FAILED: Should have returned OUT_OF_MEMORY error, got: " << result << std::endl;
        test_free(small_buffer);
        return false;
    }
    
    test_free(small_buffer);
    std::cout << "PASSED: Buffer overflow detection" << std::endl;
    
    // Test corrupted data during deserialization
    uint buffer_size = 1024;
    uchar* buffer = (uchar*)test_malloc(buffer_size);
    if (!buffer) {
        std::cout << "FAILED: Could not allocate buffer for corruption test" << std::endl;
        return false;
    }
    
    // Create corrupted serialized data
    SBT_serialized_node* corrupted = (SBT_serialized_node*)buffer;
    corrupted->has_node = 1;
    corrupted->insert_id = 1;
    corrupted->data_length = SBT_MAX_RECORD_SIZE + 1; // Invalid size
    corrupted->size = 1;
    
    offset = 0;
    SBT_node* deserialized = nullptr;
    result = tester.deserialize_tree(&deserialized, buffer, offset, buffer_size);
    if (result != SBT_ERR_CORRUPTED_DATA) {
        std::cout << "FAILED: Should have detected corrupted data, got: " << result << std::endl;
        test_free(buffer);
        return false;
    }
    
    test_free(buffer);
    std::cout << "PASSED: Corrupted data detection" << std::endl;
    
    return true;
}

bool test_data_integrity() {
    std::cout << "\n=== Test: Data Integrity ===" << std::endl;
    
    SerializationTester tester;
    
    // Test with various data types and sizes
    std::vector<std::string> test_data = {
        "",                                    // Empty string
        "A",                                   // Single character
        "Hello, World!",                       // Normal string
        "Unicode: 你好世界 🌍",                 // Unicode
        "Special chars: !@#$%^&*()_+-=[]{}|;:,.<>?", // Special characters
        std::string(1000, 'X'),                // Large string
        std::string(1, '\0'),                  // Null character
        "Line1\nLine2\rLine3\tTabbed"          // Control characters
    };
    
    for (size_t i = 0; i < test_data.size(); i++) {
        const std::string& data = test_data[i];
        
        // Create node
        SBT_node* original = tester.create_test_node(data.c_str(), i + 1);
        if (!original) {
            std::cout << "FAILED: Could not create node for test " << i << std::endl;
            return false;
        }
        
        // Serialize
        uint buffer_size = tester.calculate_serialize_size(original);
        uchar* buffer = (uchar*)test_malloc(buffer_size);
        if (!buffer) {
            std::cout << "FAILED: Could not allocate buffer for test " << i << std::endl;
            return false;
        }
        
        uint offset = 0;
        int result = tester.serialize_tree(original, buffer, offset, buffer_size);
        if (result != SBT_SUCCESS) {
            std::cout << "FAILED: Could not serialize test " << i << ", error: " << result << std::endl;
            test_free(buffer);
            return false;
        }
        
        // Deserialize
        SBT_node* deserialized = nullptr;
        offset = 0;
        result = tester.deserialize_tree(&deserialized, buffer, offset, buffer_size);
        if (result != SBT_SUCCESS) {
            std::cout << "FAILED: Could not deserialize test " << i << ", error: " << result << std::endl;
            test_free(buffer);
            return false;
        }
        
        // Verify data integrity
        if (!tester.compare_trees(original, deserialized)) {
            std::cout << "FAILED: Data integrity check failed for test " << i << std::endl;
            test_free(buffer);
            return false;
        }
        
        test_free(buffer);
        std::cout << "Test " << i << " (data length: " << data.length() << ") - PASSED" << std::endl;
    }
    
    std::cout << "PASSED: All data integrity tests" << std::endl;
    return true;
}

int main() {
    std::cout << "=== SBT Tree Serialization and Deserialization Test Suite ===" << std::endl;
    std::cout << "Testing pre-order traversal serialization and tree reconstruction..." << std::endl;
    
    int passed = 0;
    int total = 0;
    
    // Run all tests
    total++; if (test_empty_tree_serialization()) passed++;
    total++; if (test_single_node_serialization()) passed++;
    total++; if (test_complex_tree_serialization()) passed++;
    total++; if (test_error_conditions()) passed++;
    total++; if (test_data_integrity()) passed++;
    
    std::cout << "\n=== Test Results ===" << std::endl;
    std::cout << "Passed: " << passed << "/" << total << " tests" << std::endl;
    
    if (passed == total) {
        std::cout << "\n🎉 ALL SERIALIZATION TESTS PASSED! 🎉" << std::endl;
        std::cout << "Tree serialization and deserialization implementation is verified." << std::endl;
        return 0;
    } else {
        std::cout << "\n❌ SOME TESTS FAILED! ❌" << std::endl;
        std::cout << "Please review the failed tests and fix the implementation." << std::endl;
        return 1;
    }
}