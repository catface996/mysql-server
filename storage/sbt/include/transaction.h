#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "sbt_common.h"
#include <unordered_map>
#include <chrono>

namespace sbt {

// 操作记录
struct Operation {
    OperationType type;
    KeyType key;
    ValueType old_value;
    ValueType new_value;
    std::chrono::steady_clock::time_point timestamp;
    
    Operation(OperationType t, const KeyType& k)
        : type(t), key(k), timestamp(std::chrono::steady_clock::now()) {}
};

// 事务类
class Transaction {
public:
    Transaction(TransactionId tx_id);
    ~Transaction();
    
    // 基本属性
    TransactionId get_id() const { return tx_id_; }
    TransactionState get_state() const { return state_; }
    VersionId get_start_version() const { return start_version_; }
    
    // 事务操作
    SBTError begin();
    SBTError commit();
    SBTError rollback();
    SBTError abort();
    
    // 数据操作
    SBTError insert(const KeyType& key, const ValueType& value);
    SBTError update(const KeyType& key, const ValueType& old_value, 
                   const ValueType& new_value);
    SBTError remove(const KeyType& key, const ValueType& old_value);
    
    // 读操作（用于一致性检查）
    SBTError read(const KeyType& key, ValueType& value);
    
    // 锁管理
    SBTError acquire_read_lock(const KeyType& key);
    SBTError acquire_write_lock(const KeyType& key);
    void release_locks();
    
    // 状态查询
    bool is_active() const { return state_ == TransactionState::ACTIVE; }
    bool is_committed() const { return state_ == TransactionState::COMMITTED; }
    bool is_aborted() const { return state_ == TransactionState::ABORTED; }
    
    // 统计信息
    size_t get_operation_count() const { return operations_.size(); }
    std::chrono::milliseconds get_duration() const;

private:
    // 内部状态管理
    void set_state(TransactionState state);
    bool validate_operation(const Operation& op);
    
    // 回滚辅助
    SBTError undo_operation(const Operation& op);
    
private:
    TransactionId tx_id_;
    TransactionState state_;
    VersionId start_version_;
    
    // 操作日志
    std::vector<Operation> operations_;
    
    // 锁信息
    std::unordered_set<KeyType> read_locks_;
    std::unordered_set<KeyType> write_locks_;
    
    // 时间戳
    std::chrono::steady_clock::time_point start_time_;
    std::chrono::steady_clock::time_point end_time_;
    
    // 同步控制
    mutable std::mutex tx_mutex_;
};

// 事务管理器
class TransactionManager {
public:
    TransactionManager();
    ~TransactionManager();
    
    // 事务生命周期
    std::unique_ptr<Transaction> begin_transaction();
    SBTError commit_transaction(TransactionId tx_id);
    SBTError rollback_transaction(TransactionId tx_id);
    SBTError abort_transaction(TransactionId tx_id);
    
    // 事务查询
    Transaction* get_transaction(TransactionId tx_id);
    std::vector<TransactionId> get_active_transactions();
    
    // 死锁检测
    bool detect_deadlock();
    SBTError resolve_deadlock();
    
    // 版本管理
    VersionId get_current_version();
    VersionId allocate_version();
    void advance_version();
    
    // 清理操作
    void cleanup_old_transactions();
    void cleanup_old_versions();
    
    // 统计信息
    size_t get_active_transaction_count() const;
    size_t get_total_transaction_count() const;

private:
    // 内部辅助函数
    TransactionId allocate_transaction_id();
    void register_transaction(std::unique_ptr<Transaction> tx);
    void unregister_transaction(TransactionId tx_id);
    
    // 死锁检测辅助
    bool has_cycle(TransactionId tx_id, std::unordered_set<TransactionId>& visited,
                   std::unordered_set<TransactionId>& rec_stack);

private:
    // 事务存储
    std::unordered_map<TransactionId, std::unique_ptr<Transaction>> active_transactions_;
    
    // ID分配
    std::atomic<TransactionId> next_tx_id_;
    std::atomic<VersionId> current_version_;
    
    // 同步控制
    mutable std::shared_mutex tx_manager_mutex_;
    
    // 统计信息
    std::atomic<size_t> total_tx_count_;
    std::atomic<size_t> committed_tx_count_;
    std::atomic<size_t> aborted_tx_count_;
};

} // namespace sbt

#endif // TRANSACTION_H
