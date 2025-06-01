#include "ha_skiplist.h"
#include "sql/sql_class.h"
#include "sql/sql_table.h"
#include <errno.h>

// 创建处理器实例
static handler* skiplist_create_handler(handlerton* hton, TABLE_SHARE* table, bool partitioned, MEM_ROOT* mem_root) {
    fprintf(stderr, "skiplist_create_handler(hton=%p, table=%p, partitioned=%d, mem_root=%p)\n", 
            hton, table, partitioned, mem_root);
    return new (mem_root) ha_skiplist(hton, table);
}

// 初始化存储引擎
static int skiplist_init(void* p) {
    fprintf(stderr, "skiplist_init(p=%p)\n", p);
    
    handlerton* skiplist_hton = (handlerton*)p;
    skiplist_hton->state = SHOW_OPTION_YES;
    skiplist_hton->db_type = DB_TYPE_UNKNOWN;
    skiplist_hton->create = skiplist_create_handler;
    skiplist_hton->flags = HTON_CAN_RECREATE;
    
    return 0;
}

// 注册存储引擎
static struct st_mysql_storage_engine skiplist_storage_engine = {
    MYSQL_HANDLERTON_INTERFACE_VERSION
};

// 导出存储引擎
mysql_declare_plugin(skiplist) {
    MYSQL_STORAGE_ENGINE_PLUGIN,
    &skiplist_storage_engine,
    "SKIPLIST",
    "Author Name",
    "SkipList Storage Engine",
    PLUGIN_LICENSE_GPL,
    skiplist_init,
    nullptr,
    0,
    0,
    0,
    0,
    0,
} mysql_declare_plugin_end;

// 构造函数
ha_skiplist::ha_skiplist(handlerton* hton, TABLE_SHARE* table_arg)
    : handler(hton, table_arg), skip_table(nullptr), current_position(nullptr) {
    fprintf(stderr, "ha_skiplist::ha_skiplist(hton=%p, table_arg=%p)\n", hton, table_arg);
    thr_lock_init(&lock);
}

// 析构函数
ha_skiplist::~ha_skiplist() {
    fprintf(stderr, "ha_skiplist::~ha_skiplist()\n");
    thr_lock_delete(&lock);
}

// 表标志
ulonglong ha_skiplist::table_flags() const {
    fprintf(stderr, "ha_skiplist::table_flags()\n");
    return HA_NO_TRANSACTIONS | HA_NO_AUTO_INCREMENT;
}

// 索引标志
ulong ha_skiplist::index_flags(uint, uint, bool) const {
    return 0;
}

// 创建表结构
SkipTable* ha_skiplist::create_table_structure(const char* name) {
    SkipTable* table = skiptable_create(name);
    return table;
}

// 释放表结构
void ha_skiplist::free_table_structure(SkipTable* table_ptr) {
    if (table_ptr) {
        skiptable_destroy(table_ptr);
    }
}

// 创建表
int ha_skiplist::create(const char *name, TABLE *form, HA_CREATE_INFO *create_info,
                       dd::Table *table_def) {
    DBUG_TRACE;
    fprintf(stderr, "ha_skiplist::create(name=%s, form=%p, create_info=%p, table_def=%p)\n", 
            name, form, create_info, table_def);
    
    // 创建表结构
    SkipTable* table = create_table_structure(name);
    if (!table) {
        return HA_ERR_OUT_OF_MEM;
    }
    
    // 保存表结构到文件
    int result = skiptable_save(table, table->data_file_path);
    
    // 释放表结构
    free_table_structure(table);
    
    return result ? HA_ERR_CRASHED : 0;
}

// 打开表
int ha_skiplist::open(const char *name, int mode, uint test_if_locked,
                     const dd::Table *table_def) {
    DBUG_TRACE;
    fprintf(stderr, "ha_skiplist::open(name=%s, mode=%d, test_if_locked=%u, table_def=%p)\n", 
            name, mode, test_if_locked, table_def);
    
    // 从文件加载表结构
    char data_file_path[MAX_PATH_LEN];
    snprintf(data_file_path, MAX_PATH_LEN, "%s.skl", name);
    
    skip_table = skiptable_load(data_file_path);
    if (!skip_table) {
        // 如果加载失败，创建新的表结构
        skip_table = create_table_structure(name);
        if (!skip_table) {
            return HA_ERR_CRASHED;
        }
    }
    
    // 初始化锁数据
    thr_lock_data_init(&lock, &lock_data, nullptr);
    
    return 0;
}

