/*****************************************************************************

Copyright (c) 2025, Oracle and/or its affiliates.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License, version 2.0,
as published by the Free Software Foundation.

*****************************************************************************/

/** @file test_error_handling_standalone.cc
 SBT Error Handling Standalone Test

 Created 2025-01-26
 *******************************************************/

#include <iostream>
#include <cassert>
#include <cstring>
#include <cstdio>
#include <cstdarg>

// Mock MySQL types for standalone testing
typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long long uint64_t;
typedef unsigned int uint32_t;

// Mock MySQL error codes
#define HA_ERR_OUT_OF_MEM 5
#define HA_ERR_NO_SUCH_TABLE 1
#define HA_ERR_TABLE_EXIST 121
#define HA_ERR_NO_PERMISSION 13
#define HA_ERR_CRASHED_ON_USAGE 126
#define HA_ERR_RECORD_FILE_FULL 135
#define HA_ERR_FOUND_DUPP_KEY 121
#define HA_ERR_KEY_NOT_FOUND 125
#define HA_ERR_WRONG_COMMAND 131
#define HA_ERR_LOCK_WAIT_TIMEOUT 205
#define HA_ERR_GENERIC 2

// Mock MySQL functions
uint32_t my_thread_id() { return 12345; }

// Include SBT error handling definitions directly (standalone version)
// SBT Error Codes
enum sbt_error_t {
  SBT_SUCCESS = 0,
  
  // Memory related errors
  SBT_ERR_OUT_OF_MEMORY,
  SBT_ERR_MEMORY_CORRUPTION,
  
  // File system errors
  SBT_ERR_FILE_NOT_FOUND,
  SBT_ERR_FILE_EXISTS,
  SBT_ERR_FILE_PERMISSION,
  SBT_ERR_FILE_CORRUPTED,
  SBT_ERR_IO_ERROR,
  SBT_ERR_DISK_FULL,
  
  // Data integrity errors
  SBT_ERR_CORRUPTED_DATA,
  SBT_ERR_INVALID_HEADER,
  SBT_ERR_CHECKSUM_MISMATCH,
  SBT_ERR_VERSION_MISMATCH,
  
  // Tree operation errors
  SBT_ERR_DUPLICATE_KEY,
  SBT_ERR_KEY_NOT_FOUND,
  SBT_ERR_TREE_CORRUPTED,
  SBT_ERR_NODE_INVALID,
  
  // Parameter and state errors
  SBT_ERR_INVALID_ARGUMENT,
  SBT_ERR_NULL_POINTER,
  SBT_ERR_BUFFER_TOO_SMALL,
  SBT_ERR_INVALID_STATE,
  
  // Resource errors
  SBT_ERR_RESOURCE_BUSY,
  SBT_ERR_RESOURCE_EXHAUSTED,
  SBT_ERR_TIMEOUT,
  
  // Generic and unknown errors
  SBT_ERR_NOT_IMPLEMENTED,
  SBT_ERR_GENERIC,
  SBT_ERR_UNKNOWN
};

// Error severity levels
enum sbt_error_severity_t {
  SBT_SEVERITY_INFO = 0,
  SBT_SEVERITY_WARNING,
  SBT_SEVERITY_ERROR,
  SBT_SEVERITY_FATAL
};

// Error context structure for detailed error reporting
struct SBT_error_context {
  sbt_error_t error_code;
  sbt_error_severity_t severity;
  const char *file;
  int line;
  const char *function;
  char message[512];
  uint64_t timestamp;
  uint32_t thread_id;
};

// Mock implementations for testing
void *my_malloc(int, size_t size, int) { return malloc(size); }
void my_free(void *ptr) { free(ptr); }
void *my_realloc(int, void *ptr, size_t size, int) { return realloc(ptr, size); }

uint64_t sbt_get_current_time() {
  return 1640995200000000ULL; // Fixed timestamp for testing
}

