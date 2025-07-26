/*****************************************************************************

Copyright (c) 2025, Oracle and/or its affiliates.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License, version 2.0,
as published by the Free Software Foundation.

*****************************************************************************/

/** @file test_sbt_share_standalone.cc
 SBT Share Management Standalone Test

 This test verifies the SBT_share class functionality including:
 - Reference counting mechanism
 - Thread safety
 - Resource management
 - Hash table operations

 Created 2025-01-25
 *******************************************************/

#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <cassert>
#include <cstring>
#include <unordered_map>
#include <mutex>
#include <functional>

// Mock MySQL dependencies for standalone testing
typedef unsigned int uint;
typedef unsigned long ulong;
typedef unsigned char uchar;
typedef char my_bool;
typedef void* PSI_mutex_key;
#define PSI_NOT_INSTRUMENTED nullptr
#define MY_MUTEX_INIT_FAST 0

// Mock MySQL mutex
struct mysql_mutex_t {
  std::mutex m;
};

void mysql_mutex_init(PSI_mutex_key key, mysql_mutex_t* mutex, int type) {
  (void)key; (void)type; (void)mutex;
}

void mysql_mutex_lock(mysql_mutex_t* mutex) {
  mutex->m.lock();
}

void mysql_mutex_unlock(mysql_mutex_t* mutex) {
  mutex->m.unlock();
}

void mysql_mutex_destroy(mysql_mutex_t* mutex) {
  (void)mutex;
}

// Mock THR_LOCK
struct THR_LOCK {
  std::mutex lock;
};

void thr_lock_init(THR_LOCK* lock) {
  (void)lock;
}

void thr_lock_delete(THR_LOCK* lock) {
  (void)lock;
}

// Mock Handler_share
class Handler_share {
public:
  Handler_share() = default;
  virtual ~Handler_share() = default;
};

// Mock SBT common definitions
enum sbt_error_t {
  SBT_SUCCESS = 0,
  SBT_ERR_OUT_OF_MEMORY,
  SBT_ERR_FILE_NOT_FOUND,
  SBT_ERR_CORRUPTED_DATA,
  SBT_ERR_DUPLICATE_KEY,
  SBT_ERR_IO_ERROR,
  SBT_ERR_INVALID_ARGUMENT,
  SBT_ERR_GENERIC
};

// Mock implementations for dependencies
void* sbt_malloc(size_t size) { return malloc(size); }
void sbt_free(void* ptr) { free(ptr); }
void* sbt_realloc(void* ptr, size_t size) { return realloc(ptr, size); }

// Mock SBT_tree class
class SBT_tree {
public:
  SBT_tree() = default;
  ~SBT_tree() = default;
};

// Mock SBT_file class  
class SBT_file {
public:
  SBT_file() = default;
  ~SBT_file() = default;
  
  int create(const char* filename) { 
    (void)filename; 
    return SBT_SUCCESS; 
  }
  
  int open(const char* filename) { 
    (void)filename; 
    return SBT_SUCCESS; 
  }
  
  int close() { 
    return SBT_SUCCESS; 
  }
  
  int load_tree(SBT_tree* tree) { 
    (void)tree; 
    return SBT_SUCCESS; 
  }
  
  int save_tree(SBT_tree* tree) { 
    (void)tree; 
    return SBT_SUCCESS; 
  }
  
  static int delete_file(const char* filename) { 
    (void)filename; 
    return SBT_SUCCESS; 
  }
};

// Global hash table for share management
static std::unordered_map<std::string, void*> global_share_map;
static std::mutex global_share_mutex;

// Mock SBT_share class definition (simplified for testing)
class SBT_share : public Handler_share {
private:
  THR_LOCK lock;
  char *table_name;
  uint table_name_length;
  uint use_count;
  SBT_tree *tree;
  SBT_file *file;
  mysql_mutex_t mutex;

