#ifndef SKIPLIST_SKIPLIST_H
#define SKIPLIST_SKIPLIST_H

#include <cstdint>
#include "my_inttypes.h"

// 最大跳表层级
#define MAX_SKIPLIST_LEVEL 16
#define MAX_PATH_LEN 1024

// 日志操作类型
#define LOG_OP_INSERT 1
#define LOG_OP_DELETE 2
#define LOG_OP_UPDATE 3

// 索引类型
#define INDEX_TYPE_PRIMARY 1
#define INDEX_TYPE_SECONDARY 2

// 跳表节点结构
struct SkipListNode {
    uchar* data;           // 行数据
    uint32_t data_length;  // 数据长度
    uint32_t level;        // 节点层级
    SkipListNode** forward; // 前向指针数组
};

// 跳表结构
struct SkipList {
    SkipListNode* header;  // 头节点
    uint32_t level;        // 当前最大层级
    uint32_t size;         // 元素数量
    uint32_t max_level;    // 最大允许层级
};

// 索引结构
struct SkipListIndex {
    uint32_t index_type;     // 索引类型
    uint32_t key_offset;     // 键在记录中的偏移量
    uint32_t key_length;     // 键长度
    SkipList* list;          // 索引列表
    char* name;              // 索引名称
};

// 表结构
struct SkipTable {
    SkipList* primary_list;  // 主列表
    char* table_name;        // 表名
    uint32_t row_count;      // 行数
    uint64_t data_size;      // 数据总大小
    char* data_file_path;    // 数据文件路径
    char* log_file_path;     // 日志文件路径
    
    // 索引相关字段
    uint32_t index_count;    // 索引数量
    SkipListIndex** indexes; // 索引数组
};

// 日志记录结构
struct LogRecord {
    uint32_t op_type;        // 操作类型
    uint32_t data_length;    // 数据长度
    uchar data[];            // 数据内容（变长）
};

// 跳表操作函数声明
SkipList* skiplist_create(uint32_t max_level);
void skiplist_destroy(SkipList* list);
SkipListNode* skiplist_search(SkipList* list, const uchar* key, uint key_len);
int skiplist_insert(SkipList* list, uchar* data, uint data_len);
int skiplist_delete(SkipList* list, const uchar* key, uint key_len);

// 表操作函数声明
SkipTable* skiptable_create(const char* name);
void skiptable_destroy(SkipTable* table);
int skiptable_save(SkipTable* table, const char* path);
SkipTable* skiptable_load(const char* path);

// 日志操作函数声明
int log_write(SkipTable* table, uint32_t op_type, const uchar* data, uint32_t data_len);
int log_recover(SkipTable* table);

// 索引操作函数声明
SkipListIndex* skiplist_index_create(const char* name, uint32_t type, uint32_t key_offset, uint32_t key_length);
void skiplist_index_destroy(SkipListIndex* index);
int skiplist_index_insert(SkipListIndex* index, const uchar* record, uint32_t record_length);
int skiplist_index_delete(SkipListIndex* index, const uchar* record, uint32_t record_length);
SkipListNode* skiplist_index_search(SkipListIndex* index, const uchar* key, uint32_t key_length);

#endif // SKIPLIST_SKIPLIST_H