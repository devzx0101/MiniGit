#ifndef REMOTE_MANAGER_HPP
#define REMOTE_MANAGER_HPP

#include <string>
#include <vector>
#include <map>

namespace Repository {

struct RemoteConfig {
    std::string name;
    std::string path;
};

class RemoteManager {
public:
    // Add a remote
    static bool addRemote(const std::string& name, const std::string& path, const std::string& repoDir = ".minigit");

    // Remove a remote
    static bool removeRemote(const std::string& name, const std::string& repoDir = ".minigit");

    // Get remote path
    static std::string getRemotePath(const std::string& name, const std::string& repoDir = ".minigit");

    // List all remotes
    static std::map<std::string, std::string> listRemotes(const std::string& repoDir = ".minigit");

    // Clone a local repository to target destination
    static bool cloneRepo(const std::string& sourcePath, const std::string& targetPath);

    // Fetch from a remote
    static bool fetchRemote(const std::string& remoteName, const std::string& repoDir = ".minigit");

    // Push local branch to remote repository
    static bool pushRemote(const std::string& remoteName, const std::string& branchName, const std::string& repoDir = ".minigit");
};

} // namespace Repository

#endif // REMOTE_MANAGER_HPP
