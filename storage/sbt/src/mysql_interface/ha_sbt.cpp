#include "../../include/ha_sbt.h"
#include "../../include/sbt_tree.h"

// MySQL plugin includes
#include "sql/sql_plugin.h"
#include "sql/sql_class.h"
#include "sql/field.h"
#include "sql/table.h"
#include "thr_lock.h"
#include <fstream>
#include <unordered_map>

namespace sbt {

// 存储引擎handlerton
static handlerton *sbt_hton;

// 全局SBT树存储 - 用表名作为键
static std::unordered_map<std::string, std::unique_ptr<SBTTree<KeyType, ValueType>>> global_sbt_trees;

// 简单的数据持久化函数
void save_sbt_data_to_file(const std::string& filename, SBTTree<KeyType, ValueType>* tree) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        fprintf(stderr, "[SBT DEBUG] Failed to open file for writing: %s\n", filename.c_str());
        return;
    }
    
    // 写入树的大小
    size_t tree_size = tree->size();
    file.write(reinterpret_cast<const char*>(&tree_size), sizeof(tree_size));
    
    // 遍历树并写入所有键值对
    auto it = tree->begin();
    size_t count = 0;
    while (it.has_next()) {
        auto pair = it.next();
        
        // 写入键长度和键数据
        size_t key_len = pair.first.size();
        file.write(reinterpret_cast<const char*>(&key_len), sizeof(key_len));
        file.write(pair.first.data(), key_len);
        
        // 写入值长度和值数据
        size_t value_len = pair.second.size();
        file.write(reinterpret_cast<const char*>(&value_len), sizeof(value_len));
        file.write(reinterpret_cast<const char*>(pair.second.data()), value_len);
        
        count++;
    }
    
    file.close();
    fprintf(stderr, "[SBT DEBUG] Saved %zu records to file: %s\n", count, filename.c_str());
}

void load_sbt_data_from_file(const std::string& filename, SBTTree<KeyType, ValueType>* tree) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        fprintf(stderr, "[SBT DEBUG] File not found or cannot open: %s\n", filename.c_str());
        return;
    }
    
    // 读取树的大小
    size_t expected_size;
    file.read(reinterpret_cast<char*>(&expected_size), sizeof(expected_size));
    
    fprintf(stderr, "[SBT DEBUG] Loading %zu records from file: %s\n", expected_size, filename.c_str());
    
    // 读取所有键值对
    size_t loaded_count = 0;
    while (file.good() && loaded_count < expected_size) {
        // 读取键
        size_t key_len;
        if (!file.read(reinterpret_cast<char*>(&key_len), sizeof(key_len))) break;
        
        std::string key(key_len, '\0');
        if (!file.read(&key[0], key_len)) break;
        
        // 读取值
        size_t value_len;
        if (!file.read(reinterpret_cast<char*>(&value_len), sizeof(value_len))) break;
        
        ValueType value(value_len);
        if (!file.read(reinterpret_cast<char*>(value.data()), value_len)) break;
        
        // 插入到树中
        tree->insert(key, value);
        loaded_count++;
    }
    
    file.close();
    fprintf(stderr, "[SBT DEBUG] Loaded %zu records from file\n", loaded_count);
}

// 构造函数
ha_sbt::ha_sbt(handlerton *hton, TABLE_SHARE *table_arg)
    : handler(hton, table_arg),
      sbt_tree_(nullptr),
      iterator_initialized_(false) {
    // 初始化锁结构
    thr_lock_init(&sbt_lock_);
}

// 析构函数
ha_sbt::~ha_sbt() {
    if (sbt_tree_) {
        close();
    }
}

