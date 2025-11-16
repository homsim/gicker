cmake_minimum_required(VERSION 3.30)

project(gicker)

set(CMAKE_CXX_STANDARD 20)

add_executable(gicker
        include/gicker-cli.h
        src/gicker-cli.cpp
)
target_include_directories(
        gicker PRIVATE include
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-linux/include
)
