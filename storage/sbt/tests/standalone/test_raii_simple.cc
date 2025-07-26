/*****************************************************************************

Copyright (c) 2025, Oracle and/or its affiliates.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License, version 2.0,
as published by the Free Software Foundation.

*****************************************************************************/

/** @file test_raii_simple.cc
 Simple RAII and Exception Safety Tests (Standalone)

 Created 2025-01-25
 *******************************************************/

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cassert>
#include <stdexcept>
#include <functional>
#include <memory>
#include <thread>
#include <chrono>
#include <atomic>

using namespace std;

// Simple file operations for testing
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

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

/** Simple RAII File Descriptor Wrapper */
class Simple_file_guard {
private:
  int fd_;
  bool should_close_;

public:
  explicit Simple_file_guard(int fd = -1) : fd_(fd), should_close_(fd >= 0) {}

  ~Simple_file_guard() {
    close_file();
  }

  Simple_file_guard(Simple_file_guard&& other) noexcept 
    : fd_(other.fd_), should_close_(other.should_close_) {
    other.fd_ = -1;
    other.should_close_ = false;
  }

  Simple_file_guard& operator=(Simple_file_guard&& other) noexcept {
    if (this != &other) {
      close_file();
      fd_ = other.fd_;
      should_close_ = other.should_close_;
      other.fd_ = -1;
      other.should_close_ = false;
    }
    return *this;
  }

  Simple_file_guard(const Simple_file_guard&) = delete;
  Simple_file_guard& operator=(const Simple_file_guard&) = delete;

  int get() const { return fd_; }
  bool is_valid() const { return fd_ >= 0; }

  int release() {
    should_close_ = false;
    return fd_;
  }

  void reset(int fd = -1) {
    close_file();
    fd_ = fd;
    should_close_ = (fd >= 0);
  }

  void close_file() {
    if (should_close_ && fd_ >= 0) {
      ::close(fd_);
      should_close_ = false;
    }
    fd_ = -1;
  }
};

/** Simple RAII Memory Wrapper */
class Simple_memory_guard {
private:
  void* ptr_;
  size_t size_;
  bool should_free_;

public:
  explicit Simple_memory_guard(void* ptr = nullptr, size_t size = 0) 
    : ptr_(ptr), size_(size), should_free_(ptr != nullptr) {}

  ~Simple_memory_guard() {
    free_memory();
  }

  Simple_memory_guard(Simple_memory_guard&& other) noexcept 
    : ptr_(other.ptr_), size_(other.size_), should_free_(other.should_free_) {
    other.should_free_ = false;
  }

  Simple_memory_guard& operator=(Simple_memory_guard&& other) noexcept {
    if (this != &other) {
      free_memory();
      ptr_ = other.ptr_;
      size_ = other.size_;
      should_free_ = other.should_free_;
      other.should_free_ = false;
    }
    return *this;
  }

  Simple_memory_guard(const Simple_memory_guard&) = delete;
  Simple_memory_guard& operator=(const Simple_memory_guard&) = delete;

  void* get() const { return ptr_; }
  
  template<typename T>
  T* get_as() const { return static_cast<T*>(ptr_); }

  bool is_valid() const { return ptr_ != nullptr; }

  void* release() {
    should_free_ = false;
    return ptr_;
  }

  void reset(void* ptr = nullptr, size_t size = 0) {
    free_memory();
    ptr_ = ptr;
    size_ = size;
    should_free_ = (ptr != nullptr);
  }

  void free_memory();  // Defined after safe_free
};

/** Simple Transaction Guard */
class Simple_transaction_guard {
private:
  function<void()> rollback_;
  bool committed_;

public:
  explicit Simple_transaction_guard(function<void()> rollback)
    : rollback_(rollback), committed_(false) {}

  ~Simple_transaction_guard() {
    if (!committed_ && rollback_) {
      try {
        rollback_();
      } catch (...) {
        // Ignore exceptions in destructor
      }
    }
  }

  Simple_transaction_guard(const Simple_transaction_guard&) = delete;
  Simple_transaction_guard& operator=(const Simple_transaction_guard&) = delete;

  void commit() {
    committed_ = true;
  }

