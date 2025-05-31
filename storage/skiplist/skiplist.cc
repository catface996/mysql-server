#include "skiplist.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>

// 随机层级生成函数
static uint32_t random_level(uint32_t max_level) {
    static bool seed_initialized = false;
    if (!seed_initialized) {
        srand((unsigned int)time(NULL));
        seed_initialized = true;
    }
    
    uint32_t level = 1;
    while ((rand() & 0xFFFF) < (0xFFFF >> 1) && level < max_level) {
        level++;
    }
    return level;
}

// 创建跳表
SkipList* skiplist_create(uint32_t max_level) {
    fprintf(stderr, "skiplist_create(max_level=%u)\n", max_level);
    
    // 使用calloc初始化为0
    SkipList* list = (SkipList*)calloc(1, sizeof(SkipList));
    if (!list) return nullptr;
    
    list->level = 0;
    list->size = 0;
    list->max_level = (max_level > 0 && max_level <= MAX_SKIPLIST_LEVEL) ? 
                       max_level : MAX_SKIPLIST_LEVEL;
    
    // 创建头节点
    list->header = (SkipListNode*)calloc(1, sizeof(SkipListNode));
    if (!list->header) {
        free(list);
        return nullptr;
    }
    
    list->header->level = list->max_level;
    list->header->data = nullptr;
    list->header->data_length = 0;
    
    // 初始化前向指针数组
    list->header->forward = (SkipListNode**)calloc(list->max_level, sizeof(SkipListNode*));
    if (!list->header->forward) {
        free(list->header);
        free(list);
        return nullptr;
    }
    
    return list;
}

// 销毁跳表
void skiplist_destroy(SkipList* list) {
    fprintf(stderr, "skiplist_destroy(list=%p)\n", list);
    
    if (!list) return;
    
    // 释放所有节点
    if (list->header && list->header->forward) {
        SkipListNode* current = list->header->forward[0];
        SkipListNode* next;
        
        while (current) {
            next = current->forward[0];
            
            if (current->data) {
                free(current->data);
                current->data = nullptr;
            }
            if (current->forward) {
                free(current->forward);
                current->forward = nullptr;
            }
            free(current);
            
            current = next;
        }
    }
    
    // 释放头节点
    if (list->header) {
        if (list->header->forward) {
            free(list->header->forward);
            list->header->forward = nullptr;
        }
        free(list->header);
        list->header = nullptr;
    }
    
    // 释放跳表结构
    free(list);
}

// 在跳表中搜索
SkipListNode* skiplist_search(SkipList* list, const uchar* key, uint key_len) {
    fprintf(stderr, "skiplist_search(list=%p, key=%p, key_len=%u)\n", list, key, key_len);
    
    if (!list || !key || key_len == 0) return nullptr;
    
    SkipListNode* current = list->header;
    
    // 从最高层开始搜索
    for (int i = list->level - 1; i >= 0; i--) {
        while (current->forward[i] && 
               memcmp(current->forward[i]->data, key, key_len) < 0) {
            current = current->forward[i];
        }
    }
    
    // 移动到第0层的下一个节点
    current = current->forward[0];
    
    // 检查是否找到匹配的键
    if (current && memcmp(current->data, key, key_len) == 0) {
        return current;
    }
    
    return nullptr;
}

