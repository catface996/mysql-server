#ifndef SBT_NODE_H
#define SBT_NODE_H

#include "sbt_common.h"

namespace sbt {

template<typename KeyType, typename ValueType>
class SBTNode {
public:
    // 构造函数
    SBTNode(const KeyType& key, const ValueType& value);
    SBTNode(const SBTNode& other) = delete;
    SBTNode& operator=(const SBTNode& other) = delete;
    ~SBTNode() = default;

    // 基本属性访问
    const KeyType& get_key() const { return key_; }
    const ValueType& get_value() const { return value_; }
    void set_value(const ValueType& value) { value_ = value; dirty_ = true; }
    
    size_t get_size() const { return size_; }
    void set_size(size_t size) { size_ = size; }
    
    // 子节点访问
    std::shared_ptr<SBTNode> get_left() const { return left_; }
    std::shared_ptr<SBTNode> get_right() const { return right_; }
    void set_left(std::shared_ptr<SBTNode> left) { left_ = left; }
    void set_right(std::shared_ptr<SBTNode> right) { right_ = right; }
    
    // MVCC相关
    VersionId get_version() const { return version_; }
    void set_version(VersionId version) { version_ = version; }
    
    bool is_deleted() const { return deleted_; }
    void mark_deleted() { deleted_ = true; dirty_ = true; }
    
    // 持久化相关
    PageId get_page_id() const { return page_id_; }
    void set_page_id(PageId page_id) { page_id_ = page_id; }
    
    bool is_dirty() const { return dirty_; }
    void mark_clean() { dirty_ = false; }
    void mark_dirty() { dirty_ = true; }
    
    // 序列化支持
    size_t serialize_size() const;
    void serialize(char* buffer) const;
    static std::shared_ptr<SBTNode> deserialize(const char* buffer);

private:
    KeyType key_;                           // 键
    ValueType value_;                       // 值
    size_t size_;                          // 子树大小
    
    std::shared_ptr<SBTNode> left_;        // 左子树
    std::shared_ptr<SBTNode> right_;       // 右子树
    
    // MVCC相关
    VersionId version_;                     // 版本号
    bool deleted_;                         // 删除标记
    
    // 持久化相关
    PageId page_id_;                       // 页面ID
    bool dirty_;                           // 脏页标记
    
    mutable std::shared_mutex node_mutex_; // 节点级锁
};

// 类型别名
using SBTNodePtr = std::shared_ptr<SBTNode<KeyType, ValueType>>;

} // namespace sbt

#endif // SBT_NODE_H
