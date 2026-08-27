#ifndef MERGE_ENGINE_HPP
#define MERGE_ENGINE_HPP

#include <string>
#include <vector>
#include <map>
#include "index.hpp"

namespace Repository {

struct MergeResult {
    bool isFastForward;
    bool isAlreadyUpToDate;
    bool hasConflicts;
    std::string baseCommitHash;
    std::string targetCommitHash;
    std::vector<std::string> conflictFiles;
    std::vector<std::string> autoMergedFiles;
    std::string newCommitHash;
};

class MergeEngine {
public:
    // Find Lowest Common Ancestor (LCA) of two commits in the DAG
    static std::string findLCA(const std::string& commitA, const std::string& commitB, const std::string& repoDir = ".minigit");

    // Perform full 3-way merge of target branch/commit into current HEAD
    static MergeResult merge(const std::string& targetBranchOrCommit, const std::string& repoDir = ".minigit");

    // Perform line-level 3-way merge between base, ours, and theirs
    static bool merge3WayLines(const std::string& baseContent,
                              const std::string& ourContent,
                              const std::string& theirContent,
                              const std::string& ourBranchLabel,
                              const std::string& theirBranchLabel,
                              std::string& outMergedContent);
};

} // namespace Repository

#endif // MERGE_ENGINE_HPP
