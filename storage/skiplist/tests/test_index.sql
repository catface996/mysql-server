-- 使用数据库
USE test;

-- 删除已有的表
DROP TABLE IF EXISTS test_index;

-- 创建带索引的表
CREATE TABLE test_index (
    id INT,
    name VARCHAR(50),
    PRIMARY KEY (id)
) ENGINE=SKIPLIST;

-- 插入数据
INSERT INTO test_index VALUES (1, 'Alice');
INSERT INTO test_index VALUES (2, 'Bob');
INSERT INTO test_index VALUES (3, 'Charlie');
INSERT INTO test_index VALUES (4, 'David');
INSERT INTO test_index VALUES (5, 'Eve');

-- 通过主键查询
SELECT * FROM test_index WHERE id = 3;

-- 范围查询
SELECT * FROM test_index WHERE id > 2 AND id < 5;

-- 查询所有数据
SELECT * FROM test_index ORDER BY id;