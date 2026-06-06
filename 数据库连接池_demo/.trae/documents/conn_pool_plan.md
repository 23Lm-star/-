
# RAII连接池组件实现计划

## 一、项目概述

本项目基于C++11开发一套通用连接池组件，采用RAII设计思想，包含MySQL连接池和Redis连接池两套实现，通过单例模式管理全局连接池实例，内置独立定时器线程完成空闲连接自动回收、超时连接销毁、连接保活检测。

**核心特性**：
1. **RAII自动管理**：连接获取后离开作用域自动归还，杜绝连接泄漏
2. **连接失效检测**：每个连接内置失效状态标记，支持使用前有效性检查
3. **自动重连机制**：通过 `AutoReconnectPtr` 包装器实现透明重连，不侵蚀业务代码
4. **线程安全**：互斥锁+条件变量保证多线程安全存取

---

## 二、架构设计

### 2.1 整体架构分层

| 层级 | 名称 | 职责 | 文件位置 |
| :--- | :--- | :--- | :--- |
| 连接封装层 | MysqlConn / RedisConn | 封装数据库原生连接，提供业务接口，内置失效检测 | src/MysqlConn.h/cpp, src/RedisConn.h/cpp |
| 连接池管理层 | ConnPool | 连接池基类及MySQL/Redis子类，管理连接生命周期 | src/ConnPool.h/cpp |
| 定时任务层 | Timer | 独立定时器线程，处理空闲连接回收和保活检测 | src/Timer.h/cpp |
| 自动重连层 | AutoReconnectPtr | 模板包装器，自动检测失效并重新获取连接 | src/AutoReconnectPtr.h |
| 对外接口层 | DBConnection | 全局统一接口，简化连接获取方式 | src/DBConnection.h |

### 2.2 核心设计模式

#### 2.2.1 RAII设计思想

通过智能指针（std::unique_ptr）自定义析构回调函数，实现连接自动归还：

```cpp
// 获取连接时返回智能指针，析构时自动归还
std::unique_ptr<MysqlConn, std::function<void(MysqlConn*)>> getConnection();
```

#### 2.2.2 单例模式

采用懒汉式单例，双重检查锁定确保线程安全：

```cpp
static ConnPool* getInstance();
```

#### 2.2.3 生产者-消费者模式

使用线程安全队列 + 互斥锁 + 条件变量实现连接的安全存取。

#### 2.2.4 连接失效检测机制

每个连接对象内置失效状态标记，在使用前自动检测连接有效性：

```cpp
// 连接使用前检查有效性
if (!conn->isValid()) {
    if (!conn->reconnect()) {
        conn->markInvalid();
    }
}
```

#### 2.2.5 AutoReconnectPtr自动重连模式（新增）

通过模板包装器实现透明重连，业务代码无需关心连接失效：

```cpp
// 获取自动重连连接，业务代码直接使用
AutoReconnectPtr<MysqlConn> conn = DBConnection::getMysqlConnection();
conn->executeQuery("SELECT * FROM users");  // 失效时自动重连
```

**设计价值**：
- **透明重连**：业务代码无需处理连接失效
- **RAII保证**：构造时获取，析构时归还
- **异常安全**：智能指针保证资源不泄漏
- **使用简单**：语法与普通指针一致

---

## 三、功能需求分解

### 3.1 连接池参数配置

| 参数 | 类型 | 含义 | 默认值 |
| :--- | :--- | :--- | :--- |
| initSize | int | 初始连接数 | 5 |
| minIdle | int | 最小空闲连接数 | 2 |
| maxActive | int | 最大连接数 | 20 |
| maxWaitMs | int | 获取连接阻塞超时时间(ms) | 3000 |
| idleTimeoutMs | int | 连接最大空闲销毁时间(ms) | 30000 |

### 3.2 定时器后台任务

1. **空闲连接回收**：周期性遍历空闲队列，销毁空闲超时的连接
2. **最小空闲保留**：始终保留minIdle数量的空闲连接
3. **连接保活检测**：执行心跳检测（MySQL: SELECT 1, Redis: PING）
4. **失效连接重建**：检测到失效连接时销毁并重建

### 3.3 连接失效检测机制

1. **连接状态标记**：每个连接对象维护一个`m_isValid`成员变量（原子变量）
2. **使用前检测**：每次执行数据库操作前检查连接有效性
3. **自动重连**：检测到失效时自动尝试重新连接
4. **重连失败处理**：重连失败时标记连接失效，通过AutoReconnectPtr重新获取

### 3.4 MySQL连接封装接口

