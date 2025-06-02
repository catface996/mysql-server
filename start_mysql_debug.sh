#!/bin/bash

# 停止可能正在运行的MySQL进程
ps aux | grep mysqld | grep -v grep | awk '{print $2}' | xargs kill -9 2>/dev/null

# 切换到项目根目录
cd "$(dirname "$0")"
PROJECT_ROOT=$(pwd)

# 确保日志目录存在
mkdir -p logs

# 创建MySQL配置文件
cat > /tmp/my.cnf << EOF
[mysqld]
default-storage-engine=SKIPLIST
plugin-load-add=ha_skiplist.so
port=3306
log_error=${PROJECT_ROOT}/logs/mysql_error.log
log_error_verbosity=3
general_log=1
general_log_file=${PROJECT_ROOT}/logs/mysql_general.log
EOF

# 启动MySQL服务器，使用调试模式
./build/runtime_output_directory/mysqld --defaults-file=/tmp/my.cnf --user=`whoami` --datadir=./data --debug > logs/mysql_stdout.log 2> logs/mysql_stderr.log &

echo "MySQL服务器已启动（调试模式），等待5秒后设置密码..."
sleep 5

# 设置root密码
./build/runtime_output_directory/mysql -uroot -e "ALTER USER 'root'@'localhost' IDENTIFIED BY 'password123';"

echo "MySQL root密码已设置为 'password123'"
echo "连接MySQL: ./build/runtime_output_directory/mysql -uroot -ppassword123"