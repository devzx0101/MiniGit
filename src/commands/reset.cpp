#include "commands/reset.hpp"
#include "repository/index.hpp"
#include "repository/reference.hpp"
#include "repository/working_tree.hpp"
#include "objects/commit.hpp"
#include "objects/tree.hpp"
#include "filesystem/file_utils.hpp"
#include <iostream>
#include <vector>
#include <string>

int handleReset(int argc, char* argv[]) {
    if (!FileUtils::directoryExists(".minigit")) {
        std::cerr << "fatal: not a minigit repository (or any of the parent directories): .minigit\n";
        return 1;
    }

    bool isHard = false;
    std::string targetCommitOrRef;
    std::vector<std::string> targetFiles;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--hard") {
            isHard = true;
        } else if (arg == "--mixed" || arg == "--soft") {
            // default mode
        } else if (arg.rfind("-", 0) != 0 && targetCommitOrRef.empty() && Repository::Reference::resolveRef(arg, targetCommitOrRef, ".minigit")) {
            // It resolved to a commit
        } else {
            targetFiles.push_back(FileUtils::normalizePath(arg));
        }
    }

    // Resolve target commit (defaults to HEAD)
    std::string targetCommitHash;
    if (!targetCommitOrRef.empty()) {
        targetCommitHash = targetCommitOrRef;
    } else {
        Repository::Reference::resolveHead(targetCommitHash, ".minigit");
    }

    std::map<std::string, Repository::IndexEntry> targetTreeFiles;
    if (!targetCommitHash.empty()) {
        Objects::Commit commit;
        if (Objects::Commit::load(targetCommitHash, commit, ".minigit")) {
            Objects::Tree::flatten(commit.getTreeHash(), targetTreeFiles, ".minigit");
        }
    }

    if (isHard) {
        // Reset working tree and index to target commit
        if (targetCommitHash.empty()) {
            std::cerr << "fatal: ambiguous argument, cannot resolve target commit for --hard\n";
            return 1;
        }

        Objects::Commit commit;
        if (!Objects::Commit::load(targetCommitHash, commit, ".minigit")) {
            std::cerr << "fatal: cannot load commit " << targetCommitHash << "\n";
            return 1;
        }

        if (!Repository::WorkingTree::checkoutTree(commit.getTreeHash(), true, ".minigit")) {
            std::cerr << "fatal: failed to checkout tree for hard reset\n";
            return 1;
        }

        // Update branch or detached HEAD to targetCommitHash if resetting to another commit
        std::string branch = Repository::Reference::getCurrentBranchName(".minigit");
        if (!branch.empty()) {
            Repository::Reference::setBranchCommit(branch, targetCommitHash, ".minigit");
        } else {
            Repository::Reference::setHeadDetached(targetCommitHash, ".minigit");
        }
        Repository::Reference::appendReflog("HEAD", "", targetCommitHash, "reset: moving to " + targetCommitHash.substr(0, 7), ".minigit");

        std::cout << "HEAD is now at " << targetCommitHash.substr(0, 7) << "\n";
        return 0;
    }

    // Mixed reset: update index
    Repository::Index index;
    index.load(".minigit");

    if (targetFiles.empty()) {
        // Reset all staged entries to match targetTreeFiles
        Repository::Index newIndex;
        for (const auto& pair : targetTreeFiles) {
            newIndex.addEntry(pair.first, pair.second.hash, pair.second.mode);
        }
        if (!newIndex.save(".minigit")) {
            std::cerr << "fatal: failed to update index\n";
            return 1;
        }
        std::cout << "Unstaged all changes.\n";
    } else {
        // Reset specific files in index
        for (const auto& file : targetFiles) {
            auto it = targetTreeFiles.find(file);
            if (it != targetTreeFiles.end()) {
                index.addEntry(file, it->second.hash, it->second.mode);
            } else {
                index.removeEntry(file);
            }
            std::cout << "Unstaged changes for " << file << "\n";
        }
        if (!index.save(".minigit")) {
            std::cerr << "fatal: failed to update index\n";
            return 1;
        }
    }

    return 0;
}
