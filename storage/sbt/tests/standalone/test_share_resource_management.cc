/*****************************************************************************

Copyright (c) 2025, Oracle and/or its affiliates.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License, version 2.0,
as published by the Free Software Foundation.

*****************************************************************************/

/** @file test_share_resource_management.cc
 SBT Share Resource Management Test - Task 4.2

 This test specifically verifies Task 4.2 requirements:
 - get_share and release_share static methods
 - Hash table management for shared resources
 - Concurrent access synchronization
 - Reference counting correctness
 - Multi-threaded safety

 Created 2025-01-26
 *******************************************************/

#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <cassert>
#include <cstring>
#include <unordered_set>
#include <mutex>
#include <functional>
#include <random>

// Include the actual SBT headers
#include "../../include/sbt_common.h"
#include "../../include/sbt_share.h"

// Test Results Tracking
struct TestResults {
  std::atomic<int> total_tests{0};
  std::atomic<int> passed_tests{0};
  
  void run_test(const std::string& test_name, std::function<bool()> test_func) {
    total_tests++;
    std::cout << "Running: " << test_name << "... ";
    
    try {
      if (test_func()) {
        std::cout << "✓ PASSED" << std::endl;
        passed_tests++;
      } else {
        std::cout << "✗ FAILED" << std::endl;
      }
    } catch (const std::exception& e) {
      std::cout << "✗ FAILED (Exception: " << e.what() << ")" << std::endl;
    } catch (...) {
      std::cout << "✗ FAILED (Unknown exception)" << std::endl;
    }
  }
  
  void print_summary() {
    std::cout << "\n=== Test Summary ===" << std::endl;
    std::cout << "Total tests: " << total_tests.load() << std::endl;
    std::cout << "Passed: " << passed_tests.load() << std::endl;
    std::cout << "Failed: " << (total_tests.load() - passed_tests.load()) << std::endl;
    
    if (passed_tests.load() == total_tests.load()) {
      std::cout << "🎉 ALL SHARE RESOURCE MANAGEMENT TESTS PASSED! 🎉" << std::endl;
    } else {
      std::cout << "❌ SOME TESTS FAILED" << std::endl;
    }
  }
  
  bool all_passed() const {
    return passed_tests.load() == total_tests.load();
  }
};

TestResults results;

// Test Functions for Task 4.2 Requirements

/** Test 1: get_share and release_share static methods */
bool test_get_release_share_methods() {
  const char* table_name = "test_get_release_table";
  
  // Test get_share method
  SBT_share* share1 = SBT_share::get_share(table_name);
  if (!share1) {
    return false;
  }
  
  // Verify share properties
  if (strcmp(share1->get_table_name(), table_name) != 0) {
    SBT_share::release_share(share1);
    return false;
  }
  
  if (share1->get_use_count() != 1) {
    SBT_share::release_share(share1);
    return false;
  }
  
  // Test getting the same share again (should return same object)
  SBT_share* share2 = SBT_share::get_share(table_name);
  if (share2 != share1) {
    SBT_share::release_share(share1);
    SBT_share::release_share(share2);
    return false;
  }
  
  if (share1->get_use_count() != 2) {
    SBT_share::release_share(share1);
    SBT_share::release_share(share2);
    return false;
  }
  
  // Test release_share method
  SBT_share::release_share(share1);
  if (share2->get_use_count() != 1) {
    SBT_share::release_share(share2);
    return false;
  }
  
  SBT_share::release_share(share2);
  
  return true;
}

/** Test 2: Hash table management for multiple tables */
bool test_hash_table_management() {
  const int num_tables = 20;
  std::vector<SBT_share*> shares;
  std::vector<std::string> table_names;
  
  // Create multiple shares
  for (int i = 0; i < num_tables; i++) {
    std::string table_name = "hash_test_table_" + std::to_string(i);
    table_names.push_back(table_name);
    
    SBT_share* share = SBT_share::get_share(table_name.c_str());
    if (!share) {
      // Clean up on failure
      for (auto s : shares) {
        SBT_share::release_share(s);
      }
      return false;
    }
    
    shares.push_back(share);
  }
  
  // Verify all shares are different objects
  for (size_t i = 0; i < shares.size(); i++) {
    for (size_t j = i + 1; j < shares.size(); j++) {
      if (shares[i] == shares[j]) {
        // Clean up on failure
        for (auto s : shares) {
          SBT_share::release_share(s);
        }
        return false;
      }
    }
  }
  
  // Verify each share has correct table name and reference count
  for (size_t i = 0; i < shares.size(); i++) {
    if (strcmp(shares[i]->get_table_name(), table_names[i].c_str()) != 0) {
      // Clean up on failure
      for (auto s : shares) {
        SBT_share::release_share(s);
      }
      return false;
    }
    
    if (shares[i]->get_use_count() != 1) {
      // Clean up on failure
      for (auto s : shares) {
        SBT_share::release_share(s);
      }
      return false;
    }
  }
  
  // Test hash table lookup by getting shares again
  for (size_t i = 0; i < table_names.size(); i++) {
    SBT_share* share = SBT_share::get_share(table_names[i].c_str());
    if (share != shares[i]) {
      // Clean up on failure
      for (auto s : shares) {
        SBT_share::release_share(s);
      }
      SBT_share::release_share(share);
      return false;
    }
    
    if (share->get_use_count() != 2) {
      // Clean up on failure
      for (auto s : shares) {
        SBT_share::release_share(s);
      }
      SBT_share::release_share(share);
      return false;
    }
    
    SBT_share::release_share(share);
  }
  
  // Clean up all shares
  for (auto s : shares) {
    SBT_share::release_share(s);
  }
  
  return true;
}

