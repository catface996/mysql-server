#include "skiplist.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// 创建索引
SkipListIndex* skiplist_index_create(const char* name, uint32_t type, uint32_t key_offset, uint32_t key_length) {
    fprintf(stderr, "skiplist_index_create(name=%s, type=%u, key_offset=%u, key_length=%u)\n", 
            name, type, key_offset, key_length);
    
    SkipListIndex* index = (SkipListIndex*)calloc(1, sizeof(SkipListIndex));
    if (!index) return nullptr;
    
    index->index_type = type;
    index->key_offset = key_offset;
    index->key_length = key_length;
    
    // 创建索引列表
    index->list = skiplist_create(MAX_SKIPLIST_LEVEL);
    if (!index->list) {
        free(index);
        return nullptr;
    }
    
    // 设置索引名称
    if (name) {
        size_t name_len = strlen(name);
        index->name = (char*)malloc(name_len + 1);
        if (!index->name) {
            skiplist_destroy(index->list);
            free(index);
            return nullptr;
        }
        strcpy(index->name, name);
    }
    
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
    fprintf(stderr, "skiplist_index_search(index=%p, key=%p, key_length=%u)\n", 
            index, key, key_length);
    
    if (!index || !key || key_length == 0) {
        return nullptr;  // 参数错误
    }
    
    // 检查键长度是否有效
    if (key_length != index->key_length) {
        return nullptr;  // 参数错误
    }
    
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
    if (current && memcmp(current->data + index->key_offset, key, key_length) == 0) {
        return current;
    }
    
    return nullptr;
}