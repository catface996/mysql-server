# 设计文档

## 概述

本文档描述了基于SBT（Size Balanced Tree）算法的MySQL存储引擎的详细设计。SBT存储引擎是一个简化的存储引擎实现，提供基本的CRUD操作和数据持久化功能。该设计遵循MySQL存储引擎框架的标准接口，并实现了SBT数据结构来管理表数据。

SBT（Size Balanced Tree）是一种自平衡二叉搜索树，通过维护子树大小的平衡来保证操作的时间复杂度。与AVL树或红黑树不同，SBT使用子树大小作为平衡条件，具有实现简单、性能稳定的特点。

## 架构

### 整体架构

```
MySQL Server
    ↓
Handler Interface (ha_sbt)
    ↓
SBT Storage Engine
    ├── SBT Tree Manager (内存中的SBT数据结构)
    ├── File Manager (磁盘持久化)
    ├── Record Manager (记录格式处理)
    └── Share Manager (表共享信息)
```

### 模块分层

1. **MySQL接口层**: 实现MySQL handler接口，处理SQL操作到存储引擎的转换
2. **SBT管理层**: 实现SBT数据结构的核心算法，包括插入、删除、查找和平衡操作
3. **存储管理层**: 处理数据的磁盘持久化，包括文件格式、读写操作
4. **记录管理层**: 处理MySQL记录格式与SBT节点数据的转换

## 组件和接口

### 1. Handler接口实现 (ha_sbt)

**职责**: 实现MySQL存储引擎的标准接口

**关键方法**:
- `open()`: 打开表，初始化SBT结构
- `close()`: 关闭表，清理资源
- `write_row()`: 插入记录到SBT
- `update_row()`: 通过全表扫描找到旧记录并更新
- `delete_row()`: 通过全表扫描找到记录并删除
- `rnd_init()`: 初始化全表扫描
- `rnd_next()`: 获取下一条记录（中序遍历SBT）
- `create()`: 创建表文件
- `delete_table()`: 删除表文件

**无主键设计说明**: 
- 不实现`index_*`系列方法，因为不支持索引
- 所有记录定位通过全表扫描实现
- UPDATE和DELETE操作通过比较记录内容来定位目标记录

**接口定义**:
```cpp
class ha_sbt : public handler {
private:
    SBT_share *share;
    SBT_tree *tree;
    SBT_node *current_node;
    
public:
    ha_sbt(handlerton *hton, TABLE_SHARE *table_arg);
    ~ha_sbt();
    
    // MySQL handler interface methods
    int open(const char *name, int mode, uint test_if_locked, const dd::Table *table_def) override;
    int close() override;
    int write_row(uchar *buf) override;
    int update_row(const uchar *old_data, uchar *new_data) override;
    int delete_row(const uchar *buf) override;
    int rnd_init(bool scan) override;
    int rnd_next(uchar *buf) override;
    int create(const char *name, TABLE *table_arg, HA_CREATE_INFO *create_info, dd::Table *table_def) override;
    int delete_table(const char *name, const dd::Table *table_def) override;
};
```

### 2. SBT树管理器 (SBT_tree)

**职责**: 实现SBT数据结构的核心算法

**关键功能**:
- SBT节点的插入、删除、查找
- 树的平衡维护（左旋、右旋）
- 中序遍历支持全表扫描

**数据结构**:
```cpp
struct SBT_node {
    uchar *data;           // 记录数据
    uint data_length;      // 数据长度
    uint64_t insert_id;    // 插入顺序ID（仅用于排序，非主键）
    SBT_node *left;        // 左子树
    SBT_node *right;       // 右子树
    uint size;             // 子树大小（包含自身）
};

class SBT_tree {
private:
    SBT_node *root;
    MEM_ROOT mem_root;
    uint64_t next_insert_id;
    
public:
    SBT_tree();
    ~SBT_tree();
    
    int insert(const uchar *data, uint length);
    int remove(const uchar *old_data, uint length);  // 通过数据内容删除
    int update(const uchar *old_data, uint old_length, const uchar *new_data, uint new_length);
    SBT_node* find_by_data(const uchar *data, uint length);  // 通过数据内容查找
    SBT_node* get_first();
    SBT_node* get_next(SBT_node *current);
    
private:
    SBT_node* insert_node(SBT_node *node, const uchar *data, uint length, uint64_t insert_id);
    SBT_node* remove_node(SBT_node *node, const uchar *data, uint length);
    SBT_node* maintain(SBT_node *node, bool flag);
    SBT_node* rotate_left(SBT_node *node);
    SBT_node* rotate_right(SBT_node *node);
    void update_size(SBT_node *node);
};
```

### 3. 文件管理器 (SBT_file)

**职责**: 处理数据的磁盘持久化

**文件格式**:
- 文件头：包含表元数据（记录数、下一个行ID等）
- 数据区：存储序列化的SBT节点数据

