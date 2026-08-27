#ifndef FILE_UTILS_HPP
#define FILE_UTILS_HPP

#include <string>
#include <vector>

namespace FileUtils {
    // Check if a directory exists
    bool directoryExists(const std::string& path);

    // Check if a regular file exists
    bool fileExists(const std::string& path);

    // Create a single directory
    bool createDirectory(const std::string& path);

    // Create directories recursively (e.g. .minigit/refs/heads)
    bool createDirectories(const std::string& path);

    // Write text/binary content to a file
    bool writeFile(const std::string& path, const std::string& content);

    // Read full text/binary content from a file
    bool readFile(const std::string& path, std::string& outContent);

    // Remove a single file
    bool removeFile(const std::string& path);

    // Remove a directory recursively
    bool removeDirectoryRecursive(const std::string& path);

    // Copy a file from src to dest (creating parent directories if needed)
    bool copyFile(const std::string& src, const std::string& dest);

    // List all files in a directory (optionally recursive, returning relative paths)
    std::vector<std::string> listFiles(const std::string& dirPath, bool recursive = true);

    // Normalize path separators to forward slash '/'
    std::string normalizePath(const std::string& path);

    // Find the repository root directory containing .minigit (returns "" if not found)
    std::string findRepoRoot(const std::string& startPath = ".");

    // Get current formatted timestamp (e.g., "1700000000 +0000" or ISO-like)
    std::string getCurrentTimestamp();
    std::string getFormattedDate();
}

#endif // FILE_UTILS_HPP
