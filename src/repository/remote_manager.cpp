#include "remote_manager.hpp"
#include "reference.hpp"
#include "working_tree.hpp"
#include "../objects/commit.hpp"
#include "../filesystem/file_utils.hpp"
#include <iostream>
#include <sstream>

namespace Repository {

static std::string getRemotesDir(const std::string& repoDir) {
    return repoDir + "/remotes";
}

bool RemoteManager::addRemote(const std::string& name, const std::string& path, const std::string& repoDir) {
    std::string rDir = getRemotesDir(repoDir);
    FileUtils::createDirectories(rDir);
    return FileUtils::writeFile(rDir + "/" + name, path + "\n");
}

bool RemoteManager::removeRemote(const std::string& name, const std::string& repoDir) {
    std::string rPath = getRemotesDir(repoDir) + "/" + name;
    return FileUtils::removeFile(rPath);
}

std::string RemoteManager::getRemotePath(const std::string& name, const std::string& repoDir) {
    std::string rPath = getRemotesDir(repoDir) + "/" + name;
    std::string content;
    if (!FileUtils::readFile(rPath, content)) {
        return "";
    }
    while (!content.empty() && (content.back() == '\n' || content.back() == '\r' || content.back() == ' ')) {
        content.pop_back();
    }
    return content;
}

std::map<std::string, std::string> RemoteManager::listRemotes(const std::string& repoDir) {
    std::map<std::string, std::string> result;
    std::string rDir = getRemotesDir(repoDir);
    auto files = FileUtils::listFiles(rDir, false);
    for (const auto& name : files) {
        std::string path = getRemotePath(name, repoDir);
        if (!path.empty()) {
            result[name] = path;
        }
    }
    return result;
}

bool RemoteManager::cloneRepo(const std::string& sourcePath, const std::string& targetPath) {
    std::string srcRepo = sourcePath + "/.minigit";
    if (!FileUtils::directoryExists(srcRepo)) {
        std::cerr << "fatal: repository '" << sourcePath << "' does not exist\n";
        return false;
    }

    if (FileUtils::directoryExists(targetPath)) {
        auto existing = FileUtils::listFiles(targetPath, false);
        if (!existing.empty()) {
            std::cerr << "fatal: destination path '" << targetPath << "' already exists and is not an empty directory.\n";
            return false;
        }
    }

    std::string destRepo = targetPath + "/.minigit";
    FileUtils::createDirectories(destRepo + "/objects");
    FileUtils::createDirectories(destRepo + "/refs/heads");
    FileUtils::createDirectories(destRepo + "/refs/remotes/origin");

    // Copy all objects
    auto objectFiles = FileUtils::listFiles(srcRepo + "/objects", true);
    for (const auto& objRel : objectFiles) {
        std::string srcObj = srcRepo + "/objects/" + objRel;
        std::string dstObj = destRepo + "/objects/" + objRel;
        FileUtils::copyFile(srcObj, dstObj);
    }

    // Copy branch refs
    auto branchFiles = FileUtils::listFiles(srcRepo + "/refs/heads", false);
    for (const auto& br : branchFiles) {
        std::string srcBr = srcRepo + "/refs/heads/" + br;
        std::string dstBr = destRepo + "/refs/heads/" + br;
        FileUtils::copyFile(srcBr, dstBr);

        // Also create remote tracker
        std::string dstRemoteBr = destRepo + "/refs/remotes/origin/" + br;
        FileUtils::copyFile(srcBr, dstRemoteBr);
    }

    // Copy HEAD
    std::string headContent;
    FileUtils::readFile(srcRepo + "/HEAD", headContent);
    FileUtils::writeFile(destRepo + "/HEAD", headContent);

    // Set origin remote
    addRemote("origin", sourcePath, destRepo);

    // Checkout default branch in destination
    std::string headCommit;
    if (Reference::resolveHead(headCommit, destRepo)) {
        Objects::Commit commit;
        if (Objects::Commit::load(headCommit, commit, destRepo)) {
            // Checkout tree to targetPath
            // Temporarily set repoDir
            WorkingTree::checkoutTree(commit.getTreeHash(), true, destRepo);
        }
    }

    return true;
}

bool RemoteManager::fetchRemote(const std::string& remoteName, const std::string& repoDir) {
    std::string remotePath = getRemotePath(remoteName, repoDir);
    if (remotePath.empty()) {
        std::cerr << "fatal: '" << remoteName << "' does not appear to be a minigit repository\n";
        return false;
    }

    std::string remoteRepo = remotePath + "/.minigit";
    if (!FileUtils::directoryExists(remoteRepo)) {
        std::cerr << "fatal: remote repository at '" << remotePath << "' not found\n";
        return false;
    }

    // Copy all objects from remote
    auto objectFiles = FileUtils::listFiles(remoteRepo + "/objects", true);
    for (const auto& objRel : objectFiles) {
        std::string srcObj = remoteRepo + "/objects/" + objRel;
        std::string dstObj = repoDir + "/objects/" + objRel;
        if (!FileUtils::fileExists(dstObj)) {
            FileUtils::copyFile(srcObj, dstObj);
        }
    }

    // Update remote tracking refs
    std::string remotesTrackDir = repoDir + "/refs/remotes/" + remoteName;
    FileUtils::createDirectories(remotesTrackDir);

    auto branchFiles = FileUtils::listFiles(remoteRepo + "/refs/heads", false);
    for (const auto& br : branchFiles) {
        std::string srcBr = remoteRepo + "/refs/heads/" + br;
        std::string dstBr = remotesTrackDir + "/" + br;
        FileUtils::copyFile(srcBr, dstBr);
    }

    return true;
}

bool RemoteManager::pushRemote(const std::string& remoteName, const std::string& branchName, const std::string& repoDir) {
    std::string remotePath = getRemotePath(remoteName, repoDir);
    if (remotePath.empty()) {
        std::cerr << "fatal: '" << remoteName << "' does not appear to be a minigit repository\n";
        return false;
    }

    std::string remoteRepo = remotePath + "/.minigit";
    if (!FileUtils::directoryExists(remoteRepo)) {
        std::cerr << "fatal: remote repository at '" << remotePath << "' not found\n";
        return false;
    }

    std::string localCommit;
    if (!Reference::getBranchCommit(branchName, localCommit, repoDir)) {
        std::cerr << "error: src refspec " << branchName << " does not match any\n";
        return false;
    }

    // Copy missing objects from local to remote
    auto objectFiles = FileUtils::listFiles(repoDir + "/objects", true);
    for (const auto& objRel : objectFiles) {
        std::string srcObj = repoDir + "/objects/" + objRel;
        std::string dstObj = remoteRepo + "/objects/" + objRel;
        if (!FileUtils::fileExists(dstObj)) {
            FileUtils::copyFile(srcObj, dstObj);
        }
    }

    // Update remote branch ref
    Reference::setBranchCommit(branchName, localCommit, remoteRepo);

    // Update local remote-tracking ref
    std::string remotesTrackDir = repoDir + "/refs/remotes/" + remoteName;
    FileUtils::createDirectories(remotesTrackDir);
    FileUtils::writeFile(remotesTrackDir + "/" + branchName, localCommit + "\n");

    return true;
}

} // namespace Repository
