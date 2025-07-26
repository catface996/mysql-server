/*****************************************************************************

Copyright (c) 2025, Oracle and/or its affiliates.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License, version 2.0,
as published by the Free Software Foundation.

*****************************************************************************/

/** @file test_exception_safety.cc
 SBT Exception Safety and Resource Management Tests

 Created 2025-01-25
 *******************************************************/

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cassert>
#include <stdexcept>
#include <thread>
#include <chrono>

// Define debug memory tracking
#define SBT_DEBUG_MEMORY

#include "../../include/sbt_common.h"
#include "../../include/sbt_raii.h"
#include "../../include/sbt_tree.h"
#include "../../include/sbt_file.h"

using namespace std;

// Test counters
static int tests_run = 0;
static int tests_passed = 0;

#define TEST_ASSERT(condition, message) \
  do { \
    tests_run++; \
    if (condition) { \
      tests_passed++; \
      cout << "✓ " << message << endl; \
    } else { \
      cout << "✗ " << message << " - FAILED" << endl; \
    } \
  } while(0)

/** Test RAII File Guard */
void test_file_guard() {
  cout << "\n=== Testing RAII File Guard ===" << endl;
  
  // Test 1: Automatic file closure
  {
    const char* test_file = "/tmp/sbt_test_file_guard.dat";
    
    // Create a file and let guard close it automatically
    {
      SBT_file_guard guard(my_create(test_file, 0, O_RDWR | O_CREAT | O_TRUNC, MYF(0)));
      TEST_ASSERT(guard.is_valid(), "File guard should be valid after creation");
      
      // Write some data
      const char* data = "test data";
      my_write(guard.get(), (uchar*)data, strlen(data), MYF(0));
    } // Guard should automatically close file here
    
    // Try to read the file to verify it was properly closed and data written
    File fd = my_open(test_file, O_RDONLY, MYF(0));
    TEST_ASSERT(fd >= 0, "File should be readable after guard destruction");
    
    char buffer[100];
    size_t bytes_read = my_read(fd, (uchar*)buffer, sizeof(buffer), MYF(0));
    buffer[bytes_read] = '\0';
    my_close(fd, MYF(0));
    
    TEST_ASSERT(strcmp(buffer, "test data") == 0, "File data should be preserved");
    
    // Clean up
    my_delete(test_file, MYF(0));
  }
  
  // Test 2: Manual release
  {
    const char* test_file = "/tmp/sbt_test_file_guard2.dat";
    File released_fd;
    
    {
      SBT_file_guard guard(my_create(test_file, 0, O_RDWR | O_CREAT | O_TRUNC, MYF(0)));
      TEST_ASSERT(guard.is_valid(), "File guard should be valid");
      
      released_fd = guard.release();
      TEST_ASSERT(released_fd >= 0, "Released file descriptor should be valid");
    } // Guard should not close file since it was released
    
    // File should still be open
    const char* data = "released data";
    int result = my_write(released_fd, (uchar*)data, strlen(data), MYF(0));
    TEST_ASSERT(result == 0, "Should be able to write to released file");
    
    my_close(released_fd, MYF(0));
    my_delete(test_file, MYF(0));
  }
  
  // Test 3: Move semantics
  {
    const char* test_file = "/tmp/sbt_test_file_guard3.dat";
    
    SBT_file_guard guard1(my_create(test_file, 0, O_RDWR | O_CREAT | O_TRUNC, MYF(0)));
    TEST_ASSERT(guard1.is_valid(), "Original guard should be valid");
    
    SBT_file_guard guard2 = std::move(guard1);
    TEST_ASSERT(guard2.is_valid(), "Moved-to guard should be valid");
    TEST_ASSERT(!guard1.is_valid(), "Moved-from guard should be invalid");
    
    my_delete(test_file, MYF(0));
  }
}

