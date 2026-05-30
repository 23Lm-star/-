# libcoro - 轻量级 C++ 协程库

libcoro 是一个专为 C++ 设计的高性能协程库，旨在简化异步编程模型。它通过提供协程、异步 IO、事件同步等核心功能，让开发者能够以同步代码的风格编写高并发程序。
项目采用纯 C++ 实现，零外部依赖，最大程度保证了代码的可移植性和稳定性。在底层实现上，libcoro 利用操作系统提供的轻量级线程切换机制，在 Windows 平台使用 SwitchToThread 和结构化异常处理，在 Unix 平台则通过手写的汇编代码实现高效的上下文切换。这种设计使得协程切换的开销极低，能够轻松支撑数万个并发协程。
同时，库中内置的异步 Socket 和事件驱动调度器，使得网络编程和并发处理变得异常简单。无论是开发高性能服务器、游戏后端还是处理大规模并发任务，libcoro 都能提供优雅而高效的解决方案。
并且libcoro也同样支持多线程并发，在多协程的基础上为开发者提供了更强大的并发处理能力，可以解决更加复杂的并发场景。

## 🌟 项目特性

- **轻量级设计** - 零依赖，纯 C++ 实现
- **跨平台支持** - 完美支持 Windows 和 Unix 系统
- **高性能调度** - 基于事件驱动的协程调度器
- **异步 IO** - 内置异步 Socket 支持
- **事件同步** - 提供 Event 和 Selector 机制
- **优雅的 API** - 简洁直观的接口设计

## 📁 项目结构

```
libcoro/
├── include/coro/          # 头文件目录
│   ├── coro.hpp          # 统一头文件入口
│   ├── Coroutine.hpp     # 协程核心类
│   ├── Hub.hpp           # 协程调度器
│   ├── Socket.hpp        # 异步Socket
│   ├── Event.hpp         # 事件同步
│   ├── Selector.hpp      # 事件选择器
│   ├── Time.hpp          # 时间工具
│   └── Common.hpp        # 通用工具
├── src/                  # 源代码目录
│   ├── Coroutine.cpp     # 协程实现
│   ├── Hub.cpp           # 调度器实现
│   ├── Socket.cpp        # Socket实现
│   ├── Event.cpp         # 事件实现
│   └── ...
└── test/                 # 测试用例
    ├── Basic.cpp         # 基础测试
    ├── Event.cpp         # 事件测试
    ├── Socket.cpp        # Socket测试
    └── ...
```

## 🚀 快速开始

### 环境要求

- C++11 或更高版本
- Visual Studio 2017+ (Windows)
- GCC 5+ 或 Clang (Unix)

### 编译项目

```bash
# 使用 Visual Studio (Windows)
# 打开 libcoro.sln 解决方案，直接编译

# 使用 CMake (跨平台)
mkdir build && cd build
cmake ..
make
```

### 简单示例

```cpp
#include "coro/coro.hpp"
#include <iostream>

int main() {
    // 创建协程
    coro::start([]() {
        std::cout << "Hello from coroutine!" << std::endl;
        coro::yield();  // 让出执行权
        std::cout << "Back to coroutine!" << std::endl;
    });
    
    // 运行协程调度器
    coro::run();
    return 0;
}
```

## 📖 API 文档

### 协程管理

```cpp
// 创建协程
Ptr<Coroutine> coro::start(std::function<void()> func);
Ptr<Coroutine> coro::start(std::function<void()> func, uint32_t stackSize);

// 获取当前协程
Ptr<Coroutine> coro::current();

// 获取主线程协程
Ptr<Coroutine> coro::main();

// 让出执行权
void coro::yield();

// 睡眠指定时间
void coro::sleep(Time const& time);
```

### 协程类

```cpp
class Coroutine {
    Status status() const;    // 获取协程状态
    void join();              // 等待协程结束
    bool join(const Time& timeout);  // 带超时等待
};
```

### 异步 Socket

```cpp
class Socket {
    void bind(SocketAddr const& addr);   // 绑定地址
    void listen(int backlog);            // 监听
    void connect(SocketAddr const& addr); // 连接
    Ptr<Socket> accept();                // 接受连接
    ssize_t write(char const* buf, size_t len); // 写入数据
    ssize_t read(char* buf, size_t len);        // 读取数据
};
```

