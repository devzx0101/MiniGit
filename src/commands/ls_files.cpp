#include "commands/ls_files.hpp"
#include "repository/index.hpp"
#include "filesystem/file_utils.hpp"
#include <iostream>

int handleLsFiles(int argc, char* argv[]) {
    if (!FileUtils::directoryExists(".minigit")) {
        std::cerr << "fatal: not a minigit repository (or any of the parent directories): .minigit\n";
        return 1;
    }

    Repository::Index index;
    if (!index.load(".minigit")) {
        std::cerr << "fatal: unable to read index file\n";
        return 1;
    }

    bool showStage = false;
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--stage" || arg == "-s") {
            showStage = true;
        }
    }

    const auto& entries = index.getEntries();
    for (const auto& pair : entries) {
        const auto& entry = pair.second;
        if (showStage) {
            std::cout << entry.mode << " " << entry.hash << " 0\t" << entry.path << "\n";
        } else {
            std::cout << entry.path << "\n";
        }
    }

    return 0;
}
