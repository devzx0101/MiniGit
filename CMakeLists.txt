cmake_minimum_required(VERSION 3.10)
project(mini-git VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(src)

add_executable(minigit
    src/main.cpp
    src/commands/init.cpp
    src/commands/add.cpp
    src/commands/ls_files.cpp
    src/commands/hash_object.cpp
    src/commands/cat_file.cpp
    src/repository/index.cpp
    src/objects/object_store.cpp
    src/objects/blob.cpp
    src/hashing/sha1.cpp
    src/filesystem/file_utils.cpp
)
