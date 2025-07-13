#ifndef SBT_COMMON_H
#define SBT_COMMON_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <shared_mutex>

// MySQL includes
#include "my_base.h"
#include "my_dbug.h"
#include "my_inttypes.h"
#include "sql/handler.h"
#include "sql/table.h"

// SBT存储引擎命名空间
namespace sbt {

// 基本类型定义
using KeyType = std::string;
using ValueType = std::vector<uchar>;
using PageId = uint64_t;
using TransactionId = uint64_t;
using VersionId = uint64_t;

// 常量定义
static constexpr size_t PAGE_SIZE = 16384;      // 16KB页面大小
static constexpr size_t MAX_KEY_SIZE = 767;     // 最大键长度
static constexpr size_t MAX_VALUE_SIZE = 65535; // 最大值长度
static constexpr size_t CACHE_SIZE = 1024;      // 默认缓存页面数

// MySQL兼容性常量
#ifndef HA_REC_NOT_IN_SEQ
#define HA_REC_NOT_IN_SEQ 0
#endif

// 错误码定义
enum class SBTError {
    SUCCESS = 0,
    KEY_NOT_FOUND,
    DUPLICATE_KEY,
    INVALID_PARAMETER,
    OUT_OF_MEMORY,
    IO_ERROR,
    TRANSACTION_ABORTED,
    DEADLOCK_DETECTED
};

// 操作类型
enum class OperationType {
    INSERT,
    UPDATE,
    DELETE,
    SELECT
};

// 事务状态
enum class TransactionState {
    ACTIVE,
    COMMITTED,
    ABORTED,
    PREPARING
};

// 前向声明
template<typename K, typename V> class SBTNode;
template<typename K, typename V> class SBTTree;
class PageManager;
class BufferPool;
class Transaction;
class MVCCManager;
class LockManager;

} // namespace sbt

#endif // SBT_COMMON_H
