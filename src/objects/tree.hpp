#ifndef TREE_HPP
#define TREE_HPP

#include <string>
#include <vector>
#include <map>
#include "repository/index.hpp"

namespace Objects {

struct TreeEntry {
    std::string mode; // "100644" for blob, "040000" for tree
    std::string type; // "blob" or "tree"
    std::string hash; // 40-char SHA-1 hash
    std::string name; // filename or directory name
};

class Tree {
public:
    Tree();

    // Add an entry to this tree
    void addEntry(const TreeEntry& entry);
    void addEntry(const std::string& mode, const std::string& type, const std::string& hash, const std::string& name);

    // Get all entries (sorted by name)
    const std::vector<TreeEntry>& getEntries() const;

    // Serialize tree entries to standard string format
    std::string serialize() const;

    // Deserialize tree entries from string content
    bool deserialize(const std::string& content);

    // Save tree to object store and return its 40-char SHA-1 hash
    std::string save(const std::string& repoDir = ".minigit") const;

    // Load tree from object store by hash
    static bool load(const std::string& hash, Tree& outTree, const std::string& repoDir = ".minigit");

    // Build hierarchical tree objects from the staged index and return root tree hash
    static std::string buildFromIndex(const Repository::Index& index, const std::string& repoDir = ".minigit");

    // Recursively flatten a tree object into a map of full relative file paths -> IndexEntry
    static bool flatten(const std::string& treeHash, 
                        std::map<std::string, Repository::IndexEntry>& outFiles,
                        const std::string& repoDir = ".minigit",
                        const std::string& prefix = "");

private:
    std::vector<TreeEntry> entries;
};

} // namespace Objects

#endif // TREE_HPP
