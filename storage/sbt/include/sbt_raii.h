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

/** @file include/sbt_raii.h
 SBT RAII Resource Management Utilities

 Created 2025-01-25
 *******************************************************/

#ifndef sbt_raii_h
#define sbt_raii_h

#include "sbt_common.h"
#include "my_io.h"
#include <functional>
#include <memory>

/** RAII File Descriptor Wrapper
 * 
 * Automatically closes file descriptor when going out of scope.
 * Provides exception-safe file handling.
 */
class SBT_file_guard {
private:
  File fd_;
  bool should_close_;

public:
  /** Constructor with file descriptor
   * @param[in] fd File descriptor to manage
   */
  explicit SBT_file_guard(File fd = -1) : fd_(fd), should_close_(fd >= 0) {}

  /** Destructor - automatically closes file */
  ~SBT_file_guard() {
    close();
  }

  /** Move constructor */
  SBT_file_guard(SBT_file_guard&& other) noexcept 
    : fd_(other.fd_), should_close_(other.should_close_) {
    other.should_close_ = false;
  }

  /** Move assignment */
  SBT_file_guard& operator=(SBT_file_guard&& other) noexcept {
    if (this != &other) {
      close();
      fd_ = other.fd_;
      should_close_ = other.should_close_;
      other.should_close_ = false;
    }
    return *this;
  }

  /** Disable copy constructor and assignment */
  SBT_file_guard(const SBT_file_guard&) = delete;
  SBT_file_guard& operator=(const SBT_file_guard&) = delete;

  /** Get file descriptor */
  File get() const { return fd_; }

  /** Check if file is valid */
  bool is_valid() const { return fd_ >= 0; }

  /** Release ownership of file descriptor */
  File release() {
    should_close_ = false;
    return fd_;
  }

  /** Reset with new file descriptor */
  void reset(File fd = -1) {
    close();
    fd_ = fd;
    should_close_ = (fd >= 0);
  }

  /** Close file manually */
  void close() {
    if (should_close_ && fd_ >= 0) {
      my_close(fd_, MYF(0));
      should_close_ = false;
    }
    fd_ = -1;
  }
};

/** RAII Memory Buffer Wrapper
 * 
 * Automatically frees allocated memory when going out of scope.
 * Provides exception-safe memory management.
 */
class SBT_memory_guard {
private:
  void* ptr_;
  bool should_free_;

public:
  /** Constructor with memory pointer
   * @param[in] ptr Memory pointer to manage
   */
  explicit SBT_memory_guard(void* ptr = nullptr) : ptr_(ptr), should_free_(ptr != nullptr) {}

  /** Destructor - automatically frees memory */
  ~SBT_memory_guard() {
    free_memory();
  }

  /** Move constructor */
  SBT_memory_guard(SBT_memory_guard&& other) noexcept 
    : ptr_(other.ptr_), should_free_(other.should_free_) {
    other.should_free_ = false;
  }

  /** Move assignment */
  SBT_memory_guard& operator=(SBT_memory_guard&& other) noexcept {
    if (this != &other) {
      free_memory();
      ptr_ = other.ptr_;
      should_free_ = other.should_free_;
      other.should_free_ = false;
    }
    return *this;
  }

  /** Disable copy constructor and assignment */
  SBT_memory_guard(const SBT_memory_guard&) = delete;
  SBT_memory_guard& operator=(const SBT_memory_guard&) = delete;

  /** Get memory pointer */
  void* get() const { return ptr_; }

  /** Get typed memory pointer */
  template<typename T>
  T* get_as() const { return static_cast<T*>(ptr_); }

  /** Check if memory is valid */
  bool is_valid() const { return ptr_ != nullptr; }

  /** Release ownership of memory */
  void* release() {
    should_free_ = false;
    return ptr_;
  }

  /** Reset with new memory pointer */
  void reset(void* ptr = nullptr) {
    free_memory();
    ptr_ = ptr;
    should_free_ = (ptr != nullptr);
  }

  /** Free memory manually */
  void free_memory() {
    if (should_free_ && ptr_) {
      sbt_free(ptr_);
      should_free_ = false;
    }
    ptr_ = nullptr;
  }
};

/** RAII Generic Resource Wrapper
 * 
 * Generic RAII wrapper that can manage any resource with a cleanup function.
 * Uses std::function for maximum flexibility.
 */
template<typename T>
class SBT_resource_guard {
private:
  T resource_;
  std::function<void(T)> cleanup_;
  bool should_cleanup_;

public:
  /** Constructor with resource and cleanup function
   * @param[in] resource Resource to manage
   * @param[in] cleanup Cleanup function to call
   */
  SBT_resource_guard(T resource, std::function<void(T)> cleanup)
    : resource_(resource), cleanup_(cleanup), should_cleanup_(true) {}

  /** Destructor - automatically calls cleanup */
  ~SBT_resource_guard() {
    if (should_cleanup_ && cleanup_) {
      cleanup_(resource_);
    }
  }

