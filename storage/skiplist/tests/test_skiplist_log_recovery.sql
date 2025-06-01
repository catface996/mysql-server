-- 使用数据库
USE test;

-- 插入更多数据
INSERT INTO persistent VALUES ('新数据1');
INSERT INTO persistent VALUES ('新数据2');

-- 删除一些数据
DELETE FROM persistent WHERE data = '测试数据2';

-- 查询数据
SELECT * FROM persistent;