/** Test RAII Memory Guard */
void test_memory_guard() {
  cout << "\n=== Testing RAII Memory Guard ===" << endl;
  
  // Test 1: Automatic memory cleanup
  size_t initial_allocated = SBT_memory_tracker::get_allocated_bytes();
  size_t initial_count = SBT_memory_tracker::get_allocation_count();
  
  {
    SBT_memory_guard guard = sbt_make_memory_guard(1024);
    TEST_ASSERT(guard.is_valid(), "Memory guard should be valid");
    TEST_ASSERT(SBT_memory_tracker::get_allocated_bytes() > initial_allocated, 
                "Memory should be tracked as allocated");
    
    // Use the memory
    memset(guard.get(), 0xAA, 1024);
    TEST_ASSERT(((char*)guard.get())[0] == (char)0xAA, "Memory should be writable");
  } // Guard should automatically free memory here
  
  TEST_ASSERT(SBT_memory_tracker::get_allocated_bytes() == initial_allocated,
              "Memory should be freed after guard destruction");
  TEST_ASSERT(SBT_memory_tracker::get_allocation_count() == initial_count,
              "Allocation count should be restored");
  
  // Test 2: Manual release
  {
    SBT_memory_guard guard = sbt_make_memory_guard(512);
    void* released_ptr = guard.release();
    TEST_ASSERT(released_ptr != nullptr, "Released pointer should be valid");
  } // Guard should not free memory since it was released
  
  // Note: We can't easily test that memory wasn't freed without causing a leak
  // In a real scenario, the released pointer would be managed elsewhere
  
  // Test 3: Reset functionality
  {
    SBT_memory_guard guard = sbt_make_memory_guard(256);
    TEST_ASSERT(guard.is_valid(), "Guard should be valid initially");
    
    guard.reset(sbt_safe_malloc(128));
    TEST_ASSERT(guard.is_valid(), "Guard should be valid after reset");
    
    guard.reset(); // Reset to nullptr
    TEST_ASSERT(!guard.is_valid(), "Guard should be invalid after reset to nullptr");
  }
}

/** Test Transaction Guard */
void test_transaction_guard() {
  cout << "\n=== Testing Transaction Guard ===" << endl;
  
  // Test 1: Automatic rollback on exception
  int counter = 0;
  bool rollback_called = false;
  
  try {
    SBT_transaction_guard transaction([&]() {
      rollback_called = true;
      counter = 0; // Rollback
    });
    
    counter = 10; // Some operation
    
    // Simulate an exception before commit
    throw std::runtime_error("Test exception");
    
    transaction.commit(); // This should not be reached
  } catch (const std::exception&) {
    // Expected exception
  }
  
  TEST_ASSERT(rollback_called, "Rollback should be called on exception");
  TEST_ASSERT(counter == 0, "Counter should be rolled back");
  
  // Test 2: No rollback when committed
  counter = 0;
  rollback_called = false;
  
  {
    SBT_transaction_guard transaction([&]() {
      rollback_called = true;
      counter = 0; // Rollback
    });
    
    counter = 20; // Some operation
    transaction.commit(); // Commit the transaction
  } // Destructor should not call rollback
  
  TEST_ASSERT(!rollback_called, "Rollback should not be called after commit");
  TEST_ASSERT(counter == 20, "Counter should retain committed value");
  
  // Test 3: Manual rollback
  counter = 0;
  rollback_called = false;
  
  {
    SBT_transaction_guard transaction([&]() {
      rollback_called = true;
      counter = 0; // Rollback
    });
    
    counter = 30; // Some operation
    transaction.rollback(); // Manual rollback
  }
  
  TEST_ASSERT(rollback_called, "Rollback should be called manually");
  TEST_ASSERT(counter == 0, "Counter should be rolled back");
}

