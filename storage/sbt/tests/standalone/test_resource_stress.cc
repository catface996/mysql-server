/*****************************************************************************

Copyright (c) 2025, Oracle and/or its affiliates.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License, version 2.0,
as published by the Free Software Foundation.

*****************************************************************************/

/** @file test_resource_stress.cc
 SBT Resource Management Stress Tests

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
#include <atomic>
#include <future>

// Enable debug memory tracking
#define SBT_DEBUG_MEMORY

#include "../../include/sbt_common.h"
#include "../../include/sbt_raii.h"
#include "../../include/sbt_tree.h"
#include "../../include/sbt_file.h"

using namespace std;
using namespace chrono;

// Test configuration
const int STRESS_DURATION_SECONDS = 30;
const int NUM_WORKER_THREADS = 4;
const int OPERATIONS_PER_SECOND = 100;
const int MAX_RECORDS_PER_TREE = 1000;
const int MAX_MEMORY_ALLOCATIONS = 500;

// Global statistics
atomic<uint64_t> total_operations(0);
atomic<uint64_t> successful_operations(0);
atomic<uint64_t> failed_operations(0);
atomic<uint64_t> memory_operations(0);
atomic<uint64_t> file_operations(0);
atomic<uint64_t> tree_operations(0);

// Test results
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

/** Resource Monitor Class */
class ResourceMonitor {
private:
  atomic<bool> running_;
  thread monitor_thread_;
  vector<pair<time_point<steady_clock>, size_t>> memory_samples_;
  mutex samples_mutex_;

public:
  ResourceMonitor() : running_(false) {}
  
  void start() {
    running_ = true;
    monitor_thread_ = thread([this]() {
      while (running_) {
        auto now = steady_clock::now();
        size_t current_memory = SBT_memory_tracker::get_allocated_bytes();
        
        {
          lock_guard<mutex> lock(samples_mutex_);
          memory_samples_.emplace_back(now, current_memory);
        }
        
        this_thread::sleep_for(milliseconds(100));
      }
    });
  }
  
  void stop() {
    running_ = false;
    if (monitor_thread_.joinable()) {
      monitor_thread_.join();
    }
  }
  
  void print_statistics() {
    lock_guard<mutex> lock(samples_mutex_);
    
    if (memory_samples_.empty()) {
      cout << "No memory samples collected" << endl;
      return;
    }
    
    size_t min_memory = memory_samples_[0].second;
    size_t max_memory = memory_samples_[0].second;
    size_t total_memory = 0;
    
    for (const auto& sample : memory_samples_) {
      min_memory = min(min_memory, sample.second);
      max_memory = max(max_memory, sample.second);
      total_memory += sample.second;
    }
    
    size_t avg_memory = total_memory / memory_samples_.size();
    
    cout << "\n=== Resource Usage Statistics ===" << endl;
    cout << "Memory samples: " << memory_samples_.size() << endl;
    cout << "Min memory: " << min_memory << " bytes" << endl;
    cout << "Max memory: " << max_memory << " bytes" << endl;
    cout << "Avg memory: " << avg_memory << " bytes" << endl;
    cout << "Memory growth: " << (memory_samples_.back().second - memory_samples_[0].second) << " bytes" << endl;
  }
  
  bool has_memory_leaks() {
    lock_guard<mutex> lock(samples_mutex_);
    if (memory_samples_.size() < 2) return false;
    
    // Check if final memory usage is significantly higher than initial
    size_t initial = memory_samples_[0].second;
    size_t final = memory_samples_.back().second;
    
    // Allow for some variance, but flag if growth is > 10% of initial
    return (final > initial) && ((final - initial) > initial * 0.1);
  }
};

/** Stress Test Worker */
class StressTestWorker {
private:
  int worker_id_;
  atomic<bool> running_;
  random_device rd_;
  mt19937 gen_;
  uniform_int_distribution<> operation_dist_;
  uniform_int_distribution<> size_dist_;

public:
  StressTestWorker(int id) 
    : worker_id_(id), running_(false), gen_(rd_()), 
      operation_dist_(1, 4), size_dist_(100, 2000) {}
  
