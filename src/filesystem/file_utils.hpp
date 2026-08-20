#ifndef FILE_UTILS_HPP
#define FILE_UTILS_HPP

#include <string>

namespace FileUtils {
    // Check if a directory exists
    bool directoryExists(const std::string& path);

    // Check if a regular file exists
    bool fileExists(const std::string& path);

    // Create a single directory
    bool createDirectory(const std::string& path);

    // Create directories recursively (e.g. .minigit/refs/heads)
    bool createDirectories(const std::string& path);

    // Write text content to a file
    bool writeFile(const std::string& path, const std::string& content);

    // Read full text content from a file
    bool readFile(const std::string& path, std::string& outContent);
}

#endif // FILE_UTILS_HPP