// 打开表 - 修复方法签名
int ha_sbt::open(const char *name, int mode, uint test_if_locked, 
                 const dd::Table *table_def) {
    DBUG_ENTER("ha_sbt::open");
    
    table_name_ = name;
    data_file_path_ = std::string(name) + ".sbt";
    
    fprintf(stderr, "[SBT DEBUG] Opening table: %s\n", name);
    
    try {
        // 从全局存储中获取或创建SBT树
        auto it = global_sbt_trees.find(table_name_);
        if (it == global_sbt_trees.end()) {
            // 创建新的SBT树
            global_sbt_trees[table_name_] = std::make_unique<SBTTree<KeyType, ValueType>>();
            fprintf(stderr, "[SBT DEBUG] Created new SBT tree for table: %s\n", table_name_.c_str());
            
            // 尝试从磁盘加载数据
            load_sbt_data_from_file(data_file_path_, global_sbt_trees[table_name_].get());
        } else {
            fprintf(stderr, "[SBT DEBUG] Found existing SBT tree for table: %s, size: %zu\n", 
                    table_name_.c_str(), it->second->size());
        }
        
        // 获取SBT树引用
        sbt_tree_ = global_sbt_trees[table_name_].get();
        
        // 初始化锁数据
        thr_lock_data_init(&sbt_lock_, &lock_data_, nullptr);
        
        DBUG_RETURN(0);
        
    } catch (const std::exception& e) {
        fprintf(stderr, "[SBT DEBUG] Exception in open: %s\n", e.what());
        DBUG_RETURN(HA_ERR_GENERIC);
    }
}

// 关闭表
int ha_sbt::close(void) {
    DBUG_ENTER("ha_sbt::close");
    
    fprintf(stderr, "[SBT DEBUG] Closing table: %s\n", table_name_.c_str());
    
    // 不删除SBT树，保持在全局存储中
    sbt_tree_ = nullptr;
    
    DBUG_RETURN(0);
}

// 从记录生成键
std::string ha_sbt::make_key_from_record() {
    std::string key;
    
    // 简化版：使用第一个字段作为键
    if (table->s->fields > 0) {
        Field *field = table->field[0];
        
        if (field->is_null()) {
            key = "__NULL__";
            fprintf(stderr, "[SBT DEBUG] Generated key: %s (NULL field)\n", key.c_str());
        } else {
            switch (field->type()) {
                case MYSQL_TYPE_TINY:
                case MYSQL_TYPE_SHORT:
                case MYSQL_TYPE_LONG:
                case MYSQL_TYPE_LONGLONG: {
                    longlong val = field->val_int();
                    key = std::to_string(val);
                    fprintf(stderr, "[SBT DEBUG] Generated key: %s (INT: %lld)\n", key.c_str(), val);
                    break;
                }
                case MYSQL_TYPE_FLOAT:
                case MYSQL_TYPE_DOUBLE: {
                    double val = field->val_real();
                    key = std::to_string(val);
                    fprintf(stderr, "[SBT DEBUG] Generated key: %s (DOUBLE: %f)\n", key.c_str(), val);
                    break;
                }
                case MYSQL_TYPE_STRING:
                case MYSQL_TYPE_VAR_STRING:
                case MYSQL_TYPE_VARCHAR: {
                    String str;
                    field->val_str(&str);
                    key = std::string(str.ptr(), str.length());
                    fprintf(stderr, "[SBT DEBUG] Generated key: %s (STRING)\n", key.c_str());
                    break;
                }
                default: {
                    String str;
                    field->val_str(&str);
                    key = std::string(str.ptr(), str.length());
                    fprintf(stderr, "[SBT DEBUG] Generated key: %s (DEFAULT)\n", key.c_str());
                    break;
                }
            }
        }
    } else {
        key = "no_fields";
        fprintf(stderr, "[SBT DEBUG] No fields found, using default key: %s\n", key.c_str());
    }
    
    return key;
}

// 打包行数据
int ha_sbt::pack_row(uchar **row, size_t *length) {
    // 计算记录长度
    size_t record_length = table->s->reclength;
    
    // 分配内存
    *row = new uchar[record_length];
    if (!*row) {
        return HA_ERR_OUT_OF_MEM;
    }
    
    // 复制记录数据
    memcpy(*row, table->record[0], record_length);
    *length = record_length;
    
    return 0;
}

