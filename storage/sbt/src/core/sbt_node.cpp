#include "../../include/sbt_node.h"
#include <cstring>

namespace sbt {

template<typename KeyType, typename ValueType>
SBTNode<KeyType, ValueType>::SBTNode(const KeyType& key, const ValueType& value)
    : key_(key), value_(value), size_(1), left_(nullptr), right_(nullptr),
      version_(0), deleted_(false), page_id_(0), dirty_(true) {
}

template<typename KeyType, typename ValueType>
size_t SBTNode<KeyType, ValueType>::serialize_size() const {
    // 简化版序列化大小计算
    size_t size = 0;
    size += sizeof(size_t);  // key size
    size += key_.size();     // key data
    size += sizeof(size_t);  // value size  
    size += value_.size();   // value data
    size += sizeof(size_t);  // tree size
    size += sizeof(bool);    // deleted flag
    size += sizeof(VersionId); // version
    return size;
}

template<typename KeyType, typename ValueType>
void SBTNode<KeyType, ValueType>::serialize(char* buffer) const {
    char* ptr = buffer;
    
    // 序列化key
    size_t key_size = key_.size();
    memcpy(ptr, &key_size, sizeof(size_t));
    ptr += sizeof(size_t);
    memcpy(ptr, key_.data(), key_size);
    ptr += key_size;
    
    // 序列化value
    size_t value_size = value_.size();
    memcpy(ptr, &value_size, sizeof(size_t));
    ptr += sizeof(size_t);
    memcpy(ptr, value_.data(), value_size);
    ptr += value_size;
    
    // 序列化其他属性
    memcpy(ptr, &size_, sizeof(size_t));
    ptr += sizeof(size_t);
    memcpy(ptr, &deleted_, sizeof(bool));
    ptr += sizeof(bool);
    memcpy(ptr, &version_, sizeof(VersionId));
}

template<typename KeyType, typename ValueType>
std::shared_ptr<SBTNode<KeyType, ValueType>> 
SBTNode<KeyType, ValueType>::deserialize(const char* buffer) {
    const char* ptr = buffer;
    
    // 反序列化key
    size_t key_size;
    memcpy(&key_size, ptr, sizeof(size_t));
    ptr += sizeof(size_t);
    KeyType key(ptr, key_size);
    ptr += key_size;
    
    // 反序列化value
    size_t value_size;
    memcpy(&value_size, ptr, sizeof(size_t));
    ptr += sizeof(size_t);
    ValueType value(ptr, ptr + value_size);
    ptr += value_size;
    
    // 创建节点
    auto node = std::make_shared<SBTNode>(key, value);
    
    // 反序列化其他属性
    memcpy(&node->size_, ptr, sizeof(size_t));
    ptr += sizeof(size_t);
    memcpy(&node->deleted_, ptr, sizeof(bool));
    ptr += sizeof(bool);
    memcpy(&node->version_, ptr, sizeof(VersionId));
    
    node->dirty_ = false;
    return node;
}

// 显式实例化模板
template class SBTNode<KeyType, ValueType>;

} // namespace sbt