### 事件同步

```cpp
class Event {
    void notifyAll();          // 通知所有等待的协程
    void wait();               // 等待事件
    bool timedWaitFor(const Time& timeout); // 带超时等待
};
```

## 🧪 测试用例

项目包含多个测试用例，覆盖核心功能：

| 测试文件 | 测试内容 |
|---------|---------|
| Basic.cpp | 基础协程功能测试 |
| Event.cpp | 事件同步机制测试 |
| Socket.cpp | Socket通信测试 |
| Selector.cpp | 事件选择器测试 |
| Join.cpp | 协程join测试 |
| Stack.cpp | 栈大小测试 |
| MultiThread.cpp | 多线程测试 |

## 🔧 核心实现

### 协程状态机

libcoro 为每个协程定义了完整的状态流转模型，包含七种状态：NEW（新建）、RUNNABLE（可运行）、RUNNING（运行中）、BLOCKED（阻塞）、WAITING（等待）、DELETED（已删除）和 EXITED（已退出）。状态机确保协程在生命周期内的每一步转换都是安全可控的，避免出现无效状态转换导致的未定义行为。当协程被创建时处于 NEW 状态，start() 调用后进入 RUNNABLE 队列，调度器选择它时变为 RUNNING，调用 yield() 时回到 RUNNABLE，被 socket 读写阻塞时进入 BLOCKED 状态，等待事件时进入 WAITING 状态。

### 栈管理

每个协程拥有独立的栈空间，由 Stack 类负责管理。默认栈大小为 64KB，开发者可以根据实际需求在创建协程时指定自定义栈大小。Stack 类在构造时分配原始内存，析构时自动释放，确保无内存泄漏。当协程发生切换时，当前栈指针位置会被保存，待协程恢复时从保存点继续执行。这种设计使得多个协程可以共享有限的线程栈空间，每个协程都有独立的内存区域，互不干扰。

### 调度器设计

Hub 类作为协程调度器，是整个系统的心脏。它维护两个核心数据结构：runnable_ 队列存储处于 RUNNABLE 状态的协程列表，timeout_ 优先级队列管理所有超时事件。调度器采用事件驱动模型，主循环不断轮询就绪队列和超时队列，当没有协程处于可运行状态时，调度器会阻塞等待 IO 事件或超时到期。Hub 类提供了 start() 方法创建新协程并加入调度队列，run() 方法启动事件循环，quiesce() 和 poll() 方法用于处理非阻塞场景下的调度需求。

### 事件同步

libcoro 提供了 Event 和 Selector 两套事件同步机制。Event 类采用计数器+等待队列的模式，支持 notifyAll() 广播通知和 wait() 阻塞等待。Selector 类则是对 Event 的高级封装，允许将多个事件与对应的回调函数注册到选择器中，通过单一入口等待任意一个或多个事件触发。这套机制借鉴了 Unix 的 select/poll 模型，但针对协程场景做了专门优化，等待事件的协程会被安全地挂起，不会消耗 CPU 资源。

### 异步 IO

基于操作系统原生异步 API 实现：
- **Windows**: IOCP (IO Completion Ports)
- **Unix**: epoll/kqueue

### 超时管理

超时管理集成在调度器内部，通过 Timeout 类和优先级队列实现。Timeout 对象保存了到期时间和对应的协程指针，按到期时间排序，最先到期的超时事件位于队列头部。调度器每次循环都会检查队首的超时事件是否已到期，若到期则唤醒对应的协程。这使得协程可以方便地实现超时等待、超时重试等常见模式，例如 `coro::sleep(Time const& time)` 函数就是基于超时机制实现的。

### 多线程支持

libcoro 支持在多线程环境下使用，每个线程都可以拥有独立的 Hub 实例。通过 `cleanupHub()` 函数可以清理当前线程的 Hub 资源，确保线程退出时不会留下悬挂状态。多线程场景下，不同线程的协程调度相互独立，线程间通过标准的同步原语（如互斥锁、条件变量）进行通信。MultiThread 测试用例展示了如何在多线程环境中安全地创建和使用协程。