  void run(int duration_seconds) {
    running_ = true;
    auto start_time = steady_clock::now();
    auto end_time = start_time + seconds(duration_seconds);
    
    while (running_ && steady_clock::now() < end_time) {
      try {
        int operation = operation_dist_(gen_);
        
        switch (operation) {
          case 1:
            test_memory_operations();
            break;
          case 2:
            test_tree_operations();
            break;
          case 3:
            test_file_operations();
            break;
          case 4:
            test_combined_operations();
            break;
        }
        
        total_operations++;
        successful_operations++;
        
        // Throttle operations
        this_thread::sleep_for(microseconds(1000000 / OPERATIONS_PER_SECOND));
        
      } catch (...) {
        failed_operations++;
        total_operations++;
      }
    }
    
    running_ = false;
  }
  
  void stop() {
    running_ = false;
  }

private:
  void test_memory_operations() {
    vector<SBT_memory_guard> guards;
    
    // Allocate random number of memory blocks
    int num_allocations = size_dist_(gen_) % MAX_MEMORY_ALLOCATIONS + 1;
    
    for (int i = 0; i < num_allocations; i++) {
      size_t size = size_dist_(gen_);
      guards.emplace_back(sbt_make_memory_guard(size));
      
      if (guards.back().is_valid()) {
        // Use the memory
        memset(guards.back().get(), (worker_id_ * 100 + i) % 256, size);
      }
    }
    
    // Randomly release some guards
    for (int i = 0; i < num_allocations / 2; i++) {
      int idx = gen_() % guards.size();
      guards[idx].free_memory();
    }
    
    memory_operations++;
  }
  
  void test_tree_operations() {
    SBT_tree tree;
    vector<string> inserted_data;
    
    // Insert random records
    int num_records = gen_() % MAX_RECORDS_PER_TREE + 1;
    
    for (int i = 0; i < num_records; i++) {
      string data = "worker" + to_string(worker_id_) + "_record" + to_string(i) + 
                   "_" + to_string(gen_() % 10000);
      inserted_data.push_back(data);
      
      tree.insert((uchar*)data.c_str(), data.length());
    }
    
    // Perform random operations
    for (int i = 0; i < num_records / 2; i++) {
      int op = gen_() % 3;
      int idx = gen_() % inserted_data.size();
      
      switch (op) {
        case 0: {
          // Search
          tree.find_by_data((uchar*)inserted_data[idx].c_str(), 
                           inserted_data[idx].length());
          break;
        }
        case 1: {
          // Update
          string new_data = inserted_data[idx] + "_updated";
          tree.update((uchar*)inserted_data[idx].c_str(), inserted_data[idx].length(),
                     (uchar*)new_data.c_str(), new_data.length());
          inserted_data[idx] = new_data;
          break;
        }
        case 2: {
          // Remove
          tree.remove((uchar*)inserted_data[idx].c_str(), inserted_data[idx].length());
          inserted_data.erase(inserted_data.begin() + idx);
          break;
        }
      }
    }
    
    // Traverse tree
    SBT_node* current = tree.get_first();
    int traversed = 0;
    while (current && traversed < 100) { // Limit traversal
      current = tree.get_next(current);
      traversed++;
    }
    
    tree_operations++;
  }
  
  void test_file_operations() {
    string filename = "/tmp/sbt_stress_worker" + to_string(worker_id_) + 
                     "_" + to_string(gen_() % 10000) + ".sbt";
    
    try {
      SBT_file file;
      SBT_tree tree;
      
      // Create tree with some data
      int num_records = gen_() % 50 + 1;
      for (int i = 0; i < num_records; i++) {
        string data = "file_test_worker" + to_string(worker_id_) + "_" + to_string(i);
        tree.insert((uchar*)data.c_str(), data.length());
      }
      
      // Save tree
      file.create(filename.c_str());
      file.save_tree(&tree);
      file.close();
      
      // Load tree
      SBT_tree loaded_tree;
      file.open(filename.c_str());
      file.load_tree(&loaded_tree);
      file.close();
      
      // Verify loaded tree
      if (loaded_tree.get_record_count() != tree.get_record_count()) {
        throw runtime_error("Record count mismatch");
      }
      
      file_operations++;
      
    } catch (...) {
      // Clean up on error
    }
    
    // Clean up file
    SBT_file::delete_file(filename.c_str());
  }
  