// 向跳表中插入数据 - 简化版，不需要key参数
int skiplist_insert(SkipList* list, uchar* data, uint data_len) {
    fprintf(stderr, "skiplist_insert(list=%p, data=%p, data_len=%u)\n", 
            list, data, data_len);
    
    if (!list || !data || data_len == 0) {
        return -1;  // 参数错误
    }
    
    SkipListNode* update[MAX_SKIPLIST_LEVEL];
    SkipListNode* current = list->header;
    
    // 从最高层开始查找插入位置
    for (int i = list->level - 1; i >= 0; i--) {
        while (current->forward[i] && 
               memcmp(current->forward[i]->data, data, data_len) < 0) {
            current = current->forward[i];
        }
        update[i] = current;
    }
    
    // 移动到第0层的下一个节点
    current = current->forward[0];
    
    // 检查是否已存在相同的数据
    if (current && memcmp(current->data, data, data_len) == 0) {
        return 1;  // 数据已存在
    }
    
    // 生成随机层级
    uint32_t new_level = random_level(list->max_level);
    
    // 如果新层级高于当前层级，更新update数组
    if (new_level > list->level) {
        for (uint32_t i = list->level; i < new_level; i++) {
            update[i] = list->header;
        }
        list->level = new_level;
    }
    
    // 创建新节点
    SkipListNode* new_node = (SkipListNode*)calloc(1, sizeof(SkipListNode));
    if (!new_node) return -1;  // 内存分配失败
    
    new_node->level = new_level;
    new_node->data_length = data_len;
    
    // 分配前向指针数组
    new_node->forward = (SkipListNode**)calloc(new_level, sizeof(SkipListNode*));
    if (!new_node->forward) {
        free(new_node);
        return -1;
    }
    
    // 复制数据
    new_node->data = (uchar*)malloc(data_len);
    if (!new_node->data) {
        free(new_node->forward);
        free(new_node);
        return -1;
    }
    memcpy(new_node->data, data, data_len);
    
    // 更新指针
    for (uint32_t i = 0; i < new_level; i++) {
        new_node->forward[i] = update[i]->forward[i];
        update[i]->forward[i] = new_node;
    }
    
    list->size++;
    return 0;  // 插入成功
}

// 从跳表中删除数据
int skiplist_delete(SkipList* list, const uchar* key, uint key_len) {
    fprintf(stderr, "skiplist_delete(list=%p, key=%p, key_len=%u)\n", list, key, key_len);
    
    if (!list || !key || key_len == 0) return -1;  // 参数错误
    
    SkipListNode* update[MAX_SKIPLIST_LEVEL];
    SkipListNode* current = list->header;
    
    // 从最高层开始查找删除位置
    for (int i = list->level - 1; i >= 0; i--) {
        while (current->forward[i] && 
               memcmp(current->forward[i]->data, key, key_len) < 0) {
            current = current->forward[i];
        }
        update[i] = current;
    }
    
    // 移动到第0层的下一个节点
    current = current->forward[0];
    
    // 检查是否找到要删除的节点
    if (!current || memcmp(current->data, key, key_len) != 0) {
        return 1;  // 节点不存在
    }
    
    // 更新指针，删除节点
    for (uint32_t i = 0; i < list->level; i++) {
        if (update[i]->forward[i] != current) {
            break;
        }
        update[i]->forward[i] = current->forward[i];
    }
    
    // 在释放节点内存前，确保没有其他引用指向该节点
    // 这里需要检查是否有游标指向该节点
    
    // 释放节点内存
    if (current->data) {
        free(current->data);
        current->data = nullptr;
    }
    
    if (current->forward) {
        free(current->forward);
        current->forward = nullptr;
    }
    
    free(current);
    
    // 更新跳表层级
    while (list->level > 0 && list->header->forward[list->level - 1] == nullptr) {
        list->level--;
    }
    
    list->size--;
    return 0;  // 删除成功
}

// 创建表结构
SkipTable* skiptable_create(const char* name) {
    fprintf(stderr, "skiptable_create(name=%s)\n", name);
    
    if (!name) return nullptr;
    
    SkipTable* table = (SkipTable*)calloc(1, sizeof(SkipTable));
    if (!table) return nullptr;
    
    // 初始化表名
    size_t name_len = strlen(name);
    table->table_name = (char*)malloc(name_len + 1);
    if (!table->table_name) {
        free(table);
        return nullptr;
    }
    strcpy(table->table_name, name);
    
    // 创建主列表
    table->primary_list = skiplist_create(MAX_SKIPLIST_LEVEL);
    if (!table->primary_list) {
        free(table->table_name);
        free(table);
        return nullptr;
    }
    
    // 初始化其他字段
    table->row_count = 0;
    table->data_size = 0;
    
    return table;
}

// 销毁表结构
void skiptable_destroy(SkipTable* table) {
    fprintf(stderr, "skiptable_destroy(table=%p)\n", table);
    
    if (!table) return;
    
    // 释放主列表
    if (table->primary_list) {
        skiplist_destroy(table->primary_list);
        table->primary_list = nullptr;
    }
    
    // 释放表名
    if (table->table_name) {
        free(table->table_name);
        table->table_name = nullptr;
    }
    
    // 释放表结构
    free(table);
}