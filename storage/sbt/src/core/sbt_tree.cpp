#include "../../include/sbt_tree.h"
#include <algorithm>
#include <iostream>
#include <stack>

namespace sbt {

template<typename KeyType, typename ValueType>
SBTTree<KeyType, ValueType>::SBTTree() : root_(nullptr), total_size_(0) {
}

template<typename KeyType, typename ValueType>
SBTTree<KeyType, ValueType>::~SBTTree() {
    // 智能指针会自动清理
}

// 获取节点大小（SBT核心属性）
template<typename KeyType, typename ValueType>
size_t SBTTree<KeyType, ValueType>::get_size(const NodePtr& node) const {
    return node ? node->get_size() : 0;
}

// 左旋转
template<typename KeyType, typename ValueType>
typename SBTTree<KeyType, ValueType>::NodePtr 
SBTTree<KeyType, ValueType>::left_rotate(NodePtr node) {
    if (!node || !node->get_right()) return node;
    
    NodePtr right = node->get_right();
    node->set_right(right->get_left());
    right->set_left(node);
    
    // 更新size
    node->set_size(get_size(node->get_left()) + get_size(node->get_right()) + 1);
    right->set_size(get_size(right->get_left()) + get_size(right->get_right()) + 1);
    
    return right;
}

// 右旋转
template<typename KeyType, typename ValueType>
typename SBTTree<KeyType, ValueType>::NodePtr 
SBTTree<KeyType, ValueType>::right_rotate(NodePtr node) {
    if (!node || !node->get_left()) return node;
    
    NodePtr left = node->get_left();
    node->set_left(left->get_right());
    left->set_right(node);
    
    // 更新size
    node->set_size(get_size(node->get_left()) + get_size(node->get_right()) + 1);
    left->set_size(get_size(left->get_left()) + get_size(left->get_right()) + 1);
    
    return left;
}

// SBT维护操作（核心算法）
template<typename KeyType, typename ValueType>
typename SBTTree<KeyType, ValueType>::NodePtr 
SBTTree<KeyType, ValueType>::maintain(NodePtr node) {
    if (!node) return node;
    
    // 获取各子树大小
    size_t left_size = get_size(node->get_left());
    size_t right_size = get_size(node->get_right());
    size_t left_left_size = node->get_left() ? get_size(node->get_left()->get_left()) : 0;
    size_t left_right_size = node->get_left() ? get_size(node->get_left()->get_right()) : 0;
    size_t right_left_size = node->get_right() ? get_size(node->get_right()->get_left()) : 0;
    size_t right_right_size = node->get_right() ? get_size(node->get_right()->get_right()) : 0;
    
    // Case 1: size[left[left[T]]] > size[right[T]]
    if (left_left_size > right_size) {
        node = right_rotate(node);
        node->set_right(maintain(node->get_right()));
        node = maintain(node);
    }
    // Case 2: size[right[left[T]]] > size[right[T]]
    else if (left_right_size > right_size) {
        node->set_left(left_rotate(node->get_left()));
        node = right_rotate(node);
        node->set_left(maintain(node->get_left()));
        node->set_right(maintain(node->get_right()));
        node = maintain(node);
    }
    // Case 3: size[right[right[T]]] > size[left[T]]
    else if (right_right_size > left_size) {
        node = left_rotate(node);
        node->set_left(maintain(node->get_left()));
        node = maintain(node);
    }
    // Case 4: size[left[right[T]]] > size[left[T]]
    else if (right_left_size > left_size) {
        node->set_right(right_rotate(node->get_right()));
        node = left_rotate(node);
        node->set_left(maintain(node->get_left()));
        node->set_right(maintain(node->get_right()));
        node = maintain(node);
    }
    
    return node;
}

// 插入操作
template<typename KeyType, typename ValueType>
SBTError SBTTree<KeyType, ValueType>::insert(const KeyType& key, const ValueType& value) {
    std::unique_lock<std::shared_mutex> lock(tree_mutex_);
    
    fprintf(stderr, "[SBT TREE DEBUG] Inserting key: %s, value size: %zu\n", 
            key.c_str(), value.size());
    
    try {
        root_ = insert_helper(root_, key, value);
        total_size_++;
        fprintf(stderr, "[SBT TREE DEBUG] Insert successful, total size: %zu\n", total_size_.load());
        return SBTError::SUCCESS;
    } catch (const std::exception& e) {
        fprintf(stderr, "[SBT TREE DEBUG] Insert failed with exception: %s\n", e.what());
        return SBTError::DUPLICATE_KEY;
    }
}

template<typename KeyType, typename ValueType>
typename SBTTree<KeyType, ValueType>::NodePtr 
SBTTree<KeyType, ValueType>::insert_helper(NodePtr node, const KeyType& key, const ValueType& value) {
    if (!node) {
        return std::make_shared<SBTNode<KeyType, ValueType>>(key, value);
    }
    
    if (key < node->get_key()) {
        node->set_left(insert_helper(node->get_left(), key, value));
    } else if (key > node->get_key()) {
        node->set_right(insert_helper(node->get_right(), key, value));
    } else {
        // 重复键
        throw std::runtime_error("Duplicate key");
    }
    
    // 更新size
    node->set_size(get_size(node->get_left()) + get_size(node->get_right()) + 1);
    
    // 维护SBT性质
    return maintain(node);
}

// 查找操作
template<typename KeyType, typename ValueType>
SBTError SBTTree<KeyType, ValueType>::find(const KeyType& key, ValueType& value) const {
    std::shared_lock<std::shared_mutex> lock(tree_mutex_);
    
    NodePtr node = find_helper(root_, key);
    if (node && !node->is_deleted()) {
        value = node->get_value();
        return SBTError::SUCCESS;
    }
    
    return SBTError::KEY_NOT_FOUND;
}

template<typename KeyType, typename ValueType>
typename SBTTree<KeyType, ValueType>::NodePtr 
SBTTree<KeyType, ValueType>::find_helper(NodePtr node, const KeyType& key) const {
    if (!node) return nullptr;
    
    if (key < node->get_key()) {
        return find_helper(node->get_left(), key);
    } else if (key > node->get_key()) {
        return find_helper(node->get_right(), key);
    } else {
        return node;
    }
}

// 检查键是否存在
template<typename KeyType, typename ValueType>
bool SBTTree<KeyType, ValueType>::exists(const KeyType& key) const {
    ValueType dummy;
    return find(key, dummy) == SBTError::SUCCESS;
}

// 更新操作
template<typename KeyType, typename ValueType>
SBTError SBTTree<KeyType, ValueType>::update(const KeyType& key, const ValueType& value) {
    std::unique_lock<std::shared_mutex> lock(tree_mutex_);
    
    NodePtr node = find_helper(root_, key);
    if (node && !node->is_deleted()) {
        node->set_value(value);
        return SBTError::SUCCESS;
    }
    
    return SBTError::KEY_NOT_FOUND;
}

// 删除操作（简化版 - 只标记删除）
template<typename KeyType, typename ValueType>
SBTError SBTTree<KeyType, ValueType>::remove(const KeyType& key) {
    std::unique_lock<std::shared_mutex> lock(tree_mutex_);
    
    NodePtr node = find_helper(root_, key);
    if (node && !node->is_deleted()) {
        node->mark_deleted();
        total_size_--;
        return SBTError::SUCCESS;
    }
    
    return SBTError::KEY_NOT_FOUND;
}

// 获取树的大小
template<typename KeyType, typename ValueType>
size_t SBTTree<KeyType, ValueType>::size() const {
    return total_size_.load();
}

// 检查树是否为空
template<typename KeyType, typename ValueType>
bool SBTTree<KeyType, ValueType>::empty() const {
    return size() == 0;
}

// 计算树的高度
template<typename KeyType, typename ValueType>
size_t SBTTree<KeyType, ValueType>::height() const {
    std::shared_lock<std::shared_mutex> lock(tree_mutex_);
    return calculate_height(root_);
}

template<typename KeyType, typename ValueType>
size_t SBTTree<KeyType, ValueType>::calculate_height(NodePtr node) const {
    if (!node) return 0;
    return 1 + std::max(calculate_height(node->get_left()), 
                        calculate_height(node->get_right()));
}

// 验证SBT性质
template<typename KeyType, typename ValueType>
bool SBTTree<KeyType, ValueType>::validate() const {
    std::shared_lock<std::shared_mutex> lock(tree_mutex_);
    return validate_sbt_property(root_);
}

template<typename KeyType, typename ValueType>
bool SBTTree<KeyType, ValueType>::validate_sbt_property(NodePtr node) const {
    if (!node) return true;
    
    // 检查size属性
    size_t expected_size = get_size(node->get_left()) + get_size(node->get_right()) + 1;
    if (node->get_size() != expected_size) {
        return false;
    }
    
    // 检查SBT平衡条件
    size_t left_size = get_size(node->get_left());
    size_t right_size = get_size(node->get_right());
    size_t left_left_size = node->get_left() ? get_size(node->get_left()->get_left()) : 0;
    size_t left_right_size = node->get_left() ? get_size(node->get_left()->get_right()) : 0;
    size_t right_left_size = node->get_right() ? get_size(node->get_right()->get_left()) : 0;
    size_t right_right_size = node->get_right() ? get_size(node->get_right()->get_right()) : 0;
    
    // SBT平衡条件
    if (left_left_size > right_size || left_right_size > right_size ||
        right_right_size > left_size || right_left_size > left_size) {
        return false;
    }
    
    // 递归检查子树
    return validate_sbt_property(node->get_left()) && validate_sbt_property(node->get_right());
}

// 打印树结构（调试用）
template<typename KeyType, typename ValueType>
void SBTTree<KeyType, ValueType>::print_tree() const {
    std::shared_lock<std::shared_mutex> lock(tree_mutex_);
    print_helper(root_, 0);
}

template<typename KeyType, typename ValueType>
void SBTTree<KeyType, ValueType>::print_helper(NodePtr node, int depth) const {
    if (!node) return;
    
    print_helper(node->get_right(), depth + 1);
    
    for (int i = 0; i < depth; ++i) {
        std::cout << "  ";
    }
    std::cout << node->get_key() << "(" << node->get_size() << ")" << std::endl;
    
    print_helper(node->get_left(), depth + 1);
}

// 范围查询
template<typename KeyType, typename ValueType>
std::vector<std::pair<KeyType, ValueType>> 
SBTTree<KeyType, ValueType>::range_query(const KeyType& start_key, const KeyType& end_key) const {
    std::shared_lock<std::shared_mutex> lock(tree_mutex_);
    std::vector<std::pair<KeyType, ValueType>> result;
    range_query_helper(root_, start_key, end_key, result);
    return result;
}

template<typename KeyType, typename ValueType>
void SBTTree<KeyType, ValueType>::range_query_helper(NodePtr node, 
                                                     const KeyType& start_key, 
                                                     const KeyType& end_key,
                                                     std::vector<std::pair<KeyType, ValueType>>& result) const {
    if (!node || node->is_deleted()) return;
    
    // 中序遍历
    if (node->get_key() > start_key) {
        range_query_helper(node->get_left(), start_key, end_key, result);
    }
    
    if (node->get_key() >= start_key && node->get_key() <= end_key) {
        result.emplace_back(node->get_key(), node->get_value());
    }
    
    if (node->get_key() < end_key) {
        range_query_helper(node->get_right(), start_key, end_key, result);
    }
}

// 迭代器实现
template<typename KeyType, typename ValueType>
typename SBTTree<KeyType, ValueType>::Iterator 
SBTTree<KeyType, ValueType>::begin() const {
    fprintf(stderr, "[SBT TREE DEBUG] Creating iterator, tree size: %zu\n", total_size_.load());
    return Iterator(root_);
}

template<typename KeyType, typename ValueType>
typename SBTTree<KeyType, ValueType>::Iterator 
SBTTree<KeyType, ValueType>::end() const {
    return Iterator();
}

// 迭代器类实现
template<typename KeyType, typename ValueType>
SBTTree<KeyType, ValueType>::Iterator::Iterator() : current_(nullptr) {
}

template<typename KeyType, typename ValueType>
SBTTree<KeyType, ValueType>::Iterator::Iterator(NodePtr root) : current_(nullptr) {
    reset(root);
}

template<typename KeyType, typename ValueType>
SBTTree<KeyType, ValueType>::Iterator::Iterator(const Iterator& other) 
    : stack_(other.stack_), current_(other.current_) {
}

template<typename KeyType, typename ValueType>
typename SBTTree<KeyType, ValueType>::Iterator& 
SBTTree<KeyType, ValueType>::Iterator::operator=(const Iterator& other) {
    if (this != &other) {
        stack_ = other.stack_;
        current_ = other.current_;
    }
    return *this;
}

template<typename KeyType, typename ValueType>
void SBTTree<KeyType, ValueType>::Iterator::reset(NodePtr root) {
    stack_.clear();
    current_ = nullptr;
    push_left(root);
    if (!stack_.empty()) {
        current_ = stack_.back();
        stack_.pop_back();
    }
}

template<typename KeyType, typename ValueType>
void SBTTree<KeyType, ValueType>::Iterator::push_left(NodePtr node) {
    while (node) {
        stack_.push_back(node);
        node = node->get_left();
    }
}

template<typename KeyType, typename ValueType>
bool SBTTree<KeyType, ValueType>::Iterator::has_next() const {
    bool result = current_ != nullptr;
    fprintf(stderr, "[SBT ITERATOR DEBUG] has_next() = %d\n", result);
    return result;
}

template<typename KeyType, typename ValueType>
std::pair<KeyType, ValueType> SBTTree<KeyType, ValueType>::Iterator::next() {
    if (!current_) {
        throw std::runtime_error("Iterator exhausted");
    }
    
    auto result = std::make_pair(current_->get_key(), current_->get_value());
    
    // 移动到下一个节点
    push_left(current_->get_right());
    
    if (!stack_.empty()) {
        current_ = stack_.back();
        stack_.pop_back();
    } else {
        current_ = nullptr;
    }
    
    return result;
}

// 显式实例化模板
template class SBTTree<KeyType, ValueType>;

} // namespace sbt
