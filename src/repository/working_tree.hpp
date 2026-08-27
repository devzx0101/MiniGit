#ifndef WORKING_TREE_HPP
#define WORKING_TREE_HPP

#include <string>
#include <vector>
#include <map>
#include "index.hpp"

namespace Repository {

struct WorkingTreeStatus {
    std::map<std::string, IndexEntry> stagedNew;
    std::map<std::string, IndexEntry> stagedModified;
    std::vector<std::string> stagedDeleted;

    std::vector<std::string> unstagedModified;
    std::vector<std::string> unstagedDeleted;

    std::vector<std::string> untracked;

    bool hasStagedChanges() const {
        return !stagedNew.empty() || !stagedModified.empty() || !stagedDeleted.empty();
    }

    bool hasUnstagedChanges() const {
        return !unstagedModified.empty() || !unstagedDeleted.empty();
    }

    bool isClean() const {
        return !hasStagedChanges() && !hasUnstagedChanges() && untracked.empty();
    }
};

class WorkingTree {
public:
    // Compute the 3-way status: Working Directory vs Index vs HEAD
    static WorkingTreeStatus getStatus(const std::string& repoDir = ".minigit");

    // Restore a single file on disk from Index or HEAD
    static bool restoreFile(const std::string& filePath, bool fromIndex = true, const std::string& repoDir = ".minigit");

    // Checkout all files from a target tree hash safely into working directory and index
    static bool checkoutTree(const std::string& targetTreeHash, bool force = false, const std::string& repoDir = ".minigit");

    // Check if working directory has uncommitted tracked modifications that would conflict
    static bool canSafelyCheckout(const std::string& targetTreeHash, std::string& outConflictFile, const std::string& repoDir = ".minigit");
};

} // namespace Repository

#endif // WORKING_TREE_HPP
