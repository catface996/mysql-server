-- 使用数据库
USE test;

-- 插入数据
INSERT INTO simple VALUES ('测试数据1');
INSERT INTO simple VALUES ('测试数据2');
INSERT INTO simple VALUES ('测试数据3');

-- 查询数据
SELECT * FROM simple;

-- 更新数据
UPDATE simple SET data = '更新后的数据2' WHERE data = '测试数据2';

-- 再次查询
SELECT * FROM simple;

-- 删除数据
DELETE FROM simple WHERE data = '测试数据3';

-- 最终查询
SELECT * FROM simple;
