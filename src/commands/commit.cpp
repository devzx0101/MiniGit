#include "commands/commit.hpp"
#include "repository/index.hpp"
#include "repository/reference.hpp"
#include "objects/tree.hpp"
#include "objects/commit.hpp"
#include "filesystem/file_utils.hpp"
#include <iostream>
#include <string>

int handleCommit(int argc, char* argv[]) {
    if (!FileUtils::directoryExists(".minigit")) {
        std::cerr << "fatal: not a minigit repository (or any of the parent directories): .minigit\n";
        return 1;
    }

    std::string message;
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "-m" || arg == "--message") && i + 1 < argc) {
            message = argv[++i];
        }
    }

    if (message.empty()) {
        std::cerr << "error: switch 'm' requires a value\n"
                  << "usage: minigit commit -m <msg>\n";
        return 1;
    }

    Repository::Index index;
    if (!index.load(".minigit")) {
        std::cerr << "fatal: unable to read index file\n";
        return 1;
    }

    if (index.getEntries().empty()) {
        std::cerr << "nothing to commit, working tree clean\n";
        return 1;
    }

    // Build root tree from index
    std::string treeHash = Objects::Tree::buildFromIndex(index, ".minigit");
    if (treeHash.empty()) {
        std::cerr << "fatal: failed to write tree object from index\n";
        return 1;
    }

    // Resolve parent commit (if any)
    std::string parentHash;
    bool hasParent = Repository::Reference::resolveHead(parentHash, ".minigit");

    // Check if tree has changed compared to parent commit
    if (hasParent) {
        Objects::Commit parentCommit;
        if (Objects::Commit::load(parentHash, parentCommit, ".minigit")) {
            if (parentCommit.getTreeHash() == treeHash) {
                std::cout << "nothing to commit, working tree clean\n";
                return 0;
            }
        }
    }

    // Create commit object
    Objects::Commit commit;
    commit.setTreeHash(treeHash);
    if (hasParent) {
        commit.addParentHash(parentHash);
    }
    commit.setMessage(message);

    std::string commitHash = commit.save(".minigit");
    if (commitHash.empty()) {
        std::cerr << "fatal: failed to store commit object\n";
        return 1;
    }

    // Update branch reference or detached HEAD
    std::string branchName = Repository::Reference::getCurrentBranchName(".minigit");
    if (!branchName.empty()) {
        Repository::Reference::setBranchCommit(branchName, commitHash, ".minigit");
        Repository::Reference::appendReflog("refs/heads/" + branchName, parentHash, commitHash, "commit: " + message, ".minigit");
    } else {
        Repository::Reference::setHeadDetached(commitHash, ".minigit");
    }
    Repository::Reference::appendReflog("HEAD", parentHash, commitHash, "commit: " + message, ".minigit");

    // Pretty summary output
    std::string displayName = branchName.empty() ? "(detached HEAD)" : branchName;
    std::string shortHash = commitHash.substr(0, 7);
    std::string firstLine = message;
    size_t nlPos = firstLine.find('\n');
    if (nlPos != std::string::npos) {
        firstLine = firstLine.substr(0, nlPos);
    }

    size_t fileCount = index.getEntries().size();
    std::cout << "[" << displayName << " " << shortHash << "] " << firstLine << "\n";
    std::cout << " " << fileCount << " file" << (fileCount == 1 ? "" : "s") << " tracked\n";

    return 0;
}