**关键功能**:
- 表数据的加载和保存
- 文件的创建和删除
- 数据完整性检查

**接口定义**:
```cpp
class SBT_file {
private:
    File fd;
    char *file_name;
    
public:
    SBT_file();
    ~SBT_file();
    
    int create(const char *name);
    int open(const char *name);
    int close();
    int load_tree(SBT_tree *tree);
    int save_tree(SBT_tree *tree);
    int delete_file(const char *name);
    
private:
    int write_header(const SBT_header *header);
    int read_header(SBT_header *header);
    int serialize_tree(SBT_node *node, uchar *buffer, uint &offset);
    int deserialize_tree(SBT_node **node, const uchar *buffer, uint &offset);
};
```

### 4. 共享信息管理器 (SBT_share)

**职责**: 管理表的共享信息和并发访问

**关键功能**:
- 表级锁管理
- 共享资源的引用计数
- 表元数据缓存

**数据结构**:
```cpp
class SBT_share : public Handler_share {
public:
    THR_LOCK lock;
    char *table_name;
    uint table_name_length;
    uint use_count;
    SBT_tree *tree;
    SBT_file *file;
    
    SBT_share();
    ~SBT_share();
    
    static SBT_share* get_share(const char *table_name);
    static void release_share(SBT_share *share);
};
```

## 数据模型

### SBT节点结构

每个SBT节点包含以下信息：
- **数据指针**: 指向实际的记录数据
- **数据长度**: 记录数据的字节长度
- **插入顺序ID**: 简单的递增序列号，仅用于SBT树的排序，不是主键
- **左右子树指针**: SBT树结构指针
- **子树大小**: 用于SBT平衡算法

**注意**: 插入顺序ID仅用于维护SBT树的有序性，不提供主键功能。记录查找通过全表扫描实现。

### 记录格式

记录在内存中的存储格式：
```
[Insert ID (8 bytes)] [Data Length (4 bytes)] [Record Data (variable)]
```

**说明**: Insert ID仅用于SBT树的内部排序，不提供主键功能。所有记录操作（查找、更新、删除）都通过全表扫描和数据内容比较实现。

### 文件格式

磁盘文件的存储格式：
```
File Header:
- Magic Number (4 bytes): "SBT\0"
- Version (4 bytes)
- Record Count (8 bytes)
- Next Insert ID (8 bytes)
- Tree Root Offset (8 bytes)

Data Section:
- Serialized SBT nodes in pre-order traversal
```

## 错误处理

### 错误类型

1. **文件操作错误**: 磁盘I/O失败、文件损坏
2. **内存分配错误**: 内存不足
3. **数据完整性错误**: 记录格式错误、树结构损坏
4. **并发访问错误**: 锁冲突

### 错误处理策略

- **返回MySQL标准错误码**: 使用HA_ERR_*系列错误码
- **日志记录**: 记录详细的错误信息用于调试
- **资源清理**: 确保在错误情况下正确释放资源
- **优雅降级**: 在可能的情况下提供部分功能

### 错误码映射

```cpp
// SBT内部错误码到MySQL错误码的映射
int sbt_error_to_mysql_error(int sbt_error) {
    switch (sbt_error) {
        case SBT_ERR_OUT_OF_MEMORY:
            return HA_ERR_OUT_OF_MEM;
        case SBT_ERR_FILE_NOT_FOUND:
            return HA_ERR_NO_SUCH_TABLE;
        case SBT_ERR_CORRUPTED_DATA:
            return HA_ERR_CRASHED_ON_USAGE;
        case SBT_ERR_DUPLICATE_KEY:
            return HA_ERR_FOUND_DUPP_KEY;
        default:
            return HA_ERR_GENERIC;
    }
}
```

## 测试策略

### 单元测试

1. **SBT算法测试**:
   - 插入、删除、查找操作的正确性
   - 树平衡性质的维护
   - 边界条件测试

2. **文件操作测试**:
   - 数据序列化和反序列化
   - 文件创建、读写、删除
   - 错误恢复测试

3. **并发测试**:
   - 多线程访问安全性
   - 锁机制正确性

### 集成测试

1. **MySQL集成测试**:
   - 基本SQL操作（CREATE, INSERT, SELECT, UPDATE, DELETE）
   - 表管理操作（CREATE TABLE, DROP TABLE）
   - 事务边界测试

2. **性能测试**:
   - 大数据量插入性能
   - 查询性能测试
   - 内存使用效率

3. **稳定性测试**:
   - 长时间运行测试
   - 异常情况恢复测试
   - 数据完整性验证

### 测试数据

- **小数据集**: 100-1000条记录，验证基本功能
- **中等数据集**: 10万条记录，测试性能和稳定性
- **大数据集**: 100万条记录，压力测试

### 测试工具

- **MySQL Test Runner (MTR)**: 使用MySQL标准测试框架
- **自定义测试工具**: 针对SBT特定功能的测试
- **性能分析工具**: 内存和CPU使用分析