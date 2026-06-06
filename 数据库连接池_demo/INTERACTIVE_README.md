
# 交互式Shell模块 - 集成文档

## 一、模块概述

交互式Shell模块是一个独立、低耦合的组件，为数据库连接池项目提供实时命令行交互功能。该模块采用模块化设计，与现有代码保持最小耦合度。

## 二、架构设计

### 2.1 目录结构
```
src/
├── interactive/              # 交互式模块根目录（独立）
│   ├── Command.h             # 命令接口和数据结构定义
│   ├── Command.cpp           # 命令解析器实现
│   ├── MysqlCommandHandler.h # MySQL命令处理器接口
│   ├── MysqlCommandHandler.cpp # MySQL命令处理器实现
│   ├── RedisCommandHandler.h # Redis命令处理器接口
│   ├── RedisCommandHandler.cpp # Redis命令处理器实现
│   ├── InteractiveShell.h    # 主Shell类接口
│   └── InteractiveShell.cpp  # 主Shell类实现
├── interactive_main.cpp      # 交互式模式入口
└── [现有文件...]            # 现有项目文件保持不变
```

### 2.2 核心组件

#### 1. Command 模块
- **职责**: 定义命令数据结构和提供命令解析功能
- **接口**:
  - `CommandType`: 命令类型枚举
  - `ParsedCommand`: 解析后的命令结构
  - `CommandResult`: 命令执行结果
  - `ICommandHandler`: 命令处理器接口
  - `CommandParser`: 命令解析器

#### 2. 命令处理器
- **MysqlCommandHandler**: 处理MySQL相关命令
- **RedisCommandHandler**: 处理Redis相关命令
- 均通过依赖注入获取连接，与具体连接池实现解耦

#### 3. InteractiveShell 主类
- **职责**: 整合各组件，提供交互式界面
- 采用注册模式，可以灵活添加/替换命令处理器

### 2.3 设计原则

1. **低耦合**: 通过接口和依赖注入与现有代码解耦
2. **高内聚**: 每个组件职责单一明确
3. **可扩展**: 易于添加新的数据库类型或命令
4. **可替换**: 整个模块可以被其他交互方案替换

## 三、与现有系统的集成点

### 3.1 连接提供者接口

模块定义了连接提供者接口，用于从现有连接池获取连接：

```cpp
// 定义在 MysqlCommandHandler.h 中
class IMysqlConnectionProvider {
public:
    virtual ~IMysqlConnectionProvider() = default;
    virtual std::shared_ptr&lt;MysqlConn&gt; getConnection() = 0;
};

// 定义在 RedisCommandHandler.h 中
class IRedisConnectionProvider {
public:
    virtual ~IRedisConnectionProvider() = default;
    virtual std::shared_ptr&lt;RedisConn&gt; getConnection() = 0;
};
```

### 3.2 集成方式

只需要在集成代码中实现上述接口并注册到Shell即可：

```cpp
// 1. 实现连接提供者
class MyMysqlProvider : public IMysqlConnectionProvider {
public:
    std::shared_ptr&lt;MysqlConn&gt; getConnection() override {
        // 从现有连接池获取连接的代码
        return DBConnection::getMysqlConnection(); // 需要适配
    }
};

// 2. 注册到Shell
InteractiveShell shell;
shell.registerMysqlHandler(
    std::make_shared&lt;MysqlCommandHandler&gt;(
        std::make_shared&lt;MyMysqlProvider&gt;()
    )
);

// 3. 运行
shell.run();
```

## 四、编译和运行

### 4.1 使用 Makefile

```bash
# 进入 src 目录
cd src

# 编译交互式Shell
make interactive

# 运行
./interactive_shell  # Linux/Mac
interactive_shell.exe  # Windows
```

### 4.2 使用 CMake

```bash
# 创建构建目录
mkdir build &amp;&amp; cd build

# 配置
cmake ..

# 编译
cmake --build .

# 运行
./interactive_shell
```

## 五、使用说明

### 5.1 启动Shell

运行 `interactive_shell` 程序，将看到欢迎界面：

```
========================================
    数据库连接池 - 交互式Shell
========================================

输入 'help' 查看帮助，输入 'exit' 或 'quit' 退出

已连接的数据库:
  [x] MySQL
  [x] Redis
```

### 5.2 基本命令

#### 通用命令
- `help` 或 `h` 或 `?`: 显示帮助
- `help mysql`: 显示MySQL命令帮助
- `help redis`: 显示Redis命令帮助
- `exit` 或 `quit` 或 `q`: 退出Shell

#### MySQL 命令
```sql
select * from users
select id, name from users where age &gt; 25
insert into users (name, age) values ('张三', 30)
update users set age = 31 where name = '张三'
delete from users where name = '张三'
show tables
use testdb
```

#### Redis 命令
```
set mykey hello
get mykey
hset user:1 name zhangsan
hset user:1 age 30
hget user:1 name
hgetall user:1
```

## 六、模块替换指南

如果需要替换为其他交互方案（如REST API、GUI等），只需：

1. 保留 `interactive_main.cpp` 中的连接池初始化部分
2. 删除或替换 `interactive/` 目录下的所有文件
3. 实现新的交互模块
4. 更新构建文件（Makefile/CMakeLists.txt）

现有连接池代码完全不需要修改。

## 七、测试建议

### 7.1 功能测试
1. 测试MySQL的SELECT/INSERT/UPDATE/DELETE
2. 测试Redis的String/Hash操作
3. 测试命令解析（带引号、空格等）
4. 测试表格显示格式

### 7.2 解耦性验证
1. 修改现有连接池代码，确认交互式模块不受影响
2. 尝试替换连接提供者实现，确认功能正常

## 八、扩展开发

### 8.1 添加新的数据库类型

1. 创建新的命令处理器（继承 `ICommandHandler`）
2. 定义对应的连接提供者接口
3. 在 `InteractiveShell` 中添加注册方法
4. 更新帮助信息

### 8.2 添加新的命令

在对应的命令处理器中添加新的命令处理分支即可。

---

## 九、文件清单

### 新增文件（交互式模块）
- `src/interactive/Command.h`
- `src/interactive/Command.cpp`
- `src/interactive/MysqlCommandHandler.h`
- `src/interactive/MysqlCommandHandler.cpp`
- `src/interactive/RedisCommandHandler.h`
- `src/interactive/RedisCommandHandler.cpp`
- `src/interactive/InteractiveShell.h`
- `src/interactive/InteractiveShell.cpp`
- `src/interactive_main.cpp`
- `INTERACTIVE_README.md` (本文件)

### 修改文件（构建配置）
- `src/Makefile` (添加 interactive 目标)
- `CMakeLists.txt` (添加 interactive_shell 目标)

### 未修改的现有文件
- 所有其他源文件保持原样！