| 接口 | 功能 |
| :--- | :--- |
| executeCreateTable | 创建数据表 |
| executeInsert | 插入数据 |
| executeUpdate | 更新数据 |
| executeDelete | 删除数据 |
| executeQuery | 查询数据 |
| beginTransaction | 开启事务 |
| commitTransaction | 提交事务 |
| rollbackTransaction | 回滚事务 |
| isValid | 判断连接是否有效 |
| reconnect | 重新建立连接 |
| markInvalid | 标记连接失效 |

### 3.5 Redis连接封装接口

| 接口 | 功能 |
| :--- | :--- |
| set | 设置String值 |
| get | 获取String值 |
| hset | 设置Hash字段 |
| hget | 获取Hash字段 |
| hgetall | 获取Hash所有字段 |
| isValid | 判断连接是否有效 |
| reconnect | 重新建立连接 |
| markInvalid | 标记连接失效 |

### 3.6 AutoReconnectPtr接口（新增）

| 接口 | 功能 |
| :--- | :--- |
| operator->() | 重载箭头运算符，自动检测并重新连接 |
| operator*() | 重载解引用运算符 |
| isValid() | 判断底层连接是否有效 |

### 3.7 DBConnection对外接口（新增）

| 接口 | 功能 |
| :--- | :--- |
| getMysqlConnection() | 获取自动重连的MySQL连接 |
| getRedisConnection() | 获取自动重连的Redis连接 |

---

## 四、目录结构

```
src/
├── MysqlConn.h          # MySQL连接封装头文件
├── MysqlConn.cpp        # MySQL连接封装实现
├── RedisConn.h          # Redis连接封装头文件
├── RedisConn.cpp        # Redis连接封装实现
├── ConnPool.h           # 连接池基类及子类声明
├── ConnPool.cpp         # 连接池实现
├── Timer.h              # 定时器模块头文件
├── Timer.cpp            # 定时器模块实现
├── AutoReconnectPtr.h   # 自动重连包装器（新增）
├── DBConnection.h       # 对外统一接口（新增）
├── main.cpp             # 测试用例
└── Makefile             # 编译脚本
```

---

## 五、关键类设计

### 5.1 MysqlConn类

| 成员变量 | 类型 | 说明 |
| :--- | :--- | :--- |
| m_conn | MYSQL* | MySQL原生连接指针 |
| m_host | std::string | 数据库主机地址 |
| m_port | int | 数据库端口 |
| m_user | std::string | 用户名 |
| m_password | std::string | 密码 |
| m_dbname | std::string | 数据库名称 |
| m_isValid | std::atomic<bool> | 连接有效性标记（原子变量） |
| m_lastUsedTime | std::chrono::steady_clock::time_point | 最后使用时间 |

| 成员函数 | 功能 |
| :--- | :--- |
| MysqlConn() | 构造函数 |
| ~MysqlConn() | 析构函数，关闭连接 |
| connect() | 建立数据库连接 |
| disconnect() | 断开连接 |
| isValid() | 判断连接是否有效 |
| reconnect() | 重新建立连接 |
| markInvalid() | 标记连接失效 |
| executeCreateTable() | 创建数据表 |
| executeInsert() | 插入数据 |
| executeUpdate() | 更新数据 |
| executeDelete() | 删除数据 |
| executeQuery() | 查询数据 |
| beginTransaction() | 开启事务 |
| commitTransaction() | 提交事务 |
| rollbackTransaction() | 回滚事务 |
| ping() | 心跳检测（执行SELECT 1） |

### 5.2 RedisConn类

| 成员变量 | 类型 | 说明 |
| :--- | :--- | :--- |
| m_conn | redisContext* | Redis原生连接指针 |
| m_host | std::string | Redis主机地址 |
| m_port | int | Redis端口 |
| m_timeout | timeval | 连接超时时间 |
| m_isValid | std::atomic<bool> | 连接有效性标记（原子变量） |
| m_lastUsedTime | std::chrono::steady_clock::time_point | 最后使用时间 |

| 成员函数 | 功能 |
| :--- | :--- |
| RedisConn() | 构造函数 |
| ~RedisConn() | 析构函数，关闭连接 |
| connect() | 建立Redis连接 |
| disconnect() | 断开连接 |
| isValid() | 判断连接是否有效 |
| reconnect() | 重新建立连接 |
| markInvalid() | 标记连接失效 |
| set() | 设置String值 |
| get() | 获取String值 |
| hset() | 设置Hash字段 |
| hget() | 获取Hash字段 |
| hgetall() | 获取Hash所有字段 |
| ping() | 心跳检测（执行PING命令） |

### 5.3 ConnPool类（模板类）

