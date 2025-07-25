# 技术栈与构建系统

## 构建系统
- **主要工具**: CMake（最低版本 3.14.6，macOS 11+ 需要 3.18+，Xcode 需要 3.19+）
- **生成器**: Make、Ninja、Visual Studio、Xcode
- **跨平台**: Linux、macOS、Windows、Solaris

## 核心技术
- **编程语言**: C++（主要）、C
- **标准**: C++17/C++20（最新标准）
- **编译器**: GCC 11+、Clang、MSVC（Visual Studio 2019+）

## 主要依赖
- **SSL/TLS**: OpenSSL（系统或捆绑版本）
- **压缩**: zlib、zstd、lz4
- **Protocol Buffers**: 用于内部通信
- **ICU**: 国际化组件 Unicode
- **Boost**: 仅头文件库
- **线程**: pthreads（Unix）、Windows 线程
- **网络**: 原生套接字 API

## 存储引擎

### InnoDB 存储引擎（默认）
- **事务支持**: ACID 合规，支持提交、回滚和崩溃恢复
- **锁机制**: 行级锁定，支持多版本并发控制（MVCC）
- **索引结构**: B+树聚簇索引，主键索引包含完整行数据
- **缓冲池**: 智能缓存系统，缓存数据页和索引页
- **日志系统**: Redo Log（重做日志）和 Undo Log（撤销日志）
- **外键约束**: 完整的外键支持和级联操作
- **压缩**: 表和索引压缩支持
- **全文索引**: 支持全文搜索功能

### 其他存储引擎
- **MyISAM**: 传统非事务性引擎，表级锁定
- **NDB**: MySQL 集群分布式存储
- **Archive、CSV、Memory**: 专用引擎

## 常用构建命令

### 编译并行度设置
- **当前系统**: 16 核 CPU (物理核心: 16)
- **推荐并行度**: 使用至少 70% 的 CPU 核心进行编译
- **建议参数**: `-j12` (16 * 0.75 = 12)

### 基本构建（发布版）
```bash
mkdir build && cd build
cmake ..
make -j12
```

### 调试构建
```bash
cmake -DWITH_DEBUG=1 ..
make -j12
```

### macOS 特定构建
```bash
# 检查 CPU 核心数
sysctl -n hw.ncpu
# 使用 70% 的核心数进行编译
make -j$(echo "$(sysctl -n hw.ncpu) * 0.7 / 1" | bc)
```

### Windows 构建
```bash
cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build . --config RelWithDebInfo --parallel 12
```

### 常用 CMake 选项
- `-DWITH_DEBUG=1`: 调试构建
- `-DWITH_SSL=system`: 使用系统 OpenSSL
- `-DWITH_BOOST=<path>`: 自定义 Boost 位置
- `-DWITH_NDB=ON`: 启用 MySQL 集群
- `-DWITH_UNIT_TESTS=ON`: 构建单元测试
- `-DFORCE_INSOURCE_BUILD=1`: 允许源码内构建（不推荐）

### 测试
```bash
# 运行 MySQL 测试套件
cd mysql-test
perl mysql-test-run.pl
```

## 开发工具
- **代码格式化**: clang-format（版本 15）
- **静态分析**: clang-tidy
- **文档生成**: Doxygen
- **内存调试**: Valgrind、AddressSanitizer、ThreadSanitizer

## InnoDB 特定构建选项
- `-DWITH_INNODB_EXTRA_DEBUG=1`: 启用额外的 InnoDB 调试功能
- `-DWITH_INNODB_MEMCACHED=ON`: 启用 InnoDB Memcached 插件
- `-DWITH_INNODB_ZIP_DEBUG=1`: 启用压缩调试
- `-DWITH_VALGRIND=1`: 启用 Valgrind 支持（影响 InnoDB 内存管理）

## InnoDB 性能监控
- **Performance Schema**: 内置性能监控表
- **Information Schema**: InnoDB 状态和统计信息表
- **监控工具**: `SHOW ENGINE INNODB STATUS` 命令
- **调试标志**: `UNIV_DEBUG` 宏用于调试构建