// 关闭表
int ha_skiplist::close(void) {
    DBUG_TRACE;
    fprintf(stderr, "ha_skiplist::close()\n");
    
    // 保存表结构到文件
    if (skip_table && skip_table->data_file_path) {
        fprintf(stderr, "Saving table to %s\n", skip_table->data_file_path);
        int result = skiptable_save(skip_table, skip_table->data_file_path);
        if (result) {
            fprintf(stderr, "Failed to save table to %s\n", skip_table->data_file_path);
        } else {
            fprintf(stderr, "Table saved successfully to %s\n", skip_table->data_file_path);
        }
        free_table_structure(skip_table);
        skip_table = nullptr;
    }
    
    return 0;
}

// 初始化表扫描
int ha_skiplist::rnd_init(bool scan) {
    DBUG_TRACE;
    fprintf(stderr, "ha_skiplist::rnd_init(scan=%d)\n", scan);
    
    // 重置当前位置
    current_position = nullptr;
    
    return 0;
}

// 结束表扫描
int ha_skiplist::rnd_end() {
    DBUG_TRACE;
    fprintf(stderr, "ha_skiplist::rnd_end()\n");
    
    // 重置当前位置
    current_position = nullptr;
    
    return 0;
}

// 获取下一行
int ha_skiplist::rnd_next(uchar *buf) {
    DBUG_TRACE;
    fprintf(stderr, "ha_skiplist::rnd_next(buf=%p)\n", buf);
    
    if (!skip_table || !skip_table->primary_list) {
        return HA_ERR_END_OF_FILE;
    }
    
    SkipListNode* node;
    
    if (!current_position) {
        // 第一次调用，获取第一个节点
        node = skip_table->primary_list->header->forward[0];
    } else {
        // 获取下一个节点，检查当前位置是否有效
        if (current_position->forward) {
            node = current_position->forward[0];
        } else {
            // 当前位置无效，重新从头开始
            node = skip_table->primary_list->header->forward[0];
        }
    }
    
    if (!node) {
        return HA_ERR_END_OF_FILE;
    }
    
    // 更新当前位置
    current_position = node;
    
    // 复制数据到缓冲区
    if (node->data && node->data_length > 0) {
        memcpy(buf, node->data, node->data_length);
    } else {
        return HA_ERR_CRASHED;
    }
    
    return 0;
}

// 根据位置获取行
int ha_skiplist::rnd_pos(uchar *buf, uchar *pos) {
    DBUG_TRACE;
    fprintf(stderr, "ha_skiplist::rnd_pos(buf=%p, pos=%p)\n", buf, pos);
    
    // pos 包含指向节点的指针
    SkipListNode* node = *reinterpret_cast<SkipListNode**>(pos);
    
    if (!node) {
        return HA_ERR_KEY_NOT_FOUND;
    }
    
    // 复制数据到缓冲区
    memcpy(buf, node->data, node->data_length);
    
    return 0;
}

// 记录当前位置
void ha_skiplist::position(const uchar *record) {
    DBUG_TRACE;
    fprintf(stderr, "ha_skiplist::position(record=%p)\n", record);
    
    // 将当前节点指针存储到ref变量中
    memcpy(ref, &current_position, sizeof(SkipListNode*));
}

// 获取表信息
int ha_skiplist::info(uint flag) {
    DBUG_TRACE;
    fprintf(stderr, "ha_skiplist::info(flag=%u)\n", flag);
    
    if (!skip_table) {
        return HA_ERR_CRASHED;
    }
    
    // 设置表统计信息
    stats.records = skip_table->row_count;
    stats.data_file_length = skip_table->data_size;
    stats.index_file_length = 0;
    stats.mean_rec_length = skip_table->row_count ? 
                           (ulong)(skip_table->data_size / skip_table->row_count) : 0;
    stats.delete_length = 0;
    stats.create_time = 0;
    
    return 0;
}

// 写入行
int ha_skiplist::write_row(uchar *buf) {
    DBUG_TRACE;
    fprintf(stderr, "ha_skiplist::write_row(buf=%p)\n", buf);
    
    if (!skip_table) {
        return HA_ERR_CRASHED;
    }
    
    // 复制数据
    uchar* data_copy = (uchar*)malloc(table->s->rec_buff_length);
    if (!data_copy) {
        return HA_ERR_OUT_OF_MEM;
    }
    memcpy(data_copy, buf, table->s->rec_buff_length);
    
    // 写入日志
    log_write(skip_table, LOG_OP_INSERT, data_copy, table->s->rec_buff_length);
    
    // 插入到主列表
    int result = skiplist_insert(skip_table->primary_list, data_copy, table->s->rec_buff_length);
    if (result != 0) {
        free(data_copy);
        return result == 1 ? HA_ERR_FOUND_DUPP_KEY : HA_ERR_OUT_OF_MEM;
    }
    
    // 更新表统计信息
    skip_table->row_count++;
    skip_table->data_size += table->s->rec_buff_length;
    
    // 每次写入后保存表结构
    if (skip_table->data_file_path) {
        fprintf(stderr, "Saving table after write_row to %s\n", skip_table->data_file_path);
        int save_result = skiptable_save(skip_table, skip_table->data_file_path);
        if (save_result) {
            fprintf(stderr, "Failed to save table after write_row\n");
        }
    }
    
    return 0;
}