// 解包行数据
int ha_sbt::unpack_row(const uchar *row, size_t length) {
    if (length != table->s->reclength) {
        return HA_ERR_GENERIC;
    }
    
    // 复制数据到表记录缓冲区
    memcpy(table->record[0], row, length);
    
    return 0;
}

// 插入行
int ha_sbt::write_row(uchar *buf) {
    DBUG_ENTER("ha_sbt::write_row");
    
    fprintf(stderr, "[SBT DEBUG] write_row called\n");
    
    try {
        // 从记录中提取键
        std::string key = make_key_from_record();
        fprintf(stderr, "[SBT DEBUG] Extracted key: %s\n", key.c_str());
        
        // 打包行数据
        uchar *packed_row;
        size_t row_length;
        int result = pack_row(&packed_row, &row_length);
        if (result != 0) {
            fprintf(stderr, "[SBT DEBUG] pack_row failed with error: %d\n", result);
            DBUG_RETURN(result);
        }
        
        fprintf(stderr, "[SBT DEBUG] Packed row length: %zu\n", row_length);
        
        ValueType value(packed_row, packed_row + row_length);
        
        // 插入到SBT树
        SBTError error = sbt_tree_->insert(key, value);
        fprintf(stderr, "[SBT DEBUG] SBT insert result: %d\n", (int)error);
        
        if (error == SBTError::SUCCESS) {
            fprintf(stderr, "[SBT DEBUG] Tree size after insert: %zu\n", sbt_tree_->size());
            
            // 保存数据到磁盘
            save_sbt_data_to_file(data_file_path_, sbt_tree_);
        }
        
        delete[] packed_row;
        
        DBUG_RETURN(sbt_error_to_mysql_error(error));
        
    } catch (const std::exception& e) {
        fprintf(stderr, "[SBT DEBUG] Exception in write_row: %s\n", e.what());
        DBUG_RETURN(HA_ERR_GENERIC);
    }
}

// 更新行
int ha_sbt::update_row(const uchar *old_data, uchar *new_data) {
    DBUG_ENTER("ha_sbt::update_row");
    
    try {
        // 提取键（假设键不变）
        std::string key = make_key_from_record();
        
        // 打包新行数据
        uchar *packed_row;
        size_t row_length;
        int result = pack_row(&packed_row, &row_length);
        if (result != 0) {
            DBUG_RETURN(result);
        }
        
        ValueType new_value(packed_row, packed_row + row_length);
        
        // 更新SBT树
        SBTError error = sbt_tree_->update(key, new_value);
        
        delete[] packed_row;
        
        DBUG_RETURN(sbt_error_to_mysql_error(error));
        
    } catch (const std::exception& e) {
        DBUG_RETURN(HA_ERR_GENERIC);
    }
}

// 删除行
int ha_sbt::delete_row(const uchar *buf) {
    DBUG_ENTER("ha_sbt::delete_row");
    
    try {
        std::string key = make_key_from_record();
        SBTError error = sbt_tree_->remove(key);
        
        DBUG_RETURN(sbt_error_to_mysql_error(error));
        
    } catch (const std::exception& e) {
        DBUG_RETURN(HA_ERR_GENERIC);
    }
}

// 初始化随机扫描
int ha_sbt::rnd_init(bool scan) {
    DBUG_ENTER("ha_sbt::rnd_init");
    
    fprintf(stderr, "[SBT DEBUG] rnd_init called with scan=%d\n", scan);
    fprintf(stderr, "[SBT DEBUG] Tree size: %zu\n", sbt_tree_ ? sbt_tree_->size() : 0);
    
    if (scan && sbt_tree_) {
        current_iterator_ = sbt_tree_->begin();
        iterator_initialized_ = true;
        fprintf(stderr, "[SBT DEBUG] Iterator initialized\n");
    }
    
    DBUG_RETURN(0);
}