/** Test 3: Reference counting correctness */
bool test_reference_counting_correctness() {
  const char* table_name = "ref_count_test_table";
  const int max_refs = 10;
  std::vector<SBT_share*> refs;
  
  // Get multiple references to the same table
  for (int i = 0; i < max_refs; i++) {
    SBT_share* share = SBT_share::get_share(table_name);
    if (!share) {
      // Clean up on failure
      for (auto s : refs) {
        SBT_share::release_share(s);
      }
      return false;
    }
    
    refs.push_back(share);
    
    // Verify reference count
    if (share->get_use_count() != static_cast<uint>(i + 1)) {
      // Clean up on failure
      for (auto s : refs) {
        SBT_share::release_share(s);
      }
      return false;
    }
    
    // Verify all references point to the same object
    if (i > 0 && refs[i] != refs[0]) {
      // Clean up on failure
      for (auto s : refs) {
        SBT_share::release_share(s);
      }
      return false;
    }
  }
  
  // Release references one by one and verify count
  for (int i = max_refs - 1; i >= 0; i--) {
    SBT_share::release_share(refs[i]);
    
    // Check remaining references (if any)
    if (i > 0) {
      if (refs[0]->get_use_count() != static_cast<uint>(i)) {
        // Clean up remaining references
        for (int j = 0; j < i; j++) {
          SBT_share::release_share(refs[j]);
        }
        return false;
      }
    }
  }
  
  return true;
}

/** Test 4: Concurrent access synchronization */
bool test_concurrent_access_synchronization() {
  const char* table_name = "concurrent_test_table";
  const int num_threads = 8;
  const int operations_per_thread = 200;
  
  std::atomic<int> success_count{0};
  std::atomic<int> error_count{0};
  std::atomic<int> max_ref_count{0};
  std::vector<std::thread> threads;
  
  // Launch multiple threads that get and release shares
  for (int i = 0; i < num_threads; i++) {
    threads.emplace_back([&]() {
      std::random_device rd;
      std::mt19937 gen(rd());
      std::uniform_int_distribution<> dis(1, 10);
      
      for (int j = 0; j < operations_per_thread; j++) {
        SBT_share* share = SBT_share::get_share(table_name);
        if (share) {
          // Update max reference count seen
          uint current_count = share->get_use_count();
          int current_max = max_ref_count.load();
          while (current_count > static_cast<uint>(current_max) && 
                 !max_ref_count.compare_exchange_weak(current_max, current_count)) {
            // Retry if another thread updated max_ref_count
          }
          
          // Hold the share for a random short time
          std::this_thread::sleep_for(std::chrono::microseconds(dis(gen)));
          
          // Verify basic properties
          if (share->get_table_name() && share->get_use_count() > 0) {
            success_count++;
          } else {
            error_count++;
          }
          
          SBT_share::release_share(share);
        } else {
          error_count++;
        }
      }
    });
  }
  
  // Wait for all threads to complete
  for (auto& thread : threads) {
    thread.join();
  }
  
  // Check results
  int expected_operations = num_threads * operations_per_thread;
  bool success = (success_count.load() == expected_operations && error_count.load() == 0);
  
  if (success) {
    std::cout << " (Max concurrent refs: " << max_ref_count.load() << ")";
  }
  
  return success;
}