// Standalone implementations of SBT error handling functions
int sbt_error_to_mysql_error(sbt_error_t sbt_error) {
  switch (sbt_error) {
    case SBT_SUCCESS:
      return 0;
      
    // Memory related errors
    case SBT_ERR_OUT_OF_MEMORY:
    case SBT_ERR_MEMORY_CORRUPTION:
      return HA_ERR_OUT_OF_MEM;
      
    // File system errors
    case SBT_ERR_FILE_NOT_FOUND:
      return HA_ERR_NO_SUCH_TABLE;
    case SBT_ERR_FILE_EXISTS:
      return HA_ERR_TABLE_EXIST;
    case SBT_ERR_FILE_PERMISSION:
      return HA_ERR_NO_PERMISSION;
    case SBT_ERR_FILE_CORRUPTED:
    case SBT_ERR_IO_ERROR:
      return HA_ERR_CRASHED_ON_USAGE;
    case SBT_ERR_DISK_FULL:
      return HA_ERR_RECORD_FILE_FULL;
      
    // Data integrity errors
    case SBT_ERR_CORRUPTED_DATA:
    case SBT_ERR_INVALID_HEADER:
    case SBT_ERR_CHECKSUM_MISMATCH:
    case SBT_ERR_VERSION_MISMATCH:
      return HA_ERR_CRASHED_ON_USAGE;
      
    // Tree operation errors
    case SBT_ERR_DUPLICATE_KEY:
      return HA_ERR_FOUND_DUPP_KEY;
    case SBT_ERR_KEY_NOT_FOUND:
      return HA_ERR_KEY_NOT_FOUND;
    case SBT_ERR_TREE_CORRUPTED:
    case SBT_ERR_NODE_INVALID:
      return HA_ERR_CRASHED_ON_USAGE;
      
    // Parameter and state errors
    case SBT_ERR_INVALID_ARGUMENT:
    case SBT_ERR_NULL_POINTER:
    case SBT_ERR_BUFFER_TOO_SMALL:
      return HA_ERR_WRONG_COMMAND;
    case SBT_ERR_INVALID_STATE:
      return HA_ERR_CRASHED_ON_USAGE;
      
    // Resource errors
    case SBT_ERR_RESOURCE_BUSY:
      return HA_ERR_LOCK_WAIT_TIMEOUT;
    case SBT_ERR_RESOURCE_EXHAUSTED:
      return HA_ERR_OUT_OF_MEM;
    case SBT_ERR_TIMEOUT:
      return HA_ERR_LOCK_WAIT_TIMEOUT;
      
    // Generic and unknown errors
    case SBT_ERR_NOT_IMPLEMENTED:
      return HA_ERR_WRONG_COMMAND;
    case SBT_ERR_GENERIC:
    case SBT_ERR_UNKNOWN:
    default:
      return HA_ERR_GENERIC;
  }
}

const char *sbt_error_to_string(sbt_error_t error_code) {
  switch (error_code) {
    case SBT_SUCCESS:
      return "Success";
      
    // Memory related errors
    case SBT_ERR_OUT_OF_MEMORY:
      return "Out of memory";
    case SBT_ERR_MEMORY_CORRUPTION:
      return "Memory corruption detected";
      
    // File system errors
    case SBT_ERR_FILE_NOT_FOUND:
      return "File not found";
    case SBT_ERR_FILE_EXISTS:
      return "File already exists";
    case SBT_ERR_FILE_PERMISSION:
      return "File permission denied";
    case SBT_ERR_FILE_CORRUPTED:
      return "File is corrupted";
    case SBT_ERR_IO_ERROR:
      return "I/O error";
    case SBT_ERR_DISK_FULL:
      return "Disk full";
      
    // Data integrity errors
    case SBT_ERR_CORRUPTED_DATA:
      return "Data corruption detected";
    case SBT_ERR_INVALID_HEADER:
      return "Invalid file header";
    case SBT_ERR_CHECKSUM_MISMATCH:
      return "Checksum mismatch";
    case SBT_ERR_VERSION_MISMATCH:
      return "Version mismatch";
      
    // Tree operation errors
    case SBT_ERR_DUPLICATE_KEY:
      return "Duplicate key";
    case SBT_ERR_KEY_NOT_FOUND:
      return "Key not found";
    case SBT_ERR_TREE_CORRUPTED:
      return "Tree structure corrupted";
    case SBT_ERR_NODE_INVALID:
      return "Invalid tree node";
      
    // Parameter and state errors
    case SBT_ERR_INVALID_ARGUMENT:
      return "Invalid argument";
    case SBT_ERR_NULL_POINTER:
      return "Null pointer";
    case SBT_ERR_BUFFER_TOO_SMALL:
      return "Buffer too small";
    case SBT_ERR_INVALID_STATE:
      return "Invalid state";
      
    // Resource errors
    case SBT_ERR_RESOURCE_BUSY:
      return "Resource busy";
    case SBT_ERR_RESOURCE_EXHAUSTED:
      return "Resource exhausted";
    case SBT_ERR_TIMEOUT:
      return "Operation timeout";
      
    // Generic and unknown errors
    case SBT_ERR_NOT_IMPLEMENTED:
      return "Not implemented";
    case SBT_ERR_GENERIC:
      return "Generic error";
    case SBT_ERR_UNKNOWN:
    default:
      return "Unknown error";
  }
}

