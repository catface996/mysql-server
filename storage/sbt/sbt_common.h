#ifndef SBT_COMMON_H
#define SBT_COMMON_H

#include "my_inttypes.h"
#include "my_sys.h"
#include "my_dbug.h"
#include "my_base.h"
#include "sql/table.h"
#include "sql/log.h"

// SBT Storage Engine constants
#define SBT_ENGINE_NAME "SBT"
#define SBT_FILE_EXTENSION ".sbt"
#define SBT_MAX_RECORD_SIZE (64 * 1024)  // 64KB max record size
#define SBT_DEFAULT_CACHE_SIZE (16 * 1024 * 1024)  // 16MB default cache

// SBT specific error codes
#define HA_ERR_SBT_CORRUPTED_DATA    150
#define HA_ERR_SBT_INVALID_TREE      151
#define HA_ERR_SBT_FILE_FORMAT       152
#define HA_ERR_SBT_TREE_UNBALANCED   153

// SBT balance threshold (alpha value for SBT algorithm)
#define SBT_BALANCE_ALPHA 0.75

// Position encoding for record identification
typedef uint64_t sbt_position_t;
#define SBT_INVALID_POSITION 0ULL

/**
 * SBT Error Handler
 * 
 * Provides centralized error handling and logging for the SBT storage engine.
 */
class SBT_error_handler {
public:
    static int handle_io_error(int error_code, const char *operation, const char *file_name = nullptr);
    static int handle_memory_error(const char *operation = nullptr);
    static int handle_corruption_error(const char *table_name, const char *details = nullptr);
    static void log_error(int level, const char *format, ...);
    static void log_warning(const char *format, ...);
    static void log_info(const char *format, ...);
    
    // Error code conversion
    static int mysql_error_from_errno(int sys_errno);
    static const char* get_error_message(int error_code);
};

/**
 * SBT Utility Functions
 * 
 * Common utility functions used throughout the SBT storage engine.
 */
class SBT_utils {
public:
    // String utilities
    static char* make_table_name(const char *db_name, const char *table_name);
    static char* make_file_name(const char *table_name, const char *extension);
    static void free_string(char *str);
    
    // Memory utilities
    static void* safe_malloc(size_t size);
    static void* safe_realloc(void *ptr, size_t size);
    static void safe_free(void *ptr);
    
    // Record utilities
    static size_t get_record_size(const uchar *record, TABLE *table);
    static int copy_record(uchar *dest, const uchar *src, TABLE *table);
    static int compare_records(const uchar *rec1, const uchar *rec2, TABLE *table);
    
    // File path utilities
    static std::string get_data_dir_path();
    static std::string build_file_path(const char *table_name);
    static bool is_valid_table_name(const char *name);
    
    // Checksum utilities
    static uint32_t calculate_record_checksum(const uchar *data, size_t length);
    static uint32_t calculate_table_checksum(TABLE *table);
    
    // Tree validation utilities
    static bool is_balanced_node(size_t left_size, size_t right_size, size_t total_size);
    static double get_balance_ratio(size_t left_size, size_t right_size);
};

// Debugging and testing macros
#ifdef NDEBUG
#define SBT_DBUG_ENTER(name)
#define SBT_DBUG_PRINT(keyword, arglist)
#define SBT_DBUG_RETURN(value) return(value)
#define SBT_DBUG_VOID_RETURN return
#else
#define SBT_DBUG_ENTER(name) DBUG_ENTER(name)
#define SBT_DBUG_PRINT(keyword, arglist) DBUG_PRINT(keyword, arglist)
#define SBT_DBUG_RETURN(value) DBUG_RETURN(value)
#define SBT_DBUG_VOID_RETURN DBUG_VOID_RETURN
#endif

// Performance monitoring macros
#define SBT_PERF_START(timer) auto timer = std::chrono::high_resolution_clock::now()
#define SBT_PERF_END(timer, operation) \
    do { \
        auto end_time = std::chrono::high_resolution_clock::now(); \
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - timer); \
        SBT_error_handler::log_info("SBT: %s took %ld microseconds", operation, duration.count()); \
    } while(0)

#endif // SBT_COMMON_H