/** Test SBT Tree Exception Safety */
void test_tree_exception_safety() {
  cout << "\n=== Testing SBT Tree Exception Safety ===" << endl;
  
  SBT_tree tree;
  
  // Test 1: Insert with rollback on failure
  size_t initial_count = tree.get_record_count();
  sbt_insert_id_t initial_next_id = tree.get_next_insert_id();
  
  // Insert some valid data first
  const char* data1 = "test data 1";
  int result = tree.insert((uchar*)data1, strlen(data1));
  TEST_ASSERT(result == SBT_SUCCESS, "First insert should succeed");
  
  // Try to insert invalid data (nullptr)
  result = tree.insert(nullptr, 10);
  TEST_ASSERT(result == SBT_ERR_INVALID_ARGUMENT, "Insert with nullptr should fail");
  TEST_ASSERT(tree.get_record_count() == initial_count + 1, 
              "Record count should not change on failed insert");
  
  // Test 2: Update with rollback
  const char* old_data = "test data 1";
  const char* new_data = "updated data 1";
  
  result = tree.update((uchar*)old_data, strlen(old_data), 
                      (uchar*)new_data, strlen(new_data));
  TEST_ASSERT(result == SBT_SUCCESS, "Update should succeed");
  
  // Verify the data was actually updated
  SBT_node* found = tree.find_by_data((uchar*)new_data, strlen(new_data));
  TEST_ASSERT(found != nullptr, "Updated data should be found");
  
  // Test 3: Update with invalid parameters should not change tree
  size_t count_before_invalid = tree.get_record_count();
  result = tree.update(nullptr, 0, (uchar*)new_data, strlen(new_data));
  TEST_ASSERT(result == SBT_ERR_INVALID_ARGUMENT, "Invalid update should fail");
  TEST_ASSERT(tree.get_record_count() == count_before_invalid, 
              "Record count should not change on invalid update");
}

/** Test SBT File Exception Safety */
void test_file_exception_safety() {
  cout << "\n=== Testing SBT File Exception Safety ===" << endl;
  
  const char* test_file = "/tmp/sbt_test_exception_safety.sbt";
  
  // Test 1: File creation with automatic cleanup on failure
  {
    SBT_file file;
    
    // Create file successfully
    int result = file.create(test_file);
    TEST_ASSERT(result == SBT_SUCCESS, "File creation should succeed");
    
    // File should exist
    TEST_ASSERT(SBT_file::file_exists(test_file), "File should exist after creation");
  } // File should be properly closed here
  
  // Test 2: File opening with validation
  {
    SBT_file file;
    
    // Open existing file
    int result = file.open(test_file);
    TEST_ASSERT(result == SBT_SUCCESS, "File opening should succeed");
    
    // Try to open non-existent file
    SBT_file file2;
    result = file2.open("/tmp/non_existent_file.sbt");
    TEST_ASSERT(result == SBT_ERR_FILE_NOT_FOUND, "Opening non-existent file should fail");
  }
  
  // Test 3: Tree save/load with exception safety
  {
    SBT_file file;
    SBT_tree tree;
    
    // Add some data to tree
    const char* data1 = "exception safety test 1";
    const char* data2 = "exception safety test 2";
    tree.insert((uchar*)data1, strlen(data1));
    tree.insert((uchar*)data2, strlen(data2));
    
    // Open file and save tree
    int result = file.open(test_file);
    TEST_ASSERT(result == SBT_SUCCESS, "File open should succeed");
    
    result = file.save_tree(&tree);
    TEST_ASSERT(result == SBT_SUCCESS, "Tree save should succeed");
    
    // Load tree into new tree object
    SBT_tree loaded_tree;
    result = file.load_tree(&loaded_tree);
    TEST_ASSERT(result == SBT_SUCCESS, "Tree load should succeed");
    
    // Verify data integrity
    TEST_ASSERT(loaded_tree.get_record_count() == tree.get_record_count(),
                "Loaded tree should have same record count");
    
    SBT_node* found1 = loaded_tree.find_by_data((uchar*)data1, strlen(data1));
    SBT_node* found2 = loaded_tree.find_by_data((uchar*)data2, strlen(data2));
    TEST_ASSERT(found1 != nullptr && found2 != nullptr, 
                "All data should be found in loaded tree");
  }
  
  // Clean up
  SBT_file::delete_file(test_file);
}

