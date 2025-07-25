#ifndef SBT_SHARE_H
#define SBT_SHARE_H

#include "sql/handler.h"
#include "thr_lock.h"
#include <atomic>
#include <string>

// Forward declarations
class SBT_tree;
class SBT_storage;

/**
 * SBT Share Class
 * 
 * Manages shared data structures for SBT tables, including reference counting,
 * thread safety, and resource management.
 */
class SBT_share : public Handler_share {
private:
    std::string table_name;         // Table name
    std::string data_file_name;     // Data file name
    SBT_tree *tree;                 // Shared SBT tree
    SBT_storage *storage;           // Storage manager
    THR_LOCK lock;                  // Thread lock
    std::atomic<uint> ref_count;    // Reference count
    bool is_loaded;                 // Whether data is loaded from disk
    
public:
    SBT_share(const char *name);
    ~SBT_share() override;
    
    // Reference counting
    void add_ref() { ref_count.fetch_add(1); }
    void release_ref();
    uint get_ref_count() const { return ref_count.load(); }
    
    // Access methods
    SBT_tree* get_tree() { return tree; }
    SBT_storage* get_storage() { return storage; }
    THR_LOCK* get_lock() { return &lock; }
    const char* get_table_name() const { return table_name.c_str(); }
    const char* get_data_file_name() const { return data_file_name.c_str(); }
    
    // Persistence operations
    int load_from_disk();
    int save_to_disk();
    int create_data_file();
    int delete_data_file();
    
    // Initialization
    int initialize(TABLE *table);
    void cleanup();
    
    // Status
    bool is_data_loaded() const { return is_loaded; }
};

// Global share management functions
SBT_share* get_sbt_share(const char *table_name, TABLE *table);
void free_sbt_share(SBT_share *share);

#endif // SBT_SHARE_H