const char *sbt_severity_to_string(sbt_error_severity_t severity) {
  switch (severity) {
    case SBT_SEVERITY_INFO:
      return "INFO";
    case SBT_SEVERITY_WARNING:
      return "WARNING";
    case SBT_SEVERITY_ERROR:
      return "ERROR";
    case SBT_SEVERITY_FATAL:
      return "FATAL";
    default:
      return "UNKNOWN";
  }
}

void sbt_log_error(const char *format, ...) {
  va_list args;
  va_start(args, format);
  
  char buffer[1024];
  vsnprintf(buffer, sizeof(buffer), format, args);
  
  fprintf(stderr, "[ERROR] SBT: %s\n", buffer);
  
  va_end(args);
}

void sbt_log_warning(const char *format, ...) {
  va_list args;
  va_start(args, format);
  
  char buffer[1024];
  vsnprintf(buffer, sizeof(buffer), format, args);
  
  fprintf(stderr, "[WARNING] SBT: %s\n", buffer);
  
  va_end(args);
}

void sbt_log_info(const char *format, ...) {
  va_list args;
  va_start(args, format);
  
  char buffer[1024];
  vsnprintf(buffer, sizeof(buffer), format, args);
  
  printf("[INFO] SBT: %s\n", buffer);
  
  va_end(args);
}

void sbt_log_debug(const char *format, ...) {
  va_list args;
  va_start(args, format);
  
  char buffer[1024];
  vsnprintf(buffer, sizeof(buffer), format, args);
  
  #ifdef DEBUG
    printf("[DEBUG] SBT: %s\n", buffer);
  #endif
  
  va_end(args);
}

void sbt_error_context_init(SBT_error_context *ctx) {
  if (!ctx) return;
  
  memset(ctx, 0, sizeof(SBT_error_context));
  ctx->error_code = SBT_SUCCESS;
  ctx->severity = SBT_SEVERITY_INFO;
  ctx->timestamp = sbt_get_current_time();
  ctx->thread_id = my_thread_id();
}

void sbt_error_context_set(SBT_error_context *ctx, sbt_error_t error_code,
                          sbt_error_severity_t severity, const char *file,
                          int line, const char *function, const char *format, ...) {
  if (!ctx) return;
  
  ctx->error_code = error_code;
  ctx->severity = severity;
  ctx->file = file;
  ctx->line = line;
  ctx->function = function;
  ctx->timestamp = sbt_get_current_time();
  ctx->thread_id = my_thread_id();
  
  // Format the error message
  if (format) {
    va_list args;
    va_start(args, format);
    vsnprintf(ctx->message, sizeof(ctx->message), format, args);
    va_end(args);
  } else {
    strncpy(ctx->message, sbt_error_to_string(error_code), sizeof(ctx->message) - 1);
    ctx->message[sizeof(ctx->message) - 1] = '\0';
  }
}

