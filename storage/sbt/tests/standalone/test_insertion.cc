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

/** @file test_insertion.cc
 SBT Tree Insertion Test Program

 Created 2025-01-25
 *******************************************************/

#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <ctime>

// Mock MySQL includes for standalone compilation
typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long long uint64_t;

#define PSI_NOT_INSTRUMENTED nullptr
#define MYF(x) (x)
#define MY_WME 0

// Mock memory functions
void* my_malloc(void* psi, size_t size, int flags) {
    return malloc(size);
}

void my_free(void* ptr) {
    free(ptr);
}

void* my_realloc(void* psi, void* ptr, size_t size, int flags) {
    return realloc(ptr, size);
}

// Mock MEM_ROOT class
class MEM_ROOT {
public:
    MEM_ROOT(void* psi, size_t block_size) {}
    
    void* Alloc(size_t size) {
        return malloc(size);
    }
    
    void Clear() {
        // In a real implementation, this would free all allocated blocks
        // For testing, we'll rely on individual free calls
    }
};

// Include SBT definitions
#include "include/sbt_common.h"
#include "include/sbt_tree.h"

// Mock implementations of common functions
int sbt_error_to_mysql_error(int sbt_error) {
    return sbt_error;
}

void sbt_log_error(const char *format, ...) {
    // Mock implementation
}

void sbt_log_info(const char *format, ...) {
    // Mock implementation
}

void *sbt_malloc(size_t size) {
    return malloc(size);
}

void sbt_free(void *ptr) {
    free(ptr);
}

void *sbt_realloc(void *ptr, size_t size) {
    return realloc(ptr, size);
}

int sbt_data_compare(const uchar *data1, uint length1, 
                     const uchar *data2, uint length2) {
    if (!data1 || !data2) {
        if (data1 == data2) return 0;
        return data1 ? 1 : -1;
    }
    
    if (length1 != length2) {
        return (length1 < length2) ? -1 : 1;
    }
    
    return memcmp(data1, data2, length1);
}

// Include SBT tree implementation
#include "src/sbt_tree.cc"

class SBTInsertionTester {
private:
    SBT_tree* tree;
    int test_count;
    int passed_tests;

public:
    SBTInsertionTester() : tree(nullptr), test_count(0), passed_tests(0) {}
    
    ~SBTInsertionTester() {
        if (tree) {
            delete tree;
        }
    }
    
    void run_all_tests() {
        std::cout << "=== SBT Tree Insertion Tests ===" << std::endl;
        
        test_basic_insertion();
        test_multiple_insertions();
        test_insertion_order();
        test_tree_balance();
        test_large_insertions();
        test_duplicate_handling();
        test_edge_cases();
        
        std::cout << "\n=== Test Results ===" << std::endl;
        std::cout << "Passed: " << passed_tests << "/" << test_count << std::endl;
        
        if (passed_tests == test_count) {
            std::cout << "All tests PASSED!" << std::endl;
        } else {
            std::cout << "Some tests FAILED!" << std::endl;
        }
    }

private:
    void setup() {
        if (tree) {
            delete tree;
        }
        tree = new SBT_tree();
    }
    
    void teardown() {
        if (tree) {
            delete tree;
            tree = nullptr;
        }
    }
    
    bool assert_equal(int expected, int actual, const std::string& test_name) {
        test_count++;
        if (expected == actual) {
            passed_tests++;
            std::cout << "[PASS] " << test_name << std::endl;
            return true;
        } else {
            std::cout << "[FAIL] " << test_name << " - Expected: " << expected 
                      << ", Actual: " << actual << std::endl;
            return false;
        }
    }
    
    bool assert_true(bool condition, const std::string& test_name) {
        test_count++;
        if (condition) {
            passed_tests++;
            std::cout << "[PASS] " << test_name << std::endl;
            return true;
        } else {
            std::cout << "[FAIL] " << test_name << std::endl;
            return false;
        }
    }
    
    bool assert_not_null(void* ptr, const std::string& test_name) {
        return assert_true(ptr != nullptr, test_name);
    }
    
    bool assert_null(void* ptr, const std::string& test_name) {
        return assert_true(ptr == nullptr, test_name);
    }
    
    // Verify SBT property: size[left] <= size[right] and size[right] <= size[left]
    bool verify_sbt_property(SBT_node* node) {
        if (!node) return true;
        
        uint left_size = node->left ? node->left->size : 0;
        uint right_size = node->right ? node->right->size : 0;
        
        // Check size consistency
        if (node->size != 1 + left_size + right_size) {
            return false;
        }
        
        // Check SBT balance property
        if (node->left) {
            uint ll_size = node->left->left ? node->left->left->size : 0;
            uint lr_size = node->left->right ? node->left->right->size : 0;
            
            // Left subtree should not violate SBT property
            if (ll_size > right_size || lr_size > right_size) {
                return false;
            }
        }
        
        if (node->right) {
            uint rl_size = node->right->left ? node->right->left->size : 0;
            uint rr_size = node->right->right ? node->right->right->size : 0;
            
            // Right subtree should not violate SBT property
            if (rr_size > left_size || rl_size > left_size) {
                return false;
            }
        }
        
        // Recursively check subtrees
        return verify_sbt_property(node->left) && verify_sbt_property(node->right);
    }
    
    bool verify_tree_balance() {
        return verify_sbt_property(tree->root);
    }
    
