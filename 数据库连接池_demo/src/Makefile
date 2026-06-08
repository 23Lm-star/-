# 检测操作系统
ifeq ($(OS),Windows_NT)
    TARGET := conn_pool_demo.exe
    INTERACTIVE_TARGET := interactive_shell.exe
    WEB_TARGET := web_server.exe
    RM := del /Q
    EXTRA_LIBS := -lws2_32
    CXXFLAGS := -std=c++11 -Wall -Wextra -I.
else
    TARGET := conn_pool_demo
    INTERACTIVE_TARGET := interactive_shell
    WEB_TARGET := web_server
    RM := rm -f
    EXTRA_LIBS := -lpthread
    CXXFLAGS := -std=c++11 -Wall -Wextra -I. -D_GNU_SOURCE
endif

CXX := g++
LIBS := -lmysqlclient -lhiredis -lcrypto $(EXTRA_LIBS)

# 原程序源文件
SRCS := MysqlConn.cpp RedisConn.cpp Timer.cpp Logger.cpp main.cpp
HEADERS := ConnPool.h ConnPool.hpp DBConnection.h AutoReconnectPtr.h MysqlConn.h RedisConn.h Timer.h Logger.h
OBJS := $(SRCS:.cpp=.o)

# 交互式Shell源文件
INTERACTIVE_SRCS := interactive/Command.cpp interactive/MysqlCommandHandler.cpp \
                   interactive/RedisCommandHandler.cpp interactive/InteractiveShell.cpp \
                   MysqlConn.cpp RedisConn.cpp Timer.cpp Logger.cpp interactive_main.cpp
INTERACTIVE_OBJS := $(INTERACTIVE_SRCS:.cpp=.o)

WEB_SRCS := web/HttpServer.cpp web/MysqlApiHandler.cpp web/RedisApiHandler.cpp web/ClusterMonitor.cpp \
            auth/SessionManager.cpp auth/AuthHandler.cpp auth/UserDBInit.cpp auth/PasswordHash.cpp \
            MysqlConn.cpp RedisConn.cpp Timer.cpp Logger.cpp web_main.cpp
WEB_OBJS := $(WEB_SRCS:.cpp=.o)

all: $(TARGET)

# 编译原程序
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

# 编译交互式Shell
interactive: $(INTERACTIVE_TARGET)

$(INTERACTIVE_TARGET): $(INTERACTIVE_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

# 编译Web服务
web: $(WEB_TARGET)

$(WEB_TARGET): $(WEB_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

# 通用编译规则
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 原程序的具体依赖
MysqlConn.o: MysqlConn.cpp MysqlConn.h
RedisConn.o: RedisConn.cpp RedisConn.h
Timer.o: Timer.cpp Timer.h
Logger.o: Logger.cpp Logger.h
main.o: main.cpp DBConnection.h ConnPool.h ConnPool.hpp AutoReconnectPtr.h MysqlConn.h RedisConn.h Timer.h

# 交互式模块的依赖
interactive/Command.o: interactive/Command.cpp interactive/Command.h
interactive/MysqlCommandHandler.o: interactive/MysqlCommandHandler.cpp interactive/MysqlCommandHandler.h interactive/Command.h MysqlConn.h
interactive/RedisCommandHandler.o: interactive/RedisCommandHandler.cpp interactive/RedisCommandHandler.h interactive/Command.h RedisConn.h
interactive/InteractiveShell.o: interactive/InteractiveShell.cpp interactive/InteractiveShell.h interactive/Command.h \
                                interactive/MysqlCommandHandler.h interactive/RedisCommandHandler.h
interactive_main.o: interactive_main.cpp DBConnection.h interactive/Command.h interactive/InteractiveShell.h \
                     interactive/MysqlCommandHandler.h interactive/RedisCommandHandler.h

# Web模块的依赖
web/HttpServer.o: web/HttpServer.cpp web/HttpServer.h
web/MysqlApiHandler.o: web/MysqlApiHandler.cpp web/MysqlApiHandler.h web/HttpServer.h DBConnection.h \
                       auth/SessionManager.h auth/AuthHandler.h
web/RedisApiHandler.o: web/RedisApiHandler.cpp web/RedisApiHandler.h web/HttpServer.h DBConnection.h
web/ClusterMonitor.o: web/ClusterMonitor.cpp web/ClusterMonitor.h DBConnection.h RedisConn.h
web_main.o: web_main.cpp DBConnection.h web/HttpServer.h web/MysqlApiHandler.h web/RedisApiHandler.h \
            web/ClusterMonitor.h

# Auth模块的依赖
auth/SessionManager.o: auth/SessionManager.cpp auth/SessionManager.h
auth/AuthHandler.o: auth/AuthHandler.cpp auth/AuthHandler.h auth/UserDBInit.h
auth/UserDBInit.o: auth/UserDBInit.cpp auth/UserDBInit.h auth/PasswordHash.h DBConnection.h
auth/PasswordHash.o: auth/PasswordHash.cpp auth/PasswordHash.h

clean:
	$(RM) $(OBJS) $(TARGET) $(INTERACTIVE_OBJS) $(INTERACTIVE_TARGET) $(WEB_OBJS) $(WEB_TARGET)

.PHONY: all interactive web clean