/** Test 5: Hash table collision handling */
bool test_hash_table_collision_handling() {
  // Create many tables with names that might cause hash collisions
  const int num_tables = 100;
  std::vector<SBT_share*> shares;
  std::vector<std::string> table_names;
  
  // Generate table names that might collide
  for (int i = 0; i < num_tables; i++) {
    // Use patterns that might create hash collisions
    std::string table_name;
    if (i % 3 == 0) {
      table_name = "collision_test_" + std::to_string(i);
    } else if (i % 3 == 1) {
      table_name = "test_collision_" + std::to_string(i);
    } else {
      table_name = std::to_string(i) + "_collision_test";
    }
    
    table_names.push_back(table_name);
    
    SBT_share* share = SBT_share::get_share(table_name.c_str());
    if (!share) {
      // Clean up on failure
      for (auto s : shares) {
        SBT_share::release_share(s);
      }
      return false;
    }
    
    shares.push_back(share);
  }
  
  // Verify all shares are correctly stored and retrievable
  for (size_t i = 0; i < table_names.size(); i++) {
    SBT_share* retrieved_share = SBT_share::get_share(table_names[i].c_str());
    if (retrieved_share != shares[i]) {
      // Clean up on failure
      for (auto s : shares) {
        SBT_share::release_share(s);
      }
      SBT_share::release_share(retrieved_share);
      return false;
    }
    
    SBT_share::release_share(retrieved_share);
  }
  
  // Clean up all shares
  for (auto s : shares) {
    SBT_share::release_share(s);
  }
  
  return true;
}

/** Test 6: Memory management under stress */
bool test_memory_management_stress() {
  const int num_iterations = 50;
  const int tables_per_iteration = 20;
  
  for (int iter = 0; iter < num_iterations; iter++) {
    std::vector<SBT_share*> shares;
    
    // Create multiple shares
    for (int i = 0; i < tables_per_iteration; i++) {
      std::string table_name = "stress_test_" + std::to_string(iter) + "_" + std::to_string(i);
      SBT_share* share = SBT_share::get_share(table_name.c_str());
      if (!share) {
        // Clean up on failure
        for (auto s : shares) {
          SBT_share::release_share(s);
        }
        return false;
      }
      shares.push_back(share);
    }
    
    // Get additional references to some shares
    std::vector<SBT_share*> additional_refs;
    for (int i = 0; i < tables_per_iteration / 2; i++) {
      std::string table_name = "stress_test_" + std::to_string(iter) + "_" + std::to_string(i);
      SBT_share* share = SBT_share::get_share(table_name.c_str());
      if (!share || share != shares[i]) {
        // Clean up on failure
        for (auto s : shares) {
          SBT_share::release_share(s);
        }
        for (auto s : additional_refs) {
          SBT_share::release_share(s);
        }
        return false;
      }
      additional_refs.push_back(share);
    }
    
    // Release additional references
    for (auto s : additional_refs) {
      SBT_share::release_share(s);
    }
    
    // Release original shares
    for (auto s : shares) {
      SBT_share::release_share(s);
    }
  }
  
  return true;
}

/** Test 7: Error handling and edge cases */
bool test_error_handling_edge_cases() {
  // Test null table name
  SBT_share* share = SBT_share::get_share(nullptr);
  if (share != nullptr) {
    SBT_share::release_share(share);
    return false;
  }
  
  // Test empty table name
  share = SBT_share::get_share("");
  if (!share) {
    return false;
  }
  
  if (strlen(share->get_table_name()) != 0) {
    SBT_share::release_share(share);
    return false;
  }
  
  SBT_share::release_share(share);
  
  // Test releasing null share (should not crash)
  SBT_share::release_share(nullptr);
  
  // Test very long table name
  std::string long_name(1000, 'x');
  share = SBT_share::get_share(long_name.c_str());
  if (!share) {
    return false;
  }
  
  if (strcmp(share->get_table_name(), long_name.c_str()) != 0) {
    SBT_share::release_share(share);
    return false;
  }
  
  SBT_share::release_share(share);
  
  return true;
}

int main() {
  std::cout << "=== SBT Share Resource Management Test - Task 4.2 ===" << std::endl;
  std::cout << "Testing get_share and release_share static methods..." << std::endl;
  std::cout << "Testing hash table management for shared resources..." << std::endl;
  std::cout << "Testing concurrent access synchronization..." << std::endl << std::endl;
  
  // Initialize the share system
  if (SBT_share::init_share_system() != 0) {
    std::cout << "❌ Failed to initialize share system" << std::endl;
    return 1;
  }
  
  // Run all tests for Task 4.2
  results.run_test("get_share and release_share Methods", test_get_release_share_methods);
  results.run_test("Hash Table Management", test_hash_table_management);
  results.run_test("Reference Counting Correctness", test_reference_counting_correctness);
  results.run_test("Concurrent Access Synchronization", test_concurrent_access_synchronization);
  results.run_test("Hash Table Collision Handling", test_hash_table_collision_handling);
  results.run_test("Memory Management Stress Test", test_memory_management_stress);
  results.run_test("Error Handling and Edge Cases", test_error_handling_edge_cases);
  
  // Clean up the share system
  SBT_share::cleanup_share_system();
  
  // Print summary
  results.print_summary();
  
  return results.all_passed() ? 0 : 1;
}