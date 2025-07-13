#ifndef SBT_TREE_H
#define SBT_TREE_H

#include "sbt_common.h"
#include "sbt_node.h"

namespace sbt {

template<typename KeyType, typename ValueType>
class SBTTree {
public:
    using NodePtr = std::shared_ptr<SBTNode<KeyType, ValueType>>;
    
    // 构造函数和析构函数
    SBTTree();
    ~SBTTree();
    
    // 基本操作
    SBTError insert(const KeyType& key, const ValueType& value);
    SBTError update(const KeyType& key, const ValueType& value);
    SBTError remove(const KeyType& key);
    SBTError find(const KeyType& key, ValueType& value) const;
    bool exists(const KeyType& key) const;
    
    // 范围查询
    std::vector<std::pair<KeyType, ValueType>> range_query(
        const KeyType& start_key, const KeyType& end_key) const;
    
    // 统计信息
    size_t size() const;
    size_t height() const;
    bool empty() const;
    
    // 迭代器支持
    class Iterator;
    Iterator begin() const;
    Iterator end() const;
    Iterator find_iterator(const KeyType& key) const;
    
    // 调试和验证
    bool validate() const;
    void print_tree() const;
    void get_statistics() const;

private:
    // SBT核心算法
    size_t get_size(const NodePtr& node) const;
    NodePtr left_rotate(NodePtr node);
    NodePtr right_rotate(NodePtr node);
    NodePtr maintain(NodePtr node);
    
    // 递归操作
    NodePtr insert_helper(NodePtr node, const KeyType& key, const ValueType& value);
    NodePtr update_helper(NodePtr node, const KeyType& key, const ValueType& value);
    NodePtr remove_helper(NodePtr node, const KeyType& key);
    NodePtr find_helper(NodePtr node, const KeyType& key) const;
    
    // 辅助函数
    NodePtr find_min(NodePtr node) const;
    NodePtr find_max(NodePtr node) const;
    NodePtr remove_min(NodePtr node);
    
    // 验证函数
    bool validate_sbt_property(NodePtr node) const;
    size_t calculate_height(NodePtr node) const;
    void print_helper(NodePtr node, int depth) const;
    
    // 范围查询辅助
    void range_query_helper(NodePtr node, const KeyType& start_key, 
                           const KeyType& end_key,
                           std::vector<std::pair<KeyType, ValueType>>& result) const;

private:
    NodePtr root_;                          // 根节点
    mutable std::shared_mutex tree_mutex_;  // 树级读写锁
    std::atomic<size_t> total_size_;        // 总节点数
};

// SBT树迭代器
template<typename KeyType, typename ValueType>
class SBTTree<KeyType, ValueType>::Iterator {
public:
    using NodePtr = std::shared_ptr<SBTNode<KeyType, ValueType>>;
    
    Iterator();
    Iterator(NodePtr root);
    Iterator(const Iterator& other);
    Iterator& operator=(const Iterator& other);
    
    // 迭代器操作
    bool has_next() const;
    std::pair<KeyType, ValueType> next();
    void reset(NodePtr root);
    
    // 操作符重载
    bool operator==(const Iterator& other) const;
    bool operator!=(const Iterator& other) const;
    Iterator& operator++();
    Iterator operator++(int);
    std::pair<KeyType, ValueType> operator*() const;

private:
    void push_left(NodePtr node);
    
    std::vector<NodePtr> stack_;
    NodePtr current_;
};

} // namespace sbt

#endif // SBT_TREE_H
