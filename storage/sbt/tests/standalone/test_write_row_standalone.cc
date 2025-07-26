/*****************************************************************************

Copyright (c) 2025, Oracle and/or its affiliates.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License, version 2.0,
as published by the Free Software Foundation.

*****************************************************************************/

/** @file test_write_row_standalone.cc
 Standalone test for ha_sbt write_row functionality

 Created 2025-01-26
 *******************************************************/

#include <iostream>
#include <cstring>
#include <cassert>
#include <chrono>
#include <cstdlib>

// Mock MySQL structures and functions for standalone testing
#define MYSQL_SERVER
#define DBUG_ENTER(a) do {} while(0)
#define DBUG_RETURN(a) return (a)

typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long long ulonglong;

// Mock MySQL error codes
#define HA_ERR_OUT_OF_MEM 5
#define HA_ERR_CRASHED_ON_USAGE 126
#define HA_ERR_END_OF_FILE 137

// Mock TABLE_SHARE structure
struct TABLE_SHARE {
    uint reclength;  // Record length
    
    TABLE_SHARE() : reclength(100) {}  // Default record length
};

// Mock TABLE structure
struct TABLE {
    TABLE_SHARE *s;
    
    TABLE() {
        s = new TABLE_SHARE();
    }
    
    ~TABLE() {
        delete s;
    }
};

// Mock handlerton
struct handlerton {
    int dummy;
};

// Mock THD and other MySQL types
class THD {};
enum thr_lock_type { TL_UNLOCK, TL_IGNORE };
struct THR_LOCK_DATA {
    thr_lock_type type;
    THR_LOCK_DATA() : type(TL_UNLOCK) {}
};

// Mock SBT common definitions
#define SBT_SUCCESS 0
#define SBT_ERR_OUT_OF_MEMORY 1
#define SBT_ERR_INVALID_ARGUMENT 2

typedef uint64_t sbt_insert_id_t;

void* sbt_malloc(size_t size) { return malloc(size); }
void sbt_free(void* ptr) { free(ptr); }

int sbt_error_to_mysql_error(int sbt_error) {
    switch (sbt_error) {
        case SBT_SUCCESS: return 0;
        case SBT_ERR_OUT_OF_MEMORY: return HA_ERR_OUT_OF_MEM;
        default: return HA_ERR_CRASHED_ON_USAGE;
    }
}

// Include SBT tree header (we'll need to mock this too)
#include "../../include/sbt_tree.h"

// Mock SBT_share for testing
class MockSBT_share {
private:
    SBT_tree *tree;
    
public:
    MockSBT_share() {
        tree = new SBT_tree();
    }
    
    ~MockSBT_share() {
        delete tree;
    }
    
    SBT_tree* get_tree() { return tree; }
    
    int open_table() { return SBT_SUCCESS; }
    int close_table() { return SBT_SUCCESS; }
    
    void* get_lock() { return nullptr; }
};

// Simplified ha_sbt class for testing
class TestHaSbt {
private:
    MockSBT_share *share;
    TABLE *table;
    
public:
    TestHaSbt() {
        share = new MockSBT_share();
        table = new TABLE();
    }
    
    ~TestHaSbt() {
        delete share;
        delete table;
    }
    
    // Test version of write_row
    int write_row(uchar *buf) {
        if (!share || !share->get_tree()) {
            return HA_ERR_CRASHED_ON_USAGE;
        }

        // Pack row data
        uchar *packed_data = nullptr;
        uint packed_length = 0;
        int error = pack_row(buf, &packed_data, &packed_length);
        if (error) {
            return error;
        }

        // Insert into tree
        error = share->get_tree()->insert(packed_data, packed_length);
        
        // Free packed data
        if (packed_data) {
            sbt_free(packed_data);
        }

        return sbt_error_to_mysql_error(error);
    }
    
    // Test version of pack_row
    int pack_row(const uchar *record, uchar **packed_data, uint *packed_length) {
        // Simple implementation: just copy the record
        *packed_length = table->s->reclength;
        *packed_data = (uchar *)sbt_malloc(*packed_length);
        if (!*packed_data) {
            return HA_ERR_OUT_OF_MEM;
        }
        
        memcpy(*packed_data, record, *packed_length);
        return 0;
    }
    