/** Test Memory Leak Detection */
void test_memory_leak_detection() {
  cout << "\n=== Testing Memory Leak Detection ===" << endl;
  
  size_t initial_bytes = SBT_memory_tracker::get_allocated_bytes();
  size_t initial_count = SBT_memory_tracker::get_allocation_count();
  
  // Test 1: No leaks with proper cleanup
  {
    SBT_memory_guard guard1 = sbt_make_memory_guard(1024);
    SBT_memory_guard guard2 = sbt_make_memory_guard(2048);
    SBT_memory_guard guard3 = sbt_make_memory_guard(512);
    
    TEST_ASSERT(SBT_memory_tracker::get_allocation_count() == initial_count + 3,
                "Should track 3 allocations");
  } // All guards should clean up automatically
  
  TEST_ASSERT(SBT_memory_tracker::get_allocated_bytes() == initial_bytes,
              "All memory should be freed");
  TEST_ASSERT(SBT_memory_tracker::get_allocation_count() == initial_count,
              "Allocation count should be restored");
  TEST_ASSERT(!SBT_memory_tracker::has_leaks(), "Should have no memory leaks");
  
  // Test 2: Detect intentional leak (for testing purposes)
  void* leaked_ptr = sbt_safe_malloc(256);
  TEST_ASSERT(SBT_memory_tracker::has_leaks(), "Should detect memory leak");
  TEST_ASSERT(SBT_memory_tracker::get_allocated_bytes() > initial_bytes,
              "Should show increased memory usage");
  
  // Clean up the intentional leak
  sbt_safe_free(leaked_ptr, 256);
  TEST_ASSERT(!SBT_memory_tracker::has_leaks(), "Should have no leaks after cleanup");
}

/** Stress Test for Resource Management */
void test_resource_management_stress() {
  cout << "\n=== Testing Resource Management Under Stress ===" << endl;
  
  const int NUM_ITERATIONS = 1000;
  const int NUM_THREADS = 4;
  
  // Test 1: Rapid allocation/deallocation
  size_t initial_bytes = SBT_memory_tracker::get_allocated_bytes();
  
  for (int i = 0; i < NUM_ITERATIONS; i++) {
    SBT_memory_guard guard = sbt_make_memory_guard(1024 + (i % 1000));
    if (guard.is_valid()) {
      memset(guard.get(), i % 256, 1024 + (i % 1000));
    }
    
    // Occasionally test file operations
    if (i % 100 == 0) {
      string filename = "/tmp/sbt_stress_" + to_string(i) + ".sbt";
      SBT_file file;
      if (file.create(filename.c_str()) == SBT_SUCCESS) {
        SBT_tree tree;
        string data = "stress test data " + to_string(i);
        tree.insert((uchar*)data.c_str(), data.length());
        file.save_tree(&tree);
      }
      SBT_file::delete_file(filename.c_str());
    }
  }
  
  TEST_ASSERT(SBT_memory_tracker::get_allocated_bytes() == initial_bytes,
              "Memory should be fully cleaned up after stress test");
  
  // Test 2: Multi-threaded resource management
  vector<thread> threads;
  atomic<int> success_count(0);
  
  for (int t = 0; t < NUM_THREADS; t++) {
    threads.emplace_back([&, t]() {
      for (int i = 0; i < NUM_ITERATIONS / NUM_THREADS; i++) {
        try {
          SBT_memory_guard guard = sbt_make_memory_guard(512);
          if (guard.is_valid()) {
            memset(guard.get(), (t * 100 + i) % 256, 512);
            success_count++;
          }
          
          // Small delay to increase contention
          this_thread::sleep_for(chrono::microseconds(1));
        } catch (...) {
          // Ignore exceptions in stress test
        }
      }
    });
  }
  
  // Wait for all threads to complete
  for (auto& thread : threads) {
    thread.join();
  }
  
  TEST_ASSERT(success_count > NUM_ITERATIONS / 2, 
              "Most allocations should succeed in multi-threaded test");
  TEST_ASSERT(SBT_memory_tracker::get_allocated_bytes() == initial_bytes,
              "Memory should be fully cleaned up after multi-threaded test");
}