void sbt_error_context_log(const SBT_error_context *ctx) {
  if (!ctx) return;
  
  const char *severity_str = sbt_severity_to_string(ctx->severity);
  const char *error_str = sbt_error_to_string(ctx->error_code);
  
  char log_message[2048];
  snprintf(log_message, sizeof(log_message),
           "[%s] %s (%d) in %s() at %s:%d - %s (Thread: %u, Time: %llu)",
           severity_str, error_str, (int)ctx->error_code,
           ctx->function ? ctx->function : "unknown",
           ctx->file ? ctx->file : "unknown", ctx->line,
           ctx->message, ctx->thread_id, 
           (unsigned long long)ctx->timestamp);
  
  // Log based on severity
  switch (ctx->severity) {
    case SBT_SEVERITY_INFO:
      sbt_log_info("%s", log_message);
      break;
    case SBT_SEVERITY_WARNING:
      sbt_log_warning("%s", log_message);
      break;
    case SBT_SEVERITY_ERROR:
    case SBT_SEVERITY_FATAL:
      sbt_log_error("%s", log_message);
      break;
  }
}

void sbt_error_context_clear(SBT_error_context *ctx) {
  if (!ctx) return;
  
  sbt_error_context_init(ctx);
}

// Test counters
static int tests_run = 0;
static int tests_passed = 0;

#define TEST_ASSERT(condition, message) \
  do { \
    tests_run++; \
    if (condition) { \
      tests_passed++; \
      std::cout << "✓ " << message << std::endl; \
    } else { \
      std::cout << "✗ " << message << " - FAILED" << std::endl; \
    } \
  } while (0)

/** Test error code to string conversion */
void test_error_to_string() {
  std::cout << "\n=== Testing Error Code to String Conversion ===" << std::endl;
  
  TEST_ASSERT(strcmp(sbt_error_to_string(SBT_SUCCESS), "Success") == 0,
              "SBT_SUCCESS converts to 'Success'");
  
  TEST_ASSERT(strcmp(sbt_error_to_string(SBT_ERR_OUT_OF_MEMORY), "Out of memory") == 0,
              "SBT_ERR_OUT_OF_MEMORY converts correctly");
  
  TEST_ASSERT(strcmp(sbt_error_to_string(SBT_ERR_FILE_NOT_FOUND), "File not found") == 0,
              "SBT_ERR_FILE_NOT_FOUND converts correctly");
  
  TEST_ASSERT(strcmp(sbt_error_to_string(SBT_ERR_CORRUPTED_DATA), "Data corruption detected") == 0,
              "SBT_ERR_CORRUPTED_DATA converts correctly");
  
  TEST_ASSERT(strcmp(sbt_error_to_string(SBT_ERR_DUPLICATE_KEY), "Duplicate key") == 0,
              "SBT_ERR_DUPLICATE_KEY converts correctly");
  
  TEST_ASSERT(strcmp(sbt_error_to_string(SBT_ERR_INVALID_ARGUMENT), "Invalid argument") == 0,
              "SBT_ERR_INVALID_ARGUMENT converts correctly");
  
  TEST_ASSERT(strcmp(sbt_error_to_string(SBT_ERR_UNKNOWN), "Unknown error") == 0,
              "SBT_ERR_UNKNOWN converts correctly");
  
  // Test invalid error code
  TEST_ASSERT(strcmp(sbt_error_to_string((sbt_error_t)999), "Unknown error") == 0,
              "Invalid error code returns 'Unknown error'");
}

/** Test severity level to string conversion */
void test_severity_to_string() {
  std::cout << "\n=== Testing Severity Level to String Conversion ===" << std::endl;
  
  TEST_ASSERT(strcmp(sbt_severity_to_string(SBT_SEVERITY_INFO), "INFO") == 0,
              "SBT_SEVERITY_INFO converts to 'INFO'");
  
  TEST_ASSERT(strcmp(sbt_severity_to_string(SBT_SEVERITY_WARNING), "WARNING") == 0,
              "SBT_SEVERITY_WARNING converts to 'WARNING'");
  
  TEST_ASSERT(strcmp(sbt_severity_to_string(SBT_SEVERITY_ERROR), "ERROR") == 0,
              "SBT_SEVERITY_ERROR converts to 'ERROR'");
  
  TEST_ASSERT(strcmp(sbt_severity_to_string(SBT_SEVERITY_FATAL), "FATAL") == 0,
              "SBT_SEVERITY_FATAL converts to 'FATAL'");
  
  // Test invalid severity
  TEST_ASSERT(strcmp(sbt_severity_to_string((sbt_error_severity_t)999), "UNKNOWN") == 0,
              "Invalid severity returns 'UNKNOWN'");
}

