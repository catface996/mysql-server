# 技术栈和构建系统

## 构建系统
- **主要工具**: CMake（最低版本 3.14.6，macOS 11+ 需要 3.18+，Xcode 需要 3.19+）
- **生成器**: Unix Makefiles、Ninja、Visual Studio、Xcode
- **需要源外构建**（不建议源内构建）

## 语言和标准
- **C++**: 主要语言，使用最新标准
- **C**: 系统级组件
- **SQL**: 解析器和语言实现
- **JavaScript**: Web 组件和工具

## 主要依赖
- **OpenSSL**: SSL/TLS 支持（系统或捆绑）
- **Boost**: C++ 库（捆绑在 extra/ 中）
- **Protocol Buffers**: 序列化（捆绑）
- **ICU**: Unicode 支持
- **zlib/zstd/lz4**: 压缩库
- **Readline/Editline**: 命令行编辑
- **FIDO2**: 身份验证支持

## 编译器
- **GCC**: 11+（RHEL 上使用 devtoolset/gcc-toolset）
- **Clang**: 支持的替代方案
- **MSVC**: Windows 构建（Visual Studio 2019+）

## 常用构建命令

### 基本构建
```bash
mkdir build && cd build
cmake ..
make -j12  # 使用12个并行任务（推荐为CPU核心数的75%）
```

### 调试构建
```bash
cmake -DWITH_DEBUG=1 ..
make -j12
```

### 带调试信息的发布版（默认）
```bash
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
make -j12
```

### 并行编译设置
- **CPU核心数**: 16核
- **推荐并行任务数**: 12（75%的核心数，避免系统过载）
- **使用方法**: `make -j12` 或 `ninja -j12`

### Windows 构建
```bash
cmake .. -G "Visual Studio 16 2019"
cmake --build . --config RelWithDebInfo --parallel 12
```

### 常用选项
- `-DWITH_DEBUG=1` - 调试构建
- `-DWITH_SSL=system` - 使用系统 OpenSSL
- `-DWITH_BOOST=</path>` - 自定义 Boost 位置
- `-DWITH_UNIT_TESTS=1` - 启用单元测试
- `-DWITH_NDB=ON` - 构建 MySQL 集群

### 测试
```bash
cd mysql-test
./mtr --suite=main
./mtr --parallel=12 --suite=innodb  # 使用12个并行测试进程
```

## 内存分配器
- **jemalloc**: `-DWITH_JEMALLOC=ON`
- **tcmalloc**: `-DWITH_TCMALLOC=ON`

## 清理器
- **AddressSanitizer**: `-DWITH_ASAN=1`
- **ThreadSanitizer**: `-DWITH_TSAN=1`
- **UBSan**: `-DWITH_UBSAN=1`