  /** Move constructor */
  SBT_resource_guard(SBT_resource_guard&& other) noexcept
    : resource_(std::move(other.resource_)), 
      cleanup_(std::move(other.cleanup_)),
      should_cleanup_(other.should_cleanup_) {
    other.should_cleanup_ = false;
  }

  /** Move assignment */
  SBT_resource_guard& operator=(SBT_resource_guard&& other) noexcept {
    if (this != &other) {
      if (should_cleanup_ && cleanup_) {
        cleanup_(resource_);
      }
      resource_ = std::move(other.resource_);
      cleanup_ = std::move(other.cleanup_);
      should_cleanup_ = other.should_cleanup_;
      other.should_cleanup_ = false;
    }
    return *this;
  }

  /** Disable copy constructor and assignment */
  SBT_resource_guard(const SBT_resource_guard&) = delete;
  SBT_resource_guard& operator=(const SBT_resource_guard&) = delete;

  /** Get resource */
  const T& get() const { return resource_; }
  T& get() { return resource_; }

  /** Release ownership of resource */
  T release() {
    should_cleanup_ = false;
    return std::move(resource_);
  }
};

/** Exception-Safe Transaction Helper
 * 
 * Provides rollback functionality for operations that need to be atomic.
 * Automatically calls rollback function if commit is not called.
 */
class SBT_transaction_guard {
private:
  std::function<void()> rollback_;
  bool committed_;

public:
  /** Constructor with rollback function
   * @param[in] rollback Function to call on rollback
   */
  explicit SBT_transaction_guard(std::function<void()> rollback)
    : rollback_(rollback), committed_(false) {}

  /** Destructor - calls rollback if not committed */
  ~SBT_transaction_guard() {
    if (!committed_ && rollback_) {
      try {
        rollback_();
      } catch (...) {
        // Ignore exceptions in destructor
      }
    }
  }

  /** Disable copy constructor and assignment */
  SBT_transaction_guard(const SBT_transaction_guard&) = delete;
  SBT_transaction_guard& operator=(const SBT_transaction_guard&) = delete;

  /** Commit the transaction (prevents rollback) */
  void commit() {
    committed_ = true;
  }

  /** Manually trigger rollback */
  void rollback() {
    if (!committed_ && rollback_) {
      rollback_();
      committed_ = true; // Prevent double rollback
    }
  }
};

/** Memory Leak Detection Helper
 * 
 * Tracks memory allocations and detects leaks in debug builds.
 * Only active when SBT_DEBUG_MEMORY is defined.
 */
class SBT_memory_tracker {
private:
  static thread_local size_t allocated_bytes_;
  static thread_local size_t allocation_count_;

public:
  /** Record memory allocation
   * @param[in] size Size of allocated memory
   */
  static void record_allocation(size_t size) {
#ifdef SBT_DEBUG_MEMORY
    allocated_bytes_ += size;
    allocation_count_++;
#endif
  }

  /** Record memory deallocation
   * @param[in] size Size of deallocated memory
   */
  static void record_deallocation(size_t size) {
#ifdef SBT_DEBUG_MEMORY
    if (allocated_bytes_ >= size) {
      allocated_bytes_ -= size;
    }
    if (allocation_count_ > 0) {
      allocation_count_--;
    }
#endif
  }

  /** Get current allocated bytes */
  static size_t get_allocated_bytes() {
#ifdef SBT_DEBUG_MEMORY
    return allocated_bytes_;
#else
    return 0;
#endif
  }

  /** Get current allocation count */
  static size_t get_allocation_count() {
#ifdef SBT_DEBUG_MEMORY
    return allocation_count_;
#else
    return 0;
#endif
  }

  /** Check for memory leaks */
  static bool has_leaks() {
#ifdef SBT_DEBUG_MEMORY
    return allocated_bytes_ > 0 || allocation_count_ > 0;
#else
    return false;
#endif
  }
};

/** Exception-Safe Memory Allocation Functions
 * 
 * Wrapper functions that integrate with memory tracking and provide
 * exception safety guarantees.
 */

/** Safe memory allocation with tracking
 * @param[in] size Size to allocate
 * @return Allocated memory pointer or nullptr on failure
 */
inline void* sbt_safe_malloc(size_t size) {
  void* ptr = sbt_malloc(size);
  if (ptr) {
    SBT_memory_tracker::record_allocation(size);
  }
  return ptr;
}

/** Safe memory deallocation with tracking
 * @param[in] ptr Memory pointer to free
 * @param[in] size Size of memory being freed (for tracking)
 */
inline void sbt_safe_free(void* ptr, size_t size = 0) {
  if (ptr) {
    SBT_memory_tracker::record_deallocation(size);
    sbt_free(ptr);
  }
}

/** Create memory guard with tracking
 * @param[in] size Size to allocate
 * @return Memory guard with allocated memory
 */
inline SBT_memory_guard sbt_make_memory_guard(size_t size) {
  void* ptr = sbt_safe_malloc(size);
  return SBT_memory_guard(ptr);
}

#endif /* sbt_raii_h */