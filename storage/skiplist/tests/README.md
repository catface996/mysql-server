# SkipList存储引擎测试脚本

本目录包含用于测试SkipList存储引擎的脚本。

## 启动脚本

- **start_mysql.sh**: 启动MySQL服务器
- **start_mysql_with_password.sh**: 启动MySQL服务器并设置root密码

## 测试脚本

### 基本功能测试
- **test_skiplist.sql**: 测试SkipList存储引擎的基本功能
- **test_skiplist_simple.sql**: 使用简单表结构测试SkipList存储引擎
- **test_skiplist_data.sql**: 测试数据操作（插入、更新、删除）

### 持久化和日志测试
- **test_skiplist_persistence.sql**: 测试数据持久化功能
- **test_skiplist_check_persistence.sql**: 检查数据持久化是否成功
- **test_skiplist_log_recovery.sql**: 测试日志恢复功能

## 使用方法

1. 启动MySQL服务器：
   ```
   ./start_mysql_with_password.sh
   ```

2. 运行测试脚本：
   ```
   mysql -uroot -ppassword123 < test_skiplist.sql
   ```
