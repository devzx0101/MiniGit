#include "objects/commit.hpp"
#include "objects/object_store.hpp"
#include "filesystem/file_utils.hpp"
#include <sstream>

namespace Objects {

Commit::Commit() {
    author = "MiniGit User <user@minigit.local>";
    committer = "MiniGit User <user@minigit.local>";
    timestamp = FileUtils::getCurrentTimestamp();
}

void Commit::setTreeHash(const std::string& hash) {
    treeHash = hash;
}

void Commit::addParentHash(const std::string& hash) {
    if (!hash.empty()) {
        parentHashes.push_back(hash);
    }
}

void Commit::setParentHashes(const std::vector<std::string>& parents) {
    parentHashes = parents;
}

void Commit::setAuthor(const std::string& a) {
    author = a;
}

void Commit::setCommitter(const std::string& c) {
    committer = c;
}

void Commit::setTimestamp(const std::string& ts) {
    timestamp = ts;
}

void Commit::setMessage(const std::string& msg) {
    message = msg;
}

const std::string& Commit::getTreeHash() const {
    return treeHash;
}

const std::vector<std::string>& Commit::getParentHashes() const {
    return parentHashes;
}

std::string Commit::getFirstParentHash() const {
    return parentHashes.empty() ? "" : parentHashes.front();
}

const std::string& Commit::getAuthor() const {
    return author;
}

const std::string& Commit::getCommitter() const {
    return committer;
}

const std::string& Commit::getTimestamp() const {
    return timestamp;
}

const std::string& Commit::getMessage() const {
    return message;
}

std::string Commit::serialize() const {
    std::ostringstream ss;
    ss << "tree " << treeHash << "\n";
    for (const auto& parent : parentHashes) {
        ss << "parent " << parent << "\n";
    }
    ss << "author " << author << " " << timestamp << "\n";
    ss << "committer " << committer << " " << timestamp << "\n";
    ss << "\n";
    ss << message;
    if (message.empty() || message.back() != '\n') {
        ss << "\n";
    }
    return ss.str();
}

bool Commit::deserialize(const std::string& content) {
    parentHashes.clear();
    treeHash.clear();
    author.clear();
    committer.clear();
    timestamp.clear();
    message.clear();

    std::istringstream stream(content);
    std::string line;
    bool readingMessage = false;
    std::ostringstream msgStream;

    while (std::getline(stream, line)) {
        if (!readingMessage) {
            if (line.empty()) {
                readingMessage = true;
                continue;
            }

            if (line.rfind("tree ", 0) == 0) {
                treeHash = line.substr(5);
            } else if (line.rfind("parent ", 0) == 0) {
                parentHashes.push_back(line.substr(7));
            } else if (line.rfind("author ", 0) == 0) {
                std::string rest = line.substr(7);
                size_t gtPos = rest.find('>');
                if (gtPos != std::string::npos) {
                    author = rest.substr(0, gtPos + 1);
                    if (gtPos + 2 < rest.size()) {
                        timestamp = rest.substr(gtPos + 2);
                    }
                } else {
                    author = rest;
                }
            } else if (line.rfind("committer ", 0) == 0) {
                std::string rest = line.substr(10);
                size_t gtPos = rest.find('>');
                if (gtPos != std::string::npos) {
                    committer = rest.substr(0, gtPos + 1);
                } else {
                    committer = rest;
                }
            }
        } else {
            msgStream << line << "\n";
        }
    }

    message = msgStream.str();
    // Trim trailing newline from message for easy display
    while (!message.empty() && (message.back() == '\n' || message.back() == '\r')) {
        message.pop_back();
    }

    return !treeHash.empty();
}

std::string Commit::save(const std::string& repoDir) const {
    return ObjectStore::storeObject("commit", serialize(), repoDir);
}

bool Commit::load(const std::string& hash, Commit& outCommit, const std::string& repoDir) {
    std::string type, content;
    if (!ObjectStore::readObject(hash, type, content, repoDir)) {
        return false;
    }
    if (type != "commit") {
        return false;
    }
    return outCommit.deserialize(content);
}

} // namespace Objects
