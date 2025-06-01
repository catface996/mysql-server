#include "skiplist.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

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
    new_node->data = data;  // 直接使用传入的数据，不再复制
    
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
    
    // 设置数据文件路径
    table->data_file_path = (char*)malloc(name_len + 5);  // +5 for ".skl"
    if (!table->data_file_path) {
        skiplist_destroy(table->primary_list);
        free(table->table_name);
        free(table);
        return nullptr;
    }
    sprintf(table->data_file_path, "%s.skl", name);
    
    // 设置日志文件路径
    table->log_file_path = (char*)malloc(name_len + 5);  // +5 for ".log"
    if (!table->log_file_path) {
        free(table->data_file_path);
        skiplist_destroy(table->primary_list);
        free(table->table_name);
        free(table);
        return nullptr;
    }
    sprintf(table->log_file_path, "%s.log", name);
    
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
    
    // 释放文件路径
    if (table->data_file_path) {
        free(table->data_file_path);
        table->data_file_path = nullptr;
    }
    
    if (table->log_file_path) {
        free(table->log_file_path);
        table->log_file_path = nullptr;
    }
    
    // 释放表结构
    free(table);
}

// 保存表结构到文件
int skiptable_save(SkipTable* table, const char* path) {
    fprintf(stderr, "skiptable_save(table=%p, path=%s)\n", table, path);
    
    if (!table || !path) return -1;
    
    // 创建目录（如果不存在）
    char dir_path[MAX_PATH_LEN];
    if (strlen(path) >= MAX_PATH_LEN) {
        fprintf(stderr, "Path too long: %s\n", path);
        return -1;
    }
    
    strcpy(dir_path, path);
    char* last_slash = strrchr(dir_path, '/');
    if (last_slash) {
        *last_slash = '\0';
        // 创建目录
        int ret = mkdir(dir_path, 0777);
        if (ret != 0 && errno != EEXIST) {
            fprintf(stderr, "Failed to create directory: %s, errno: %d\n", dir_path, errno);
            return -1;
        }
    }
    
    fprintf(stderr, "Opening file for writing: %s\n", path);
    FILE* file = fopen(path, "wb");
    if (!file) {
        fprintf(stderr, "Failed to open file: %s, errno: %d\n", path, errno);
        return -1;
    }
    
    // 写入文件头
    uint32_t magic = 0x534B4C54; // "SKLT"
    uint32_t version = 1;
    uint32_t name_len = 0;
    
    if (table->table_name) {
        name_len = strlen(table->table_name);
    }
    
    fprintf(stderr, "Writing file header: magic=%u, version=%u, name_len=%u\n", 
            magic, version, name_len);
    
    if (fwrite(&magic, sizeof(magic), 1, file) != 1 ||
        fwrite(&version, sizeof(version), 1, file) != 1 ||
        fwrite(&name_len, sizeof(name_len), 1, file) != 1) {
        fprintf(stderr, "Failed to write file header\n");
        fclose(file);
        return -1;
    }
    
    if (name_len > 0 && table->table_name) {
        fprintf(stderr, "Writing table name: %s\n", table->table_name);
        if (fwrite(table->table_name, name_len, 1, file) != 1) {
            fprintf(stderr, "Failed to write table name\n");
            fclose(file);
            return -1;
        }
    }
    
    fprintf(stderr, "Writing table info: row_count=%u, data_size=%lu\n", 
            table->row_count, table->data_size);
    
    if (fwrite(&table->row_count, sizeof(table->row_count), 1, file) != 1 ||
        fwrite(&table->data_size, sizeof(table->data_size), 1, file) != 1) {
        fprintf(stderr, "Failed to write table info\n");
        fclose(file);
        return -1;
    }
    
    // 写入主键索引信息
    uint32_t primary_list_size = 0;
    uint32_t primary_list_level = 0;
    
    if (table->primary_list) {
        primary_list_size = table->primary_list->size;
        primary_list_level = table->primary_list->level;
    }
    
    fprintf(stderr, "Writing primary list info: size=%u, level=%u\n", 
            primary_list_size, primary_list_level);
    
    if (fwrite(&primary_list_size, sizeof(primary_list_size), 1, file) != 1 ||
        fwrite(&primary_list_level, sizeof(primary_list_level), 1, file) != 1) {
        fprintf(stderr, "Failed to write primary list info\n");
        fclose(file);
        return -1;
    }
    
    // 写入主键索引节点
    if (table->primary_list && table->primary_list->header && table->primary_list->header->forward) {
        SkipListNode* current = table->primary_list->header->forward[0];
        uint32_t node_count = 0;
        
        while (current) {
            fprintf(stderr, "Writing node %u: data_length=%u\n", 
                    node_count++, current->data_length);
            
            if (fwrite(&current->data_length, sizeof(current->data_length), 1, file) != 1) {
                fprintf(stderr, "Failed to write node data length\n");
                fclose(file);
                return -1;
            }
            
            if (current->data && current->data_length > 0) {
                if (fwrite(current->data, current->data_length, 1, file) != 1) {
                    fprintf(stderr, "Failed to write node data\n");
                    fclose(file);
                    return -1;
                }
            }
            current = current->forward[0];
        }
    }
    
    // 确保数据写入磁盘
    fflush(file);
    fsync(fileno(file));
    fclose(file);
    
    fprintf(stderr, "Table saved successfully to %s\n", path);
    return 0;
}

