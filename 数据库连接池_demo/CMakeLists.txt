
cmake_minimum_required(VERSION 3.10)
project(conn_pool_demo)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 原程序源文件列表
set(SOURCES
    src/MysqlConn.cpp
    src/RedisConn.cpp
    src/Timer.cpp
    src/main.cpp
)

# 交互式Shell源文件列表
set(INTERACTIVE_SOURCES
    src/MysqlConn.cpp
    src/RedisConn.cpp
    src/Timer.cpp
    src/interactive/Command.cpp
    src/interactive/MysqlCommandHandler.cpp
    src/interactive/RedisCommandHandler.cpp
    src/interactive/InteractiveShell.cpp
    src/interactive_main.cpp
)

# 包含目录
include_directories(
    ${CMAKE_SOURCE_DIR}/src
)

# 查找MySQL库
find_path(MYSQL_INCLUDE_DIR mysql.h
    PATHS
        "C:/Program Files/MySQL/MySQL Server 8.0/include"
        "C:/Program Files/MySQL/MySQL Server 5.7/include"
        "C:/Program Files (x86)/MySQL/MySQL Server 8.0/include"
        "C:/Program Files (x86)/MySQL/MySQL Server 5.7/include"
)

find_library(MYSQL_LIBRARY
    NAMES mysqlclient mysql
    PATHS
        "C:/Program Files/MySQL/MySQL Server 8.0/lib"
        "C:/Program Files/MySQL/MySQL Server 5.7/lib"
        "C:/Program Files (x86)/MySQL/MySQL Server 8.0/lib"
        "C:/Program Files (x86)/MySQL/MySQL Server 5.7/lib"
)

# 查找Redis库
find_path(HIREDIS_INCLUDE_DIR hiredis.h
    PATHS
        "C:/Program Files/hiredis/include"
        "${CMAKE_SOURCE_DIR}/third_party/hiredis/include"
)

find_library(HIREDIS_LIBRARY
    NAMES hiredis
    PATHS
        "C:/Program Files/hiredis/lib"
        "${CMAKE_SOURCE_DIR}/third_party/hiredis/lib"
)

# 输出信息
message(STATUS "MySQL include dir: ${MYSQL_INCLUDE_DIR}")
message(STATUS "MySQL library: ${MYSQL_LIBRARY}")
message(STATUS "Hiredis include dir: ${HIREDIS_INCLUDE_DIR}")
message(STATUS "Hiredis library: ${HIREDIS_LIBRARY}")

# 包含头文件目录
if(MYSQL_INCLUDE_DIR)
    include_directories(${MYSQL_INCLUDE_DIR})
endif()

if(HIREDIS_INCLUDE_DIR)
    include_directories(${HIREDIS_INCLUDE_DIR})
endif()

# 链接库设置
set(LIBS "")
if(MYSQL_LIBRARY)
    list(APPEND LIBS ${MYSQL_LIBRARY})
endif()
if(HIREDIS_LIBRARY)
    list(APPEND LIBS ${HIREDIS_LIBRARY})
endif()

# Windows特有的链接库
if(WIN32)
    list(APPEND LIBS ws2_32)
endif()

# 创建原程序可执行文件
add_executable(conn_pool_demo ${SOURCES})
target_link_libraries(conn_pool_demo ${LIBS})

# 创建交互式Shell可执行文件
add_executable(interactive_shell ${INTERACTIVE_SOURCES})
target_link_libraries(interactive_shell ${LIBS})
