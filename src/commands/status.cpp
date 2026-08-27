#include "status.hpp"
#include "../repository/working_tree.hpp"
#include "../repository/reference.hpp"
#include "../filesystem/file_utils.hpp"
#include <iostream>
#include <string>

int handleStatus(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    if (!FileUtils::directoryExists(".minigit")) {
        std::cerr << "fatal: not a minigit repository (or any of the parent directories): .minigit\n";
        return 1;
    }

    std::string branchName = Repository::Reference::getCurrentBranchName(".minigit");
    std::string headCommit;
    bool hasHead = Repository::Reference::resolveHead(headCommit, ".minigit");

    if (!branchName.empty()) {
        std::cout << "On branch " << branchName << "\n";
    } else if (hasHead) {
        std::cout << "HEAD detached at " << headCommit.substr(0, 7) << "\n";
    } else {
        std::cout << "On branch " << (branchName.empty() ? "main" : branchName) << "\n\nNo commits yet\n";
    }

    auto status = Repository::WorkingTree::getStatus(".minigit");

    // 1. Changes to be committed (Staged)
    if (status.hasStagedChanges()) {
        std::cout << "\nChanges to be committed:\n"
                  << "  (use \"minigit restore --staged <file>...\" to unstage)\n";
        for (const auto& pair : status.stagedNew) {
            std::cout << "\tnew file:   " << pair.first << "\n";
        }
        for (const auto& pair : status.stagedModified) {
            std::cout << "\tmodified:   " << pair.first << "\n";
        }
        for (const auto& path : status.stagedDeleted) {
            std::cout << "\tdeleted:    " << path << "\n";
        }
    }

    // 2. Changes not staged for commit (Unstaged)
    if (status.hasUnstagedChanges()) {
        std::cout << "\nChanges not staged for commit:\n"
                  << "  (use \"minigit add <file>...\" to update what will be committed)\n"
                  << "  (use \"minigit restore <file>...\" to discard changes in working directory)\n";
        for (const auto& path : status.unstagedModified) {
            std::cout << "\tmodified:   " << path << "\n";
        }
        for (const auto& path : status.unstagedDeleted) {
            std::cout << "\tdeleted:    " << path << "\n";
        }
    }

    // 3. Untracked files
    if (!status.untracked.empty()) {
        std::cout << "\nUntracked files:\n"
                  << "  (use \"minigit add <file>...\" to include in what will be committed)\n";
        for (const auto& path : status.untracked) {
            std::cout << "\t" << path << "\n";
        }
    }

    if (status.isClean()) {
        std::cout << "\nnothing to commit, working tree clean\n";
    } else if (!status.hasStagedChanges() && (status.hasUnstagedChanges() || !status.untracked.empty())) {
        std::cout << "\nno changes added to commit (use \"minigit add\" to track)\n";
    }

    return 0;
}
