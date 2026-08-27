#include "repository/index.hpp"
#include "filesystem/file_utils.hpp"
#include <sstream>
#include <iostream>

namespace Repository {

Index::Index() {}

bool Index::load(const std::string& repoDir) {
    entries.clear();
    std::string indexPath = repoDir + "/index";

    if (!FileUtils::fileExists(indexPath)) {
        return true; // Empty index is valid
    }

    std::string content;
    if (!FileUtils::readFile(indexPath, content)) {
        return false;
    }

    std::istringstream stream(content);
    std::string line;

    while (std::getline(stream, line)) {
        if (line.empty()) {
            continue;
        }

        std::istringstream lineStream(line);
        std::string mode, hash, path;

        if (lineStream >> mode >> hash >> path) {
            // In case path contains remaining tokens
            std::string restOfPath;
            while (lineStream >> restOfPath) {
                path += " " + restOfPath;
            }

            IndexEntry entry;
            entry.mode = mode;
            entry.hash = hash;
            entry.path = path;
            entries[path] = entry;
        }
    }

    return true;
}

bool Index::save(const std::string& repoDir) {
    std::string indexPath = repoDir + "/index";
    std::ostringstream stream;

    for (const auto& pair : entries) {
        const IndexEntry& entry = pair.second;
        stream << entry.mode << " " << entry.hash << " " << entry.path << "\n";
    }

    return FileUtils::writeFile(indexPath, stream.str());
}

void Index::addEntry(const std::string& path, const std::string& hash, const std::string& mode) {
    IndexEntry entry;
    entry.mode = mode;
    entry.hash = hash;
    entry.path = path;
    entries[path] = entry;
}

bool Index::removeEntry(const std::string& path) {
    return entries.erase(path) > 0;
}

bool Index::hasEntry(const std::string& path) const {
    return entries.find(path) != entries.end();
}

const std::map<std::string, IndexEntry>& Index::getEntries() const {
    return entries;
}

void Index::clear() {
    entries.clear();
}

} // namespace Repository
