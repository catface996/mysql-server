# SBT Storage Engine

基于Size Balanced Tree (SBT)数据结构的MySQL存储引擎实现。

## 项目结构

```
storage/sbt/
├── CMakeLists.txt              # 主构建配置
├── README.md                   # 项目说明
├── include/                    # 头文件目录
│   ├── sbt_common.h           # 通用定义和类型
│   ├── sbt_node.h             # SBT节点定义
│   ├── sbt_tree.h             # SBT树实现
│   ├── ha_sbt.h               # MySQL Handler接口
│   ├── page_manager.h         # 页面管理器
│   ├── transaction.h          # 事务管理
│   └── ...                    # 其他头文件
├── src/                       # 源文件目录
│   ├── core/                  # SBT核心算法
│   ├── storage/               # 存储管理
│   ├── cache/                 # 缓存管理
│   ├── transaction/           # 事务处理
│   ├── mysql_interface/       # MySQL接口实现
│   └── utils/                 # 工具类
├── tests/                     # 测试目录
│   ├── unit/                  # 单元测试
│   ├── integration/           # 集成测试
│   └── benchmark/             # 性能测试
├── docs/                      # 文档目录
└── examples/                  # 示例代码
```

## 编译和安装

### 前提条件

- MySQL 8.0+ 源码
- CMake 3.16+
- GCC 9+ 或 Clang 10+
- C++17 支持

### 编译步骤

1. 在MySQL源码根目录下编译：
```bash
cd /path/to/mysql-server
mkdir build && cd build
cmake .. -DWITH_SBT_STORAGE_ENGINE=1
make -j$(nproc)
```

2. 或者单独编译存储引擎：
```bash
cd storage/sbt
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 安装插件

```sql
INSTALL PLUGIN sbt SONAME 'ha_sbt.so';
```

## 使用方法

### 创建表

```sql
CREATE TABLE test_table (
    id INT PRIMARY KEY,
    name VARCHAR(100),
    data TEXT
) ENGINE=SBT;
```

### 基本操作

```sql
-- 插入数据
INSERT INTO test_table VALUES (1, 'Alice', 'Some data');

-- 查询数据
SELECT * FROM test_table WHERE id = 1;

-- 更新数据
UPDATE test_table SET name = 'Bob' WHERE id = 1;

-- 删除数据
DELETE FROM test_table WHERE id = 1;
```

## 特性

- ✅ 基于SBT的高效平衡树结构
- ✅ 支持范围查询
- ✅ ACID事务支持
- ✅ MVCC并发控制
- ✅ 持久化存储
- ✅ 缓存管理
- ✅ 死锁检测
- ⏳ 索引支持（开发中）
- ⏳ 压缩存储（计划中）

## 性能特点

- **插入性能**: O(log n) 时间复杂度
- **查询性能**: O(log n) 时间复杂度
- **范围查询**: 高效的有序遍历
- **空间效率**: 相比B+树更紧凑的内存使用

## 开发和测试

### 运行单元测试

```bash
cd build
make test
```

### 运行性能测试

```bash
cd tests/benchmark
./benchmark_sbt
```

### 调试模式编译

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
```

## 配置选项

可以通过MySQL系统变量配置SBT存储引擎：

```sql
-- 设置缓存大小（页面数）
SET GLOBAL sbt_cache_size = 2048;

-- 设置页面大小（字节）
SET GLOBAL sbt_page_size = 16384;

-- 启用/禁用统计信息收集
SET GLOBAL sbt_enable_statistics = ON;
```

## 监控和诊断

### 查看存储引擎状态

```sql
SHOW ENGINE SBT STATUS;
```

### 查看表统计信息

```sql
SELECT * FROM INFORMATION_SCHEMA.SBT_STATISTICS 
WHERE table_name = 'your_table';
```

## 限制和注意事项

1. **键长度限制**: 最大767字节
2. **值长度限制**: 最大65535字节
3. **并发限制**: 读写锁粒度为表级别
4. **索引支持**: 目前仅支持主键索引

## 贡献指南

1. Fork 项目
2. 创建特性分支
3. 提交更改
4. 推送到分支
5. 创建 Pull Request

## 许可证

本项目采用 GPL v2 许可证。

## 联系方式

- 作者: Your Name
- 邮箱: your.email@example.com
- 项目主页: https://github.com/yourusername/sbt-storage-engine
