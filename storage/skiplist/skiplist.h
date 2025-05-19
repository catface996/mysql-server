#ifndef SKIPLIST_SKIPLIST_H
#define SKIPLIST_SKIPLIST_H

#include <map>
#include <cstdint>
#include "my_inttypes.h"
#include "my_dbug.h"
#include "my_sys.h"

// 最大跳表层级
#define MAX_SKIPLIST_LEVEL 32

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

// 表结构
struct SkipTable {
    SkipList* primary_index;  // 主键索引
    std::map<uint, SkipList*> secondary_indexes;  // 二级索引
    char* table_name;         // 表名
    uint32_t row_count;       // 行数
    uint64_t data_size;       // 数据总大小
};

// 跳表操作函数声明
SkipList* skiplist_create(uint32_t max_level);
void skiplist_destroy(SkipList* list);
SkipListNode* skiplist_search(SkipList* list, const uchar* key, uint key_len);
int skiplist_insert(SkipList* list, uchar* data, uint data_len, const uchar* key, uint key_len);
int skiplist_delete(SkipList* list, const uchar* key, uint key_len);

// 表操作函数声明
SkipTable* skiptable_create(const char* name);
void skiptable_destroy(SkipTable* table);
int skiptable_save(SkipTable* table, const char* path);
SkipTable* skiptable_load(const char* path);
int skiptable_add_index(SkipTable* table, uint index_id);

#endif // SKIPLIST_SKIPLIST_H