#ifndef OBJECT_STORE_HPP
#define OBJECT_STORE_HPP

#include <string>

namespace Objects {

class ObjectStore {
public:
    // Store an object in .minigit/objects/ and return its 40-char SHA-1 hash
    static std::string storeObject(const std::string& type, const std::string& content, const std::string& repoDir = ".minigit");

    // Read an object by its 40-char SHA-1 hash
    static bool readObject(const std::string& hash, std::string& outType, std::string& outContent, const std::string& repoDir = ".minigit");

    // Compute the SHA-1 hash of an object with its header without necessarily writing to disk
    static std::string computeHash(const std::string& type, const std::string& content);
};

} // namespace Objects

#endif // OBJECT_STORE_HPP
