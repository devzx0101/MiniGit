#include "objects/object_store.hpp"
#include "hashing/sha1.hpp"
#include "filesystem/file_utils.hpp"
#include <iostream>
#include <sstream>

namespace Objects {

std::string ObjectStore::computeHash(const std::string& type, const std::string& content) {
    // Git object format: "<type> <size>\0<content>"
    std::string header = type + " " + std::to_string(content.size());
    std::string rawData = header + '\0' + content;
    return Hashing::SHA1::hashString(rawData);
}

std::string ObjectStore::storeObject(const std::string& type, const std::string& content, const std::string& repoDir) {
    // 1. Prepare Git-formatted object buffer: "<type> <size>\0<content>"
    std::string header = type + " " + std::to_string(content.size());
    std::string rawData = header + '\0' + content;

    // 2. Compute 40-character SHA-1 hash
    std::string hash = Hashing::SHA1::hashString(rawData);

    // 3. Fanout directory: first 2 chars of hash
    std::string dirPath = repoDir + "/objects/" + hash.substr(0, 2);
    if (!FileUtils::createDirectories(dirPath)) {
        std::cerr << "Error: Failed to create object directory: " << dirPath << "\n";
        return "";
    }

    // 4. Object file path: remaining 38 chars of hash
    std::string filePath = dirPath + "/" + hash.substr(2);

    // 5. If object already exists, no need to overwrite (objects are immutable)
    if (!FileUtils::fileExists(filePath)) {
        if (!FileUtils::writeFile(filePath, rawData)) {
            std::cerr << "Error: Failed to write object file: " << filePath << "\n";
            return "";
        }
    }

    return hash;
}

bool ObjectStore::readObject(const std::string& hash, std::string& outType, std::string& outContent, const std::string& repoDir) {
    if (hash.length() != 40) {
        return false;
    }

    std::string filePath = repoDir + "/objects/" + hash.substr(0, 2) + "/" + hash.substr(2);
    std::string rawData;

    if (!FileUtils::readFile(filePath, rawData)) {
        return false;
    }

    // Find the null byte '\0' separating header and content
    size_t nullPos = rawData.find('\0');
    if (nullPos == std::string::npos) {
        return false;
    }

    std::string header = rawData.substr(0, nullPos);
    outContent = rawData.substr(nullPos + 1);

    // Parse "<type> <size>" from header
    size_t spacePos = header.find(' ');
    if (spacePos != std::string::npos) {
        outType = header.substr(0, spacePos);
    } else {
        outType = "unknown";
    }

    return true;
}

} // namespace Objects
