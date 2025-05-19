#ifndef SKIPLIST_HA_SKIPLIST_H
#define SKIPLIST_HA_SKIPLIST_H

#include "handler.h"
#include "my_global.h"
#include "thr_lock.h"
#include "skiplist.h"

// 共享结构
class Skiplist_share : public Handler_share {
public:
    THR_LOCK lock;
    Skiplist_share();
    ~Skiplist_share() override;
};

// 存储引擎处理类
class ha_skiplist : public handler {
private:
    THR_LOCK_DATA lock;          // MySQL锁
    Skiplist_share *share;       // 共享锁信息
    SkipTable *skip_table;       // 跳表结构
    SkipListNode *current_position;      // 当前位置
    SkipListNode *current_index_position; // 当前索引位置

    // 内部辅助方法
    Skiplist_share *get_share();
    SkipListNode* find_node(const uchar* key, uint key_len);
    int insert_node(uchar* data, uint data_len);
    int delete_node(const uchar* key, uint key_len);
    uint extract_key(TABLE *table_arg, uint keynr, uchar *buf, uchar *key_buf);
    SkipListNode* find_exact_node(SkipList* index, const uchar* key, uint key_len);
    SkipListNode* find_or_next_node(SkipList* index, const uchar* key, uint key_len);
    int key_cmp(const uchar* key1, uint key1_len, const uchar* key2, uint key2_len);
    SkipTable* load_table_structure(const char* name);
    SkipTable* create_table_structure(const char* name, TABLE* form, const dd::Table* table_def);
    int save_table_structure(SkipTable* table);
    void free_table_structure(SkipTable* table);

public:
    ha_skiplist(handlerton *hton, TABLE_SHARE *table_arg);
    ~ha_skiplist() override;

    // 基本操作接口
    const char *table_type() const override { return "SKIPLIST"; }
    ulonglong table_flags() const override;
    ulong index_flags(uint inx, uint part, bool all_parts) const override;

    int open(const char *name, int mode, uint test_if_locked,
             const dd::Table *table_def) override;
    int close(void) override;
    int rnd_init(bool scan) override;
    int rnd_end() override;
    int rnd_next(uchar *buf) override;
    int rnd_pos(uchar *buf, uchar *pos) override;
    void position(const uchar *record) override;
    int info(uint) override;
    int create(const char *name, TABLE *form, HA_CREATE_INFO *create_info,
               dd::Table *table_def) override;
    THR_LOCK_DATA **store_lock(THD *thd, THR_LOCK_DATA **to,
                               enum thr_lock_type lock_type) override;

    // 写操作相关接口
    int write_row(uchar *buf) override;
    int update_row(const uchar *old_data, uchar *new_data) override;
    int delete_row(const uchar *buf) override;

    // 索引相关接口
    int index_init(uint idx, bool sorted) override;
    int index_end() override;
    int index_read_map(uchar *buf, const uchar *key, key_part_map keypart_map,
                       enum ha_rkey_function find_flag) override;
    int index_next(uchar *buf) override;
    int index_prev(uchar *buf) override;
    int index_first(uchar *buf) override;
    int index_last(uchar *buf) override;
};

#endif // SKIPLIST_HA_SKIPLIST_H