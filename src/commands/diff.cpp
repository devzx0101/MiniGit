#include "diff.hpp"
#include "../repository/index.hpp"
#include "../repository/reference.hpp"
#include "../repository/diff_engine.hpp"
#include "../objects/blob.hpp"
#include "../objects/commit.hpp"
#include "../objects/tree.hpp"
#include "../filesystem/file_utils.hpp"
#include <iostream>
#include <string>

int handleDiff(int argc, char* argv[]) {
    if (!FileUtils::directoryExists(".minigit")) {
        std::cerr << "fatal: not a minigit repository (or any of the parent directories): .minigit\n";
        return 1;
    }

    bool staged = false;
    std::string targetFile;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--staged" || arg == "--cached") {
            staged = true;
        } else if (arg[0] != '-') {
            targetFile = FileUtils::normalizePath(arg);
        }
    }

    Repository::Index index;
    index.load(".minigit");
    const auto& indexEntries = index.getEntries();

    if (staged) {
        // Compare Index vs HEAD
        std::string headCommitHash;
        std::map<std::string, Repository::IndexEntry> headFiles;
        if (Repository::Reference::resolveHead(headCommitHash, ".minigit")) {
            Objects::Commit headCommit;
            if (Objects::Commit::load(headCommitHash, headCommit, ".minigit")) {
                Objects::Tree::flatten(headCommit.getTreeHash(), headFiles, ".minigit");
            }
        }

        for (const auto& pair : indexEntries) {
            const std::string& path = pair.first;
            if (!targetFile.empty() && path != targetFile) continue;

            std::string oldContent;
            auto headIt = headFiles.find(path);
            if (headIt != headFiles.end()) {
                Objects::Blob headBlob;
                if (Objects::Blob::load(headIt->second.hash, headBlob, ".minigit")) {
                    oldContent = headBlob.getContent();
                }
            }

            std::string newContent;
            Objects::Blob stagedBlob;
            if (Objects::Blob::load(pair.second.hash, stagedBlob, ".minigit")) {
                newContent = stagedBlob.getContent();
            }

            std::string diffOutput = Repository::DiffEngine::unifiedDiff(path, oldContent, newContent, "HEAD", "index");
            if (!diffOutput.empty()) {
                std::cout << diffOutput;
            }
        }
    } else {
        // Compare Working Directory vs Index
        for (const auto& pair : indexEntries) {
            const std::string& path = pair.first;
            if (!targetFile.empty() && path != targetFile) continue;

            std::string indexContent;
            Objects::Blob stagedBlob;
            if (Objects::Blob::load(pair.second.hash, stagedBlob, ".minigit")) {
                indexContent = stagedBlob.getContent();
            }

            std::string worktreeContent;
            if (FileUtils::fileExists(path)) {
                FileUtils::readFile(path, worktreeContent);
            }

            std::string diffOutput = Repository::DiffEngine::unifiedDiff(path, indexContent, worktreeContent, "index", "worktree");
            if (!diffOutput.empty()) {
                std::cout << diffOutput;
            }
        }
    }

    return 0;
}
