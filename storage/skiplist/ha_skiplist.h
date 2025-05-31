#ifndef SKIPLIST_HA_SKIPLIST_H
#define SKIPLIST_HA_SKIPLIST_H

#include "sql/handler.h"
#include "my_inttypes.h"
#include "thr_lock.h"
#include "skiplist.h"
#include "sql/table.h"

// 定义最大键长度
#define MAX_KEY_LENGTH 1024

// SkipList存储引擎处理器
class ha_skiplist : public handler {
public:
    ha_skiplist(handlerton *hton, TABLE_SHARE *table_arg);
    ~ha_skiplist() override;
    
    // 基本信息
    const char *table_type() const override { return "SKIPLIST"; }
    ulonglong table_flags() const override;
    ulong index_flags(uint idx, uint part, bool all_parts) const override;
    
    // 表操作
    int create(const char *name, TABLE *form, HA_CREATE_INFO *create_info,
               dd::Table *table_def) override;
    int open(const char *name, int mode, uint test_if_locked,
             const dd::Table *table_def) override;
    int close(void) override;
    
    // 表扫描
    int rnd_init(bool scan) override;
    int rnd_end() override;
    int rnd_next(uchar *buf) override;
    int rnd_pos(uchar *buf, uchar *pos) override;
    void position(const uchar *record) override;
    
    // 行操作
    int write_row(uchar *buf) override;
    int update_row(const uchar *old_data, uchar *new_data) override;
    int delete_row(const uchar *buf) override;
    
    // 表信息
    int info(uint flag) override;
    
    // 锁
    THR_LOCK_DATA **store_lock(THD *thd, THR_LOCK_DATA **to,
                               enum thr_lock_type lock_type) override;
    
private:
    // 内部辅助方法
    SkipTable* create_table_structure(const char* name);
    void free_table_structure(SkipTable* table_ptr);
    
    // 成员变量
    SkipTable *skip_table;
    SkipListNode *current_position;
    THR_LOCK lock;
    THR_LOCK_DATA lock_data;
};

#endif // SKIPLIST_HA_SKIPLIST_H