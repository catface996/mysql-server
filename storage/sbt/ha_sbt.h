#ifndef HA_SBT_H
#define HA_SBT_H

#include "sql/handler.h"
#include "sql/table.h"
#include "thr_lock.h"
#include "my_base.h"

// Forward declarations
class SBT_share;
class SBT_tree;
class SBT_cursor;

/**
 * SBT Storage Engine Handler
 * 
 * This class implements the MySQL handler interface for the SBT (Size Balanced Tree)
 * storage engine. It provides basic CRUD operations using a balanced tree structure.
 */
class ha_sbt : public handler {
private:
    SBT_share *share;           // Shared data structure
    SBT_tree *tree;             // SBT tree instance
    SBT_cursor *cursor;         // Traversal cursor
    THR_LOCK_DATA lock;         // MySQL lock data
    
public:
    // Constructor and destructor
    ha_sbt(handlerton *hton, TABLE_SHARE *table_arg);
    ~ha_sbt() override;
    
    // Required handler methods
    int open(const char *name, int mode, uint test_if_locked, 
             const dd::Table *table_def) override;
    int close() override;
    int create(const char *name, TABLE *form, HA_CREATE_INFO *create_info, 
               dd::Table *table_def) override;
    int delete_table(const char *from, const dd::Table *table_def) override;
    
    // CRUD operations
    int write_row(uchar *buf) override;
    int update_row(const uchar *old_data, uchar *new_data) override;
    int delete_row(const uchar *buf) override;
    
    // Scan operations
    int rnd_init(bool scan) override;
    int rnd_end() override;
    int rnd_next(uchar *buf) override;
    int rnd_pos(uchar *buf, uchar *pos) override;
    void position(const uchar *record) override;
    
    // Metadata and configuration
    const char *table_type() const override { return "SBT"; }
    ulonglong table_flags() const override;
    int info(uint flag) override;
    int external_lock(THD *thd, int lock_type) override;
    THR_LOCK_DATA **store_lock(THD *thd, THR_LOCK_DATA **to, 
                               enum thr_lock_type lock_type) override;
    
    // Statistics and optimization
    ha_rows records_in_range(uint inx, key_range *min_key, 
                            key_range *max_key) override;
    int analyze(THD *thd, HA_CHECK_OPT *check_opt) override;
    int optimize(THD *thd, HA_CHECK_OPT *check_opt) override;
    int check(THD *thd, HA_CHECK_OPT *check_opt) override;
    int repair(THD *thd, HA_CHECK_OPT *check_opt) override;
    
    // Index operations (required pure virtual functions)
    ulong index_flags(uint idx, uint part, bool all_parts) const override;
    uint max_supported_record_length() const override;
    uint max_supported_keys() const override;
    uint max_supported_key_parts() const override;
    uint max_supported_key_length() const override;
    
private:
    // Helper methods
    int get_share();
    int free_share();
    int write_record_to_tree(const uchar *buf);
    int read_record_from_tree(uchar *buf, uint64_t position);
};

#endif // HA_SBT_H