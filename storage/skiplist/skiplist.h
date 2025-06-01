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

// 表结构 - 简化版，移除二级索引
struct SkipTable {
    SkipList* primary_list;  // 主列表
    char* table_name;        // 表名
    uint32_t row_count;      // 行数
    uint64_t data_size;      // 数据总大小
    char* data_file_path;    // 数据文件路径
    char* log_file_path;     // 日志文件路径
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

#endif // SKIPLIST_SKIPLIST_H