# MySQL SkipList存储引擎编译指南

本文档详细介绍了如何编译MySQL及其SkipList存储引擎，并将其设置为默认存储引擎。

## 1. 环境准备

确保系统已安装以下依赖：

- CMake 3.5+
- GCC/Clang 编译器
- Boost库
- OpenSSL
- ncurses
- zlib

## 2. 编译步骤

### 2.1 创建构建目录

```bash
cd /Users/catface/Documents/GitHub/catface996/mysql-server
mkdir -p build
cd build
```

### 2.2 配置编译选项

```bash
cmake .. -DWITH_SKIPLIST_STORAGE_ENGINE=1 -DCMAKE_BUILD_TYPE=RelWithDebInfo
```

参数说明：
- `-DWITH_SKIPLIST_STORAGE_ENGINE=1`: 启用SkipList存储引擎
- `-DCMAKE_BUILD_TYPE=RelWithDebInfo`: 编译类型为带调试信息的发布版本

### 2.3 编译MySQL

```bash
make -j$(nproc)
```

如果只想编译SkipList存储引擎：

```bash
make skiplist
```

## 3. 运行MySQL

### 3.1 初始化数据目录

```bash
cd /Users/catface/Documents/GitHub/catface996/mysql-server/build
bin/mysqld --initialize-insecure --user=`whoami` --datadir=./data
```

### 3.2 创建配置文件

创建一个配置文件，设置SkipList为默认存储引擎：

```bash
cat > /tmp/my.cnf << EOF
[mysqld]
default-storage-engine=SKIPLIST
plugin-load-add=ha_skiplist.so
EOF
```

### 3.3 启动MySQL服务器

```bash
cd /Users/catface/Documents/GitHub/catface996/mysql-server/build
bin/mysqld --defaults-file=/tmp/my.cnf --user=`whoami` --datadir=./data
```

### 3.4 连接到MySQL

打开另一个终端窗口：

```bash
cd /Users/catface/Documents/GitHub/catface996/mysql-server/build
bin/mysql -uroot
```

## 4. 验证SkipList存储引擎

在MySQL客户端中执行以下命令：

```sql
SHOW ENGINES;
SHOW VARIABLES LIKE 'default_storage_engine';
CREATE TABLE test_table (id INT PRIMARY KEY, name VARCHAR(100));
SHOW CREATE TABLE test_table;
```

如果一切正常，`SHOW CREATE TABLE test_table` 应该显示 `ENGINE=SKIPLIST`，表示表是使用SkipList存储引擎创建的。

## 5. 调试技巧

### 5.1 查看日志

MySQL服务器日志通常位于数据目录中：

```bash
tail -f ./data/`hostname`.err
```

### 5.2 使用GDB调试

```bash
gdb --args bin/mysqld --defaults-file=/tmp/my.cnf --user=`whoami` --datadir=./data
```

### 5.3 查看存储引擎打印的调试信息

由于我们在代码中添加了 `fprintf(stderr, ...)` 语句，可以通过查看错误日志来查看这些调试信息：

```bash
grep "skiplist" ./data/`hostname`.err
```