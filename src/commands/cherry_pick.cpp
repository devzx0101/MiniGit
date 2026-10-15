#include "commands/cherry_pick.hpp"
#include "repository/reference.hpp"
#include "repository/index.hpp"
#include "repository/working_tree.hpp"
#include "repository/merge_engine.hpp"
#include "objects/commit.hpp"
#include "objects/tree.hpp"
#include "objects/blob.hpp"
#include "filesystem/file_utils.hpp"
#include "terminal.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>

int handleCherryPick(int argc, char* argv[]) {
    if (!FileUtils::directoryExists(".minigit")) {
        std::cerr << "fatal: not a minigit repository (or any of the parent directories): .minigit\n";
        return 1;
    }

    if (argc < 3) {
        std::cerr << "usage: minigit cherry-pick <commit>\n";
        return 1;
    }

    std::string targetArg = argv[2];
    std::string targetCommitHash;
    if (!Repository::Reference::resolveRef(targetArg, targetCommitHash, ".minigit")) {
        std::cerr << "fatal: bad revision '" << targetArg << "'\n";
        return 1;
    }

    std::string headCommitHash;
    if (!Repository::Reference::resolveHead(headCommitHash, ".minigit")) {
        std::cerr << "fatal: cannot cherry-pick on an empty branch\n";
        return 1;
    }

    auto status = Repository::WorkingTree::getStatus(".minigit");
    if (status.hasStagedChanges() || status.hasUnstagedChanges()) {
        std::cerr << "error: your local changes would be overwritten by cherry-pick.\n"
                  << "hint: commit your changes or stash them to proceed.\n";
        return 1;
    }

    Objects::Commit targetCommit;
    if (!Objects::Commit::load(targetCommitHash, targetCommit, ".minigit")) {
        std::cerr << "fatal: could not load commit object " << targetCommitHash << "\n";
        return 1;
    }

    Objects::Commit headCommit;
    if (!Objects::Commit::load(headCommitHash, headCommit, ".minigit")) {
        std::cerr << "fatal: could not load HEAD commit " << headCommitHash << "\n";
        return 1;
    }

    std::map<std::string, Repository::IndexEntry> baseFiles;
    std::string parentHash = targetCommit.getFirstParentHash();
    if (!parentHash.empty()) {
        Objects::Commit parentCommit;
        if (Objects::Commit::load(parentHash, parentCommit, ".minigit")) {
            Objects::Tree::flatten(parentCommit.getTreeHash(), baseFiles, ".minigit");
        }
    }

    std::map<std::string, Repository::IndexEntry> headFiles;
    Objects::Tree::flatten(headCommit.getTreeHash(), headFiles, ".minigit");

    std::map<std::string, Repository::IndexEntry> targetFiles;
    Objects::Tree::flatten(targetCommit.getTreeHash(), targetFiles, ".minigit");

    // Compute all involved files
    std::set<std::string> allFiles;
    for (const auto& p : baseFiles) allFiles.insert(p.first);
    for (const auto& p : headFiles) allFiles.insert(p.first);
    for (const auto& p : targetFiles) allFiles.insert(p.first);

    Repository::Index index;
    index.load(".minigit");

    bool hasConflicts = false;
    std::vector<std::string> conflictFiles;

    for (const auto& file : allFiles) {
        bool inBase = baseFiles.count(file) > 0;
        bool inHead = headFiles.count(file) > 0;
        bool inTarget = targetFiles.count(file) > 0;

        std::string baseHash = inBase ? baseFiles[file].hash : "";
        std::string headHash = inHead ? headFiles[file].hash : "";
        std::string targetHash = inTarget ? targetFiles[file].hash : "";

        // If target made no changes to this file compared to base, keep head
        if (inBase && inTarget && baseHash == targetHash) {
            continue;
        }

        // Target added file
        if (!inBase && inTarget) {
            if (!inHead) {
                // Clean add
                Objects::Blob b;
                if (Objects::Blob::load(targetHash, b, ".minigit")) {
                    FileUtils::writeFile(file, b.getContent());
                    index.addEntry(file, targetHash);
                }
            } else if (headHash != targetHash) {
                // Conflict: both added differently
                hasConflicts = true;
                conflictFiles.push_back(file);
            }
            continue;
        }

        // Target deleted file
        if (inBase && !inTarget) {
            if (inHead) {
                if (headHash == baseHash) {
                    // Clean delete
                    FileUtils::removeFile(file);
                    index.removeEntry(file);
                } else {
                    hasConflicts = true;
                    conflictFiles.push_back(file);
                }
            }
            continue;
        }

        // Target modified file
        if (inBase && inTarget && baseHash != targetHash) {
            if (!inHead) {
                hasConflicts = true;
                conflictFiles.push_back(file);
            } else if (headHash == baseHash) {
                // Clean update from target
                Objects::Blob b;
                if (Objects::Blob::load(targetHash, b, ".minigit")) {
                    FileUtils::writeFile(file, b.getContent());
                    index.addEntry(file, targetHash);
                }
            } else if (headHash != targetHash) {
                // 3-way line merge
                Objects::Blob baseB, headB, targetB;
                Objects::Blob::load(baseHash, baseB, ".minigit");
                Objects::Blob::load(headHash, headB, ".minigit");
                Objects::Blob::load(targetHash, targetB, ".minigit");

                std::string mergedContent;
                bool clean = Repository::MergeEngine::merge3WayLines(
                    baseB.getContent(),
                    headB.getContent(),
                    targetB.getContent(),
                    "HEAD",
                    targetCommitHash.substr(0, 7),
                    mergedContent
                );

                FileUtils::writeFile(file, mergedContent);
                if (clean) {
                    Objects::Blob newBlob(mergedContent);
                    std::string newBlobHash = newBlob.save(".minigit");
                    index.addEntry(file, newBlobHash);
                } else {
                    hasConflicts = true;
                    conflictFiles.push_back(file);
                }
            }
        }
    }

    index.save(".minigit");

    if (hasConflicts) {
        std::cerr << Color::BoldRed << "CONFLICT: " << Color::Reset 
                  << "Merge conflict in files:\n";
        for (const auto& f : conflictFiles) {
            std::cerr << "  " << f << "\n";
        }
        std::cerr << "Automatic cherry-pick failed. Fix conflicts and commit the result.\n";
        return 1;
    }

    // Write new commit
    std::string newTreeHash = Objects::Tree::buildFromIndex(index, ".minigit");
    Objects::Commit newCommit;
    newCommit.setTreeHash(newTreeHash);
    newCommit.addParentHash(headCommitHash);
    newCommit.setAuthor(targetCommit.getAuthor());
    newCommit.setMessage(targetCommit.getMessage());
    std::string newCommitHash = newCommit.save(".minigit");

    // Advance branch / HEAD
    std::string branchName = Repository::Reference::getCurrentBranchName(".minigit");
    if (!branchName.empty()) {
        Repository::Reference::setBranchCommit(branchName, newCommitHash, ".minigit");
    } else {
        Repository::Reference::setHeadDetached(newCommitHash, ".minigit");
    }

    // Reflog
    Repository::Reference::appendReflog("HEAD", headCommitHash, newCommitHash, 
                                        "cherry-pick: " + targetCommit.getMessage(), ".minigit");

    std::string firstLine = targetCommit.getMessage();
    size_t nl = firstLine.find('\n');
    if (nl != std::string::npos) firstLine = firstLine.substr(0, nl);

    std::cout << "[" << (branchName.empty() ? "detached HEAD" : branchName) << " " 
              << Color::BoldYellow << newCommitHash.substr(0, 7) << Color::Reset << "] " 
              << firstLine << "\n";

    return 0;
}
