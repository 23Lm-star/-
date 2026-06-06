
#include <iostream>
#include <mysql/mysql.h>
#include "DBConnection.h"

void testMysqlConnection() {
    std::cout << "=== MySQL连接池测试 ===" << std::endl;
    
    auto conn = DBConnection::getMysqlConnection();
    
    if (!conn.isValid()) {
        std::cerr << "获取MySQL连接失败" << std::endl;
        return;
    }
    
    std::cout << "1. 创建测试表..." << std::endl;
    bool ret = conn->executeCreateTable(
        "CREATE TABLE IF NOT EXISTS users ("
        "id INT PRIMARY KEY AUTO_INCREMENT, "
        "name VARCHAR(100) NOT NULL, "
        "age INT, "
        "email VARCHAR(255), "
        "created_at DATETIME DEFAULT CURRENT_TIMESTAMP)"
        " ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci"
    );
    if (ret) {
        std::cout << "   创建表成功" << std::endl;
    } else {
        std::cerr << "   创建表失败" << std::endl;
        return;
    }
    
    std::cout << "2. 插入测试数据..." << std::endl;
    ret = conn->executeInsert(
        "INSERT INTO users (name, age, email) "
        "VALUES ('张三', 25, 'zhangsan@example.com')"
    );
    if (ret) {
        std::cout << "   插入成功，ID: " << conn->getLastInsertId() << std::endl;
    } else {
        std::cerr << "   插入失败" << std::endl;
        return;
    }
    
    std::cout << "3. 查询数据..." << std::endl;
    MYSQL_RES* result = conn->executeQuery("SELECT * FROM users");
    if (result) {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(result))) {
            std::cout << "   ID: " << row[0] 
                      << ", Name: " << row[1] 
                      << ", Age: " << row[2] 
                      << ", Email: " << row[3] << std::endl;
        }
        mysql_free_result(result);
    } else {
        std::cerr << "   查询失败" << std::endl;
        return;
    }
    
    std::cout << "4. 更新数据..." << std::endl;
    ret = conn->executeUpdate("UPDATE users SET age = 26 WHERE name = '张三'");
    if (ret) {
        std::cout << "   更新成功，影响行数: " << conn->getAffectedRows() << std::endl;
    } else {
        std::cerr << "   更新失败" << std::endl;
        return;
    }
    
    std::cout << "5. 事务测试..." << std::endl;
    ret = conn->beginTransaction();
    if (ret) {
        conn->executeInsert("INSERT INTO users (name, age) VALUES ('事务测试', 30)");
        conn->rollbackTransaction();
        std::cout << "   事务回滚完成" << std::endl;
    } else {
        std::cerr << "   开启事务失败" << std::endl;
    }
    
    std::cout << "5. 最终数据查询..." << std::endl;
    MYSQL_RES* finalResult = conn->executeQuery("SELECT * FROM users");
    if (finalResult) {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(finalResult))) {
            std::cout << "   ID: " << row[0] 
                      << ", Name: " << row[1] 
                      << ", Age: " << row[2] 
                      << ", Email: " << row[3] << std::endl;
        }
        mysql_free_result(finalResult);
    } else {
        std::cerr << "   查询失败" << std::endl;
    }
    
    std::cout << "MySQL测试完成!" << std::endl;
}

void testRedisConnection() {
    std::cout << "\n=== Redis连接池测试 ===" << std::endl;
    
    auto conn = DBConnection::getRedisConnection();
    
    if (!conn.isValid()) {
        std::cerr << "获取Redis连接失败" << std::endl;
        return;
    }
    
    std::cout << "1. 设置String值..." << std::endl;
    bool ret = conn->set("test_key", "test_value");
    if (ret) {
        std::cout << "   设置成功" << std::endl;
    } else {
        std::cerr << "   设置失败" << std::endl;
        return;
    }
    
    std::cout << "2. 获取String值..." << std::endl;
    std::string value = conn->get("test_key");
    std::cout << "   获取结果: " << value << std::endl;
    
    std::cout << "3. 设置Hash字段..." << std::endl;
    ret = conn->hset("user:1", "name", "李四");
    ret &= conn->hset("user:1", "age", "30");
    if (ret) {
        std::cout << "   设置成功" << std::endl;
    } else {
        std::cerr << "   设置失败" << std::endl;
        return;
    }
    
    std::cout << "4. 获取Hash字段..." << std::endl;
    std::string name = conn->hget("user:1", "name");
    std::cout << "   name: " << name << std::endl;
    
    std::cout << "5. 获取Hash所有字段..." << std::endl;
    auto hashData = conn->hgetall("user:1");
    for (const auto& pair : hashData) {
        std::cout << "   " << pair.first << ": " << pair.second << std::endl;
    }
    
    std::cout << "Redis测试完成!" << std::endl;
}

int main() {
    std::cout << "=== RAII连接池组件测试 ===" << std::endl;
    std::cout << "初始化连接池..." << std::endl;
    
    bool mysqlInit = DBConnection::initMysqlPool();
    bool redisInit = DBConnection::initRedisPool();
    
    if (!mysqlInit) {
        std::cerr << "MySQL连接池初始化失败" << std::endl;
    }
    if (!redisInit) {
        std::cerr << "Redis连接池初始化失败" << std::endl;
    }
    
    if (mysqlInit) {
        testMysqlConnection();
    }
    
    if (redisInit) {
        testRedisConnection();
    }
    
    std::cout << "\n=== 测试完成 ===" << std::endl;
    
    return 0;
}