  void test_combined_operations() {
    // Test that combines multiple resource types
    SBT_memory_guard buffer = sbt_make_memory_guard(4096);
    if (!buffer.is_valid()) return;
    
    SBT_tree tree;
    string filename = "/tmp/sbt_combined_worker" + to_string(worker_id_) + 
                     "_" + to_string(gen_() % 1000) + ".sbt";
    
    try {
      // Use memory buffer for temporary data
      char* temp_data = buffer.get_as<char>();
      
      // Create tree data
      for (int i = 0; i < 20; i++) {
        snprintf(temp_data, 4096, "combined_test_worker%d_record%d_%d", 
                worker_id_, i, gen_() % 10000);
        tree.insert((uchar*)temp_data, strlen(temp_data));
      }
      
      // File operations with transaction-like behavior
      {
        SBT_file file;
        
        // Create transaction guard for rollback
        bool file_created = false;
        SBT_transaction_guard transaction([&]() {
          if (file_created) {
            SBT_file::delete_file(filename.c_str());
          }
        });
        
        file.create(filename.c_str());
        file_created = true;
        
        file.save_tree(&tree);
        file.close();
        
        // Verify by loading
        SBT_tree verify_tree;
        file.open(filename.c_str());
        file.load_tree(&verify_tree);
        
        if (verify_tree.get_record_count() == tree.get_record_count()) {
          transaction.commit(); // Success
        }
      }
      
    } catch (...) {
      // Exception handling - resources should clean up automatically
    }
    
    // Clean up
    SBT_file::delete_file(filename.c_str());
  }
};

/** Run Stress Test */
void run_stress_test() {
  cout << "\n=== Starting Resource Management Stress Test ===" << endl;
  cout << "Duration: " << STRESS_DURATION_SECONDS << " seconds" << endl;
  cout << "Worker threads: " << NUM_WORKER_THREADS << endl;
  cout << "Operations per second per thread: " << OPERATIONS_PER_SECOND << endl;
  
  // Start resource monitor
  ResourceMonitor monitor;
  monitor.start();
  
  // Record initial memory state
  size_t initial_memory = SBT_memory_tracker::get_allocated_bytes();
  size_t initial_count = SBT_memory_tracker::get_allocation_count();
  
  cout << "Initial memory: " << initial_memory << " bytes, " 
       << initial_count << " allocations" << endl;
  
  // Create and start worker threads
  vector<StressTestWorker> workers;
  vector<future<void>> futures;
  
  for (int i = 0; i < NUM_WORKER_THREADS; i++) {
    workers.emplace_back(i);
  }
  
  auto start_time = steady_clock::now();
  
  for (auto& worker : workers) {
    futures.emplace_back(async(launch::async, [&worker]() {
      worker.run(STRESS_DURATION_SECONDS);
    }));
  }
  
  // Wait for all workers to complete
  for (auto& future : futures) {
    future.wait();
  }
  
  auto end_time = steady_clock::now();
  auto duration = duration_cast<seconds>(end_time - start_time);
  
  // Stop resource monitor
  monitor.stop();
  
  // Record final memory state
  size_t final_memory = SBT_memory_tracker::get_allocated_bytes();
  size_t final_count = SBT_memory_tracker::get_allocation_count();
  
  cout << "\n=== Stress Test Results ===" << endl;
  cout << "Actual duration: " << duration.count() << " seconds" << endl;
  cout << "Total operations: " << total_operations.load() << endl;
  cout << "Successful operations: " << successful_operations.load() << endl;
  cout << "Failed operations: " << failed_operations.load() << endl;
  cout << "Memory operations: " << memory_operations.load() << endl;
  cout << "Tree operations: " << tree_operations.load() << endl;
  cout << "File operations: " << file_operations.load() << endl;
  
  double ops_per_second = (double)total_operations.load() / duration.count();
  double success_rate = (double)successful_operations.load() / total_operations.load() * 100.0;
  
  cout << "Operations per second: " << ops_per_second << endl;
  cout << "Success rate: " << success_rate << "%" << endl;
  
  cout << "\nFinal memory: " << final_memory << " bytes, " 
       << final_count << " allocations" << endl;
  cout << "Memory change: " << (long long)(final_memory - initial_memory) << " bytes" << endl;
  cout << "Allocation count change: " << (long long)(final_count - initial_count) << endl;
  
  // Print resource statistics
  monitor.print_statistics();
  
  // Validate results
  TEST_ASSERT(total_operations.load() > 0, "Should have performed some operations");
  TEST_ASSERT(success_rate > 90.0, "Success rate should be > 90%");
  TEST_ASSERT(ops_per_second > 10.0, "Should achieve > 10 operations per second");
  TEST_ASSERT(final_memory == initial_memory, "Memory should return to initial state");
  TEST_ASSERT(final_count == initial_count, "Allocation count should return to initial state");
  TEST_ASSERT(!monitor.has_memory_leaks(), "Should not have memory leaks during stress test");
  TEST_ASSERT(!SBT_memory_tracker::has_leaks(), "Should have no memory leaks at end");
}