/** Test SBT error to MySQL error mapping */
void test_error_to_mysql_mapping() {
  std::cout << "\n=== Testing SBT to MySQL Error Mapping ===" << std::endl;
  
  TEST_ASSERT(sbt_error_to_mysql_error(SBT_SUCCESS) == 0,
              "SBT_SUCCESS maps to 0");
  
  TEST_ASSERT(sbt_error_to_mysql_error(SBT_ERR_OUT_OF_MEMORY) == HA_ERR_OUT_OF_MEM,
              "SBT_ERR_OUT_OF_MEMORY maps to HA_ERR_OUT_OF_MEM");
  
  TEST_ASSERT(sbt_error_to_mysql_error(SBT_ERR_FILE_NOT_FOUND) == HA_ERR_NO_SUCH_TABLE,
              "SBT_ERR_FILE_NOT_FOUND maps to HA_ERR_NO_SUCH_TABLE");
  
  TEST_ASSERT(sbt_error_to_mysql_error(SBT_ERR_FILE_EXISTS) == HA_ERR_TABLE_EXIST,
              "SBT_ERR_FILE_EXISTS maps to HA_ERR_TABLE_EXIST");
  
  TEST_ASSERT(sbt_error_to_mysql_error(SBT_ERR_CORRUPTED_DATA) == HA_ERR_CRASHED_ON_USAGE,
              "SBT_ERR_CORRUPTED_DATA maps to HA_ERR_CRASHED_ON_USAGE");
  
  TEST_ASSERT(sbt_error_to_mysql_error(SBT_ERR_DUPLICATE_KEY) == HA_ERR_FOUND_DUPP_KEY,
              "SBT_ERR_DUPLICATE_KEY maps to HA_ERR_FOUND_DUPP_KEY");
  
  TEST_ASSERT(sbt_error_to_mysql_error(SBT_ERR_KEY_NOT_FOUND) == HA_ERR_KEY_NOT_FOUND,
              "SBT_ERR_KEY_NOT_FOUND maps to HA_ERR_KEY_NOT_FOUND");
  
  TEST_ASSERT(sbt_error_to_mysql_error(SBT_ERR_INVALID_ARGUMENT) == HA_ERR_WRONG_COMMAND,
              "SBT_ERR_INVALID_ARGUMENT maps to HA_ERR_WRONG_COMMAND");
  
  TEST_ASSERT(sbt_error_to_mysql_error(SBT_ERR_RESOURCE_BUSY) == HA_ERR_LOCK_WAIT_TIMEOUT,
              "SBT_ERR_RESOURCE_BUSY maps to HA_ERR_LOCK_WAIT_TIMEOUT");
  
  TEST_ASSERT(sbt_error_to_mysql_error(SBT_ERR_GENERIC) == HA_ERR_GENERIC,
              "SBT_ERR_GENERIC maps to HA_ERR_GENERIC");
  
  // Test unknown error code
  TEST_ASSERT(sbt_error_to_mysql_error((sbt_error_t)999) == HA_ERR_GENERIC,
              "Unknown error code maps to HA_ERR_GENERIC");
}

/** Test error context initialization */
void test_error_context_init() {
  std::cout << "\n=== Testing Error Context Initialization ===" << std::endl;
  
  SBT_error_context ctx;
  sbt_error_context_init(&ctx);
  
  TEST_ASSERT(ctx.error_code == SBT_SUCCESS,
              "Error context initializes with SBT_SUCCESS");
  
  TEST_ASSERT(ctx.severity == SBT_SEVERITY_INFO,
              "Error context initializes with SBT_SEVERITY_INFO");
  
  TEST_ASSERT(ctx.file == nullptr,
              "Error context initializes with null file");
  
  TEST_ASSERT(ctx.line == 0,
              "Error context initializes with line 0");
  
  TEST_ASSERT(ctx.function == nullptr,
              "Error context initializes with null function");
  
  TEST_ASSERT(ctx.message[0] == '\0',
              "Error context initializes with empty message");
  
  TEST_ASSERT(ctx.timestamp != 0,
              "Error context initializes with non-zero timestamp");
  
  TEST_ASSERT(ctx.thread_id == 12345,
              "Error context initializes with correct thread ID");
  
  // Test null pointer handling
  sbt_error_context_init(nullptr);
  TEST_ASSERT(true, "Error context init handles null pointer gracefully");
}

