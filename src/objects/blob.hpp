#ifndef BLOB_HPP
#define BLOB_HPP

#include <string>

namespace Objects {

class Blob {
public:
    // Default constructor
    Blob();

    // Create a Blob from raw string content
    explicit Blob(const std::string& rawContent);

    // Factory method: Read a file from disk and construct a Blob
    static bool fromFile(const std::string& filePath, Blob& outBlob);

    // Factory method: Load an existing Blob from .minigit/objects/ by its SHA-1 hash
    static bool load(const std::string& hash, Blob& outBlob, const std::string& repoDir = ".minigit");

    // Save this blob to .minigit/objects/ and return its 40-char SHA-1 hash
    std::string save(const std::string& repoDir = ".minigit");

    // Getters
    const std::string& getContent() const;
    const std::string& getHash() const;
    size_t getSize() const;

private:
    std::string content;
    std::string hash;
};

} // namespace Objects

#endif // BLOB_HPP
