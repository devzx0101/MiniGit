#include "commands/restore.hpp"
#include "repository/index.hpp"
#include "repository/reference.hpp"
#include "repository/working_tree.hpp"
#include "objects/commit.hpp"
#include "objects/tree.hpp"
#include "filesystem/file_utils.hpp"
#include <iostream>
#include <vector>
#include <string>

int handleRestore(int argc, char* argv[]) {
    if (!FileUtils::directoryExists(".minigit")) {
        std::cerr << "fatal: not a minigit repository (or any of the parent directories): .minigit\n";
        return 1;
    }

    bool staged = false;
    std::string sourceRef = "HEAD";
    std::vector<std::string> files;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--staged" || arg == "-S") {
            staged = true;
        } else if (arg.rfind("--source=", 0) == 0) {
            sourceRef = arg.substr(9);
        } else if (arg[0] != '-') {
            files.push_back(FileUtils::normalizePath(arg));
        }
    }

    if (files.empty()) {
        std::cerr << "fatal: you must specify path(s) to restore\n";
        return 1;
    }

    if (staged) {
        // Unstage files from index (reset them to match source commit, default HEAD)
        std::string sourceCommitHash;
        std::map<std::string, Repository::IndexEntry> sourceFiles;
        if (Repository::Reference::resolveRef(sourceRef, sourceCommitHash, ".minigit")) {
            Objects::Commit commit;
            if (Objects::Commit::load(sourceCommitHash, commit, ".minigit")) {
                Objects::Tree::flatten(commit.getTreeHash(), sourceFiles, ".minigit");
            }
        }

        Repository::Index index;
        index.load(".minigit");

        for (const auto& file : files) {
            auto it = sourceFiles.find(file);
            if (it != sourceFiles.end()) {
                index.addEntry(file, it->second.hash, it->second.mode);
            } else {
                index.removeEntry(file);
            }
        }
        index.save(".minigit");
        return 0;
    } else {
        // Restore working copy files from index
        bool hasError = false;
        for (const auto& file : files) {
            if (!Repository::WorkingTree::restoreFile(file, true, ".minigit")) {
                std::cerr << "error: pathspec '" << file << "' did not match any file(s) known to minigit\n";
                hasError = true;
            }
        }
        return hasError ? 1 : 0;
    }
}
