/*****************************************************************************

Copyright (c) 2025, Oracle and/or its affiliates.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License, version 2.0,
as published by the Free Software Foundation.

*****************************************************************************/

/** @file test_memory_leaks.cc
 SBT Memory Leak Detection Tests

 Created 2025-01-25
 *******************************************************/

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cassert>
#include <thread>
#include <chrono>
#include <random>

// Enable debug memory tracking
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

/** Memory Leak Test Helper Class */
class MemoryLeakTester {
private:
  size_t initial_bytes_;
  size_t initial_count_;
  string test_name_;

public:
  MemoryLeakTester(const string& test_name) : test_name_(test_name) {
    initial_bytes_ = SBT_memory_tracker::get_allocated_bytes();
    initial_count_ = SBT_memory_tracker::get_allocation_count();
    cout << "\n--- Starting " << test_name_ << " ---" << endl;
    cout << "Initial memory: " << initial_bytes_ << " bytes, " 
         << initial_count_ << " allocations" << endl;
  }
  
  ~MemoryLeakTester() {
    size_t final_bytes = SBT_memory_tracker::get_allocated_bytes();
    size_t final_count = SBT_memory_tracker::get_allocation_count();
    
    cout << "Final memory: " << final_bytes << " bytes, " 
         << final_count << " allocations" << endl;
    
    bool no_leaks = (final_bytes == initial_bytes_) && (final_count == initial_count_);
    TEST_ASSERT(no_leaks, test_name_ + " should have no memory leaks");
    
    if (no_leaks) {
      cout << "✅ " << test_name_ << " - No memory leaks detected" << endl;
    } else {
      cout << "❌ " << test_name_ << " - Memory leak detected!" << endl;
      cout << "   Leaked: " << (final_bytes - initial_bytes_) << " bytes, "
           << (final_count - initial_count_) << " allocations" << endl;
    }
  }
};

/** Test SBT Tree Memory Management */
void test_tree_memory_management() {
  MemoryLeakTester tester("SBT Tree Memory Management");
  
  // Test 1: Basic insert/remove cycle
  {
    SBT_tree tree;
    
    // Insert multiple records
    vector<string> test_data;
    for (int i = 0; i < 100; i++) {
      string data = "test record " + to_string(i) + " with some additional data to make it longer";
      test_data.push_back(data);
      
      int result = tree.insert((uchar*)data.c_str(), data.length());
      TEST_ASSERT(result == SBT_SUCCESS, "Insert should succeed");
    }
    
    TEST_ASSERT(tree.get_record_count() == 100, "Should have 100 records");
    
    // Remove all records
    for (const auto& data : test_data) {
      int result = tree.remove((uchar*)data.c_str(), data.length());
      TEST_ASSERT(result == SBT_SUCCESS, "Remove should succeed");
    }
    
    TEST_ASSERT(tree.get_record_count() == 0, "Should have 0 records after removal");
  } // Tree destructor should clean up all memory
  
  // Test 2: Update operations
  {
    SBT_tree tree;
    
    const char* original_data = "original data for update test";
    tree.insert((uchar*)original_data, strlen(original_data));
    
    // Update with same length
    const char* updated_data1 = "updated  data for update test"; // Same length
    int result = tree.update((uchar*)original_data, strlen(original_data),
                            (uchar*)updated_data1, strlen(updated_data1));
    TEST_ASSERT(result == SBT_SUCCESS, "Same-length update should succeed");
    
    // Update with different length (requires remove + insert)
    const char* updated_data2 = "completely different data with different length for update test";
    result = tree.update((uchar*)updated_data1, strlen(updated_data1),
                        (uchar*)updated_data2, strlen(updated_data2));
    TEST_ASSERT(result == SBT_SUCCESS, "Different-length update should succeed");
    
    // Verify final data
    SBT_node* found = tree.find_by_data((uchar*)updated_data2, strlen(updated_data2));
    TEST_ASSERT(found != nullptr, "Updated data should be found");
  }
  
  // Test 3: Clear operation
  {
    SBT_tree tree;
    
    // Insert many records
    for (int i = 0; i < 500; i++) {
      string data = "clear test record " + to_string(i);
      tree.insert((uchar*)data.c_str(), data.length());
    }
    
    TEST_ASSERT(tree.get_record_count() == 500, "Should have 500 records");
    
    // Clear all at once
    tree.clear();
    
    TEST_ASSERT(tree.get_record_count() == 0, "Should have 0 records after clear");
    TEST_ASSERT(tree.is_empty(), "Tree should be empty after clear");
  }
}

