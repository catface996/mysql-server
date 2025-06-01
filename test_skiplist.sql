-- 使用数据库
USE test;

-- 删除已有的表
DROP TABLE IF EXISTS students;

-- 创建学生表
CREATE TABLE students (
    id INT,
    name VARCHAR(100),
    age INT
) ENGINE=SKIPLIST;

-- 插入数据
INSERT INTO students VALUES (1, '张三', 20);
INSERT INTO students VALUES (2, '李四', 21);
INSERT INTO students VALUES (3, '王五', 22);

-- 查询数据
SELECT * FROM students;

-- 更新数据
UPDATE students SET age = 23 WHERE id = 2;

-- 再次查询
SELECT * FROM students;

-- 删除数据
DELETE FROM students WHERE id = 3;

-- 最终查询
SELECT * FROM students;