  void rollback() {
    if (!committed_ && rollback_) {
      rollback_();
      committed_ = true;
    }
  }
};

/** Simple Memory Tracker */
class Simple_memory_tracker {
private:
  static atomic<size_t> allocated_bytes_;
  static atomic<size_t> allocation_count_;

public:
  static void record_allocation(size_t size) {
    allocated_bytes_ += size;
    allocation_count_++;
  }

  static void record_deallocation(size_t size) {
    size_t current = allocated_bytes_.load();
    while (current >= size && !allocated_bytes_.compare_exchange_weak(current, current - size)) {
      // Retry if CAS failed
    }
    
    size_t count = allocation_count_.load();
    while (count > 0 && !allocation_count_.compare_exchange_weak(count, count - 1)) {
      // Retry if CAS failed
    }
  }

  static size_t get_allocated_bytes() {
    return allocated_bytes_.load();
  }

  static size_t get_allocation_count() {
    return allocation_count_.load();
  }

  static bool has_leaks() {
    return allocated_bytes_.load() > 0 || allocation_count_.load() > 0;
  }

  static void reset() {
    allocated_bytes_ = 0;
    allocation_count_ = 0;
  }
};

// Forward declarations
void safe_free(void* ptr, size_t size);

// Static member definitions
atomic<size_t> Simple_memory_tracker::allocated_bytes_(0);
atomic<size_t> Simple_memory_tracker::allocation_count_(0);

/** Safe memory allocation with tracking */
void* safe_malloc(size_t size) {
  void* ptr = malloc(size);
  if (ptr) {
    Simple_memory_tracker::record_allocation(size);
  }
  return ptr;
}

/** Safe memory deallocation with tracking */
void safe_free(void* ptr, size_t size) {
  if (ptr) {
    Simple_memory_tracker::record_deallocation(size);
    free(ptr);
  }
}

/** Implementation of Simple_memory_guard::free_memory */
void Simple_memory_guard::free_memory() {
  if (should_free_ && ptr_) {
    safe_free(ptr_, size_);
    should_free_ = false;
  }
  ptr_ = nullptr;
  size_ = 0;
}

/** Create memory guard with tracking */
Simple_memory_guard make_memory_guard(size_t size) {
  void* ptr = safe_malloc(size);
  return Simple_memory_guard(ptr, size);
}

/** Test RAII File Guard */
void test_file_guard() {
  cout << "\n=== Testing RAII File Guard ===" << endl;
  
  // Test 1: Automatic file closure
  {
    const char* test_file = "/tmp/sbt_test_file_guard.dat";
    
    {
      Simple_file_guard guard(open(test_file, O_RDWR | O_CREAT | O_TRUNC, 0644));
      TEST_ASSERT(guard.is_valid(), "File guard should be valid after creation");
      
      const char* data = "test data";
      write(guard.get(), data, strlen(data));
    } // Guard should automatically close file here
    
    // Try to read the file to verify it was properly closed and data written
    int fd = open(test_file, O_RDONLY);
    TEST_ASSERT(fd >= 0, "File should be readable after guard destruction");
    
    char buffer[100];
    ssize_t bytes_read = read(fd, buffer, sizeof(buffer));
    buffer[bytes_read] = '\0';
    ::close(fd);
    
    TEST_ASSERT(strcmp(buffer, "test data") == 0, "File data should be preserved");
    
    unlink(test_file);
  }
  
  // Test 2: Manual release
  {
    const char* test_file = "/tmp/sbt_test_file_guard2.dat";
    int released_fd;
    
    {
      Simple_file_guard guard(open(test_file, O_RDWR | O_CREAT | O_TRUNC, 0644));
      TEST_ASSERT(guard.is_valid(), "File guard should be valid");
      
      released_fd = guard.release();
      TEST_ASSERT(released_fd >= 0, "Released file descriptor should be valid");
    } // Guard should not close file since it was released
    
    const char* data = "released data";
    ssize_t result = write(released_fd, data, strlen(data));
    TEST_ASSERT(result > 0, "Should be able to write to released file");
    
    ::close(released_fd);
    unlink(test_file);
  }
  
  // Test 3: Move semantics
  {
    const char* test_file = "/tmp/sbt_test_file_guard3.dat";
    
    Simple_file_guard guard1(open(test_file, O_RDWR | O_CREAT | O_TRUNC, 0644));
    TEST_ASSERT(guard1.is_valid(), "Original guard should be valid");
    
    Simple_file_guard guard2 = std::move(guard1);
    TEST_ASSERT(guard2.is_valid(), "Moved-to guard should be valid");
    TEST_ASSERT(!guard1.is_valid(), "Moved-from guard should be invalid");
    
    unlink(test_file);
  }
}

