#include "working_tree.hpp"
#include "reference.hpp"
#include "ignore.hpp"
#include "../objects/tree.hpp"
#include "../objects/commit.hpp"
#include "../objects/blob.hpp"
#include "../filesystem/file_utils.hpp"
#include <iostream>
#include <set>

namespace Repository {

WorkingTreeStatus WorkingTree::getStatus(const std::string& repoDir) {
    WorkingTreeStatus status;

    // 1. Load staging area (Index)
    Index index;
    index.load(repoDir);
    const auto& indexEntries = index.getEntries();

    // 2. Load HEAD tree files (if HEAD commit exists)
    std::map<std::string, IndexEntry> headFiles;
    std::string headCommitHash;
    if (Reference::resolveHead(headCommitHash, repoDir)) {
        Objects::Commit headCommit;
        if (Objects::Commit::load(headCommitHash, headCommit, repoDir)) {
            Objects::Tree::flatten(headCommit.getTreeHash(), headFiles, repoDir);
        }
    }

    // 3. Load Ignore patterns
    Ignore ignore;
    ignore.load(".");

    // 4. Compare Index vs HEAD (Staged Changes)
    for (const auto& pair : indexEntries) {
        const std::string& path = pair.first;
        const IndexEntry& ie = pair.second;

        auto headIt = headFiles.find(path);
        if (headIt == headFiles.end()) {
            status.stagedNew[path] = ie;
        } else if (headIt->second.hash != ie.hash) {
            status.stagedModified[path] = ie;
        }
    }

    for (const auto& pair : headFiles) {
        const std::string& path = pair.first;
        if (indexEntries.find(path) == indexEntries.end()) {
            status.stagedDeleted.push_back(path);
        }
    }

    // 5. Compare Working Directory vs Index (Unstaged & Untracked)
    for (const auto& pair : indexEntries) {
        const std::string& path = pair.first;
        const IndexEntry& ie = pair.second;

        if (!FileUtils::fileExists(path)) {
            status.unstagedDeleted.push_back(path);
        } else {
            Objects::Blob currentBlob;
            if (Objects::Blob::fromFile(path, currentBlob)) {
                if (currentBlob.getHash() != ie.hash) {
                    status.unstagedModified.push_back(path);
                }
            }
        }
    }

    // Scan all on-disk files in current working directory
    std::vector<std::string> diskFiles = FileUtils::listFiles(".", true);
    for (const auto& f : diskFiles) {
        std::string norm = FileUtils::normalizePath(f);
        if (ignore.isIgnored(norm)) {
            continue;
        }
        if (indexEntries.find(norm) == indexEntries.end()) {
            status.untracked.push_back(norm);
        }
    }

    return status;
}

bool WorkingTree::restoreFile(const std::string& filePath, bool fromIndex, const std::string& repoDir) {
    std::string normPath = FileUtils::normalizePath(filePath);
    std::string targetHash;

    if (fromIndex) {
        Index index;
        if (!index.load(repoDir)) return false;
        const auto& entries = index.getEntries();
        auto it = entries.find(normPath);
        if (it == entries.end()) return false;
        targetHash = it->second.hash;
    } else {
        std::string headCommitHash;
        if (!Reference::resolveHead(headCommitHash, repoDir)) return false;
        Objects::Commit headCommit;
        if (!Objects::Commit::load(headCommitHash, headCommit, repoDir)) return false;
        std::map<std::string, IndexEntry> headFiles;
        if (!Objects::Tree::flatten(headCommit.getTreeHash(), headFiles, repoDir)) return false;
        auto it = headFiles.find(normPath);
        if (it == headFiles.end()) return false;
        targetHash = it->second.hash;
    }

    Objects::Blob blob;
    if (!Objects::Blob::load(targetHash, blob, repoDir)) {
        return false;
    }

    return FileUtils::writeFile(normPath, blob.getContent());
}

bool WorkingTree::canSafelyCheckout(const std::string& targetTreeHash, std::string& outConflictFile, const std::string& repoDir) {
    std::map<std::string, IndexEntry> targetFiles;
    if (!Objects::Tree::flatten(targetTreeHash, targetFiles, repoDir)) {
        return false;
    }

    Index index;
    index.load(repoDir);
    const auto& indexEntries = index.getEntries();

    // Check if any untracked or locally modified file would be overwritten
    for (const auto& pair : targetFiles) {
        const std::string& path = pair.first;
        const std::string& targetHash = pair.second.hash;

        if (FileUtils::fileExists(path)) {
            Objects::Blob currentBlob;
            if (Objects::Blob::fromFile(path, currentBlob)) {
                auto indexIt = indexEntries.find(path);
                if (indexIt == indexEntries.end()) {
                    // Untracked file on disk would be overwritten by checkout
                    if (currentBlob.getHash() != targetHash) {
                        outConflictFile = path;
                        return false;
                    }
                } else if (indexIt->second.hash != currentBlob.getHash()) {
                    // Modified tracked file would be overwritten
                    if (currentBlob.getHash() != targetHash) {
                        outConflictFile = path;
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

bool WorkingTree::checkoutTree(const std::string& targetTreeHash, bool force, const std::string& repoDir) {
    std::map<std::string, IndexEntry> targetFiles;
    if (!Objects::Tree::flatten(targetTreeHash, targetFiles, repoDir)) {
        return false;
    }

    // Safety check
    if (!force) {
        std::string conflictFile;
        if (!canSafelyCheckout(targetTreeHash, conflictFile, repoDir)) {
            std::cerr << "error: Your local changes to the following file would be overwritten by checkout:\n"
                      << "        " << conflictFile << "\n"
                      << "Please commit your changes or discard them before you switch branches.\n";
            return false;
        }
    }

    // Load current HEAD files to clean up files no longer present in target
    std::map<std::string, IndexEntry> headFiles;
    std::string headCommitHash;
    if (Reference::resolveHead(headCommitHash, repoDir)) {
        Objects::Commit headCommit;
        if (Objects::Commit::load(headCommitHash, headCommit, repoDir)) {
            Objects::Tree::flatten(headCommit.getTreeHash(), headFiles, repoDir);
        }
    }

    // Remove old tracked files not in target tree
    for (const auto& pair : headFiles) {
        const std::string& oldPath = pair.first;
        if (targetFiles.find(oldPath) == targetFiles.end()) {
            FileUtils::removeFile(oldPath);
        }
    }

    // Write all target files to working directory
    Index newIndex;
    for (const auto& pair : targetFiles) {
        const std::string& path = pair.first;
        const IndexEntry& ie = pair.second;

        Objects::Blob blob;
        if (Objects::Blob::load(ie.hash, blob, repoDir)) {
            FileUtils::writeFile(path, blob.getContent());
            newIndex.addEntry(path, ie.hash, ie.mode);
        } else {
            std::cerr << "error: unable to read blob " << ie.hash << " for " << path << "\n";
            return false;
        }
    }

    // Save updated index matching target tree
    return newIndex.save(repoDir);
}

} // namespace Repository
