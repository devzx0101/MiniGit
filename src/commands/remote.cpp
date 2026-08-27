#include "remote.hpp"
#include "../repository/remote_manager.hpp"
#include "../repository/reference.hpp"
#include "../repository/merge_engine.hpp"
#include "../filesystem/file_utils.hpp"
#include <iostream>
#include <string>

int handleRemote(int argc, char* argv[]) {
    if (!FileUtils::directoryExists(".minigit")) {
        std::cerr << "fatal: not a minigit repository (or any of the parent directories): .minigit\n";
        return 1;
    }

    if (argc == 2 || (argc == 3 && (std::string(argv[2]) == "-v" || std::string(argv[2]) == "--verbose"))) {
        bool verbose = (argc == 3);
        auto remotes = Repository::RemoteManager::listRemotes(".minigit");
        for (const auto& pair : remotes) {
            if (verbose) {
                std::cout << pair.first << "\t" << pair.second << " (fetch)\n";
                std::cout << pair.first << "\t" << pair.second << " (push)\n";
            } else {
                std::cout << pair.first << "\n";
            }
        }
        return 0;
    }

    std::string subCmd = argv[2];
    if (subCmd == "add") {
        if (argc < 5) {
            std::cerr << "usage: minigit remote add <name> <url/path>\n";
            return 1;
        }
        std::string name = argv[3];
        std::string path = argv[4];
        if (Repository::RemoteManager::addRemote(name, path, ".minigit")) {
            return 0;
        } else {
            std::cerr << "error: failed to add remote '" << name << "'\n";
            return 1;
        }
    } else if (subCmd == "remove" || subCmd == "rm") {
        if (argc < 4) {
            std::cerr << "usage: minigit remote remove <name>\n";
            return 1;
        }
        std::string name = argv[3];
        if (Repository::RemoteManager::removeRemote(name, ".minigit")) {
            return 0;
        } else {
            std::cerr << "error: Could not remove config section 'remote." << name << "'\n";
            return 1;
        }
    }

    return 1;
}

int handleClone(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "fatal: You must specify a repository to clone.\n"
                  << "usage: minigit clone <repository> [<directory>]\n";
        return 1;
    }

    std::string sourcePath = argv[2];
    std::string targetPath;

    if (argc >= 4) {
        targetPath = argv[3];
    } else {
        // Derive directory name from source path
        std::string clean = FileUtils::normalizePath(sourcePath);
        while (!clean.empty() && clean.back() == '/') clean.pop_back();
        size_t lastSlash = clean.find_last_of('/');
        targetPath = (lastSlash == std::string::npos) ? clean : clean.substr(lastSlash + 1);
    }

    std::cout << "Cloning into '" << targetPath << "'...\n";
    if (Repository::RemoteManager::cloneRepo(sourcePath, targetPath)) {
        std::cout << "done.\n";
        return 0;
    }
    return 1;
}

int handleFetch(int argc, char* argv[]) {
    if (!FileUtils::directoryExists(".minigit")) {
        std::cerr << "fatal: not a minigit repository (or any of the parent directories): .minigit\n";
        return 1;
    }

    std::string remoteName = (argc >= 3) ? argv[2] : "origin";
    if (Repository::RemoteManager::fetchRemote(remoteName, ".minigit")) {
        std::cout << "From " << Repository::RemoteManager::getRemotePath(remoteName, ".minigit") << "\n"
                  << " * [new tracking branch] " << remoteName << " -> " << remoteName << "/*\n";
        return 0;
    }
    return 1;
}

int handlePush(int argc, char* argv[]) {
    if (!FileUtils::directoryExists(".minigit")) {
        std::cerr << "fatal: not a minigit repository (or any of the parent directories): .minigit\n";
        return 1;
    }

    std::string remoteName = (argc >= 3) ? argv[2] : "origin";
    std::string branchName;
    if (argc >= 4) {
        branchName = argv[3];
    } else {
        branchName = Repository::Reference::getCurrentBranchName(".minigit");
    }

    if (branchName.empty()) {
        std::cerr << "fatal: You are not currently on a branch.\n";
        return 1;
    }

    if (Repository::RemoteManager::pushRemote(remoteName, branchName, ".minigit")) {
        std::string targetCommit;
        Repository::Reference::getBranchCommit(branchName, targetCommit, ".minigit");
        std::cout << "To " << Repository::RemoteManager::getRemotePath(remoteName, ".minigit") << "\n"
                  << " * [new branch]      " << branchName << " -> " << branchName << "\n";
        return 0;
    }
    return 1;
}

int handlePull(int argc, char* argv[]) {
    if (!FileUtils::directoryExists(".minigit")) {
        std::cerr << "fatal: not a minigit repository (or any of the parent directories): .minigit\n";
        return 1;
    }

    std::string remoteName = (argc >= 3) ? argv[2] : "origin";
    std::string branchName;
    if (argc >= 4) {
        branchName = argv[3];
    } else {
        branchName = Repository::Reference::getCurrentBranchName(".minigit");
    }

    if (branchName.empty()) {
        std::cerr << "fatal: You are not currently on a branch.\n";
        return 1;
    }

    if (!Repository::RemoteManager::fetchRemote(remoteName, ".minigit")) {
        return 1;
    }

    // Merge remote tracking branch commit
    std::string remoteRefPath = ".minigit/refs/remotes/" + remoteName + "/" + branchName;
    std::string remoteCommit;
    if (!FileUtils::readFile(remoteRefPath, remoteCommit)) {
        std::cerr << "fatal: couldn't find remote ref for " << remoteName << "/" << branchName << "\n";
        return 1;
    }
    while (!remoteCommit.empty() && (remoteCommit.back() == '\n' || remoteCommit.back() == '\r' || remoteCommit.back() == ' ')) {
        remoteCommit.pop_back();
    }

    auto result = Repository::MergeEngine::merge(remoteCommit, ".minigit");
    if (result.isAlreadyUpToDate) {
        std::cout << "Already up to date.\n";
        return 0;
    }
    if (result.isFastForward) {
        std::cout << "Updating " << result.baseCommitHash.substr(0, 7) << ".." << result.targetCommitHash.substr(0, 7) << "\n"
                  << "Fast-forward\n";
        return 0;
    }
    if (result.hasConflicts) {
        for (const auto& file : result.conflictFiles) {
            std::cout << "CONFLICT (content): Merge conflict in " << file << "\n";
        }
        std::cout << "Automatic merge failed; fix conflicts and then commit the result.\n";
        return 1;
    }

    std::cout << "Merge made by the 'ort' strategy.\n";
    return 0;
}