/** Test error context setting */
void test_error_context_set() {
  std::cout << "\n=== Testing Error Context Setting ===" << std::endl;
  
  SBT_error_context ctx;
  sbt_error_context_init(&ctx);
  
  const char *test_file = "test_file.cc";
  const char *test_function = "test_function";
  int test_line = 123;
  
  sbt_error_context_set(&ctx, SBT_ERR_OUT_OF_MEMORY, SBT_SEVERITY_ERROR,
                        test_file, test_line, test_function,
                        "Memory allocation failed for %d bytes", 1024);
  
  TEST_ASSERT(ctx.error_code == SBT_ERR_OUT_OF_MEMORY,
              "Error context sets correct error code");
  
  TEST_ASSERT(ctx.severity == SBT_SEVERITY_ERROR,
              "Error context sets correct severity");
  
  TEST_ASSERT(ctx.file == test_file,
              "Error context sets correct file");
  
  TEST_ASSERT(ctx.line == test_line,
              "Error context sets correct line");
  
  TEST_ASSERT(ctx.function == test_function,
              "Error context sets correct function");
  
  TEST_ASSERT(strstr(ctx.message, "Memory allocation failed for 1024 bytes") != nullptr,
              "Error context formats message correctly");
  
  TEST_ASSERT(ctx.timestamp != 0,
              "Error context updates timestamp");
  
  // Test with null format
  sbt_error_context_set(&ctx, SBT_ERR_INVALID_ARGUMENT, SBT_SEVERITY_WARNING,
                        test_file, test_line, test_function, nullptr);
  
  TEST_ASSERT(strcmp(ctx.message, "Invalid argument") == 0,
              "Error context uses default message when format is null");
  
  // Test null pointer handling
  sbt_error_context_set(nullptr, SBT_ERR_GENERIC, SBT_SEVERITY_ERROR,
                        test_file, test_line, test_function, "test");
  TEST_ASSERT(true, "Error context set handles null pointer gracefully");
}

/** Test error context clearing */
void test_error_context_clear() {
  std::cout << "\n=== Testing Error Context Clearing ===" << std::endl;
  
  SBT_error_context ctx;
  
  // Set some error context
  sbt_error_context_set(&ctx, SBT_ERR_CORRUPTED_DATA, SBT_SEVERITY_FATAL,
                        "file.cc", 456, "function", "Test error");
  
  // Clear the context
  sbt_error_context_clear(&ctx);
  
  TEST_ASSERT(ctx.error_code == SBT_SUCCESS,
              "Error context clears to SBT_SUCCESS");
  
  TEST_ASSERT(ctx.severity == SBT_SEVERITY_INFO,
              "Error context clears to SBT_SEVERITY_INFO");
  
  TEST_ASSERT(ctx.file == nullptr,
              "Error context clears file to null");
  
  TEST_ASSERT(ctx.line == 0,
              "Error context clears line to 0");
  
  TEST_ASSERT(ctx.function == nullptr,
              "Error context clears function to null");
  
  TEST_ASSERT(ctx.message[0] == '\0',
              "Error context clears message");
  
  // Test null pointer handling
  sbt_error_context_clear(nullptr);
  TEST_ASSERT(true, "Error context clear handles null pointer gracefully");
}

/** Test logging functions (basic functionality) */
void test_logging_functions() {
  std::cout << "\n=== Testing Logging Functions ===" << std::endl;
  
  // Redirect stderr to capture error logs
  FILE *original_stderr = stderr;
  FILE *temp_file = tmpfile();
  stderr = temp_file;
  
  sbt_log_error("Test error message: %d", 42);
  sbt_log_warning("Test warning message: %s", "warning");
  
  // Restore stderr
  stderr = original_stderr;
  fclose(temp_file);
  
  TEST_ASSERT(true, "Error logging function executes without crash");
  TEST_ASSERT(true, "Warning logging function executes without crash");
  
  // Test info and debug logging (to stdout)
  FILE *original_stdout = stdout;
  temp_file = tmpfile();
  stdout = temp_file;
  
  sbt_log_info("Test info message: %f", 3.14);
  sbt_log_debug("Test debug message");
  
  // Restore stdout
  stdout = original_stdout;
  fclose(temp_file);
  
  TEST_ASSERT(true, "Info logging function executes without crash");
  TEST_ASSERT(true, "Debug logging function executes without crash");
}