/** Test Exception Safety Under Stress */
void test_exception_safety_stress() {
  cout << "\n=== Testing Exception Safety Under Stress ===" << endl;
  
  size_t initial_memory = SBT_memory_tracker::get_allocated_bytes();
  size_t initial_count = SBT_memory_tracker::get_allocation_count();
  
  const int NUM_ITERATIONS = 1000;
  int exceptions_caught = 0;
  
  for (int i = 0; i < NUM_ITERATIONS; i++) {
    try {
      // Create objects that might throw exceptions
      SBT_tree tree;
      SBT_file file;
      
      // Perform operations that might fail
      string data = "exception test " + to_string(i);
      tree.insert((uchar*)data.c_str(), data.length());
      
      // Try invalid operations
      tree.insert(nullptr, 10);
      tree.remove(nullptr, 5);
      
      // File operations that might fail
      file.open("/tmp/non_existent_directory/file.sbt");
      
    } catch (...) {
      exceptions_caught++;
      // Continue - we expect some exceptions
    }
    
    // Check memory periodically
    if (i % 100 == 0) {
      size_t current_memory = SBT_memory_tracker::get_allocated_bytes();
      if (current_memory > initial_memory * 2) {
        cout << "Warning: Memory usage growing at iteration " << i << endl;
      }
    }
  }
  
  size_t final_memory = SBT_memory_tracker::get_allocated_bytes();
  size_t final_count = SBT_memory_tracker::get_allocation_count();
  
  cout << "Exceptions caught: " << exceptions_caught << " out of " << NUM_ITERATIONS << endl;
  cout << "Memory change: " << (long long)(final_memory - initial_memory) << " bytes" << endl;
  
  TEST_ASSERT(exceptions_caught > 0, "Should have caught some exceptions");
  TEST_ASSERT(final_memory == initial_memory, "Memory should be stable despite exceptions");
  TEST_ASSERT(final_count == initial_count, "Allocation count should be stable");
}

