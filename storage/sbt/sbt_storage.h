#ifndef SBT_STORAGE_H
#define SBT_STORAGE_H

#include "my_inttypes.h"
#include <string>

/**
 * SBT File Header Structure
 * 
 * Defines the format of the SBT data file header containing metadata
 * about the stored tree structure.
 */
struct SBT_file_header {
    char magic[4];              // Magic number "SBT\0"
    uint32_t version;           // File format version
    uint64_t record_count;      // Total number of records
    uint64_t next_position;     // Next position identifier
    uint64_t table_checksum;    // Table structure checksum
    char reserved[32];          // Reserved for future use
    
    SBT_file_header();
    bool is_valid() const;
    void set_defaults();
};

/**
 * SBT Record Header Structure
 * 
 * Header for each record stored in the data file.
 */
struct SBT_record_header {
    uint32_t record_length;     // Record data length
    uint64_t position;          // Position identifier
    uint32_t checksum;          // Record checksum
    
    SBT_record_header();
    SBT_record_header(uint32_t len, uint64_t pos);
    void calculate_checksum(const uchar *data);
    bool verify_checksum(const uchar *data) const;
};

/**
 * SBT Storage Manager
 * 
 * Handles disk I/O operations for the SBT storage engine, including
 * file management, record serialization, and data persistence.
 */
class SBT_storage {
private:
    int data_file;              // Data file descriptor
    std::string file_path;      // File path
    bool is_open;               // File open status
    SBT_file_header header;     // File header
    
    // Internal helper methods
    int read_data(void *buffer, size_t size, off_t offset);
    int write_data(const void *buffer, size_t size, off_t offset);
    uint32_t calculate_checksum(const void *data, size_t size);
    
public:
    SBT_storage(const char *path);
    ~SBT_storage();
    
    // File operations
    int open_file(int mode);
    int close_file();
    int create_file();
    int delete_file();
    bool file_exists() const;
    
    // Header operations
    int read_header();
    int write_header();
    const SBT_file_header& get_header() const { return header; }
    void set_header(const SBT_file_header& new_header) { header = new_header; }
    
    // Record operations
    int read_record(uint64_t offset, uchar *buffer, size_t &length, uint64_t &position);
    int write_record(const uchar *data, size_t length, uint64_t position, uint64_t &offset);
    int delete_record(uint64_t offset);
    int update_record(uint64_t offset, const uchar *data, size_t length);
    
    // File management
    int sync_file();
    int truncate_file(off_t size);
    off_t get_file_size();
    
    // Validation and repair
    int validate_file();
    int repair_file();
    
    // Constants
    static const size_t HEADER_SIZE = sizeof(SBT_file_header);
    static const size_t RECORD_HEADER_SIZE = sizeof(SBT_record_header);
    static const uint32_t FILE_VERSION = 1;
    static const char MAGIC_NUMBER[4];
};

#endif // SBT_STORAGE_H