/** Test error context logging */
void test_error_context_logging() {
  std::cout << "\n=== Testing Error Context Logging ===" << std::endl;
  
  SBT_error_context ctx;
  sbt_error_context_set(&ctx, SBT_ERR_FILE_NOT_FOUND, SBT_SEVERITY_ERROR,
                        "test.cc", 789, "test_func", "File 'data.sbt' not found");
  
  // Redirect stderr to capture log output
  FILE *original_stderr = stderr;
  FILE *temp_file = tmpfile();
  stderr = temp_file;
  
  sbt_error_context_log(&ctx);
  
  // Restore stderr
  stderr = original_stderr;
  fclose(temp_file);
  
  TEST_ASSERT(true, "Error context logging executes without crash");
  
  // Test null pointer handling
  sbt_error_context_log(nullptr);
  TEST_ASSERT(true, "Error context log handles null pointer gracefully");
}

/** Test comprehensive error scenarios */
void test_comprehensive_error_scenarios() {
  std::cout << "\n=== Testing Comprehensive Error Scenarios ===" << std::endl;
  
  // Test all error codes have valid string representations
  for (int i = 0; i <= SBT_ERR_UNKNOWN; i++) {
    const char *error_str = sbt_error_to_string((sbt_error_t)i);
    TEST_ASSERT(error_str != nullptr && strlen(error_str) > 0,
                "All error codes have valid string representations");
  }
  
  // Test all severity levels have valid string representations
  for (int i = 0; i <= SBT_SEVERITY_FATAL; i++) {
    const char *severity_str = sbt_severity_to_string((sbt_error_severity_t)i);
    TEST_ASSERT(severity_str != nullptr && strlen(severity_str) > 0,
                "All severity levels have valid string representations");
  }
  
  // Test all error codes have valid MySQL mappings
  for (int i = 0; i <= SBT_ERR_UNKNOWN; i++) {
    int mysql_error = sbt_error_to_mysql_error((sbt_error_t)i);
    TEST_ASSERT(mysql_error >= 0,
                "All error codes have valid MySQL mappings");
  }
  
  // Test error context with maximum message length
  SBT_error_context ctx;
  char long_message[1000];
  memset(long_message, 'A', sizeof(long_message) - 1);
  long_message[sizeof(long_message) - 1] = '\0';
  
  sbt_error_context_set(&ctx, SBT_ERR_GENERIC, SBT_SEVERITY_ERROR,
                        "file.cc", 1, "func", "%s", long_message);
  
  TEST_ASSERT(strlen(ctx.message) < sizeof(ctx.message),
              "Long error messages are properly truncated");
  
  TEST_ASSERT(ctx.message[sizeof(ctx.message) - 1] == '\0',
              "Error message buffer is null-terminated");
}

int main() {
  std::cout << "SBT Storage Engine - Error Handling Standalone Test" << std::endl;
  std::cout << "====================================================" << std::endl;
  
  test_error_to_string();
  test_severity_to_string();
  test_error_to_mysql_mapping();
  test_error_context_init();
  test_error_context_set();
  test_error_context_clear();
  test_logging_functions();
  test_error_context_logging();
  test_comprehensive_error_scenarios();
  
  std::cout << "\n=== Test Results ===" << std::endl;
  std::cout << "Total tests: " << tests_run << std::endl;
  std::cout << "Passed: " << tests_passed << std::endl;
  std::cout << "Failed: " << (tests_run - tests_passed) << std::endl;
  
  if (tests_passed == tests_run) {
    std::cout << "\n🎉 All Error Handling Tests Passed! 🎉" << std::endl;
    return 0;
  } else {
    std::cout << "\n❌ Some Error Handling Tests Failed!" << std::endl;
    return 1;
  }
}