  static mysql_mutex_t sbt_mutex;
  static bool sbt_init_done;

public:
  SBT_share(const char *table_name_arg, uint table_name_length_arg)
      : table_name_length(table_name_length_arg),
        use_count(0),
        tree(nullptr),
        file(nullptr) {
    
    thr_lock_init(&lock);
    mysql_mutex_init(PSI_NOT_INSTRUMENTED, &mutex, MY_MUTEX_INIT_FAST);
    
    table_name = (char *)sbt_malloc(table_name_length + 1);
    if (table_name) {
      memcpy(table_name, table_name_arg, table_name_length);
      table_name[table_name_length] = '\0';
    }
  }

  ~SBT_share() {
    if (tree) {
      delete tree;
      tree = nullptr;
    }
    
    if (file) {
      delete file;
      file = nullptr;
    }
    
    if (table_name) {
      sbt_free(table_name);
      table_name = nullptr;
    }
    
    thr_lock_delete(&lock);
    mysql_mutex_destroy(&mutex);
  }

  static SBT_share *get_share(const char *table_name) {
    if (!table_name) {
      return nullptr;
    }

    mysql_mutex_lock(&sbt_mutex);
    
    SBT_share *share = nullptr;
    uint name_length = strlen(table_name);
    std::string key_str(table_name, name_length);
    
    // Look up in our mock hash table
    {
      std::lock_guard<std::mutex> guard(global_share_mutex);
      auto it = global_share_map.find(key_str);
      if (it != global_share_map.end()) {
        share = (SBT_share *)it->second;
      }
    }
    
    if (share) {
      share->increment_use_count();
    } else {
      share = new SBT_share(table_name, name_length);
      if (share) {
        if (share->init_table_data(table_name) != SBT_SUCCESS) {
          delete share;
          share = nullptr;
        } else {
          share->increment_use_count();
          {
            std::lock_guard<std::mutex> guard(global_share_mutex);
            global_share_map[key_str] = share;
          }
        }
      }
    }
    
    mysql_mutex_unlock(&sbt_mutex);
    return share;
  }

  static void release_share(SBT_share *share) {
    if (!share) {
      return;
    }

    mysql_mutex_lock(&sbt_mutex);
    
    share->decrement_use_count();
    
    if (share->get_use_count() == 0) {
      std::string key_str(share->table_name, share->table_name_length);
      {
        std::lock_guard<std::mutex> guard(global_share_mutex);
        global_share_map.erase(key_str);
      }
      delete share;
    }
    
    mysql_mutex_unlock(&sbt_mutex);
  }

  static int init_share_system() {
    if (sbt_init_done) {
      return 0;
    }

    mysql_mutex_init(PSI_NOT_INSTRUMENTED, &sbt_mutex, MY_MUTEX_INIT_FAST);
    sbt_init_done = true;
    return 0;
  }

  static void cleanup_share_system() {
    if (!sbt_init_done) {
      return;
    }

    mysql_mutex_lock(&sbt_mutex);
    {
      std::lock_guard<std::mutex> guard(global_share_mutex);
      global_share_map.clear();
    }
    mysql_mutex_unlock(&sbt_mutex);
    mysql_mutex_destroy(&sbt_mutex);
    
    sbt_init_done = false;
  }

  THR_LOCK *get_lock() { return &lock; }
  SBT_tree *get_tree() { return tree; }
  SBT_file *get_file() { return file; }
  const char *get_table_name() const { return table_name; }
  uint get_use_count() const { return use_count; }

  void lock_share() {
    mysql_mutex_lock(&mutex);
  }

  void unlock_share() {
    mysql_mutex_unlock(&mutex);
  }

  int init_table_data(const char *table_name) {
    (void)table_name;
    tree = new SBT_tree();
    if (!tree) {
      return SBT_ERR_OUT_OF_MEMORY;
    }
    
    file = new SBT_file();
    if (!file) {
      delete tree;
      tree = nullptr;
      return SBT_ERR_OUT_OF_MEMORY;
    }
    
    return SBT_SUCCESS;
  }