/** Test RAII Memory Guard */
void test_memory_guard() {
  cout << "\n=== Testing RAII Memory Guard ===" << endl;
  
  Simple_memory_tracker::reset();
  
  size_t initial_allocated = Simple_memory_tracker::get_allocated_bytes();
  size_t initial_count = Simple_memory_tracker::get_allocation_count();
  
  {
    Simple_memory_guard guard = make_memory_guard(1024);
    TEST_ASSERT(guard.is_valid(), "Memory guard should be valid");
    TEST_ASSERT(Simple_memory_tracker::get_allocated_bytes() > initial_allocated, 
                "Memory should be tracked as allocated");
    
    memset(guard.get(), 0xAA, 1024);
    TEST_ASSERT(((char*)guard.get())[0] == (char)0xAA, "Memory should be writable");
  } // Guard should automatically free memory here
  
  TEST_ASSERT(Simple_memory_tracker::get_allocated_bytes() == initial_allocated,
              "Memory should be freed after guard destruction");
  TEST_ASSERT(Simple_memory_tracker::get_allocation_count() == initial_count,
              "Allocation count should be restored");
  
  // Test 2: Reset functionality
  {
    Simple_memory_guard guard = make_memory_guard(256);
    TEST_ASSERT(guard.is_valid(), "Guard should be valid initially");
    
    guard.reset(safe_malloc(128), 128);
    TEST_ASSERT(guard.is_valid(), "Guard should be valid after reset");
    
    guard.reset();
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
    Simple_transaction_guard transaction([&]() {
      rollback_called = true;
      counter = 0; // Rollback
    });
    
    counter = 10;
    throw runtime_error("Test exception");
    transaction.commit(); // This should not be reached
  } catch (const exception&) {
    // Expected exception
  }
  
  TEST_ASSERT(rollback_called, "Rollback should be called on exception");
  TEST_ASSERT(counter == 0, "Counter should be rolled back");
  
  // Test 2: No rollback when committed
  counter = 0;
  rollback_called = false;
  
  {
    Simple_transaction_guard transaction([&]() {
      rollback_called = true;
      counter = 0;
    });
    
    counter = 20;
    transaction.commit();
  }
  
  TEST_ASSERT(!rollback_called, "Rollback should not be called after commit");
  TEST_ASSERT(counter == 20, "Counter should retain committed value");
}

/** Test Exception Safety */
void test_exception_safety() {
  cout << "\n=== Testing Exception Safety ===" << endl;
  
  Simple_memory_tracker::reset();
  size_t initial_memory = Simple_memory_tracker::get_allocated_bytes();
  
  // Test exception safety with multiple resources
  for (int i = 0; i < 100; i++) {
    try {
      Simple_memory_guard guard1 = make_memory_guard(1024);
      Simple_memory_guard guard2 = make_memory_guard(2048);
      
      string filename = "/tmp/sbt_exception_test_" + to_string(i) + ".tmp";
      Simple_file_guard file_guard(open(filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644));
      
      if (i % 10 == 0) {
        throw runtime_error("Simulated exception");
      }
      
      // Use resources
      if (guard1.is_valid()) {
        memset(guard1.get(), i % 256, 1024);
      }
      
      if (file_guard.is_valid()) {
        string data = "test data " + to_string(i);
        write(file_guard.get(), data.c_str(), data.length());
      }
      
      unlink(filename.c_str());
      
    } catch (...) {
      // Resources should clean up automatically
    }
  }
  
  size_t final_memory = Simple_memory_tracker::get_allocated_bytes();
  TEST_ASSERT(final_memory == initial_memory, 
              "Memory should return to initial state after exceptions");
  TEST_ASSERT(!Simple_memory_tracker::has_leaks(), "Should have no memory leaks");
}

