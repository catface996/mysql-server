#!/bin/bash

# 停止可能正在运行的MySQL进程
ps aux | grep mysqld | grep -v grep | awk '{print $2}' | xargs kill -9 2>/dev/null

# 切换到当前目录
cd /Users/catface/Documents/code/GitHub/mysql-server

# 确保日志目录存在
mkdir -p logs

# 创建MySQL配置文件
cat > /tmp/my.cnf << EOF
[mysqld]
default-storage-engine=SKIPLIST
plugin-load-add=ha_skiplist.so
port=3306
log_error=/Users/catface/Documents/code/GitHub/mysql-server/logs/mysql_error.log
EOF

# 检查数据目录是否存在，如果不存在则初始化
if [ ! -d "./data" ]; then
    echo "初始化数据目录..."
    ./build/runtime_output_directory/mysqld --initialize-insecure --user=`whoami` --datadir=./data
fi

# 启动MySQL服务器
./build/runtime_output_directory/mysqld --defaults-file=/tmp/my.cnf --user=`whoami` --datadir=./data > logs/mysql_stdout.log 2> logs/mysql_stderr.log &

echo "MySQL服务器已启动，日志文件位于:"
echo "- 错误日志: logs/mysql_error.log"
echo "- 标准输出: logs/mysql_stdout.log"
echo "- 标准错误: logs/mysql_stderr.log"
echo ""
echo "连接MySQL: ./build/runtime_output_directory/mysql -uroot"