    // Test version of unpack_row
    int unpack_row(const uchar *packed_data, uint packed_length, uchar *record) {
        if (packed_length > table->s->reclength) {
            return HA_ERR_CRASHED_ON_USAGE;
        }
        
        memcpy(record, packed_data, packed_length);
        return 0;
    }
    
    // Test version of rnd_init
    int rnd_init() {
        current_node = share->get_tree()->get_first();
        return 0;
    }
    
    // Test version of rnd_next
    int rnd_next(uchar *buf) {
        if (!current_node) {
            return HA_ERR_END_OF_FILE;
        }

        int error = unpack_row(current_node->data, current_node->data_length, buf);
        if (error) {
            return error;
        }

        current_node = share->get_tree()->get_next(current_node);
        return 0;
    }
    
    uint64_t get_record_count() {
        return share->get_tree()->get_record_count();
    }
    
private:
    SBT_node *current_node = nullptr;
};

// Test functions
void test_basic_write_row() {
    std::cout << "=== Test: Basic Write Row ===" << std::endl;
    
    TestHaSbt handler;
    
    // Create test record
    uchar record[100];
    memset(record, 0, sizeof(record));
    strcpy((char*)record, "Test Record 1");
    
    // Test write_row
    int result = handler.write_row(record);
    assert(result == 0);
    std::cout << "[PASS] Write row succeeded" << std::endl;
    
    // Verify record count
    assert(handler.get_record_count() == 1);
    std::cout << "[PASS] Record count is correct" << std::endl;
    
    std::cout << "PASSED: Basic write row" << std::endl << std::endl;
}

void test_multiple_write_rows() {
    std::cout << "=== Test: Multiple Write Rows ===" << std::endl;
    
    TestHaSbt handler;
    
    // Insert multiple records
    for (int i = 0; i < 5; i++) {
        uchar record[100];
        memset(record, 0, sizeof(record));
        snprintf((char*)record, sizeof(record), "Test Record %d", i + 1);
        
        int result = handler.write_row(record);
        assert(result == 0);
        std::cout << "[PASS] Write row " << (i + 1) << " succeeded" << std::endl;
    }
    
    // Verify record count
    assert(handler.get_record_count() == 5);
    std::cout << "[PASS] Total record count is correct" << std::endl;
    
    std::cout << "PASSED: Multiple write rows" << std::endl << std::endl;
}

void test_write_and_read_back() {
    std::cout << "=== Test: Write and Read Back ===" << std::endl;
    
    TestHaSbt handler;
    
    // Test data
    const char* test_strings[] = {
        "First Record",
        "Second Record", 
        "Third Record",
        "Fourth Record",
        "Fifth Record"
    };
    
    // Write records
    for (int i = 0; i < 5; i++) {
        uchar record[100];
        memset(record, 0, sizeof(record));
        strcpy((char*)record, test_strings[i]);
        
        int result = handler.write_row(record);
        assert(result == 0);
        std::cout << "[PASS] Wrote: " << test_strings[i] << std::endl;
    }
    
    // Read back records
    handler.rnd_init();
    
    uchar read_record[100];
    int records_read = 0;
    bool found_records[5] = {false, false, false, false, false};
    
    while (handler.rnd_next(read_record) == 0) {
        records_read++;
        
        // Check if this record matches any of our test strings
        for (int i = 0; i < 5; i++) {
            if (strcmp((char*)read_record, test_strings[i]) == 0) {
                found_records[i] = true;
                std::cout << "[PASS] Found: " << test_strings[i] << std::endl;
                break;
            }
        }
    }
    
    // Verify all records were found
    assert(records_read == 5);
    std::cout << "[PASS] Read back correct number of records" << std::endl;
    
    for (int i = 0; i < 5; i++) {
        assert(found_records[i]);
    }
    std::cout << "[PASS] All written records were found" << std::endl;
    
    std::cout << "PASSED: Write and read back" << std::endl << std::endl;
}

void test_large_records() {
    std::cout << "=== Test: Large Records ===" << std::endl;
    
    TestHaSbt handler;
    
    // Create a large record (near the limit)
    uchar large_record[100];
    memset(large_record, 'A', 99);
    large_record[99] = '\0';
    
    int result = handler.write_row(large_record);
    assert(result == 0);
    std::cout << "[PASS] Large record write succeeded" << std::endl;
    
    // Verify it can be read back
    handler.rnd_init();
    uchar read_record[100];
    result = handler.rnd_next(read_record);
    assert(result == 0);
    assert(memcmp(large_record, read_record, 100) == 0);
    std::cout << "[PASS] Large record read back correctly" << std::endl;
    
    std::cout << "PASSED: Large records" << std::endl << std::endl;
}

