#ifndef INDEX_HPP
#define INDEX_HPP

#include <string>
#include <map>
#include <vector>

namespace Repository {

struct IndexEntry {
    std::string mode; // e.g. "100644"
    std::string hash; // 40-char SHA-1 hash of blob
    std::string path; // relative file path, e.g. "hello.txt"
};

class Index {
public:
    Index();

    // Load entries from .minigit/index
    bool load(const std::string& repoDir = ".minigit");

    // Save current entries to .minigit/index
    bool save(const std::string& repoDir = ".minigit");

    // Add or update a file entry in the staging area
    void addEntry(const std::string& path, const std::string& hash, const std::string& mode = "100644");

    // Remove a file entry from the staging area
    bool removeEntry(const std::string& path);

    // Check if an entry exists
    bool hasEntry(const std::string& path) const;

    // Get all staged entries (sorted alphabetically by path)
    const std::map<std::string, IndexEntry>& getEntries() const;

    // Clear all staged entries
    void clear();

private:
    std::map<std::string, IndexEntry> entries;
};

} // namespace Repository

#endif // INDEX_HPP