| 成员变量 | 类型 | 说明 |
| :--- | :--- | :--- |
| m_idleConnections | std::queue<T*> | 空闲连接队列 |
| m_activeConnections | std::unordered_set<T*> | 正在使用的连接集合 |
| m_initSize | int | 初始连接数 |
| m_minIdle | int | 最小空闲连接数 |
| m_maxActive | int | 最大连接数 |
| m_maxWaitMs | int | 获取连接超时时间 |
| m_idleTimeoutMs | int | 空闲超时时间 |
| m_mutex | std::mutex | 互斥锁 |
| m_cond | std::condition_variable | 条件变量 |
| m_timer | Timer* | 定时器实例 |
| m_isRunning | std::atomic<bool> | 池运行状态 |

| 成员函数 | 功能 |
| :--- | :--- |
| getInstance() | 获取单例实例 |
| init() | 初始化连接池 |
| getConnection() | 获取连接（返回智能指针） |
| returnConnection() | 归还连接 |
| destroyIdleConnections() | 销毁超时空闲连接 |
| keepAliveCheck() | 连接保活检测 |
| validateAndReconnect(T*) | 验证连接并尝试重连 |

### 5.4 Timer类

| 成员变量 | 类型 | 说明 |
| :--- | :--- | :--- |
| m_thread | std::thread | 定时器线程 |
| m_intervalMs | int | 定时执行间隔(ms) |
| m_isRunning | std::atomic<bool> | 定时器运行状态 |
| m_task | std::function<void()> | 定时执行任务 |

| 成员函数 | 功能 |
| :--- | :--- |
| Timer(int intervalMs, std::function<void()> task) | 构造函数 |
| ~Timer() | 析构函数，停止线程 |
| start() | 启动定时器 |
| stop() | 停止定时器 |

### 5.5 AutoReconnectPtr类（模板类，新增）

| 成员变量 | 类型 | 说明 |
| :--- | :--- | :--- |
| m_getter | std::function<std::unique_ptr<T>()> | 连接获取函数 |
| m_ptr | std::unique_ptr<T> | 实际连接对象（带自定义删除器） |

| 成员函数 | 功能 |
| :--- | :--- |
| AutoReconnectPtr() | 构造函数，获取初始连接 |
| ~AutoReconnectPtr() | 析构函数，自动归还连接 |
| operator->() | 重载箭头运算符，自动检测有效性 |
| operator*() | 重载解引用运算符 |
| isValid() | 判断连接是否有效 |
| ensureValid() | 确保连接有效，无效则重新获取 |

### 5.6 DBConnection类（新增）

| 成员函数 | 功能 |
| :--- | :--- |
| getMysqlConnection() | 获取自动重连的MySQL连接 |
| getRedisConnection() | 获取自动重连的Redis连接 |

---

## 六、连接失效检测与自动重连流程

### 6.1 AutoReconnectPtr使用流程

```
1. 用户调用 DBConnection::getMysqlConnection()
2. 创建 AutoReconnectPtr 对象，构造时调用 getter 获取连接
3. 用户使用 conn->executeQuery()
4. operator->() 调用 ensureValid()
5. ensureValid() 检查连接有效性：
   a. 有效：返回底层连接指针
   b. 无效：调用 getter() 重新获取，旧连接自动归还
6. 执行数据库操作
7. 作用域结束，AutoReconnectPtr 析构，连接自动归还
```

### 6.2 ensureValid() 核心逻辑

```cpp
void ensureValid() {
    if (!m_ptr || !m_ptr->isValid()) {
        // 旧连接自动释放（unique_ptr 析构时归还到池）
        // 新连接自动获取
        m_ptr = m_getter();
    }
}
```

### 6.3 连接获取流程（ConnPool）

```
1. 加锁
2. 检查空闲队列是否有连接
3. 有连接：取出并检查有效性
   a. 有效：加入活跃集合，返回
   b. 无效：销毁该连接，继续查找
4. 无连接：检查是否达到最大连接数
5. 未达到：创建新连接，加入活跃集合，返回
6. 已达到：等待条件变量（带超时）
7. 解锁
```

### 6.4 连接归还流程

```
1. 智能指针析构，触发自定义删除器
2. 调用 returnConnection(conn)
3. 检查 conn->isValid()
4. 如果有效：归还到空闲队列
5. 如果无效：销毁该连接，不归还
6. 唤醒等待条件变量的线程
```

---

## 七、线程安全设计

### 7.1 空闲队列线程安全

```cpp
std::mutex m_mutex;
std::condition_variable m_cond;
std::queue<T*> m_idleConnections;
```

### 7.2 活跃连接集合线程安全

```cpp
std::mutex m_activeMutex;
std::unordered_set<T*> m_activeConnections;
```

### 7.3 连接失效状态线程安全

```cpp
std::atomic<bool> m_isValid;  // 原子变量保证线程安全
```

### 7.4 AutoReconnectPtr线程安全

