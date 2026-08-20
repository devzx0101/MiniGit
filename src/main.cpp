#include <iostream>
#include <string>
#include <vector>
#include "commands/init.hpp"
#include "commands/add.hpp"
#include "commands/ls_files.hpp"
#include "commands/hash_object.hpp"
#include "commands/cat_file.hpp"

void printUsage() {
    std::cout << "Usage: minigit <command> [<args>]\n\n"
              << "Available commands:\n"
              << "   init                   Create an empty Mini-Git repository\n"
              << "   add <file>...          Add file contents to the staging area\n"
              << "   ls-files [--stage]     Information about files in the staging area\n"
              << "   hash-object [-w] <file> Compute object ID and optionally store object\n"
              << "   cat-file -p <hash>     Provide content of repository objects\n"
              << "   help                   Show available commands\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Error: No command specified.\n\n";
        printUsage();
        return 1;
    }

    std::string command = argv[1];

    if (command == "help" || command == "--help" || command == "-h") {
        printUsage();
        return 0;
    } else if (command == "init") {
        return handleInit(argc, argv);
    } else if (command == "add") {
        return handleAdd(argc, argv);
    } else if (command == "ls-files") {
        return handleLsFiles(argc, argv);
    } else if (command == "hash-object") {
        return handleHashObject(argc, argv);
    } else if (command == "cat-file") {
        return handleCatFile(argc, argv);
    } else {
        std::cerr << "minigit: '" << command << "' is not a minigit command. See 'minigit help'.\n";
        return 1;
    }
}
