cmake_minimum_required(VERSION 3.10)
project(libcoro VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CORO_SOURCES
    src/Common.cpp
    src/Coroutine.cpp
    src/Error.cpp
    src/Event.cpp
    src/Hub.cpp
    src/Selector.cpp
    src/Socket.cpp
    src/Time.cpp
)

add_library(coro STATIC ${CORO_SOURCES})

target_include_directories(coro PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_definitions(coro PRIVATE CORO_STACK_SIZE=1048576)

if(WIN32)
    target_compile_definitions(coro PRIVATE _WIN32)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        target_compile_definitions(coro PRIVATE _DEBUG)
    endif()
endif()

set_target_properties(coro PROPERTIES
    OUTPUT_NAME "coro"
    POSITION_INDEPENDENT_CODE ON
)

install(TARGETS coro
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp"
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR NOT DEFINED CMAKE_BUILD_TYPE)
    add_executable(example_basic example/basic.cpp)
    target_link_libraries(example_basic coro)
    install(TARGETS example_basic RUNTIME DESTINATION bin)
endif()