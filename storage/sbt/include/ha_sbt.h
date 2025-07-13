#ifndef HA_SBT_H
#define HA_SBT_H

#include "sbt_common.h"
#include "sbt_tree.h"

// MySQL includes
#include "sql/handler.h"
#include "my_base.h"
#include "thr_lock.h"

namespace sbt {

class ha_sbt : public handler {
public:
    ha_sbt(handlerton *hton, TABLE_SHARE *table_arg);
    ~ha_sbt() override;

    // 存储引擎名称
    const char *table_type() const override { return "SBT"; }
    
    // 基本表操作 - 修复方法签名
    int open(const char *name, int mode, uint test_if_locked, 
             const dd::Table *table_def) override;
    int close(void) override;
    
    // 数据操作
    int write_row(uchar *buf) override;
    int update_row(const uchar *old_data, uchar *new_data) override;
    int delete_row(const uchar *buf) override;
    
    // 随机访问
    int rnd_init(bool scan) override;
    int rnd_end() override;
    int rnd_next(uchar *buf) override;
    int rnd_pos(uchar *buf, uchar *pos) override;
    void position(const uchar *record) override;
    
    // 表信息
    int info(uint flag) override;
    int external_lock(THD *thd, int lock_type) override;
    
    // 表管理
    int create(const char *name, TABLE *form, HA_CREATE_INFO *create_info,
               dd::Table *table_def) override;
    int delete_table(const char *name, const dd::Table *table_def) override;
    
    // 锁管理 - 实现纯虚函数
    THR_LOCK_DATA **store_lock(THD *thd, THR_LOCK_DATA **to,
                               enum thr_lock_type lock_type) override;
    
    // 存储引擎特性
    ulonglong table_flags() const override;
    ulong index_flags(uint inx, uint part, bool all_parts) const override;
    uint max_supported_record_length() const override;
    uint max_supported_keys() const override;
    uint max_supported_key_parts() const override;
    uint max_supported_key_length() const override;
    uint max_supported_key_part_length(HA_CREATE_INFO *create_info) const override;
    
private:
    // 内部辅助函数
    int pack_row(uchar **row, size_t *length);
    int unpack_row(const uchar *row, size_t length);
    std::string make_key_from_record();
    
    // 错误处理
    int sbt_error_to_mysql_error(SBTError error);
    
private:
    SBTTree<KeyType, ValueType>* sbt_tree_;
    std::string table_name_;
    std::string data_file_path_;
    
    // 迭代器状态
    typename SBTTree<KeyType, ValueType>::Iterator current_iterator_;
    bool iterator_initialized_;
    
    // 当前记录
    std::vector<uchar> current_row_;
    std::string current_key_;
    
    // 表锁
    THR_LOCK_DATA lock_data_;
    THR_LOCK sbt_lock_;
};

} // namespace sbt

#endif // HA_SBT_H