- 每个线程独立持有自己的 `AutoReconnectPtr` 对象
- 底层连接由连接池保证线程安全
- 重新获取连接时通过连接池的锁机制保证安全

---

## 八、对外使用示例

### 8.1 MySQL使用示例

```cpp
void businessLogic() {
    // 获取自动重连连接（RAII保证）
    auto conn = DBConnection::getMysqlConnection();
    
    // 执行数据库操作（失效时自动重连）
    conn->executeCreateTable(
        "CREATE TABLE IF NOT EXISTS users ("
        "id INT PRIMARY KEY AUTO_INCREMENT, "
        "name VARCHAR(100) NOT NULL, "
        "age INT)"
    );
    
    conn->executeInsert(
        "INSERT INTO users (name, age) VALUES ('张三', 25)"
    );
    
    // 作用域结束，conn自动析构，连接自动归还
}
```

### 8.2 Redis使用示例

```cpp
void redisLogic() {
    // 获取自动重连的Redis连接
    auto conn = DBConnection::getRedisConnection();
    
    // 执行操作
    conn->set("key", "value");
    std::string result = conn->get("key");
    
    // 作用域结束，连接自动归还
}
```

---

## 九、编译依赖

### 9.1 外部库依赖

| 库名称 | 用途 | 安装方式 |
| :--- | :--- | :--- |
| mysqlclient | MySQL客户端库 | apt-get install libmysqlclient-dev |
| hiredis | Redis客户端库 | apt-get install libhiredis-dev |

### 9.2 Makefile编译配置

```makefile
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra
LIBS = -lmysqlclient -lhiredis

SRCS = src/MysqlConn.cpp src/RedisConn.cpp src/ConnPool.cpp src/Timer.cpp src/main.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = conn_pool_demo

all: $(TARGET)

$(TARGET): $(OBJS)
    $(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

%.o: %.cpp
    $(CXX) $(CXXFLAGS) -c $< -o $@

clean:
    rm -f $(OBJS) $(TARGET)
```

---

## 十、测试用例设计

### 10.1 MySQL测试流程

```
1. 初始化MySQL连接池
2. 获取连接（AutoReconnectPtr）
3. 创建测试表（users）
4. 插入测试数据
5. 查询数据并打印
6. 更新数据
7. 删除数据
8. 事务测试（插入后回滚）
9. 模拟连接失效测试：
   a. 获取连接后手动断开数据库
   b. 尝试使用连接执行操作
   c. 验证自动重连机制
10. 作用域结束，连接自动归还
```

### 10.2 Redis测试流程

```
1. 初始化Redis连接池
2. 获取连接（AutoReconnectPtr）
3. 设置String值
4. 获取String值并打印
5. 设置Hash字段
6. 获取Hash字段并打印
7. 获取Hash所有字段
8. 模拟连接失效测试：
   a. 获取连接后手动断开Redis
   b. 尝试使用连接执行操作
   c. 验证自动重连机制
9. 作用域结束，连接自动归还
```

---

## 十一、风险与注意事项

### 11.1 潜在风险

| 风险点 | 描述 | 解决方案 |
| :--- | :--- | :--- |
| 连接泄漏 | 连接未正确归还 | RAII智能指针自动归还 |
| 死锁 | 多线程并发访问 | 细粒度锁，避免嵌套锁 |
| 连接失效 | 数据库服务中断 | 定时器保活检测+使用前有效性检查 |
| 资源耗尽 | 连接数过多 | 最大连接数限制 |
| 正在使用连接断连 | 连接被分配后突然失效 | AutoReconnectPtr自动重连 |
| 重连失败 | 数据库服务不可用 | 标记连接失效，自动重新获取 |

### 11.2 注意事项

1. 编译前需确保已安装mysqlclient和hiredis库
2. 确保MySQL服务运行在localhost:3306，且配置正确
3. 确保Redis服务运行在localhost:6379
4. 连接池参数需根据实际业务场景调整
5. 活跃连接的保活检测频率应低于空闲连接，避免过多心跳开销

---

## 十二、实现进度计划

| 阶段 | 任务 | 预计耗时 |
| :--- | :--- | :--- |
| 第一阶段 | 基础模块开发（Timer、MysqlConn、RedisConn） | 2小时 |
| 第二阶段 | 连接池核心实现（ConnPool基类及子类） | 2小时 |
| 第三阶段 | 自动重连模块（AutoReconnectPtr、DBConnection） | 1小时 |
| 第四阶段 | 测试用例编写与调试 | 1小时 |
| 第五阶段 | 编译验证与文档完善 | 1小时 |

---

**文档版本**: v1.2  
**创建日期**: 2026-06-04  
**适用项目**: 数据库连接池组件  
**更新说明**: 新增 AutoReconnectPtr 自动重连包装器，实现透明重连，不侵蚀业务代码
