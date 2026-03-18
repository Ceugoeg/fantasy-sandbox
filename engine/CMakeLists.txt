cmake_minimum_required(VERSION 3.10)

project(FantasySandboxEngine VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

set(SOURCES
    src/main.cpp
    src/core/World.cpp
)

# 未来如果要和 Node.js 深度绑定，可以修改为生成共享库 (add_library)
add_executable(fantasy_engine ${SOURCES})

find_package(Threads REQUIRED)
target_link_libraries(fantasy_engine PRIVATE Threads::Threads)

if(MSVC)
    # Windows MSVC 的警告等级设置
    target_compile_options(fantasy_engine PRIVATE /W4 /utf-8)
else()
    # GCC 或 Clang 的警告等级设置
    target_compile_options(fantasy_engine PRIVATE -Wall -Wextra)
endif()