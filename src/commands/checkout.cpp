#include "checkout.hpp"
#include "../repository/reference.hpp"
#include "../repository/working_tree.hpp"
#include "../objects/commit.hpp"
#include "../filesystem/file_utils.hpp"
#include <iostream>
#include <string>

int handleCheckout(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "fatal: you must specify a branch or commit to checkout\n"
                  << "usage: minigit checkout [-b] <branch-or-commit>\n";
        return 1;
    }

    if (!FileUtils::directoryExists(".minigit")) {
        std::cerr << "fatal: not a minigit repository (or any of the parent directories): .minigit\n";
        return 1;
    }

    bool createBranch = false;
    std::string targetName;

    if (std::string(argv[2]) == "-b") {
        if (argc < 4) {
            std::cerr << "fatal: missing branch name for -b\n";
            return 1;
        }
        createBranch = true;
        targetName = argv[3];
    } else {
        targetName = argv[2];
    }

    std::string prevHead;
    Repository::Reference::resolveHead(prevHead, ".minigit");

    if (createBranch) {
        if (Repository::Reference::branchExists(targetName, ".minigit")) {
            std::cerr << "fatal: A branch named '" << targetName << "' already exists.\n";
            return 1;
        }

        if (prevHead.empty()) {
            std::cerr << "fatal: cannot create branch from unborn HEAD\n";
            return 1;
        }

        Repository::Reference::setBranchCommit(targetName, prevHead, ".minigit");
        Repository::Reference::setHeadSymbolic("refs/heads/" + targetName, ".minigit");
        Repository::Reference::appendReflog("HEAD", prevHead, prevHead, "checkout: moving to " + targetName, ".minigit");
        std::cout << "Switched to a new branch '" << targetName << "'\n";
        return 0;
    }

    // Checking out existing branch or commit
    if (Repository::Reference::branchExists(targetName, ".minigit")) {
        std::string targetCommit;
        if (!Repository::Reference::getBranchCommit(targetName, targetCommit, ".minigit")) {
            std::cerr << "fatal: cannot find commit for branch '" << targetName << "'\n";
            return 1;
        }

        Objects::Commit commit;
        if (!Objects::Commit::load(targetCommit, commit, ".minigit")) {
            std::cerr << "fatal: failed to load commit object " << targetCommit << "\n";
            return 1;
        }

        if (!Repository::WorkingTree::checkoutTree(commit.getTreeHash(), false, ".minigit")) {
            return 1;
        }

        Repository::Reference::setHeadSymbolic("refs/heads/" + targetName, ".minigit");
        Repository::Reference::appendReflog("HEAD", prevHead, targetCommit, "checkout: moving to " + targetName, ".minigit");
        std::cout << "Switched to branch '" << targetName << "'\n";
        return 0;
    }

    // Checking out by commit hash / ref (detached HEAD)
    std::string commitHash;
    if (Repository::Reference::resolveRef(targetName, commitHash, ".minigit")) {
        Objects::Commit commit;
        if (!Objects::Commit::load(commitHash, commit, ".minigit")) {
            std::cerr << "fatal: reference is not a commit " << targetName << "\n";
            return 1;
        }

        if (!Repository::WorkingTree::checkoutTree(commit.getTreeHash(), false, ".minigit")) {
            return 1;
        }

        Repository::Reference::setHeadDetached(commitHash, ".minigit");
        Repository::Reference::appendReflog("HEAD", prevHead, commitHash, "checkout: moving to " + commitHash.substr(0, 7), ".minigit");
        std::cout << "Note: switching to '" << targetName << "'.\n\n"
                  << "You are in 'detached HEAD' state at " << commitHash.substr(0, 7) << "\n";
        return 0;
    }

    std::cerr << "error: pathspec '" << targetName << "' did not match any file(s) or branch known to minigit\n";
    return 1;
}
