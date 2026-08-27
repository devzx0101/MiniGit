#include "tree.hpp"
#include "object_store.hpp"
#include <sstream>
#include <algorithm>
#include <iostream>

namespace Objects {

Tree::Tree() {}

void Tree::addEntry(const TreeEntry& entry) {
    entries.push_back(entry);
    // Keep entries sorted by name
    std::sort(entries.begin(), entries.end(), [](const TreeEntry& a, const TreeEntry& b) {
        return a.name < b.name;
    });
}

void Tree::addEntry(const std::string& mode, const std::string& type, const std::string& hash, const std::string& name) {
    TreeEntry entry;
    entry.mode = mode;
    entry.type = type;
    entry.hash = hash;
    entry.name = name;
    addEntry(entry);
}

const std::vector<TreeEntry>& Tree::getEntries() const {
    return entries;
}

std::string Tree::serialize() const {
    // Format: "<mode> <type> <hash>\t<name>\n"
    std::ostringstream ss;
    for (const auto& entry : entries) {
        ss << entry.mode << " " << entry.type << " " << entry.hash << "\t" << entry.name << "\n";
    }
    return ss.str();
}

bool Tree::deserialize(const std::string& content) {
    entries.clear();
    std::istringstream stream(content);
    std::string line;

    while (std::getline(stream, line)) {
        if (line.empty()) continue;

        // Parse: "<mode> <type> <hash>\t<name>"
        size_t tabPos = line.find('\t');
        if (tabPos == std::string::npos) continue;

        std::string meta = line.substr(0, tabPos);
        std::string name = line.substr(tabPos + 1);

        std::istringstream metaStream(meta);
        std::string mode, type, hash;
        if (metaStream >> mode >> type >> hash) {
            TreeEntry entry;
            entry.mode = mode;
            entry.type = type;
            entry.hash = hash;
            entry.name = name;
            entries.push_back(entry);
        }
    }

    std::sort(entries.begin(), entries.end(), [](const TreeEntry& a, const TreeEntry& b) {
        return a.name < b.name;
    });
    return true;
}

std::string Tree::save(const std::string& repoDir) const {
    return ObjectStore::storeObject("tree", serialize(), repoDir);
}

bool Tree::load(const std::string& hash, Tree& outTree, const std::string& repoDir) {
    std::string type, content;
    if (!ObjectStore::readObject(hash, type, content, repoDir)) {
        return false;
    }
    if (type != "tree") {
        return false;
    }
    return outTree.deserialize(content);
}

// In-memory directory node for building hierarchical trees
struct InMemoryTreeNode {
    std::map<std::string, TreeEntry> leaves; // blob entries: filename -> TreeEntry
    std::map<std::string, InMemoryTreeNode> subdirs; // dir name -> subtree node
};

static std::string buildSubTree(const InMemoryTreeNode& node, const std::string& repoDir) {
    Tree tree;

    // Add direct blobs
    for (const auto& pair : node.leaves) {
        tree.addEntry(pair.second);
    }

    // Recursively build and add subdirectories
    for (const auto& pair : node.subdirs) {
        const std::string& dirName = pair.first;
        std::string subTreeHash = buildSubTree(pair.second, repoDir);
        if (subTreeHash.empty()) {
            return "";
        }
        tree.addEntry("040000", "tree", subTreeHash, dirName);
    }

    return tree.save(repoDir);
}

std::string Tree::buildFromIndex(const Repository::Index& index, const std::string& repoDir) {
    const auto& entries = index.getEntries();
    if (entries.empty()) {
        // Empty tree
        Tree emptyTree;
        return emptyTree.save(repoDir);
    }

    InMemoryTreeNode rootNode;

    for (const auto& pair : entries) {
        const Repository::IndexEntry& indexEntry = pair.second;
        std::string path = indexEntry.path;

        // Normalize path
        for (char& c : path) {
            if (c == '\\') c = '/';
        }

        // Split into path segments
        std::vector<std::string> parts;
        size_t start = 0;
        size_t end = path.find('/');
        while (end != std::string::npos) {
            if (end > start) {
                parts.push_back(path.substr(start, end - start));
            }
            start = end + 1;
            end = path.find('/', start);
        }
        if (start < path.size()) {
            parts.push_back(path.substr(start));
        }

        if (parts.empty()) continue;

        // Traverse down directory hierarchy
        InMemoryTreeNode* current = &rootNode;
        for (size_t i = 0; i + 1 < parts.size(); ++i) {
            current = &(current->subdirs[parts[i]]);
        }

        // Add leaf file
        std::string fileName = parts.back();
        TreeEntry te;
        te.mode = indexEntry.mode.empty() ? "100644" : indexEntry.mode;
        te.type = "blob";
        te.hash = indexEntry.hash;
        te.name = fileName;
        current->leaves[fileName] = te;
    }

    return buildSubTree(rootNode, repoDir);
}

bool Tree::flatten(const std::string& treeHash, 
                   std::map<std::string, Repository::IndexEntry>& outFiles,
                   const std::string& repoDir,
                   const std::string& prefix) {
    if (treeHash.empty()) {
        return true;
    }

    Tree tree;
    if (!Tree::load(treeHash, tree, repoDir)) {
        return false;
    }

    for (const auto& entry : tree.getEntries()) {
        std::string fullPath = prefix.empty() ? entry.name : prefix + entry.name;

        if (entry.type == "blob") {
            Repository::IndexEntry ie;
            ie.mode = entry.mode;
            ie.hash = entry.hash;
            ie.path = fullPath;
            outFiles[fullPath] = ie;
        } else if (entry.type == "tree") {
            if (!flatten(entry.hash, outFiles, repoDir, fullPath + "/")) {
                return false;
            }
        }
    }

    return true;
}

} // namespace Objects