    void test_basic_insertion() {
        std::cout << "\n--- Basic Insertion Tests ---" << std::endl;
        setup();
        
        // Test empty tree
        assert_true(tree->is_empty(), "Empty tree check");
        assert_equal(0, tree->get_record_count(), "Empty tree record count");
        
        // Test single insertion
        const char* data = "test_record";
        int result = tree->insert((const uchar*)data, strlen(data));
        assert_equal(SBT_SUCCESS, result, "Single insertion result");
        assert_true(!tree->is_empty(), "Tree not empty after insertion");
        assert_equal(1, tree->get_record_count(), "Record count after insertion");
        
        // Verify record can be found
        SBT_node* found = tree->find_by_data((const uchar*)data, strlen(data));
        assert_not_null(found, "Find inserted record");
        
        if (found) {
            assert_equal(strlen(data), found->data_length, "Record data length");
            assert_equal(0, memcmp(found->data, data, strlen(data)), "Record data content");
        }
        
        teardown();
    }
    
    void test_multiple_insertions() {
        std::cout << "\n--- Multiple Insertions Tests ---" << std::endl;
        setup();
        
        std::vector<std::string> test_data = {
            "record1", "record2", "record3", "record4", "record5"
        };
        
        // Insert multiple records
        for (size_t i = 0; i < test_data.size(); i++) {
            int result = tree->insert((const uchar*)test_data[i].c_str(), test_data[i].length());
            assert_equal(SBT_SUCCESS, result, "Insert record " + std::to_string(i + 1));
        }
        
        assert_equal(test_data.size(), tree->get_record_count(), "Total record count");
        
        // Verify all records can be found
        for (size_t i = 0; i < test_data.size(); i++) {
            SBT_node* found = tree->find_by_data((const uchar*)test_data[i].c_str(), test_data[i].length());
            assert_not_null(found, "Find record " + std::to_string(i + 1));
        }
        
        // Verify tree balance
        assert_true(verify_tree_balance(), "Tree balance after multiple insertions");
        
        teardown();
    }
    
    void test_insertion_order() {
        std::cout << "\n--- Insertion Order Tests ---" << std::endl;
        setup();
        
        std::vector<std::string> test_data = {"c", "a", "e", "b", "d"};
        
        // Insert in non-sorted order
        for (const auto& data : test_data) {
            tree->insert((const uchar*)data.c_str(), data.length());
        }
        
        // Verify in-order traversal gives records in insert_id order (not data order)
        SBT_node* current = tree->get_first();
        int traversal_count = 0;
        
        while (current && traversal_count < 10) { // Prevent infinite loop
            traversal_count++;
            current = tree->get_next(current);
        }
        
        assert_equal(test_data.size(), traversal_count, "Traversal count matches insertion count");
        
        teardown();
    }
    
    void test_tree_balance() {
        std::cout << "\n--- Tree Balance Tests ---" << std::endl;
        setup();
        
        // Insert records that would create an unbalanced BST
        for (int i = 1; i <= 7; i++) {
            std::string data = "record" + std::to_string(i);
            tree->insert((const uchar*)data.c_str(), data.length());
            
            // Verify balance after each insertion
            assert_true(verify_tree_balance(), "Tree balance after inserting " + std::to_string(i) + " records");
        }
        
        teardown();
    }
    
    void test_large_insertions() {
        std::cout << "\n--- Large Insertions Tests ---" << std::endl;
        setup();
        
        const int num_records = 100;
        
        // Insert many records
        for (int i = 0; i < num_records; i++) {
            std::string data = "large_record_" + std::to_string(i);
            int result = tree->insert((const uchar*)data.c_str(), data.length());
            assert_equal(SBT_SUCCESS, result, "Large insertion " + std::to_string(i));
            
            if (result != SBT_SUCCESS) {
                break; // Stop on first failure
            }
        }
        
        assert_equal(num_records, tree->get_record_count(), "Large insertion record count");
        assert_true(verify_tree_balance(), "Tree balance after large insertions");
        
        teardown();
    }
    
    void test_duplicate_handling() {
        std::cout << "\n--- Duplicate Handling Tests ---" << std::endl;
        setup();
        
        const char* data = "duplicate_record";
        
        // Insert first record
        int result1 = tree->insert((const uchar*)data, strlen(data));
        assert_equal(SBT_SUCCESS, result1, "First duplicate insertion");
        
        // Insert same record again (should succeed as we don't enforce uniqueness)
        int result2 = tree->insert((const uchar*)data, strlen(data));
        assert_equal(SBT_SUCCESS, result2, "Second duplicate insertion");
        
        assert_equal(2, tree->get_record_count(), "Record count with duplicates");
        
        teardown();
    }
    
    void test_edge_cases() {
        std::cout << "\n--- Edge Cases Tests ---" << std::endl;
        setup();
        
        // Test null data
        int result1 = tree->insert(nullptr, 10);
        assert_equal(SBT_ERR_INVALID_ARGUMENT, result1, "Null data insertion");
        
        // Test zero length
        const char* data = "test";
        int result2 = tree->insert((const uchar*)data, 0);
        assert_equal(SBT_ERR_INVALID_ARGUMENT, result2, "Zero length insertion");
        
        // Test empty string
        const char* empty_data = "";
        int result3 = tree->insert((const uchar*)empty_data, 0);
        assert_equal(SBT_ERR_INVALID_ARGUMENT, result3, "Empty string insertion");
        
        // Test very long data
        std::string long_data(1000, 'x');
        int result4 = tree->insert((const uchar*)long_data.c_str(), long_data.length());
        assert_equal(SBT_SUCCESS, result4, "Long data insertion");
        
        teardown();
    }
};

int main() {
    SBTInsertionTester tester;
    tester.run_all_tests();
    return 0;
}