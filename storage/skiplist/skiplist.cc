#include "skiplist.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// 创建跳表
SkipList* skiplist_create(uint32_t max_level) {
    fprintf(stderr, "skiplist_create(max_level=%u)\n", max_level);
    // 待实现
    return nullptr;
}

// 销毁跳表
void skiplist_destroy(SkipList* list) {
    fprintf(stderr, "skiplist_destroy(list=%p)\n", list);
    // 待实现
}

// 在跳表中搜索
SkipListNode* skiplist_search(SkipList* list, const uchar* key, uint key_len) {
    fprintf(stderr, "skiplist_search(list=%p, key=%p, key_len=%u)\n", list, key, key_len);
    // 待实现
    return nullptr;
}

// 向跳表中插入数据
int skiplist_insert(SkipList* list, uchar* data, uint data_len, const uchar* key, uint key_len) {
    fprintf(stderr, "skiplist_insert(list=%p, data=%p, data_len=%u, key=%p, key_len=%u)\n", 
            list, data, data_len, key, key_len);
    // 待实现
    return 0;
}

// 从跳表中删除数据
int skiplist_delete(SkipList* list, const uchar* key, uint key_len) {
    fprintf(stderr, "skiplist_delete(list=%p, key=%p, key_len=%u)\n", list, key, key_len);
    // 待实现
    return 0;
}

// 创建表结构
SkipTable* skiptable_create(const char* name) {
    fprintf(stderr, "skiptable_create(name=%s)\n", name);
    // 待实现
    return nullptr;
}

// 销毁表结构
void skiptable_destroy(SkipTable* table) {
    fprintf(stderr, "skiptable_destroy(table=%p)\n", table);
    // 待实现
}

// 保存表结构到文件
int skiptable_save(SkipTable* table, const char* path) {
    fprintf(stderr, "skiptable_save(table=%p, path=%s)\n", table, path);
    // 待实现
    return 0;
}

// 从文件加载表结构
SkipTable* skiptable_load(const char* path) {
    fprintf(stderr, "skiptable_load(path=%s)\n", path);
    // 待实现
    return nullptr;
}

// 添加二级索引
int skiptable_add_index(SkipTable* table, uint index_id) {
    fprintf(stderr, "skiptable_add_index(table=%p, index_id=%u)\n", table, index_id);
    // 待实现
    return 0;
}