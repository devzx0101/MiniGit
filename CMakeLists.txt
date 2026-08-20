cmake_minimum_required(VERSION 3.10)
project(mini-git VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(src)

add_executable(minigit
    src/main.cpp
    src/commands/init.cpp
    src/filesystem/file_utils.cpp
)