/** Test SBT File Memory Management */
void test_file_memory_management() {
  MemoryLeakTester tester("SBT File Memory Management");
  
  const char* test_file = "/tmp/sbt_memory_leak_test.sbt";
  
  // Test 1: File create/open/close cycle
  {
    SBT_file file;
    
    int result = file.create(test_file);
    TEST_ASSERT(result == SBT_SUCCESS, "File creation should succeed");
    
    result = file.close();
    TEST_ASSERT(result == SBT_SUCCESS, "File close should succeed");
    
    result = file.open(test_file);
    TEST_ASSERT(result == SBT_SUCCESS, "File open should succeed");
  } // File destructor should clean up
  
  // Test 2: Tree save/load cycle
  {
    SBT_file file;
    SBT_tree tree;
    
    // Create tree with data
    for (int i = 0; i < 200; i++) {
      string data = "file test record " + to_string(i) + " with variable length content";
      tree.insert((uchar*)data.c_str(), data.length());
    }
    
    // Save tree
    file.open(test_file);
    int result = file.save_tree(&tree);
    TEST_ASSERT(result == SBT_SUCCESS, "Tree save should succeed");
    file.close();
    
    // Load tree into new object
    SBT_tree loaded_tree;
    file.open(test_file);
    result = file.load_tree(&loaded_tree);
    TEST_ASSERT(result == SBT_SUCCESS, "Tree load should succeed");
    
    TEST_ASSERT(loaded_tree.get_record_count() == tree.get_record_count(),
                "Loaded tree should have same record count");
  } // All objects should clean up properly
  
  // Test 3: Multiple save/load cycles
  {
    for (int cycle = 0; cycle < 10; cycle++) {
      SBT_file file;
      SBT_tree tree;
      
      // Create different data each cycle
      for (int i = 0; i < 50; i++) {
        string data = "cycle " + to_string(cycle) + " record " + to_string(i);
        tree.insert((uchar*)data.c_str(), data.length());
      }
      
      file.open(test_file);
      file.save_tree(&tree);
      file.close();
      
      // Load and verify
      SBT_tree verify_tree;
      file.open(test_file);
      file.load_tree(&verify_tree);
      
      TEST_ASSERT(verify_tree.get_record_count() == 50, 
                  "Each cycle should have 50 records");
    }
  }
  
  // Clean up
  SBT_file::delete_file(test_file);
}

/** Test RAII Wrapper Memory Management */
void test_raii_memory_management() {
  MemoryLeakTester tester("RAII Wrapper Memory Management");
  
  // Test 1: Memory guard lifecycle
  {
    vector<SBT_memory_guard> guards;
    
    // Create many memory guards
    for (int i = 0; i < 1000; i++) {
      guards.emplace_back(sbt_make_memory_guard(1024 + i));
      
      if (guards.back().is_valid()) {
        // Use the memory
        memset(guards.back().get(), i % 256, 1024 + i);
      }
    }
    
    // Release some guards
    for (int i = 0; i < 500; i += 2) {
      if (i < guards.size()) {
        guards[i].free_memory();
      }
    }
    
    // Reset some guards
    for (int i = 1; i < 500; i += 2) {
      if (i < guards.size()) {
        guards[i].reset(sbt_safe_malloc(512));
      }
    }
  } // All guards should clean up automatically
  
  // Test 2: File guard lifecycle
  {
    vector<SBT_file_guard> file_guards;
    
    // Create temporary files
    for (int i = 0; i < 50; i++) {
      string filename = "/tmp/sbt_raii_test_" + to_string(i) + ".tmp";
      file_guards.emplace_back(my_create(filename.c_str(), 0, O_RDWR | O_CREAT | O_TRUNC, MYF(0)));
      
      if (file_guards.back().is_valid()) {
        string data = "test data " + to_string(i);
        my_write(file_guards.back().get(), (uchar*)data.c_str(), data.length(), MYF(0));
      }
    }
    
    // Release some file guards
    for (int i = 0; i < 25; i++) {
      if (i < file_guards.size()) {
        File fd = file_guards[i].release();
        if (fd >= 0) {
          my_close(fd, MYF(0));
        }
      }
    }
  } // Remaining guards should close files automatically
  
  // Clean up temporary files
  for (int i = 0; i < 50; i++) {
    string filename = "/tmp/sbt_raii_test_" + to_string(i) + ".tmp";
    my_delete(filename.c_str(), MYF(0));
  }
}

