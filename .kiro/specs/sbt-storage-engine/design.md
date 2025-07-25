# 设计文档

## 概述

本设计文档描述了基于 SBT（Size Balanced Tree，尺寸平衡树）算法的 MySQL 存储引擎实现。该存储引擎将作为 MySQL 插件集成到服务器中，提供基本的 CRUD 操作，不支持主键和索引。SBT 是一种自平衡二叉搜索树，通过维护子树大小来保持平衡，确保操作的时间复杂度为 O(log n)。

## 架构

### 整体架构

```
MySQL 服务器层
    ↓
Handler 接口层 (ha_sbt)
    ↓
SBT 存储引擎核心
    ↓
磁盘存储层
```

### 分层设计

1. **Handler 接口层**: 实现 MySQL 的 handler 接口，处理与 MySQL 服务器的交互
2. **SBT 核心层**: 实现 SBT 数据结构和算法逻辑
3. **存储管理层**: 处理数据的磁盘持久化和内存管理
4. **工具层**: 提供辅助功能如错误处理、日志记录等

## 组件和接口

### 1. Handler 接口实现 (ha_sbt)

**类定义:**
```cpp
class ha_sbt : public handler {
private:
    SBT_share *share;           // 共享数据结构
    SBT_tree *tree;             // SBT 树实例
    SBT_cursor *cursor;         // 遍历游标
    THR_LOCK_DATA lock;         // MySQL 锁数据
    
public:
    // 构造函数和析构函数
    ha_sbt(handlerton *hton, TABLE_SHARE *table_arg);
    ~ha_sbt() override;
    
    // 必需的 handler 方法
    int open(const char *name, int mode, uint test_if_locked, const dd::Table *table_def) override;
    int close() override;
    int create(const char *name, TABLE *form, HA_CREATE_INFO *create_info, dd::Table *table_def) override;
    int delete_table(const char *from, const dd::Table *table_def) override;
    
    // CRUD 操作
    int write_row(uchar *buf) override;
    int update_row(const uchar *old_data, uchar *new_data) override;
    int delete_row(const uchar *buf) override;
    
    // 扫描操作
    int rnd_init(bool scan) override;
    int rnd_end() override;
    int rnd_next(uchar *buf) override;
    int rnd_pos(uchar *buf, uchar *pos) override;
    void position(const uchar *record) override;
    
    // 元数据和配置
    const char *table_type() const override { return "SBT"; }
    ulonglong table_flags() const override;
    int info(uint flag) override;
    int external_lock(THD *thd, int lock_type) override;
    THR_LOCK_DATA **store_lock(THD *thd, THR_LOCK_DATA **to, enum thr_lock_type lock_type) override;
};
```

### 2. SBT 核心数据结构

**SBT 节点结构:**
```cpp
struct SBT_node {
    uchar *record_data;         // 记录数据
    size_t record_length;       // 记录长度
    uint64_t position;          // 记录位置标识
    size_t size;                // 子树大小
    SBT_node *left;             // 左子树
    SBT_node *right;            // 右子树
    
    SBT_node(const uchar *data, size_t len, uint64_t pos);
    ~SBT_node();
};
```

**SBT 树结构:**
```cpp
class SBT_tree {
private:
    SBT_node *root;             // 根节点
    uint64_t next_position;     // 下一个位置标识
    size_t total_records;       // 总记录数
    TABLE *table_def;           // 表定义
    
    // 平衡操作
    SBT_node* left_rotate(SBT_node *node);
    SBT_node* right_rotate(SBT_node *node);
    SBT_node* maintain(SBT_node *node, bool flag);
    void update_size(SBT_node *node);
    
    // 内部操作
    SBT_node* insert_node(SBT_node *root, const uchar *data, size_t len);
    SBT_node* delete_node(SBT_node *root, uint64_t position);
    SBT_node* find_node(SBT_node *root, uint64_t position);
    
public:
    SBT_tree(TABLE *table);
    ~SBT_tree();
    
    // 公共接口
    int insert_record(const uchar *record, size_t length, uint64_t &position);
    int delete_record(uint64_t position);
    int update_record(uint64_t position, const uchar *new_data, size_t length);
    int find_record(uint64_t position, uchar *buffer, size_t &length);
    
    // 遍历支持
    SBT_cursor* create_cursor();
    void destroy_cursor(SBT_cursor *cursor);
    
    // 统计信息
    size_t get_record_count() const { return total_records; }
    void get_statistics(ha_statistics &stats);
};
```

**遍历游标:**
```cpp
class SBT_cursor {
private:
    std::stack<SBT_node*> node_stack;  // 节点栈用于中序遍历
    SBT_node *current_node;            // 当前节点
    bool initialized;                  // 是否已初始化
    
public:
    SBT_cursor();
    ~SBT_cursor();
    
    void init(SBT_node *root);
    int next_record(uchar *buffer, size_t &length, uint64_t &position);
    void reset();
    bool is_valid() const;
};
```

### 3. 共享数据结构

**SBT_share 类:**
```cpp
class SBT_share : public Handler_share {
private:
    char *table_name;           // 表名
    char *data_file_name;       // 数据文件名
    SBT_tree *tree;             // 共享的 SBT 树
    THR_LOCK lock;              // 线程锁
    std::atomic<uint> ref_count; // 引用计数
    
public:
    SBT_share(const char *name);
    ~SBT_share() override;
    
    SBT_tree* get_tree() { return tree; }
    int load_from_disk();
    int save_to_disk();
    void add_ref() { ref_count++; }
    void release_ref() { if (--ref_count == 0) delete this; }
};
```