/** Test Multi-threaded Resource Management */
void test_multithreaded_resources() {
  cout << "\n=== Testing Multi-threaded Resource Management ===" << endl;
  
  const int NUM_THREADS = 4;
  const int OPERATIONS_PER_THREAD = 100;
  
  atomic<int> success_count(0);
  vector<thread> threads;
  
  for (int t = 0; t < NUM_THREADS; t++) {
    threads.emplace_back([&, t]() {
      for (int i = 0; i < OPERATIONS_PER_THREAD; i++) {
        try {
          Simple_memory_guard guard = make_memory_guard(1024 + (i % 1000));
          if (guard.is_valid()) {
            memset(guard.get(), (t * 100 + i) % 256, 1024 + (i % 1000));
            success_count++;
          }
          
          string filename = "/tmp/sbt_thread_" + to_string(t) + "_" + to_string(i) + ".tmp";
          Simple_file_guard file_guard(open(filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644));
          
          if (file_guard.is_valid()) {
            string data = "thread data " + to_string(t) + " " + to_string(i);
            write(file_guard.get(), data.c_str(), data.length());
          }
          
          unlink(filename.c_str());
          
          this_thread::sleep_for(chrono::microseconds(10));
          
        } catch (...) {
          // Ignore exceptions in stress test
        }
      }
    });
  }
  
  for (auto& thread : threads) {
    thread.join();
  }
  
  TEST_ASSERT(success_count > NUM_THREADS * OPERATIONS_PER_THREAD / 2,
              "Most operations should succeed in multi-threaded test");
}

/** Test Resource Stress */
void test_resource_stress() {
  cout << "\n=== Testing Resource Stress ===" << endl;
  
  Simple_memory_tracker::reset();
  size_t initial_memory = Simple_memory_tracker::get_allocated_bytes();
  
  // Rapid allocation/deallocation cycles
  for (int cycle = 0; cycle < 1000; cycle++) {
    vector<Simple_memory_guard> guards;
    
    // Allocate many small blocks
    for (int i = 0; i < 100; i++) {
      guards.emplace_back(make_memory_guard(64 + (i % 100)));
      if (guards.back().is_valid()) {
        memset(guards.back().get(), cycle % 256, 64 + (i % 100));
      }
    }
    
    // Free half of them manually
    for (int i = 0; i < 50; i++) {
      guards[i].free_memory();
    }
    
    // Test file operations
    if (cycle % 100 == 0) {
      string filename = "/tmp/sbt_stress_" + to_string(cycle) + ".tmp";
      Simple_file_guard file_guard(open(filename.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644));
      
      if (file_guard.is_valid()) {
        string data = "stress test cycle " + to_string(cycle);
        write(file_guard.get(), data.c_str(), data.length());
      }
      
      unlink(filename.c_str());
    }
  } // All guards should clean up automatically
  
  size_t final_memory = Simple_memory_tracker::get_allocated_bytes();
  TEST_ASSERT(final_memory == initial_memory,
              "Memory should return to initial state after stress test");
  TEST_ASSERT(!Simple_memory_tracker::has_leaks(), "Should have no leaks after stress test");
}

int main() {
  cout << "SBT RAII and Exception Safety Tests (Standalone)" << endl;
  cout << "================================================" << endl;
  
  try {
    test_file_guard();
    test_memory_guard();
    test_transaction_guard();
    test_exception_safety();
    test_multithreaded_resources();
    test_resource_stress();
    
    cout << "\n=== Test Results ===" << endl;
    cout << "Tests run: " << tests_run << endl;
    cout << "Tests passed: " << tests_passed << endl;
    cout << "Tests failed: " << (tests_run - tests_passed) << endl;
    
    if (tests_passed == tests_run) {
      cout << "\n🎉 ALL RAII AND EXCEPTION SAFETY TESTS PASSED! 🎉" << endl;
      cout << "Resource management and exception safety mechanisms are working correctly." << endl;
      return 0;
    } else {
      cout << "\n❌ SOME TESTS FAILED!" << endl;
      cout << "RAII or exception safety issues detected." << endl;
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