// 随机扫描下一行
int ha_sbt::rnd_next(uchar *buf) {
    DBUG_ENTER("ha_sbt::rnd_next");
    
    fprintf(stderr, "[SBT DEBUG] rnd_next called\n");
    fprintf(stderr, "[SBT DEBUG] Iterator initialized: %d\n", iterator_initialized_);
    
    if (!iterator_initialized_) {
        fprintf(stderr, "[SBT DEBUG] Iterator not initialized\n");
        DBUG_RETURN(HA_ERR_END_OF_FILE);
    }
    
    if (!current_iterator_.has_next()) {
        fprintf(stderr, "[SBT DEBUG] Iterator has no more data\n");
        DBUG_RETURN(HA_ERR_END_OF_FILE);
    }
    
    try {
        auto pair = current_iterator_.next();
        const ValueType& value = pair.second;
        
        fprintf(stderr, "[SBT DEBUG] Retrieved key: %s, value size: %zu\n", 
                pair.first.c_str(), value.size());
        
        // 解包数据到记录缓冲区
        int result = unpack_row(value.data(), value.size());
        if (result != 0) {
            fprintf(stderr, "[SBT DEBUG] unpack_row failed with error: %d\n", result);
            DBUG_RETURN(result);
        }
        
        fprintf(stderr, "[SBT DEBUG] Row unpacked successfully\n");
        DBUG_RETURN(0);
        
    } catch (const std::exception& e) {
        fprintf(stderr, "[SBT DEBUG] Exception in rnd_next: %s\n", e.what());
        DBUG_RETURN(HA_ERR_GENERIC);
    }
}

// 结束随机扫描
int ha_sbt::rnd_end() {
    DBUG_ENTER("ha_sbt::rnd_end");
    iterator_initialized_ = false;
    DBUG_RETURN(0);
}

// 获取记录位置
void ha_sbt::position(const uchar *record) {
    DBUG_ENTER("ha_sbt::position");
    
    // 简化版：使用键作为位置
    current_key_ = make_key_from_record();
    memcpy(ref, current_key_.c_str(), std::min(current_key_.size(), (size_t)ref_length));
    
    DBUG_VOID_RETURN;
}

// 根据位置读取记录
int ha_sbt::rnd_pos(uchar *buf, uchar *pos) {
    DBUG_ENTER("ha_sbt::rnd_pos");
    
    try {
        std::string key(reinterpret_cast<char*>(pos), ref_length);
        ValueType value;
        
        SBTError error = sbt_tree_->find(key, value);
        if (error != SBTError::SUCCESS) {
            DBUG_RETURN(sbt_error_to_mysql_error(error));
        }
        
        int result = unpack_row(value.data(), value.size());
        DBUG_RETURN(result);
        
    } catch (const std::exception& e) {
        DBUG_RETURN(HA_ERR_GENERIC);
    }
}

// 获取表信息
int ha_sbt::info(uint flag) {
    DBUG_ENTER("ha_sbt::info");
    
    if (sbt_tree_) {
        size_t tree_size = sbt_tree_->size();
        stats.records = tree_size;
        stats.deleted = 0;
        stats.data_file_length = stats.records * table->s->reclength;
        stats.index_file_length = 0;
        stats.mean_rec_length = table->s->reclength;
        
        fprintf(stderr, "[SBT DEBUG] info() called - Tree size: %zu, Records: %llu\n", 
                tree_size, stats.records);
    } else {
        fprintf(stderr, "[SBT DEBUG] info() called - sbt_tree_ is null\n");
    }
    
    DBUG_RETURN(0);
}

// 存储引擎特性标志
ulonglong ha_sbt::table_flags() const {
    return (HA_BINLOG_STMT_CAPABLE |
            HA_BINLOG_ROW_CAPABLE |
            HA_CAN_SQL_HANDLER);
}

// 索引特性标志
ulong ha_sbt::index_flags(uint inx, uint part, bool all_parts) const {
    return 0; // 暂不支持索引
}

// 最大支持的记录长度
uint ha_sbt::max_supported_record_length() const {
    return MAX_VALUE_SIZE;
}

// 最大支持的键数量
uint ha_sbt::max_supported_keys() const {
    return 0; // 暂不支持索引
}

