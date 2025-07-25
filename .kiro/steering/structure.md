# 项目结构和组织

## 顶级目录

### 核心服务器组件
- **`sql/`** - MySQL 服务器核心（mysqld）、SQL 解析器、优化器、执行器
- **`storage/`** - 存储引擎（InnoDB、MyISAM 等）
- **`mysys/`** - MySQL 系统库（底层工具、内存管理）
- **`strings/`** - 字符串操作函数和字符集
- **`vio/`** - 网络和文件操作的虚拟 I/O 层

### 客户端和工具
- **`client/`** - 命令行客户端工具（mysql、mysqldump、mysqladmin 等）
- **`libmysql/`** - MySQL 客户端库实现
- **`sql-common/`** - 服务器和客户端之间的共享代码

### 库和服务
- **`libservices/`** - 插件服务实现
- **`libbinlogevents/`** - 二进制日志事件处理
- **`libchangestreams/`** - 变更流功能

### 构建和配置
- **`cmake/`** - CMake 构建脚本和平台特定配置
- **`include/`** - 公共头文件和 API 定义
- **`extra/`** - 第三方库（Boost、protobuf、ICU 等）

### 测试和文档
- **`mysql-test/`** - MySQL 测试运行器（MTR）和测试套件
- **`unittest/`** - 单元测试框架和测试
- **`testclients/`** - 测试客户端应用程序
- **`Docs/`** - 构建文档和规范

### 插件和组件
- **`plugin/`** - 服务器插件（身份验证、审计等）
- **`components/`** - MySQL 组件框架实现
- **`router/`** - MySQL Router 源代码

### 打包和部署
- **`packaging/`** - 包创建脚本（RPM、DEB 等）
- **`scripts/`** - 安装和实用脚本
- **`support-files/`** - 配置模板和初始化脚本
- **`man/`** - MySQL 工具的手册页

## 代码组织模式

### 头文件
- **公共 API**: `include/` 目录
- **内部头文件**: 与源文件一起
- **MySQL 特定**: 以 `my_` 为前缀（例如 `my_sys.h`）

### 源文件
- **`.cc`** 扩展名用于 C++ 文件
- **`.c`** 扩展名用于 C 文件
- **`.h`** 扩展名用于头文件

### 命名约定
- **函数**: `snake_case`（例如 `my_malloc`、`table_open`）
- **类**: `PascalCase`（例如 `Table_ref`、`Query_block`）
- **常量**: `UPPER_CASE`（例如 `MAX_KEY_LENGTH`）
- **宏**: `UPPER_CASE` 带 MySQL 前缀

### `sql/` 中的关键子目录
- **`auth/`** - 身份验证和授权
- **`binlog/`** - 二进制日志实现
- **`dd/`** - 数据字典
- **`gis/`** - 地理信息系统支持
- **`item/`** - SQL 表达式项
- **`join_optimizer/`** - 查询连接优化
- **`parse_tree/`** - 解析树节点
- **`rpl/`** - 复制功能

## 构建产物
- **构建目录**: 源外构建（例如 `build/`、`cmake-build-debug/`）
- **可执行文件**: 构建目录中的 `bin/`
- **库**: 构建目录中的 `lib/`
- **生成文件**: SQL 解析器、protobuf 文件、版本信息

## 配置文件
- **`.clang-format`** - 代码格式化规则（Google 风格带修改）
- **`.clang-tidy`** - 静态分析配置
- **`CMakeLists.txt`** - 构建配置（分层）
- **`config.h.cmake`** - 配置模板