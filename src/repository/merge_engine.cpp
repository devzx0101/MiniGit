#include "merge_engine.hpp"
#include "reference.hpp"
#include "working_tree.hpp"
#include "../objects/commit.hpp"
#include "../objects/tree.hpp"
#include "../objects/blob.hpp"
#include "../filesystem/file_utils.hpp"
#include <queue>
#include <set>
#include <sstream>
#include <iostream>

namespace Repository {

std::string MergeEngine::findLCA(const std::string& commitA, const std::string& commitB, const std::string& repoDir) {
    if (commitA == commitB) {
        return commitA;
    }

    // Traverse all ancestors of commitA
    std::set<std::string> ancestorsA;
    std::queue<std::string> qA;
    qA.push(commitA);
    ancestorsA.insert(commitA);

    while (!qA.empty()) {
        std::string curr = qA.front();
        qA.pop();

        Objects::Commit c;
        if (Objects::Commit::load(curr, c, repoDir)) {
            for (const auto& p : c.getParentHashes()) {
                if (ancestorsA.find(p) == ancestorsA.end()) {
                    ancestorsA.insert(p);
                    qA.push(p);
                }
            }
        }
    }

    // BFS from commitB until we find the first ancestor that is also in ancestorsA
    std::queue<std::string> qB;
    std::set<std::string> visitedB;
    qB.push(commitB);
    visitedB.insert(commitB);

    while (!qB.empty()) {
        std::string curr = qB.front();
        qB.pop();

        if (ancestorsA.count(curr)) {
            return curr;
        }

        Objects::Commit c;
        if (Objects::Commit::load(curr, c, repoDir)) {
            for (const auto& p : c.getParentHashes()) {
                if (visitedB.find(p) == visitedB.end()) {
                    visitedB.insert(p);
                    qB.push(p);
                }
            }
        }
    }

    return "";
}

bool MergeEngine::merge3WayLines(const std::string& baseContent,
                                const std::string& ourContent,
                                const std::string& theirContent,
                                const std::string& ourBranchLabel,
                                const std::string& theirBranchLabel,
                                std::string& outMergedContent) {
    if (ourContent == theirContent) {
        outMergedContent = ourContent;
        return true; // No conflict
    }

    if (baseContent == ourContent) {
        outMergedContent = theirContent;
        return true; // No conflict
    }

    if (baseContent == theirContent) {
        outMergedContent = ourContent;
        return true; // No conflict
    }

    // Both modified differently: create conflict block
    std::ostringstream ss;
    ss << "<<<<<<< " << ourBranchLabel << "\n";
    ss << ourContent;
    if (!ourContent.empty() && ourContent.back() != '\n') ss << "\n";
    ss << "=======\n";
    ss << theirContent;
    if (!theirContent.empty() && theirContent.back() != '\n') ss << "\n";
    ss << ">>>>>>> " << theirBranchLabel << "\n";

    outMergedContent = ss.str();
    return false; // Conflict present!
}

MergeResult MergeEngine::merge(const std::string& targetBranchOrCommit, const std::string& repoDir) {
    MergeResult result;
    result.isFastForward = false;
    result.isAlreadyUpToDate = false;
    result.hasConflicts = false;

    std::string headCommitHash;
    if (!Reference::resolveHead(headCommitHash, repoDir)) {
        return result;
    }

    std::string targetCommitHash;
    if (!Reference::resolveRef(targetBranchOrCommit, targetCommitHash, repoDir)) {
        return result;
    }
    result.targetCommitHash = targetCommitHash;

    // Check if LCA is one of the commits
    std::string lca = findLCA(headCommitHash, targetCommitHash, repoDir);
    result.baseCommitHash = lca;

    if (lca == targetCommitHash) {
        result.isAlreadyUpToDate = true;
        return result;
    }

    if (lca == headCommitHash) {
        // Fast-Forward merge
        result.isFastForward = true;
        Objects::Commit targetCommit;
        if (Objects::Commit::load(targetCommitHash, targetCommit, repoDir)) {
            WorkingTree::checkoutTree(targetCommit.getTreeHash(), true, repoDir);
            std::string branch = Reference::getCurrentBranchName(repoDir);
            if (!branch.empty()) {
                Reference::setBranchCommit(branch, targetCommitHash, repoDir);
                Reference::appendReflog("refs/heads/" + branch, headCommitHash, targetCommitHash, "merge " + targetBranchOrCommit + ": Fast-forward", repoDir);
            } else {
                Reference::setHeadDetached(targetCommitHash, repoDir);
            }
            Reference::appendReflog("HEAD", headCommitHash, targetCommitHash, "merge " + targetBranchOrCommit + ": Fast-forward", repoDir);
        }
        return result;
    }

    // 3-Way Merge
    std::map<std::string, IndexEntry> baseFiles;
    if (!lca.empty()) {
        Objects::Commit baseCommit;
        if (Objects::Commit::load(lca, baseCommit, repoDir)) {
            Objects::Tree::flatten(baseCommit.getTreeHash(), baseFiles, repoDir);
        }
    }

    Objects::Commit headCommit, targetCommit;
    Objects::Commit::load(headCommitHash, headCommit, repoDir);
    Objects::Commit::load(targetCommitHash, targetCommit, repoDir);

    std::map<std::string, IndexEntry> ourFiles, theirFiles;
    Objects::Tree::flatten(headCommit.getTreeHash(), ourFiles, repoDir);
    Objects::Tree::flatten(targetCommit.getTreeHash(), theirFiles, repoDir);

    std::set<std::string> allPaths;
    for (const auto& p : baseFiles) allPaths.insert(p.first);
    for (const auto& p : ourFiles) allPaths.insert(p.first);
    for (const auto& p : theirFiles) allPaths.insert(p.first);

    std::string ourBranch = Reference::getCurrentBranchName(repoDir);
    if (ourBranch.empty()) ourBranch = "HEAD";

    Index mergedIndex;

    for (const auto& path : allPaths) {
        std::string baseHash = (baseFiles.count(path)) ? baseFiles[path].hash : "";
        std::string ourHash = (ourFiles.count(path)) ? ourFiles[path].hash : "";
        std::string theirHash = (theirFiles.count(path)) ? theirFiles[path].hash : "";

        if (ourHash == theirHash) {
            // Unchanged or identically modified
            if (!ourHash.empty()) {
                mergedIndex.addEntry(path, ourHash, ourFiles[path].mode);
            }
        } else if (baseHash == ourHash) {
            // Theirs changed, ours untouched -> take theirs
            if (!theirHash.empty()) {
                Objects::Blob blob;
                if (Objects::Blob::load(theirHash, blob, repoDir)) {
                    FileUtils::writeFile(path, blob.getContent());
                    mergedIndex.addEntry(path, theirHash, theirFiles[path].mode);
                    result.autoMergedFiles.push_back(path);
                }
            } else {
                // Deleted in theirs
                FileUtils::removeFile(path);
            }
        } else if (baseHash == theirHash) {
            // Ours changed, theirs untouched -> keep ours
            if (!ourHash.empty()) {
                mergedIndex.addEntry(path, ourHash, ourFiles[path].mode);
            } else {
                FileUtils::removeFile(path);
            }
        } else {
            // Both changed differently: 3-way content merge
            std::string baseContent, ourContent, theirContent;
            if (!baseHash.empty()) {
                Objects::Blob b;
                Objects::Blob::load(baseHash, b, repoDir);
                baseContent = b.getContent();
            }
            if (!ourHash.empty()) {
                Objects::Blob b;
                Objects::Blob::load(ourHash, b, repoDir);
                ourContent = b.getContent();
            }
            if (!theirHash.empty()) {
                Objects::Blob b;
                Objects::Blob::load(theirHash, b, repoDir);
                theirContent = b.getContent();
            }

            std::string mergedContent;
            bool clean = merge3WayLines(baseContent, ourContent, theirContent, ourBranch, targetBranchOrCommit, mergedContent);

            FileUtils::writeFile(path, mergedContent);

            if (clean) {
                Objects::Blob newBlob(mergedContent);
                std::string newBlobHash = newBlob.save(repoDir);
                mergedIndex.addEntry(path, newBlobHash, "100644");
                result.autoMergedFiles.push_back(path);
            } else {
                result.hasConflicts = true;
                result.conflictFiles.push_back(path);
            }
        }
    }

    mergedIndex.save(repoDir);

    // If clean merge, automatically create merge commit
    if (!result.hasConflicts) {
        std::string treeHash = Objects::Tree::buildFromIndex(mergedIndex, repoDir);
        Objects::Commit mergeCommit;
        mergeCommit.setTreeHash(treeHash);
        mergeCommit.addParentHash(headCommitHash);
        mergeCommit.addParentHash(targetCommitHash);
        mergeCommit.setMessage("Merge branch '" + targetBranchOrCommit + "'");

        std::string newCommitHash = mergeCommit.save(repoDir);
        result.newCommitHash = newCommitHash;

        if (!ourBranch.empty() && ourBranch != "HEAD") {
            Reference::setBranchCommit(ourBranch, newCommitHash, repoDir);
            Reference::appendReflog("refs/heads/" + ourBranch, headCommitHash, newCommitHash, "merge " + targetBranchOrCommit + ": Merge made by 'ort' strategy", repoDir);
        } else {
            Reference::setHeadDetached(newCommitHash, repoDir);
        }
        Reference::appendReflog("HEAD", headCommitHash, newCommitHash, "merge " + targetBranchOrCommit + ": Merge made by 'ort' strategy", repoDir);
    }

    return result;
}

} // namespace Repository
