#ifndef PACKFILE_HPP
#define PACKFILE_HPP

#include <string>
#include <vector>
#include <cstdint>

namespace Objects {

struct PackEntry {
    std::string hash;      // 40-char SHA-1
    std::string type;      // "blob", "tree", "commit", "tag"
    std::string content;   // Raw content
    uint32_t offset;       // Byte offset inside .pack file
    uint32_t crc32;        // CRC32 of compressed entry
};

class Packfile {
public:
    // Create a .pack file and matching .idx (version 2) fanout index in repoDir/objects/pack/
    // Returns the packfile base hash (the SHA-1 checksum of the packfile payload)
    static std::string createPack(const std::vector<std::string>& objectHashes, 
                                  const std::string& repoDir = ".minigit");

    // Read an object directly from .pack archives if not found in loose files
    static bool readPackedObject(const std::string& hash, 
                                 std::string& outType, 
                                 std::string& outContent, 
                                 const std::string& repoDir = ".minigit");

    // Discover all loose object hashes currently in repoDir/objects/
    static std::vector<std::string> findLooseObjects(const std::string& repoDir = ".minigit");
};

} // namespace Objects

#endif // PACKFILE_HPP