### 4. 存储管理

**磁盘存储格式:**
```
文件头部 (64 字节):
- 魔数 (4 字节): "SBT\0"
- 版本号 (4 字节)
- 记录总数 (8 字节)
- 下一个位置 (8 字节)
- 表结构校验和 (8 字节)
- 保留字段 (32 字节)

记录区域:
每条记录格式:
- 记录长度 (4 字节)
- 位置标识 (8 字节)
- 记录数据 (变长)
- 校验和 (4 字节)
```

**存储管理器:**
```cpp
class SBT_storage {
private:
    int data_file;              // 数据文件描述符
    char *file_path;            // 文件路径
    bool is_open;               // 文件是否打开
    
public:
    SBT_storage(const char *path);
    ~SBT_storage();
    
    int open_file(int mode);
    int close_file();
    int read_header(SBT_file_header &header);
    int write_header(const SBT_file_header &header);
    int read_record(uint64_t offset, uchar *buffer, size_t &length);
    int write_record(const uchar *data, size_t length, uint64_t &offset);
    int delete_record(uint64_t offset);
    int sync_file();
};
```

## 数据模型

### SBT 算法特性

1. **平衡条件**: 对于任意节点 T，满足 size[right[T]] ≤ α × size[T] 且 size[left[T]] ≤ α × size[T]，其中 α 通常取 0.75
2. **大小维护**: 每个节点维护其子树的大小信息
3. **旋转操作**: 通过左旋和右旋保持树的平衡
4. **维护操作**: 在插入和删除后调用 maintain 函数恢复平衡

### 记录存储模型

1. **记录标识**: 使用递增的 64 位整数作为记录的唯一标识
2. **数据存储**: 记录数据按 MySQL 的行格式存储
3. **位置映射**: SBT 树中的节点通过位置标识关联到磁盘上的实际记录
4. **内存缓存**: 频繁访问的节点保持在内存中

## 错误处理

### 错误类型和处理策略

1. **磁盘 I/O 错误**:
   - 返回 HA_ERR_CRASHED_ON_USAGE
   - 记录详细错误日志
   - 尝试数据恢复

2. **内存分配错误**:
   - 返回 HA_ERR_OUT_OF_MEM
   - 清理已分配的资源
   - 降级到只读模式

3. **数据损坏错误**:
   - 返回 HA_ERR_CRASHED_ON_REPAIR
   - 标记表为需要修复
   - 提供数据恢复建议

4. **并发冲突**:
   - 使用 MySQL 的锁机制
   - 返回 HA_ERR_LOCK_WAIT_TIMEOUT
   - 支持死锁检测

### 错误恢复机制

```cpp
class SBT_error_handler {
public:
    static int handle_io_error(int error_code, const char *operation);
    static int handle_memory_error();
    static int handle_corruption_error(const char *table_name);
    static void log_error(int level, const char *format, ...);
};
```

## 测试策略

### 单元测试

1. **SBT 算法测试**:
   - 插入、删除、查找操作的正确性
   - 平衡性维护测试
   - 边界条件测试

2. **存储管理测试**:
   - 文件读写操作
   - 数据持久化和恢复
   - 错误处理测试

3. **并发测试**:
   - 多线程访问测试
   - 锁机制测试
   - 死锁检测测试

### 集成测试

1. **MySQL 集成测试**:
   - CREATE/DROP TABLE 操作
   - INSERT/UPDATE/DELETE/SELECT 操作
   - 事务处理测试

2. **性能测试**:
   - 大数据量插入性能
   - 查询性能测试
   - 内存使用测试

3. **稳定性测试**:
   - 长时间运行测试
   - 异常情况恢复测试
   - 数据一致性验证

### 测试工具和框架

```cpp
class SBT_test_framework {
public:
    static void run_algorithm_tests();
    static void run_storage_tests();
    static void run_integration_tests();
    static void run_performance_tests();
    static void generate_test_data(size_t record_count);
    static bool verify_tree_balance(SBT_node *root);
};
```

## 性能考虑

### 优化策略

1. **内存管理优化**:
   - 使用内存池减少分配开销
   - 实现节点缓存机制
   - 延迟删除策略

2. **磁盘 I/O 优化**:
   - 批量写入操作
   - 预读机制
   - 压缩存储

3. **并发优化**:
   - 读写锁分离
   - 无锁数据结构
   - 分段锁机制

### 性能指标

- 插入操作: O(log n) 时间复杂度
- 查询操作: O(log n) 时间复杂度  
- 删除操作: O(log n) 时间复杂度
- 空间复杂度: O(n)
- 平衡因子: ≤ 0.75

## 部署和配置

### 编译配置

```cmake
# CMakeLists.txt for SBT storage engine
SET(SBT_SOURCES
    ha_sbt.cc
    sbt_tree.cc
    sbt_storage.cc
    sbt_share.cc
    sbt_cursor.cc
)

MYSQL_ADD_PLUGIN(sbt ${SBT_SOURCES} STORAGE_ENGINE)
```

### 运行时配置

```sql
-- 安装存储引擎
INSTALL PLUGIN sbt SONAME 'ha_sbt.so';

-- 创建使用 SBT 引擎的表
CREATE TABLE test_table (
    id INT,
    name VARCHAR(100),
    data TEXT
) ENGINE=SBT;
```

### 系统变量

- `sbt_cache_size`: SBT 节点缓存大小 (默认: 16MB)
- `sbt_sync_frequency`: 同步到磁盘的频率 (默认: 1000 次操作)
- `sbt_balance_threshold`: 触发重平衡的阈值 (默认: 0.75)