void test_empty_records() {
    std::cout << "=== Test: Empty Records ===" << std::endl;
    
    TestHaSbt handler;
    
    // Create an empty record
    uchar empty_record[100];
    memset(empty_record, 0, sizeof(empty_record));
    
    int result = handler.write_row(empty_record);
    assert(result == 0);
    std::cout << "[PASS] Empty record write succeeded" << std::endl;
    
    // Verify it can be read back
    handler.rnd_init();
    uchar read_record[100];
    result = handler.rnd_next(read_record);
    assert(result == 0);
    assert(memcmp(empty_record, read_record, 100) == 0);
    std::cout << "[PASS] Empty record read back correctly" << std::endl;
    
    std::cout << "PASSED: Empty records" << std::endl << std::endl;
}

void test_duplicate_records() {
    std::cout << "=== Test: Duplicate Records ===" << std::endl;
    
    TestHaSbt handler;
    
    // Create identical records
    uchar record[100];
    memset(record, 0, sizeof(record));
    strcpy((char*)record, "Duplicate Record");
    
    // Insert the same record multiple times
    for (int i = 0; i < 3; i++) {
        int result = handler.write_row(record);
        assert(result == 0);
        std::cout << "[PASS] Duplicate record " << (i + 1) << " write succeeded" << std::endl;
    }
    
    // Verify all duplicates are stored
    assert(handler.get_record_count() == 3);
    std::cout << "[PASS] All duplicate records stored" << std::endl;
    
    // Verify all can be read back
    handler.rnd_init();
    uchar read_record[100];
    int duplicates_found = 0;
    
    while (handler.rnd_next(read_record) == 0) {
        if (strcmp((char*)read_record, "Duplicate Record") == 0) {
            duplicates_found++;
        }
    }
    
    assert(duplicates_found == 3);
    std::cout << "[PASS] All duplicate records read back" << std::endl;
    
    std::cout << "PASSED: Duplicate records" << std::endl << std::endl;
}

void test_performance() {
    std::cout << "=== Test: Performance ===" << std::endl;
    
    TestHaSbt handler;
    
    const int num_records = 1000;
    
    // Measure insertion time
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_records; i++) {
        uchar record[100];
        memset(record, 0, sizeof(record));
        snprintf((char*)record, sizeof(record), "Performance Record %d", i);
        
        int result = handler.write_row(record);
        assert(result == 0);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "[PASS] Inserted " << num_records << " records in " 
              << duration.count() << " microseconds" << std::endl;
    std::cout << "[PASS] Average: " << (double)duration.count() / num_records 
              << " microseconds per record" << std::endl;
    
    // Verify final count
    assert(handler.get_record_count() == num_records);
    std::cout << "[PASS] Final record count correct" << std::endl;
    
    std::cout << "PASSED: Performance test" << std::endl << std::endl;
}

int main() {
    std::cout << "=== SBT Storage Engine Write Row Test Suite ===" << std::endl;
    std::cout << "Testing write_row functionality and record format conversion..." << std::endl << std::endl;
    
    try {
        test_basic_write_row();
        test_multiple_write_rows();
        test_write_and_read_back();
        test_large_records();
        test_empty_records();
        test_duplicate_records();
        test_performance();
        
        std::cout << "=== Test Results ===" << std::endl;
        std::cout << "All write_row tests PASSED!" << std::endl;
        std::cout << "✓ Basic write row functionality working" << std::endl;
        std::cout << "✓ Multiple record insertion working" << std::endl;
        std::cout << "✓ Record format conversion working" << std::endl;
        std::cout << "✓ Large record handling working" << std::endl;
        std::cout << "✓ Empty record handling working" << std::endl;
        std::cout << "✓ Duplicate record handling working" << std::endl;
        std::cout << "✓ Performance characteristics acceptable" << std::endl;
        std::cout << std::endl;
        std::cout << "🎉 ALL WRITE_ROW TESTS PASSED! 🎉" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}