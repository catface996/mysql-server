-- 使用数据库
USE test;

-- 删除已有的表
DROP TABLE IF EXISTS persistent;

-- 创建简单表
CREATE TABLE persistent (
    data VARCHAR(100)
) ENGINE=SKIPLIST;

-- 插入数据
INSERT INTO persistent VALUES ('测试数据1');
INSERT INTO persistent VALUES ('测试数据2');
INSERT INTO persistent VALUES ('测试数据3');

-- 查询数据
SELECT * FROM persistent;
