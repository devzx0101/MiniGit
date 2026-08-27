cmake_minimum_required(VERSION 3.10)
project(mini-git VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(src)

set(MINIGIT_SOURCES
    src/commands/init.cpp
    src/commands/add.cpp
    src/commands/commit.cpp
    src/commands/status.cpp
    src/commands/log.cpp
    src/commands/branch.cpp
    src/commands/checkout.cpp
    src/commands/diff.cpp
    src/commands/reset.cpp
    src/commands/restore.cpp
    src/commands/tag.cpp
    src/commands/merge.cpp
    src/commands/reflog.cpp
    src/commands/cat_file.cpp
    src/commands/ls_files.cpp
    src/commands/hash_object.cpp
    src/commands/remote.cpp
    src/commands/help.cpp
    src/repository/index.cpp
    src/repository/reference.cpp
    src/repository/ignore.cpp
    src/repository/working_tree.cpp
    src/repository/diff_engine.cpp
    src/repository/merge_engine.cpp
    src/repository/remote_manager.cpp
    src/objects/object_store.cpp
    src/objects/blob.cpp
    src/objects/tree.cpp
    src/objects/commit.cpp
    src/hashing/sha1.cpp
    src/filesystem/file_utils.cpp
)

add_executable(minigit
    src/main.cpp
    ${MINIGIT_SOURCES}
)

enable_testing()
add_executable(minigit_tests
    tests/test_runner.cpp
    ${MINIGIT_SOURCES}
)
add_test(NAME MiniGitIntegrationTests COMMAND minigit_tests)
