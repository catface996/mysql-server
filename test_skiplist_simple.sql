-- 使用数据库
USE test;

-- 删除已有的表
DROP TABLE IF EXISTS simple;

-- 创建简单表
CREATE TABLE simple (
    data VARCHAR(100)
) ENGINE=SKIPLIST;

-- 查看表结构
SHOW CREATE TABLE simple;