/** Test Error Recovery Scenarios */
void test_error_recovery() {
  cout << "\n=== Testing Error Recovery Scenarios ===" << endl;
  
  // Test 1: File operation failure recovery
  {
    SBT_file file;
    
    // Try to open a directory as a file (should fail)
    int result = file.open("/tmp");
    TEST_ASSERT(result != SBT_SUCCESS, "Opening directory as file should fail");
    
    // File object should be in clean state after failure
    const char* test_file = "/tmp/sbt_recovery_test.sbt";
    result = file.create(test_file);
    TEST_ASSERT(result == SBT_SUCCESS, "Should be able to create file after previous failure");
    
    SBT_file::delete_file(test_file);
  }
  
  // Test 2: Tree operation failure recovery
  {
    SBT_tree tree;
    
    // Insert valid data
    const char* data1 = "recovery test 1";
    int result = tree.insert((uchar*)data1, strlen(data1));
    TEST_ASSERT(result == SBT_SUCCESS, "Valid insert should succeed");
    
    size_t count_before = tree.get_record_count();
    
    // Try invalid operation
    result = tree.insert(nullptr, 10);
    TEST_ASSERT(result != SBT_SUCCESS, "Invalid insert should fail");
    
    // Tree should be in consistent state
    TEST_ASSERT(tree.get_record_count() == count_before, 
                "Record count should be unchanged after failed insert");
    
    // Should still be able to perform valid operations
    const char* data2 = "recovery test 2";
    result = tree.insert((uchar*)data2, strlen(data2));
    TEST_ASSERT(result == SBT_SUCCESS, "Valid insert should succeed after failure");
  }
  
  // Test 3: Memory exhaustion simulation
  {
    // This test is conceptual - actual memory exhaustion is hard to simulate safely
    SBT_tree tree;
    
    // Insert data until we might hit memory limits
    int successful_inserts = 0;
    for (int i = 0; i < 10000; i++) {
      string data = "memory test " + to_string(i) + " with some padding to use more memory";
      int result = tree.insert((uchar*)data.c_str(), data.length());
      if (result == SBT_SUCCESS) {
        successful_inserts++;
      } else {
        break; // Stop on first failure
      }
    }
    
    TEST_ASSERT(successful_inserts > 0, "Should be able to insert at least some data");
    
    // Tree should still be functional after hitting limits
    SBT_node* first = tree.get_first();
    TEST_ASSERT(first != nullptr, "Should be able to traverse tree after memory pressure");
  }
}

int main() {
  cout << "SBT Exception Safety and Resource Management Tests" << endl;
  cout << "=================================================" << endl;
  
  try {
    test_file_guard();
    test_memory_guard();
    test_transaction_guard();
    test_tree_exception_safety();
    test_file_exception_safety();
    test_memory_leak_detection();
    test_resource_management_stress();
    test_error_recovery();
    
    cout << "\n=== Test Results ===" << endl;
    cout << "Tests run: " << tests_run << endl;
    cout << "Tests passed: " << tests_passed << endl;
    cout << "Tests failed: " << (tests_run - tests_passed) << endl;
    
    if (tests_passed == tests_run) {
      cout << "\n🎉 ALL EXCEPTION SAFETY TESTS PASSED! 🎉" << endl;
      cout << "Resource management and exception safety mechanisms are working correctly." << endl;
      return 0;
    } else {
      cout << "\n❌ SOME TESTS FAILED!" << endl;
      cout << "Exception safety or resource management issues detected." << endl;
      return 1;
    }
    
  } catch (const exception& e) {
    cout << "\n💥 UNEXPECTED EXCEPTION: " << e.what() << endl;
    return 1;
  } catch (...) {
    cout << "\n💥 UNKNOWN EXCEPTION OCCURRED!" << endl;
    return 1;
  }
}