#include "ignore.hpp"
#include "../filesystem/file_utils.hpp"
#include <sstream>
#include <algorithm>

namespace Repository {

Ignore::Ignore() {
    // Default system ignores
    patterns.push_back(".minigit");
    patterns.push_back(".minigit/*");
    patterns.push_back(".git");
    patterns.push_back(".git/*");
}

void Ignore::load(const std::string& rootDir) {
    std::vector<std::string> candidateFiles = {
        rootDir + "/.minigitignore",
        rootDir + "/.gitignore"
    };

    for (const auto& file : candidateFiles) {
        std::string content;
        if (FileUtils::readFile(file, content)) {
            std::istringstream stream(content);
            std::string line;
            while (std::getline(stream, line)) {
                // Trim whitespace
                while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) {
                    line.pop_back();
                }
                size_t start = line.find_first_not_of(" \t");
                if (start != std::string::npos) {
                    line = line.substr(start);
                } else {
                    line.clear();
                }

                if (!line.empty() && line[0] != '#') {
                    addPattern(line);
                }
            }
        }
    }
}

void Ignore::addPattern(const std::string& pattern) {
    if (pattern.empty()) return;
    std::string pat = FileUtils::normalizePath(pattern);
    if (std::find(patterns.begin(), patterns.end(), pat) == patterns.end()) {
        patterns.push_back(pat);
    }
}

bool Ignore::matchPattern(const std::string& pattern, const std::string& path) {
    if (pattern.empty()) return false;

    // If pattern ends with '/', it matches directory prefix
    if (pattern.back() == '/') {
        std::string dirPat = pattern.substr(0, pattern.size() - 1);
        if (path == dirPat || path.rfind(dirPat + "/", 0) == 0) {
            return true;
        }
    }

    // Direct filename / relative path match
    if (pattern == path) return true;

    // Check if filename component matches
    size_t lastSlash = path.find_last_of('/');
    std::string filename = (lastSlash == std::string::npos) ? path : path.substr(lastSlash + 1);
    if (pattern == filename) return true;

    // Check wildcard prefix/suffix (e.g. *.txt, build*, etc.)
    if (pattern.rfind("*.", 0) == 0) {
        std::string ext = pattern.substr(1); // e.g. ".txt"
        if (path.size() >= ext.size() && path.compare(path.size() - ext.size(), ext.size(), ext) == 0) {
            return true;
        }
    }

    // Check trailing wildcard (e.g. "temp*")
    if (pattern.back() == '*') {
        std::string prefix = pattern.substr(0, pattern.size() - 1);
        if (path.rfind(prefix, 0) == 0 || filename.rfind(prefix, 0) == 0) {
            return true;
        }
    }

    return false;
}

bool Ignore::isIgnored(const std::string& relativePath) const {
    std::string normPath = FileUtils::normalizePath(relativePath);

    // Never ignore .minigit itself or inside it from normal operations
    if (normPath == ".minigit" || normPath.rfind(".minigit/", 0) == 0) {
        return true;
    }
    if (normPath == ".git" || normPath.rfind(".git/", 0) == 0) {
        return true;
    }

    for (const auto& pat : patterns) {
        if (matchPattern(pat, normPath)) {
            return true;
        }
    }
    return false;
}

} // namespace Repository
