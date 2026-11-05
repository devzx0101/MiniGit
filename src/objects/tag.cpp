#include "objects/tag.hpp"
#include "objects/object_store.hpp"
#include "filesystem/file_utils.hpp"
#include <sstream>

namespace Objects {

Tag::Tag() 
    : objectType("commit"), 
      tagger("MiniGit User <user@minigit.local>"), 
      timestamp(FileUtils::getCurrentTimestamp()) {}

void Tag::setObjectHash(const std::string& hash) { objectHash = hash; }
void Tag::setObjectType(const std::string& type) { objectType = type; }
void Tag::setTagName(const std::string& name) { tagName = name; }
void Tag::setTagger(const std::string& t) { tagger = t; }
void Tag::setTimestamp(const std::string& ts) { timestamp = ts; }
void Tag::setMessage(const std::string& msg) { message = msg; }

const std::string& Tag::getObjectHash() const { return objectHash; }
const std::string& Tag::getObjectType() const { return objectType; }
const std::string& Tag::getTagName() const { return tagName; }
const std::string& Tag::getTagger() const { return tagger; }
const std::string& Tag::getTimestamp() const { return timestamp; }
const std::string& Tag::getMessage() const { return message; }

std::string Tag::serialize() const {
    std::ostringstream ss;
    ss << "object " << objectHash << "\n";
    ss << "type " << objectType << "\n";
    ss << "tag " << tagName << "\n";
    ss << "tagger " << tagger << " " << timestamp << "\n\n";
    ss << message << "\n";
    return ss.str();
}

bool Tag::deserialize(const std::string& content) {
    std::istringstream stream(content);
    std::string line;
    bool inMessage = false;
    std::ostringstream msgStream;

    while (std::getline(stream, line)) {
        if (!inMessage) {
            if (line.empty() || line == "\r") {
                inMessage = true;
                continue;
            }
            if (line.rfind("object ", 0) == 0) {
                objectHash = line.substr(7);
            } else if (line.rfind("type ", 0) == 0) {
                objectType = line.substr(5);
            } else if (line.rfind("tag ", 0) == 0) {
                tagName = line.substr(4);
            } else if (line.rfind("tagger ", 0) == 0) {
                tagger = line.substr(7);
            }
        } else {
            msgStream << line << "\n";
        }
    }

    message = msgStream.str();
    while (!message.empty() && (message.back() == '\n' || message.back() == '\r')) {
        message.pop_back();
    }
    return !objectHash.empty() && !tagName.empty();
}

std::string Tag::save(const std::string& repoDir) const {
    return ObjectStore::storeObject("tag", serialize(), repoDir);
}

bool Tag::load(const std::string& hash, Tag& outTag, const std::string& repoDir) {
    std::string type, content;
    if (!ObjectStore::readObject(hash, type, content, repoDir)) {
        return false;
    }
    if (type != "tag") {
        return false;
    }
    return outTag.deserialize(content);
}

} // namespace Objects