// 最大支持的键部分数量
uint ha_sbt::max_supported_key_parts() const {
    return 0; // 暂不支持索引
}

// 最大支持的键长度
uint ha_sbt::max_supported_key_length() const {
    return MAX_KEY_SIZE;
}

// 最大支持的键部分长度
uint ha_sbt::max_supported_key_part_length(HA_CREATE_INFO *create_info) const {
    return MAX_KEY_SIZE;
}

// 锁管理 - 实现纯虚函数
THR_LOCK_DATA **ha_sbt::store_lock(THD *thd, THR_LOCK_DATA **to,
                                   enum thr_lock_type lock_type) {
    if (lock_type != TL_IGNORE && lock_data_.type == TL_UNLOCK) {
        lock_data_.type = lock_type;
    }
    *to++ = &lock_data_;
    return to;
}

// 创建表
int ha_sbt::create(const char *name, TABLE *form, HA_CREATE_INFO *create_info,
                   dd::Table *table_def) {
    DBUG_ENTER("ha_sbt::create");
    
    // 创建空的数据文件标记
    std::string data_file = std::string(name) + ".sbt";
    std::ofstream file(data_file.c_str(), std::ios::binary);
    if (!file.is_open()) {
        DBUG_RETURN(HA_ERR_GENERIC);
    }
    file.close();
    
    DBUG_RETURN(0);
}

// 删除表
int ha_sbt::delete_table(const char *name, const dd::Table *table_def) {
    DBUG_ENTER("ha_sbt::delete_table");
    
    std::string data_file = std::string(name) + ".sbt";
    
    if (std::remove(data_file.c_str()) != 0) {
        // 文件可能不存在，这是正常的
    }
    
    DBUG_RETURN(0);
}

// 外部锁
int ha_sbt::external_lock(THD *thd, int lock_type) {
    DBUG_ENTER("ha_sbt::external_lock");
    DBUG_RETURN(0); // 简化版不处理锁
}

// 错误码转换
int ha_sbt::sbt_error_to_mysql_error(SBTError error) {
    switch (error) {
        case SBTError::SUCCESS:
            return 0;
        case SBTError::KEY_NOT_FOUND:
            return HA_ERR_KEY_NOT_FOUND;
        case SBTError::DUPLICATE_KEY:
            return HA_ERR_FOUND_DUPP_KEY;
        case SBTError::OUT_OF_MEMORY:
            return HA_ERR_OUT_OF_MEM;
        case SBTError::IO_ERROR:
            return HA_ERR_GENERIC;
        default:
            return HA_ERR_GENERIC;
    }
}

} // namespace sbt

// 存储引擎初始化函数
static int sbt_init_func(void *p) {
    DBUG_ENTER("sbt_init_func");
    
    sbt::sbt_hton = (handlerton *)p;
    sbt::sbt_hton->state = SHOW_OPTION_YES;
    sbt::sbt_hton->create = [](handlerton *hton, TABLE_SHARE *table, bool, MEM_ROOT *) -> handler* {
        return new sbt::ha_sbt(hton, table);
    };
    sbt::sbt_hton->flags = HTON_CAN_RECREATE;
    
    DBUG_RETURN(0);
}

// 存储引擎清理函数
static int sbt_done_func(void *p) {
    DBUG_ENTER("sbt_done_func");
    DBUG_RETURN(0);
}

// 插件声明
struct st_mysql_storage_engine sbt_storage_engine = {
    MYSQL_HANDLERTON_INTERFACE_VERSION
};

mysql_declare_plugin(sbt) {
    MYSQL_STORAGE_ENGINE_PLUGIN,
    &sbt_storage_engine,
    "SBT",
    "Your Name",
    "Size Balanced Tree Storage Engine",
    PLUGIN_LICENSE_GPL,
    sbt_init_func,
    nullptr,
    sbt_done_func,
    0x0100,
    nullptr,
    nullptr,
    nullptr,
    0,
}
mysql_declare_plugin_end;
