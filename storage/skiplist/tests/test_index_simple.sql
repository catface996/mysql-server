-- 使用数据库
USE test;

-- 删除已有的表
DROP TABLE IF EXISTS test_index;

-- 创建简单表
CREATE TABLE test_index (
    id INT,
    name VARCHAR(50)
) ENGINE=SKIPLIST;

-- 插入数据
INSERT INTO test_index VALUES (1, 'Alice');
INSERT INTO test_index VALUES (2, 'Bob');
INSERT INTO test_index VALUES (3, 'Charlie');
INSERT INTO test_index VALUES (4, 'David');
INSERT INTO test_index VALUES (5, 'Eve');

-- 查询所有数据
SELECT * FROM test_index;