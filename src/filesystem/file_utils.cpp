#include "file_utils.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#define MKDIR(dir) _mkdir(dir)
#else
#include <unistd.h>
#define MKDIR(dir) mkdir(dir, 0755)
#endif

namespace FileUtils {

bool directoryExists(const std::string& path) {
    struct stat info;
    if (stat(path.c_str(), &info) != 0) {
        return false;
    }
    return (info.st_mode & S_IFDIR) != 0;
}

bool fileExists(const std::string& path) {
    struct stat info;
    if (stat(path.c_str(), &info) != 0) {
        return false;
    }
    return (info.st_mode & S_IFREG) != 0;
}

bool createDirectory(const std::string& path) {
    if (directoryExists(path)) {
        return true;
    }
    return MKDIR(path.c_str()) == 0;
}

bool createDirectories(const std::string& path) {
    if (path.empty() || directoryExists(path)) {
        return true;
    }

    std::string currentPath;
    for (size_t i = 0; i < path.length(); ++i) {
        char c = path[i];
        if (c == '/' || c == '\\') {
            if (!currentPath.empty() && !directoryExists(currentPath)) {
                if (!createDirectory(currentPath)) {
                    return false;
                }
            }
        }
        currentPath += c;
    }

    if (!currentPath.empty() && !directoryExists(currentPath)) {
        return createDirectory(currentPath);
    }
    return true;
}

bool writeFile(const std::string& path, const std::string& content) {
    std::ofstream outFile(path.c_str(), std::ios::binary);
    if (!outFile.is_open()) {
        return false;
    }
    outFile.write(content.data(), content.size());
    return outFile.good();
}

bool readFile(const std::string& path, std::string& outContent) {
    std::ifstream inFile(path.c_str(), std::ios::binary);
    if (!inFile.is_open()) {
        return false;
    }
    std::ostringstream ss;
    ss << inFile.rdbuf();
    outContent = ss.str();
    return true;
}

} // namespace FileUtils
