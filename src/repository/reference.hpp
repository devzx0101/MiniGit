#ifndef REFERENCE_HPP
#define REFERENCE_HPP

#include <string>
#include <vector>
#include <utility>

namespace Repository {

struct ReflogEntry {
    std::string oldHash;
    std::string newHash;
    std::string committer;
    std::string timestamp;
    std::string message;
};

class Reference {
public:
    // HEAD operations
    static bool getHead(std::string& outTarget, bool& isSymbolic, const std::string& repoDir = ".minigit");
    static bool setHeadSymbolic(const std::string& branchRef, const std::string& repoDir = ".minigit");
    static bool setHeadDetached(const std::string& commitHash, const std::string& repoDir = ".minigit");
    static std::string getCurrentBranchName(const std::string& repoDir = ".minigit");
    static bool resolveHead(std::string& outCommitHash, const std::string& repoDir = ".minigit");

    // Branch operations
    static bool branchExists(const std::string& branchName, const std::string& repoDir = ".minigit");
    static bool getBranchCommit(const std::string& branchName, std::string& outCommitHash, const std::string& repoDir = ".minigit");
    static bool setBranchCommit(const std::string& branchName, const std::string& commitHash, const std::string& repoDir = ".minigit");
    static bool deleteBranch(const std::string& branchName, const std::string& repoDir = ".minigit");
    static std::vector<std::pair<std::string, bool>> listBranches(const std::string& repoDir = ".minigit");

    // Tag operations
    static bool tagExists(const std::string& tagName, const std::string& repoDir = ".minigit");
    static bool getTagCommit(const std::string& tagName, std::string& outCommitHash, const std::string& repoDir = ".minigit");
    static bool setTag(const std::string& tagName, const std::string& commitHash, const std::string& repoDir = ".minigit");
    static bool deleteTag(const std::string& tagName, const std::string& repoDir = ".minigit");
    static std::vector<std::string> listTags(const std::string& repoDir = ".minigit");

    // General reference resolution (branch name, tag, or 40-char / short SHA-1 hash)
    static bool resolveRef(const std::string& refOrHash, std::string& outCommitHash, const std::string& repoDir = ".minigit");

    // Reflog operations
    static bool appendReflog(const std::string& refPath, 
                             const std::string& oldHash, 
                             const std::string& newHash, 
                             const std::string& message, 
                             const std::string& repoDir = ".minigit");
    static std::vector<ReflogEntry> readReflog(const std::string& refPath = "HEAD", const std::string& repoDir = ".minigit");
};

} // namespace Repository

#endif // REFERENCE_HPP
