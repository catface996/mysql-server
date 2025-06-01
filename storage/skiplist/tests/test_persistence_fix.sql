-- 使用数据库
USE test;

-- 删除已有的表
DROP TABLE IF EXISTS persistent_test;

-- 创建简单表
CREATE TABLE persistent_test (
    data VARCHAR(100)
) ENGINE=SKIPLIST;

-- 插入数据
INSERT INTO persistent_test VALUES ('测试数据1');
INSERT INTO persistent_test VALUES ('测试数据2');
INSERT INTO persistent_test VALUES ('测试数据3');

-- 查询数据
SELECT * FROM persistent_test;

-- 退出并重新连接
-- 这里需要手动断开连接并重新连接