// 从文件加载表结构
SkipTable* skiptable_load(const char* path) {
    fprintf(stderr, "skiptable_load(path=%s)\n", path);
    
    if (!path) return nullptr;
    
    fprintf(stderr, "Opening file for reading: %s\n", path);
    FILE* file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open file: %s, errno: %d\n", path, errno);
        return nullptr;
    }
    
    // 读取文件头
    uint32_t magic, version, name_len;
    if (fread(&magic, sizeof(magic), 1, file) != 1 ||
        fread(&version, sizeof(version), 1, file) != 1 ||
        fread(&name_len, sizeof(name_len), 1, file) != 1) {
        fprintf(stderr, "Failed to read file header\n");
        fclose(file);
        return nullptr;
    }
    
    fprintf(stderr, "Read file header: magic=%u, version=%u, name_len=%u\n", 
            magic, version, name_len);
    
    if (magic != 0x534B4C54) {  // "SKLT"
        fprintf(stderr, "Invalid file format\n");
        fclose(file);
        return nullptr;
    }
    
    // 读取表名
    char* table_name = nullptr;
    if (name_len > 0) {
        table_name = (char*)malloc(name_len + 1);
        if (!table_name) {
            fprintf(stderr, "Failed to allocate memory for table name\n");
            fclose(file);
            return nullptr;
        }
        
        if (fread(table_name, name_len, 1, file) != 1) {
            fprintf(stderr, "Failed to read table name\n");
            free(table_name);
            fclose(file);
            return nullptr;
        }
        
        table_name[name_len] = '\0';
        fprintf(stderr, "Read table name: %s\n", table_name);
    }
    
    // 创建表结构
    SkipTable* table = skiptable_create(table_name ? table_name : path);
    if (!table) {
        fprintf(stderr, "Failed to create table structure\n");
        if (table_name) free(table_name);
        fclose(file);
        return nullptr;
    }
    
    if (table_name) free(table_name);
    
    // 读取表信息
    if (fread(&table->row_count, sizeof(table->row_count), 1, file) != 1 ||
        fread(&table->data_size, sizeof(table->data_size), 1, file) != 1) {
        fprintf(stderr, "Failed to read table info\n");
        skiptable_destroy(table);
        fclose(file);
        return nullptr;
    }
    
    fprintf(stderr, "Read table info: row_count=%u, data_size=%lu\n", 
            table->row_count, table->data_size);
    
    // 读取主键索引信息
    uint32_t primary_list_size, primary_list_level;
    if (fread(&primary_list_size, sizeof(primary_list_size), 1, file) != 1 ||
        fread(&primary_list_level, sizeof(primary_list_level), 1, file) != 1) {
        fprintf(stderr, "Failed to read primary list info\n");
        skiptable_destroy(table);
        fclose(file);
        return nullptr;
    }
    
    fprintf(stderr, "Read primary list info: size=%u, level=%u\n", 
            primary_list_size, primary_list_level);
    
    // 读取主键索引节点
    for (uint32_t i = 0; i < primary_list_size; i++) {
        uint32_t data_length;
        if (fread(&data_length, sizeof(data_length), 1, file) != 1) {
            fprintf(stderr, "Failed to read node data length for node %u\n", i);
            skiptable_destroy(table);
            fclose(file);
            return nullptr;
        }
        
        fprintf(stderr, "Read node %u: data_length=%u\n", i, data_length);
        
        uchar* data = (uchar*)malloc(data_length);
        if (!data) {
            fprintf(stderr, "Failed to allocate memory for node data\n");
            skiptable_destroy(table);
            fclose(file);
            return nullptr;
        }
        
        if (fread(data, data_length, 1, file) != 1) {
            fprintf(stderr, "Failed to read node data\n");
            free(data);
            skiptable_destroy(table);
            fclose(file);
            return nullptr;
        }
        
        // 插入到主键索引
        if (skiplist_insert(table->primary_list, data, data_length) != 0) {
            fprintf(stderr, "Failed to insert node data\n");
            free(data);
            skiptable_destroy(table);
            fclose(file);
            return nullptr;
        }
    }
    
    fclose(file);
    
    // 恢复日志
    fprintf(stderr, "Recovering log for table\n");
    log_recover(table);
    
    fprintf(stderr, "Table loaded successfully from %s\n", path);
    return table;
}

