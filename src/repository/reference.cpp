#include "reference.hpp"
#include "../filesystem/file_utils.hpp"
#include "../objects/object_store.hpp"
#include <sstream>
#include <algorithm>
#include <iostream>

namespace Repository {

bool Reference::getHead(std::string& outTarget, bool& isSymbolic, const std::string& repoDir) {
    std::string headPath = repoDir + "/HEAD";
    std::string content;
    if (!FileUtils::readFile(headPath, content)) {
        return false;
    }

    // Trim trailing whitespace
    while (!content.empty() && (content.back() == '\n' || content.back() == '\r' || content.back() == ' ')) {
        content.pop_back();
    }

    if (content.rfind("ref: ", 0) == 0) {
        isSymbolic = true;
        outTarget = content.substr(5);
    } else {
        isSymbolic = false;
        outTarget = content;
    }
    return true;
}

bool Reference::setHeadSymbolic(const std::string& branchRef, const std::string& repoDir) {
    std::string ref = branchRef;
    if (ref.rfind("refs/heads/", 0) != 0) {
        ref = "refs/heads/" + ref;
    }
    std::string content = "ref: " + ref + "\n";
    return FileUtils::writeFile(repoDir + "/HEAD", content);
}

bool Reference::setHeadDetached(const std::string& commitHash, const std::string& repoDir) {
    return FileUtils::writeFile(repoDir + "/HEAD", commitHash + "\n");
}

std::string Reference::getCurrentBranchName(const std::string& repoDir) {
    std::string target;
    bool isSymbolic = false;
    if (getHead(target, isSymbolic, repoDir) && isSymbolic) {
        if (target.rfind("refs/heads/", 0) == 0) {
            return target.substr(11);
        }
        return target;
    }
    return "";
}

bool Reference::resolveHead(std::string& outCommitHash, const std::string& repoDir) {
    std::string target;
    bool isSymbolic = false;
    if (!getHead(target, isSymbolic, repoDir)) {
        return false;
    }

    if (isSymbolic) {
        std::string refPath = repoDir + "/" + target;
        if (!FileUtils::fileExists(refPath)) {
            // Unborn branch (e.g. fresh repo before initial commit)
            return false;
        }
        std::string content;
        if (!FileUtils::readFile(refPath, content)) {
            return false;
        }
        while (!content.empty() && (content.back() == '\n' || content.back() == '\r' || content.back() == ' ')) {
            content.pop_back();
        }
        outCommitHash = content;
        return !outCommitHash.empty();
    } else {
        outCommitHash = target;
        return !outCommitHash.empty();
    }
}

bool Reference::branchExists(const std::string& branchName, const std::string& repoDir) {
    return FileUtils::fileExists(repoDir + "/refs/heads/" + branchName);
}

bool Reference::getBranchCommit(const std::string& branchName, std::string& outCommitHash, const std::string& repoDir) {
    std::string path = repoDir + "/refs/heads/" + branchName;
    std::string content;
    if (!FileUtils::readFile(path, content)) {
        return false;
    }
    while (!content.empty() && (content.back() == '\n' || content.back() == '\r' || content.back() == ' ')) {
        content.pop_back();
    }
    outCommitHash = content;
    return !outCommitHash.empty();
}

bool Reference::setBranchCommit(const std::string& branchName, const std::string& commitHash, const std::string& repoDir) {
    std::string dir = repoDir + "/refs/heads";
    FileUtils::createDirectories(dir);
    return FileUtils::writeFile(dir + "/" + branchName, commitHash + "\n");
}

bool Reference::deleteBranch(const std::string& branchName, const std::string& repoDir) {
    return FileUtils::removeFile(repoDir + "/refs/heads/" + branchName);
}

std::vector<std::pair<std::string, bool>> Reference::listBranches(const std::string& repoDir) {
    std::vector<std::pair<std::string, bool>> result;
    std::string currentBranch = getCurrentBranchName(repoDir);
    std::string headsDir = repoDir + "/refs/heads";

    auto files = FileUtils::listFiles(headsDir, false);
    for (const auto& file : files) {
        bool isCurrent = (!currentBranch.empty() && file == currentBranch);
        result.push_back({file, isCurrent});
    }

    std::sort(result.begin(), result.end());
    return result;
}

bool Reference::tagExists(const std::string& tagName, const std::string& repoDir) {
    return FileUtils::fileExists(repoDir + "/refs/tags/" + tagName);
}

bool Reference::getTagCommit(const std::string& tagName, std::string& outCommitHash, const std::string& repoDir) {
    std::string path = repoDir + "/refs/tags/" + tagName;
    std::string content;
    if (!FileUtils::readFile(path, content)) {
        return false;
    }
    while (!content.empty() && (content.back() == '\n' || content.back() == '\r' || content.back() == ' ')) {
        content.pop_back();
    }
    outCommitHash = content;
    return !outCommitHash.empty();
}

bool Reference::setTag(const std::string& tagName, const std::string& commitHash, const std::string& repoDir) {
    std::string dir = repoDir + "/refs/tags";
    FileUtils::createDirectories(dir);
    return FileUtils::writeFile(dir + "/" + tagName, commitHash + "\n");
}

bool Reference::deleteTag(const std::string& tagName, const std::string& repoDir) {
    return FileUtils::removeFile(repoDir + "/refs/tags/" + tagName);
}

std::vector<std::string> Reference::listTags(const std::string& repoDir) {
    std::string tagsDir = repoDir + "/refs/tags";
    auto files = FileUtils::listFiles(tagsDir, false);
    std::sort(files.begin(), files.end());
    return files;
}

bool Reference::resolveRef(const std::string& refOrHash, std::string& outCommitHash, const std::string& repoDir) {
    if (refOrHash.empty()) {
        return false;
    }

    if (refOrHash == "HEAD") {
        return resolveHead(outCommitHash, repoDir);
    }

    // Check if it's a branch
    if (branchExists(refOrHash, repoDir)) {
        return getBranchCommit(refOrHash, outCommitHash, repoDir);
    }

    // Check if it's a tag
    if (tagExists(refOrHash, repoDir)) {
        return getTagCommit(refOrHash, outCommitHash, repoDir);
    }

    // Check if it's a 40-char SHA-1 hash
    if (refOrHash.length() == 40) {
        std::string type, content;
        if (Objects::ObjectStore::readObject(refOrHash, type, content, repoDir)) {
            outCommitHash = refOrHash;
            return true;
        }
    }

    // Check short hash prefix (at least 4 chars)
    if (refOrHash.length() >= 4 && refOrHash.length() < 40) {
        std::string prefixDir = repoDir + "/objects/" + refOrHash.substr(0, 2);
        if (FileUtils::directoryExists(prefixDir)) {
            auto files = FileUtils::listFiles(prefixDir, false);
            std::string subPrefix = refOrHash.substr(2);
            std::vector<std::string> matches;
            for (const auto& f : files) {
                if (f.rfind(subPrefix, 0) == 0) {
                    matches.push_back(refOrHash.substr(0, 2) + f);
                }
            }
            if (matches.size() == 1) {
                outCommitHash = matches[0];
                return true;
            }
        }
    }

    return false;
}

bool Reference::appendReflog(const std::string& refPath, 
                             const std::string& oldHash, 
                             const std::string& newHash, 
                             const std::string& message, 
                             const std::string& repoDir) {
    std::string safeOld = oldHash.empty() ? std::string(40, '0') : oldHash;
    std::string safeNew = newHash.empty() ? std::string(40, '0') : newHash;

    std::string logFilePath = repoDir + "/logs/" + refPath;
    std::string line = safeOld + " " + safeNew + " MiniGit User <user@minigit.local> " + 
                       FileUtils::getCurrentTimestamp() + "\t" + message + "\n";

    std::string existing;
    FileUtils::readFile(logFilePath, existing);
    return FileUtils::writeFile(logFilePath, existing + line);
}

std::vector<ReflogEntry> Reference::readReflog(const std::string& refPath, const std::string& repoDir) {
    std::vector<ReflogEntry> entries;
    std::string logFilePath = repoDir + "/logs/" + refPath;
    std::string content;
    if (!FileUtils::readFile(logFilePath, content)) {
        return entries;
    }

    std::istringstream stream(content);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.empty()) continue;

        size_t tabPos = line.find('\t');
        std::string meta = (tabPos != std::string::npos) ? line.substr(0, tabPos) : line;
        std::string msg = (tabPos != std::string::npos) ? line.substr(tabPos + 1) : "";

        std::istringstream metaStream(meta);
        std::string oldH, newH, name, email, timeSec, tz;
        if (metaStream >> oldH >> newH >> name >> email >> timeSec >> tz) {
            ReflogEntry re;
            re.oldHash = oldH;
            re.newHash = newH;
            re.committer = name + " " + email;
            re.timestamp = timeSec + " " + tz;
            re.message = msg;
            entries.push_back(re);
        }
    }

    return entries;
}

} // namespace Repository
