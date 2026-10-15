#include "commands/branch.hpp"
#include "repository/reference.hpp"
#include "filesystem/file_utils.hpp"
#include "terminal.hpp"
#include <iostream>
#include <string>
#include <vector>

int handleBranch(int argc, char* argv[]) {
    if (!FileUtils::directoryExists(".minigit")) {
        std::cerr << "fatal: not a minigit repository (or any of the parent directories): .minigit\n";
        return 1;
    }

    if (argc == 2) {
        // List branches
        auto branches = Repository::Reference::listBranches(".minigit");
        if (branches.empty()) {
            std::cout << Color::BoldGreen << "* main" << Color::Reset << "\n";
            return 0;
        }

        for (const auto& pair : branches) {
            if (pair.second) {
                std::cout << Color::BoldGreen << "* " << pair.first << Color::Reset << "\n";
            } else {
                std::cout << "  " << pair.first << "\n";
            }
        }
        return 0;
    }

    std::string flag = argv[2];
    if (flag == "-d" || flag == "-D") {
        // Delete branch
        if (argc < 4) {
            std::cerr << "fatal: branch name required for deletion\n";
            return 1;
        }
        std::string targetBranch = argv[3];

        if (!Repository::Reference::branchExists(targetBranch, ".minigit")) {
            std::cerr << "error: branch '" << targetBranch << "' not found.\n";
            return 1;
        }

        std::string currentBranch = Repository::Reference::getCurrentBranchName(".minigit");
        if (currentBranch == targetBranch) {
            std::cerr << "error: Cannot delete branch '" << targetBranch << "' checked out at '.minigit'\n";
            return 1;
        }

        std::string commitHash;
        Repository::Reference::getBranchCommit(targetBranch, commitHash, ".minigit");
        if (Repository::Reference::deleteBranch(targetBranch, ".minigit")) {
            std::cout << "Deleted branch " << targetBranch << " (was " << commitHash.substr(0, 7) << ").\n";
            return 0;
        } else {
            std::cerr << "error: failed to delete branch '" << targetBranch << "'\n";
            return 1;
        }
    } else {
        // Create new branch
        std::string branchName = argv[2];
        if (Repository::Reference::branchExists(branchName, ".minigit")) {
            std::cerr << "fatal: A branch named '" << branchName << "' already exists.\n";
            return 1;
        }

        std::string headCommit;
        if (!Repository::Reference::resolveHead(headCommit, ".minigit")) {
            std::cerr << "fatal: Not a valid object name: 'master' / 'main'.\n";
            return 1;
        }

        if (Repository::Reference::setBranchCommit(branchName, headCommit, ".minigit")) {
            Repository::Reference::appendReflog("refs/heads/" + branchName, "", headCommit, "branch: Created from HEAD", ".minigit");
            return 0;
        } else {
            std::cerr << "fatal: failed to create branch '" << branchName << "'\n";
            return 1;
        }
    }
}