// 写入日志
int log_write(SkipTable* table, uint32_t op_type, const uchar* data, uint32_t data_len) {
    if (!table || !table->log_file_path || !data || data_len == 0) {
        return -1;
    }
    
    // 打开日志文件
    int fd = open(table->log_file_path, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd < 0) {
        fprintf(stderr, "Failed to open log file: %s, errno: %d\n", table->log_file_path, errno);
        return -1;
    }
    
    // 写入日志头
    uint32_t log_size = sizeof(LogRecord) + data_len;
    LogRecord* log_record = (LogRecord*)malloc(log_size);
    if (!log_record) {
        close(fd);
        return -1;
    }
    
    log_record->op_type = op_type;
    log_record->data_length = data_len;
    memcpy(log_record->data, data, data_len);
    
    // 写入日志记录
    ssize_t written = write(fd, log_record, log_size);
    free(log_record);
    
    if (written != log_size) {
        fprintf(stderr, "Failed to write log record, written=%zd, expected=%u\n", written, log_size);
        close(fd);
        return -1;
    }
    
    // 刷新到磁盘
    fsync(fd);
    close(fd);
    
    return 0;
}

// 恢复日志
int log_recover(SkipTable* table) {
    if (!table || !table->log_file_path) {
        return -1;
    }
    
    // 打开日志文件
    int fd = open(table->log_file_path, O_RDONLY);
    if (fd < 0) {
        if (errno == ENOENT) {
            // 日志文件不存在，不需要恢复
            return 0;
        }
        fprintf(stderr, "Failed to open log file: %s, errno: %d\n", table->log_file_path, errno);
        return -1;
    }
    
    // 读取日志记录
    LogRecord log_header;
    ssize_t bytes_read;
    
    while ((bytes_read = read(fd, &log_header, sizeof(LogRecord))) == sizeof(LogRecord)) {
        // 读取日志数据
        uchar* data = (uchar*)malloc(log_header.data_length);
        if (!data) {
            close(fd);
            return -1;
        }
        
        if (read(fd, data, log_header.data_length) != log_header.data_length) {
            fprintf(stderr, "Failed to read log data\n");
            free(data);
            close(fd);
            return -1;
        }
        
        // 根据操作类型执行相应的操作
        switch (log_header.op_type) {
            case LOG_OP_INSERT:
                skiplist_insert(table->primary_list, data, log_header.data_length);
                table->row_count++;
                table->data_size += log_header.data_length;
                break;
                
            case LOG_OP_DELETE:
                skiplist_delete(table->primary_list, data, log_header.data_length);
                if (table->row_count > 0) table->row_count--;
                if (table->data_size >= log_header.data_length) {
                    table->data_size -= log_header.data_length;
                }
                free(data);  // 删除操作需要释放数据
                break;
                
            case LOG_OP_UPDATE:
                // 更新操作需要额外的信息，这里简化处理
                free(data);
                break;
                
            default:
                fprintf(stderr, "Unknown log operation type: %u\n", log_header.op_type);
                free(data);
                break;
        }
    }
    
    close(fd);
    
    // 清空日志文件
    fd = open(table->log_file_path, O_WRONLY | O_TRUNC);
    if (fd >= 0) {
        close(fd);
    }
    
    return 0;
}