  int open_table() {
    if (!file || !tree) {
      return SBT_ERR_INVALID_ARGUMENT;
    }
    return SBT_SUCCESS;
  }

  int close_table() {
    if (!file || !tree) {
      return SBT_ERR_INVALID_ARGUMENT;
    }
    return SBT_SUCCESS;
  }

  int create_table(const char *file_name) {
    if (!file || !file_name) {
      return SBT_ERR_INVALID_ARGUMENT;
    }
    return file->create(file_name);
  }

  int delete_table(const char *file_name) {
    if (!file_name) {
      return SBT_ERR_INVALID_ARGUMENT;
    }
    return SBT_file::delete_file(file_name);
  }

private:
  void increment_use_count() { use_count++; }
  void decrement_use_count() { use_count--; }
};

// Static member definitions
mysql_mutex_t SBT_share::sbt_mutex;
bool SBT_share::sbt_init_done = false;

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
      std::cout << "🎉 ALL SBT_SHARE TESTS PASSED! 🎉" << std::endl;
    } else {
      std::cout << "❌ SOME TESTS FAILED" << std::endl;
    }
  }
  
  bool all_passed() const {
    return passed_tests.load() == total_tests.load();
  }
};

TestResults results;

// Test Functions

bool test_share_initialization() {
  // Test share system initialization
  int result = SBT_share::init_share_system();
  if (result != 0) {
    return false;
  }
  
  // Test double initialization (should be safe)
  result = SBT_share::init_share_system();
  if (result != 0) {
    return false;
  }
  
  return true;
}

bool test_basic_share_creation() {
  const char* table_name = "test_table_1";
  
  SBT_share* share = SBT_share::get_share(table_name);
  if (!share) {
    return false;
  }
  
  // Verify basic properties
  if (strcmp(share->get_table_name(), table_name) != 0) {
    SBT_share::release_share(share);
    return false;
  }
  
  if (share->get_use_count() != 1) {
    SBT_share::release_share(share);
    return false;
  }
  
  if (!share->get_tree() || !share->get_file() || !share->get_lock()) {
    SBT_share::release_share(share);
    return false;
  }
  
  SBT_share::release_share(share);
  return true;
}

bool test_reference_counting() {
  const char* table_name = "test_table_ref";
  
  // Get first reference
  SBT_share* share1 = SBT_share::get_share(table_name);
  if (!share1 || share1->get_use_count() != 1) {
    if (share1) SBT_share::release_share(share1);
    return false;
  }
  
  // Get second reference to same table
  SBT_share* share2 = SBT_share::get_share(table_name);
  if (!share2 || share2 != share1) {
    if (share1) SBT_share::release_share(share1);
    if (share2) SBT_share::release_share(share2);
    return false;
  }
  
  // Should have same object with increased reference count
  if (share1->get_use_count() != 2) {
    SBT_share::release_share(share1);
    SBT_share::release_share(share2);
    return false;
  }
  
  // Release first reference
  SBT_share::release_share(share1);
  if (share2->get_use_count() != 1) {
    SBT_share::release_share(share2);
    return false;
  }
  
  // Release second reference
  SBT_share::release_share(share2);
  
  return true;
}

bool test_multiple_tables() {
  const char* table1 = "test_table_multi_1";
  const char* table2 = "test_table_multi_2";
  
  SBT_share* share1 = SBT_share::get_share(table1);
  SBT_share* share2 = SBT_share::get_share(table2);
  
  if (!share1 || !share2) {
    if (share1) SBT_share::release_share(share1);
    if (share2) SBT_share::release_share(share2);
    return false;
  }
  
  // Should be different objects
  if (share1 == share2) {
    SBT_share::release_share(share1);
    SBT_share::release_share(share2);
    return false;
  }
  
  // Each should have reference count of 1
  if (share1->get_use_count() != 1 || share2->get_use_count() != 1) {
    SBT_share::release_share(share1);
    SBT_share::release_share(share2);
    return false;
  }
  
  // Should have correct table names
  if (strcmp(share1->get_table_name(), table1) != 0 ||
      strcmp(share2->get_table_name(), table2) != 0) {
    SBT_share::release_share(share1);
    SBT_share::release_share(share2);
    return false;
  }
  
  SBT_share::release_share(share1);
  SBT_share::release_share(share2);
  
  return true;
}