// 更新行
int ha_skiplist::update_row(const uchar *old_data, uchar *new_data) {
    DBUG_TRACE;
    fprintf(stderr, "ha_skiplist::update_row(old_data=%p, new_data=%p)\n", old_data, new_data);
    
    if (!skip_table) {
        return HA_ERR_CRASHED;
    }
    
    // 写入日志 - 更新操作可以看作是删除+插入
    log_write(skip_table, LOG_OP_DELETE, old_data, table->s->rec_buff_length);
    log_write(skip_table, LOG_OP_INSERT, new_data, table->s->rec_buff_length);
    
    // 简化版本，先删除旧行，再插入新行
    // 注意：这里不调用delete_row和write_row，因为它们会各自保存表结构
    // 我们只想在所有操作完成后保存一次
    
    // 如果当前位置指向要删除的行，先重置当前位置
    current_position = nullptr;
    
    // 从主列表中删除旧数据
    int result = skiplist_delete(skip_table->primary_list, old_data, table->s->rec_buff_length);
    if (result != 0) {
        return result == 1 ? HA_ERR_KEY_NOT_FOUND : HA_ERR_CRASHED;
    }
    
    // 更新表统计信息
    skip_table->row_count--;
    skip_table->data_size -= table->s->rec_buff_length;
    
    // 复制新数据
    uchar* data_copy = (uchar*)malloc(table->s->rec_buff_length);
    if (!data_copy) {
        return HA_ERR_OUT_OF_MEM;
    }
    memcpy(data_copy, new_data, table->s->rec_buff_length);
    
    // 插入新数据到主列表
    result = skiplist_insert(skip_table->primary_list, data_copy, table->s->rec_buff_length);
    if (result != 0) {
        free(data_copy);
        return result == 1 ? HA_ERR_FOUND_DUPP_KEY : HA_ERR_OUT_OF_MEM;
    }
    
    // 更新表统计信息
    skip_table->row_count++;
    skip_table->data_size += table->s->rec_buff_length;
    
    // 更新完成后保存表结构
    if (skip_table->data_file_path) {
        fprintf(stderr, "Saving table after update_row to %s\n", skip_table->data_file_path);
        int save_result = skiptable_save(skip_table, skip_table->data_file_path);
        if (save_result) {
            fprintf(stderr, "Failed to save table after update_row\n");
        }
    }
    
    return 0;
}

// 删除行
int ha_skiplist::delete_row(const uchar *buf) {
    DBUG_TRACE;
    fprintf(stderr, "ha_skiplist::delete_row(buf=%p)\n", buf);
    
    if (!skip_table) {
        return HA_ERR_CRASHED;
    }
    
    // 写入日志
    log_write(skip_table, LOG_OP_DELETE, buf, table->s->rec_buff_length);
    
    // 如果当前位置指向要删除的行，先重置当前位置
    // 这样可以避免在删除后访问无效的节点
    current_position = nullptr;
    
    // 从主列表中删除
    int result = skiplist_delete(skip_table->primary_list, buf, table->s->rec_buff_length);
    if (result != 0) {
        return result == 1 ? HA_ERR_KEY_NOT_FOUND : HA_ERR_CRASHED;
    }
    
    // 更新表统计信息
    skip_table->row_count--;
    skip_table->data_size -= table->s->rec_buff_length;
    
    // 每次删除后保存表结构
    if (skip_table->data_file_path) {
        fprintf(stderr, "Saving table after delete_row to %s\n", skip_table->data_file_path);
        int save_result = skiptable_save(skip_table, skip_table->data_file_path);
        if (save_result) {
            fprintf(stderr, "Failed to save table after delete_row\n");
        }
    }
    
    return 0;
}

// 存储锁
THR_LOCK_DATA **ha_skiplist::store_lock(THD*, THR_LOCK_DATA **to,
                                       enum thr_lock_type lock_type) {
    if (lock_type != TL_IGNORE && lock_data.type == TL_UNLOCK) {
        lock_data.type = lock_type;
    }
    *to++ = &lock_data;
    return to;
}