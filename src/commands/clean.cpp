#include "commands/clean.hpp"
#include "repository/working_tree.hpp"
#include "filesystem/file_utils.hpp"
#include "terminal.hpp"
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>

namespace fs = std::filesystem;

int handleClean(int argc, char* argv[]) {
    if (!FileUtils::directoryExists(".minigit")) {
        std::cerr << "fatal: not a minigit repository (or any of the parent directories): .minigit\n";
        return 1;
    }

    bool force = false;
    bool removeDirs = false;
    bool dryRun = false;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-f" || arg == "--force") {
            force = true;
        } else if (arg == "-d") {
            removeDirs = true;
        } else if (arg == "-n" || arg == "--dry-run") {
            dryRun = true;
        }
    }

    if (!force && !dryRun) {
        std::cerr << "fatal: clean.requireForce not set to false and no -f given; refusing to clean\n"
                  << "use 'minigit clean -f' or 'minigit clean -n' (dry-run)\n";
        return 1;
    }

    auto status = Repository::WorkingTree::getStatus(".minigit");
    if (status.untracked.empty()) {
        return 0;
    }

    for (const auto& path : status.untracked) {
        if (dryRun) {
            std::cout << "Would remove " << path << "\n";
        } else {
            std::error_code ec;
            if (fs::is_directory(path, ec)) {
                if (removeDirs) {
                    fs::remove_all(path, ec);
                    std::cout << "Removing " << path << "/\n";
                }
            } else {
                fs::remove(path, ec);
                std::cout << "Removing " << path << "\n";
            }
        }
    }

    return 0;
}