bool test_thread_safety() {
  const char* table_name = "test_table_thread";
  const int num_threads = 10;
  const int operations_per_thread = 100;
  
  std::atomic<int> success_count{0};
  std::atomic<int> error_count{0};
  std::vector<std::thread> threads;
  
  // Launch multiple threads that get and release shares
  for (int i = 0; i < num_threads; i++) {
    threads.emplace_back([&]() {
      for (int j = 0; j < operations_per_thread; j++) {
        SBT_share* share = SBT_share::get_share(table_name);
        if (share) {
          // Hold the share briefly
          std::this_thread::sleep_for(std::chrono::microseconds(1));
          
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
  return (success_count.load() == expected_operations && error_count.load() == 0);
}

bool test_table_operations() {
  const char* table_name = "test_table_ops";
  
  SBT_share* share = SBT_share::get_share(table_name);
  if (!share) {
    return false;
  }
  
  // Test table creation
  int result = share->create_table("/tmp/test_sbt_table.sbt");
  if (result != SBT_SUCCESS) {
    SBT_share::release_share(share);
    return false;
  }
  
  // Test table opening
  result = share->open_table();
  if (result != SBT_SUCCESS) {
    SBT_share::release_share(share);
    return false;
  }
  
  // Test table closing
  result = share->close_table();
  if (result != SBT_SUCCESS) {
    SBT_share::release_share(share);
    return false;
  }
  
  // Test table deletion
  result = share->delete_table("/tmp/test_sbt_table.sbt");
  if (result != SBT_SUCCESS) {
    SBT_share::release_share(share);
    return false;
  }
  
  SBT_share::release_share(share);
  return true;
}

bool test_locking_mechanism() {
  const char* table_name = "test_table_lock";
  
  SBT_share* share = SBT_share::get_share(table_name);
  if (!share) {
    return false;
  }
  
  // Test basic locking
  share->lock_share();
  share->unlock_share();
  
  // Test that we can get the lock object
  THR_LOCK* lock = share->get_lock();
  if (!lock) {
    SBT_share::release_share(share);
    return false;
  }
  
  SBT_share::release_share(share);
  return true;
}

bool test_error_handling() {
  // Test null table name
  SBT_share* share = SBT_share::get_share(nullptr);
  if (share != nullptr) {
    SBT_share::release_share(share);
    return false;
  }
  
  // Test releasing null share
  SBT_share::release_share(nullptr); // Should not crash
  
  return true;
}

bool test_cleanup() {
  // Test cleanup
  SBT_share::cleanup_share_system();
  
  // Test double cleanup (should be safe)
  SBT_share::cleanup_share_system();
  
  return true;
}

int main() {
  std::cout << "=== SBT Share Management Standalone Test ===" << std::endl;
  std::cout << "Testing SBT_share class functionality..." << std::endl << std::endl;
  
  // Run all tests
  results.run_test("Share System Initialization", test_share_initialization);
  results.run_test("Basic Share Creation", test_basic_share_creation);
  results.run_test("Reference Counting", test_reference_counting);
  results.run_test("Multiple Tables", test_multiple_tables);
  results.run_test("Thread Safety", test_thread_safety);
  results.run_test("Table Operations", test_table_operations);
  results.run_test("Locking Mechanism", test_locking_mechanism);
  results.run_test("Error Handling", test_error_handling);
  results.run_test("System Cleanup", test_cleanup);
  
  // Print summary
  results.print_summary();
  
  return results.all_passed() ? 0 : 1;
}