/** Test Exception Safety Memory Management */
void test_exception_safety_memory() {
  MemoryLeakTester tester("Exception Safety Memory Management");
  
  // Test 1: Exception during tree operations
  {
    SBT_tree tree;
    
    // Insert some initial data
    for (int i = 0; i < 50; i++) {
      string data = "initial data " + to_string(i);
      tree.insert((uchar*)data.c_str(), data.length());
    }
    
    // Try operations that might fail
    try {
      // Invalid operations should not leak memory
      tree.insert(nullptr, 10);
      tree.remove(nullptr, 5);
      tree.update(nullptr, 0, (uchar*)"test", 4);
      
      // Operations on non-existent data
      const char* non_existent = "this data does not exist in the tree";
      tree.remove((uchar*)non_existent, strlen(non_existent));
      tree.update((uchar*)non_existent, strlen(non_existent), 
                 (uchar*)"new data", 8);
    } catch (...) {
      // Ignore exceptions - we're testing memory cleanup
    }
    
    // Tree should still be functional
    TEST_ASSERT(tree.get_record_count() == 50, "Tree should still have 50 records");
    
    SBT_node* first = tree.get_first();
    TEST_ASSERT(first != nullptr, "Should be able to get first record");
  }
  
  // Test 2: Exception during file operations
  {
    SBT_file file;
    
    try {
      // Try to open non-existent file
      file.open("/tmp/non_existent_file_12345.sbt");
      
      // Try to create file in non-existent directory
      file.create("/non_existent_dir/test.sbt");
      
      // Try operations on closed file
      SBT_tree tree;
      file.save_tree(&tree);
      file.load_tree(&tree);
    } catch (...) {
      // Ignore exceptions - we're testing memory cleanup
    }
    
    // File object should be in clean state
    const char* valid_file = "/tmp/sbt_exception_test.sbt";
    int result = file.create(valid_file);
    TEST_ASSERT(result == SBT_SUCCESS, "Should be able to create file after exceptions");
    
    SBT_file::delete_file(valid_file);
  }
}

/** Test Multi-threaded Memory Management */
void test_multithreaded_memory() {
  MemoryLeakTester tester("Multi-threaded Memory Management");
  
  const int NUM_THREADS = 8;
  const int OPERATIONS_PER_THREAD = 100;
  
  vector<thread> threads;
  atomic<int> success_count(0);
  
  // Test concurrent memory operations
  for (int t = 0; t < NUM_THREADS; t++) {
    threads.emplace_back([&, t]() {
      try {
        for (int i = 0; i < OPERATIONS_PER_THREAD; i++) {
          // Random memory operations
          random_device rd;
          mt19937 gen(rd());
          uniform_int_distribution<> dis(1, 4);
          
          switch (dis(gen)) {
            case 1: {
              // Memory guard test
              SBT_memory_guard guard = sbt_make_memory_guard(1024 + (i % 1000));
              if (guard.is_valid()) {
                memset(guard.get(), (t * 100 + i) % 256, 1024 + (i % 1000));
                success_count++;
              }
              break;
            }
            
            case 2: {
              // Tree operations
              SBT_tree tree;
              string data = "thread " + to_string(t) + " data " + to_string(i);
              if (tree.insert((uchar*)data.c_str(), data.length()) == SBT_SUCCESS) {
                success_count++;
              }
              break;
            }
            
            case 3: {
              // File operations
              string filename = "/tmp/sbt_thread_" + to_string(t) + "_" + to_string(i) + ".sbt";
              SBT_file file;
              if (file.create(filename.c_str()) == SBT_SUCCESS) {
                success_count++;
                SBT_file::delete_file(filename.c_str());
              }
              break;
            }
            
            case 4: {
              // Transaction guard test
              int counter = 0;
              {
                SBT_transaction_guard transaction([&]() {
                  counter = 0; // Rollback
                });
                counter = i;
                if (i % 2 == 0) {
                  transaction.commit();
                }
              }
              success_count++;
              break;
            }
          }
          
          // Small delay to increase contention
          this_thread::sleep_for(chrono::microseconds(10));
        }
      } catch (...) {
        // Ignore exceptions in stress test
      }
    });
  }
  
  // Wait for all threads
  for (auto& thread : threads) {
    thread.join();
  }
  
  TEST_ASSERT(success_count > NUM_THREADS * OPERATIONS_PER_THREAD / 2,
              "Most operations should succeed in multi-threaded test");
}

