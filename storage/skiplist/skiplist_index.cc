#include "skiplist.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// 创建索引
SkipListIndex* skiplist_index_create(const char* name, uint32_t type, uint32_t key_offset, uint32_t key_length) {
    fprintf(stderr, "DEBUG: skiplist_index_create(name=%s, type=%u, key_offset=%u, key_length=%u)\n", 
            name, type, key_offset, key_length);
    
    SkipListIndex* index = (SkipListIndex*)calloc(1, sizeof(SkipListIndex));
    if (!index) {
        fprintf(stderr, "ERROR: skiplist_index_create - 内存分配失败\n");
        return nullptr;
    }
    
    index->index_type = type;
    index->key_offset = key_offset;
    index->key_length = key_length;
    
    // 创建索引列表
    fprintf(stderr, "DEBUG: skiplist_index_create - 创建索引列表\n");
    index->list = skiplist_create(MAX_SKIPLIST_LEVEL);
    if (!index->list) {
        fprintf(stderr, "ERROR: skiplist_index_create - 创建索引列表失败\n");
        free(index);
        return nullptr;
    }
    
    // 设置索引名称
    if (name) {
        size_t name_len = strlen(name);
        index->name = (char*)malloc(name_len + 1);
        if (!index->name) {
            fprintf(stderr, "ERROR: skiplist_index_create - 索引名称内存分配失败\n");
            skiplist_destroy(index->list);
            free(index);
            return nullptr;
        }
        strcpy(index->name, name);
        fprintf(stderr, "DEBUG: skiplist_index_create - 设置索引名称: %s\n", index->name);
    }
    
    fprintf(stderr, "DEBUG: skiplist_index_create - 索引创建成功: name=%s, type=%u, offset=%u, length=%u\n", 
            index->name, index->index_type, index->key_offset, index->key_length);
    
    return index;
}

// 销毁索引
void skiplist_index_destroy(SkipListIndex* index) {
    fprintf(stderr, "skiplist_index_destroy(index=%p)\n", index);
    
    if (!index) return;
    
    // 释放索引列表
    if (index->list) {
        skiplist_destroy(index->list);
        index->list = nullptr;
    }
    
    // 释放索引名称
    if (index->name) {
        free(index->name);
        index->name = nullptr;
    }
    
    // 释放索引结构
    free(index);
}

// 向索引中插入记录
int skiplist_index_insert(SkipListIndex* index, const uchar* record, uint32_t record_length) {
    fprintf(stderr, "skiplist_index_insert(index=%p, record=%p, record_length=%u)\n", 
            index, record, record_length);
    
    if (!index || !record || record_length == 0) {
        return -1;  // 参数错误
    }
    
    // 检查键偏移量和长度是否有效
    if (index->key_offset + index->key_length > record_length) {
        return -1;  // 参数错误
    }
    
    // 复制记录
    uchar* data_copy = (uchar*)malloc(record_length);
    if (!data_copy) {
        return -1;  // 内存分配失败
    }
    memcpy(data_copy, record, record_length);
    
    // 插入到索引列表
    int result = skiplist_insert(index->list, data_copy, record_length);
    if (result != 0) {
        free(data_copy);
        return result;
    }
    
    return 0;  // 插入成功
}

// 从索引中删除记录
int skiplist_index_delete(SkipListIndex* index, const uchar* record, uint32_t record_length) {
    fprintf(stderr, "skiplist_index_delete(index=%p, record=%p, record_length=%u)\n", 
            index, record, record_length);
    
    if (!index || !record || record_length == 0) {
        return -1;  // 参数错误
    }
    
    // 检查键偏移量和长度是否有效
    if (index->key_offset + index->key_length > record_length) {
        return -1;  // 参数错误
    }
    
    // 从索引列表中删除
    int result = skiplist_delete(index->list, record, record_length);
    
    return result;
}

// 在索引中搜索记录
SkipListNode* skiplist_index_search(SkipListIndex* index, const uchar* key, uint32_t key_length) {
    fprintf(stderr, "DEBUG: skiplist_index_search(index=%p, key=%p, key_length=%u)\n", 
            index, key, key_length);
    
    if (!index || !key || key_length == 0) {
        fprintf(stderr, "ERROR: skiplist_index_search - 参数错误\n");
        return nullptr;  // 参数错误
    }
    
    // 打印索引信息
    fprintf(stderr, "DEBUG: skiplist_index_search - 索引信息: name=%s, type=%u, key_offset=%u, key_length=%u\n", 
            index->name, index->index_type, index->key_offset, index->key_length);
    
    // 打印键值的十六进制表示
    fprintf(stderr, "DEBUG: skiplist_index_search - 搜索键值(hex): ");
    for (uint32_t i = 0; i < key_length; i++) {
        fprintf(stderr, "%02x ", key[i]);
    }
    fprintf(stderr, "\n");
    
    // 检查键长度是否有效
    if (key_length != index->key_length) {
        fprintf(stderr, "ERROR: skiplist_index_search - 键长度不匹配: 期望=%u, 实际=%u\n", 
                index->key_length, key_length);
        return nullptr;  // 参数错误
    }
    
    if (!index->list || !index->list->header) {
        fprintf(stderr, "ERROR: skiplist_index_search - 索引列表或头节点为空\n");
        return nullptr;
    }
    
    fprintf(stderr, "DEBUG: skiplist_index_search - 开始搜索, 当前列表级别=%u, 大小=%u\n", 
            index->list->level, index->list->size);
    
    SkipListNode* current = index->list->header;
    
    // 从最高层开始搜索
    for (int i = index->list->level - 1; i >= 0; i--) {
        while (current->forward[i] && 
               memcmp(current->forward[i]->data + index->key_offset, key, key_length) < 0) {
            current = current->forward[i];
        }
    }
    
    // 移动到第0层的下一个节点
    current = current->forward[0];
    
    // 检查是否找到匹配的键
    if (current) {
        int cmp_result = memcmp(current->data + index->key_offset, key, key_length);
        fprintf(stderr, "DEBUG: skiplist_index_search - 比较结果: %d (0表示匹配)\n", cmp_result);
        
        if (cmp_result == 0) {
            fprintf(stderr, "DEBUG: skiplist_index_search - 找到匹配节点\n");
            return current;
        }
    }
    
    fprintf(stderr, "DEBUG: skiplist_index_search - 未找到匹配节点\n");
    return nullptr;
}