/** Test Resource Limits */
void test_resource_limits() {
  cout << "\n=== Testing Resource Limits ===" << endl;
  
  size_t initial_memory = SBT_memory_tracker::get_allocated_bytes();
  
  // Test 1: Large memory allocations
  {
    vector<SBT_memory_guard> large_allocations;
    const size_t LARGE_SIZE = 1024 * 1024; // 1MB
    const int MAX_LARGE_ALLOCS = 100;
    
    int successful_allocs = 0;
    for (int i = 0; i < MAX_LARGE_ALLOCS; i++) {
      large_allocations.emplace_back(sbt_make_memory_guard(LARGE_SIZE));
      if (large_allocations.back().is_valid()) {
        successful_allocs++;
        // Touch the memory to ensure it's really allocated
        memset(large_allocations.back().get(), i % 256, LARGE_SIZE);
      }
    }
    
    cout << "Successfully allocated " << successful_allocs << " large blocks" << endl;
    TEST_ASSERT(successful_allocs > 0, "Should be able to allocate some large blocks");
  } // All large allocations should be freed here
  
  size_t after_large_allocs = SBT_memory_tracker::get_allocated_bytes();
  TEST_ASSERT(after_large_allocs == initial_memory, 
              "Memory should return to initial state after large allocations");
  
  // Test 2: Many small allocations
  {
    vector<SBT_memory_guard> small_allocations;
    const size_t SMALL_SIZE = 64;
    const int MAX_SMALL_ALLOCS = 10000;
    
    int successful_small_allocs = 0;
    for (int i = 0; i < MAX_SMALL_ALLOCS; i++) {
      small_allocations.emplace_back(sbt_make_memory_guard(SMALL_SIZE));
      if (small_allocations.back().is_valid()) {
        successful_small_allocs++;
      }
      
      // Free some allocations to avoid exhausting memory
      if (i % 1000 == 999) {
        for (int j = 0; j < 500; j++) {
          if (j < small_allocations.size()) {
            small_allocations[j].free_memory();
          }
        }
      }
    }
    
    cout << "Successfully allocated " << successful_small_allocs << " small blocks" << endl;
    TEST_ASSERT(successful_small_allocs > MAX_SMALL_ALLOCS / 2, 
                "Should be able to allocate most small blocks");
  }
  
  size_t after_small_allocs = SBT_memory_tracker::get_allocated_bytes();
  TEST_ASSERT(after_small_allocs == initial_memory,
              "Memory should return to initial state after small allocations");
  
  // Test 3: Large tree operations
  {
    SBT_tree large_tree;
    const int LARGE_RECORD_COUNT = 10000;
    
    int successful_inserts = 0;
    for (int i = 0; i < LARGE_RECORD_COUNT; i++) {
      string data = "large tree record " + to_string(i) + 
                   " with additional padding to make it larger and test memory usage";
      
      if (large_tree.insert((uchar*)data.c_str(), data.length()) == SBT_SUCCESS) {
        successful_inserts++;
      }
      
      // Check memory usage periodically
      if (i % 1000 == 999) {
        size_t current_memory = SBT_memory_tracker::get_allocated_bytes();
        cout << "After " << (i + 1) << " inserts: " << current_memory << " bytes" << endl;
      }
    }
    
    cout << "Successfully inserted " << successful_inserts << " records into large tree" << endl;
    TEST_ASSERT(successful_inserts > LARGE_RECORD_COUNT / 2,
                "Should be able to insert most records");
    TEST_ASSERT(large_tree.get_record_count() == successful_inserts,
                "Tree record count should match successful inserts");
  }
  
  size_t final_memory = SBT_memory_tracker::get_allocated_bytes();
  TEST_ASSERT(final_memory == initial_memory,
              "Memory should return to initial state after all tests");
}

int main() {
  cout << "SBT Resource Management Stress Tests" << endl;
  cout << "====================================" << endl;
  
  try {
    run_stress_test();
    test_exception_safety_stress();
    test_resource_limits();
    
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
      cout << "\n🎉 ALL STRESS TESTS PASSED! 🎉" << endl;
      cout << "Resource management is stable under stress conditions." << endl;
      return 0;
    } else {
      cout << "\n❌ STRESS TESTS FAILED!" << endl;
      if (SBT_memory_tracker::has_leaks()) {
        cout << "Memory leaks detected under stress!" << endl;
      }
      if (tests_passed != tests_run) {
        cout << "Some stress tests failed!" << endl;
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