/** Test Long-running Memory Stability */
void test_long_running_stability() {
  MemoryLeakTester tester("Long-running Memory Stability");
  
  const int NUM_CYCLES = 1000;
  const char* test_file = "/tmp/sbt_stability_test.sbt";
  
  // Simulate long-running application behavior
  for (int cycle = 0; cycle < NUM_CYCLES; cycle++) {
    // Create and destroy objects repeatedly
    {
      SBT_tree tree;
      SBT_file file;
      
      // Add some data
      for (int i = 0; i < 10; i++) {
        string data = "stability test cycle " + to_string(cycle) + " record " + to_string(i);
        tree.insert((uchar*)data.c_str(), data.length());
      }
      
      // Save and load
      if (cycle % 10 == 0) {
        file.create(test_file);
        file.save_tree(&tree);
        file.close();
        
        SBT_tree loaded_tree;
        file.open(test_file);
        file.load_tree(&loaded_tree);
        
        TEST_ASSERT(loaded_tree.get_record_count() == tree.get_record_count(),
                    "Loaded tree should match original");
      }
      
      // Modify data
      SBT_node* first = tree.get_first();
      if (first) {
        string new_data = "modified data for cycle " + to_string(cycle);
        tree.update(first->data, first->data_length,
                   (uchar*)new_data.c_str(), new_data.length());
      }
    } // Objects destroyed here
    
    // Check memory periodically
    if (cycle % 100 == 0) {
      size_t current_bytes = SBT_memory_tracker::get_allocated_bytes();
      cout << "Cycle " << cycle << ": " << current_bytes << " bytes allocated" << endl;
    }
  }
  
  SBT_file::delete_file(test_file);
}

int main() {
  cout << "SBT Memory Leak Detection Tests" << endl;
  cout << "===============================" << endl;
  
  try {
    test_tree_memory_management();
    test_file_memory_management();
    test_raii_memory_management();
    test_exception_safety_memory();
    test_multithreaded_memory();
    test_long_running_stability();
    
    cout << "\n=== Final Test Results ===" << endl;
    cout << "Tests run: " << tests_run << endl;
    cout << "Tests passed: " << tests_passed << endl;
    cout << "Tests failed: " << (tests_run - tests_passed) << endl;
    
    // Final memory check
    size_t final_bytes = SBT_memory_tracker::get_allocated_bytes();
    size_t final_count = SBT_memory_tracker::get_allocation_count();
    
    cout << "\n=== Final Memory Status ===" << endl;
    cout << "Allocated bytes: " << final_bytes << endl;
    cout << "Allocation count: " << final_count << endl;
    cout << "Has leaks: " << (SBT_memory_tracker::has_leaks() ? "YES" : "NO") << endl;
    
    if (tests_passed == tests_run && !SBT_memory_tracker::has_leaks()) {
      cout << "\n🎉 ALL MEMORY LEAK TESTS PASSED! 🎉" << endl;
      cout << "No memory leaks detected in SBT storage engine." << endl;
      return 0;
    } else {
      cout << "\n❌ MEMORY LEAK TESTS FAILED!" << endl;
      if (SBT_memory_tracker::has_leaks()) {
        cout << "Memory leaks detected!" << endl;
      }
      if (tests_passed != tests_run) {
        cout << "Some functionality tests failed!" << endl;
      }
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