#include "file_utils.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <filesystem>
#include <ctime>

namespace fs = std::filesystem;

namespace FileUtils {

bool directoryExists(const std::string& path) {
    std::error_code ec;
    return fs::is_directory(path, ec);
}

bool fileExists(const std::string& path) {
    std::error_code ec;
    return fs::is_regular_file(path, ec);
}

bool createDirectory(const std::string& path) {
    std::error_code ec;
    return fs::create_directory(path, ec) || fs::is_directory(path, ec);
}

bool createDirectories(const std::string& path) {
    std::error_code ec;
    return fs::create_directories(path, ec) || fs::is_directory(path, ec);
}

bool writeFile(const std::string& path, const std::string& content) {
    // Ensure parent directory exists
    fs::path p(path);
    if (p.has_parent_path()) {
        std::error_code ec;
        fs::create_directories(p.parent_path(), ec);
    }

    std::ofstream outFile(path, std::ios::binary);
    if (!outFile.is_open()) {
        return false;
    }
    outFile.write(content.data(), content.size());
    return outFile.good();
}

bool readFile(const std::string& path, std::string& outContent) {
    std::ifstream inFile(path, std::ios::binary);
    if (!inFile.is_open()) {
        return false;
    }
    std::ostringstream ss;
    ss << inFile.rdbuf();
    outContent = ss.str();
    return true;
}

bool removeFile(const std::string& path) {
    std::error_code ec;
    return fs::remove(path, ec);
}

bool removeDirectoryRecursive(const std::string& path) {
    std::error_code ec;
    return fs::remove_all(path, ec) > 0;
}

bool copyFile(const std::string& src, const std::string& dest) {
    std::error_code ec;
    fs::path destPath(dest);
    if (destPath.has_parent_path()) {
        fs::create_directories(destPath.parent_path(), ec);
    }
    return fs::copy_file(src, dest, fs::copy_options::overwrite_existing, ec);
}

std::vector<std::string> listFiles(const std::string& dirPath, bool recursive) {
    std::vector<std::string> results;
    std::error_code ec;

    if (!fs::exists(dirPath, ec) || !fs::is_directory(dirPath, ec)) {
        return results;
    }

    if (recursive) {
        for (const auto& entry : fs::recursive_directory_iterator(dirPath, ec)) {
            if (entry.is_regular_file()) {
                std::string relPath = fs::relative(entry.path(), dirPath, ec).generic_string();
                results.push_back(relPath);
            }
        }
    } else {
        for (const auto& entry : fs::directory_iterator(dirPath, ec)) {
            if (entry.is_regular_file()) {
                std::string relPath = fs::relative(entry.path(), dirPath, ec).generic_string();
                results.push_back(relPath);
            }
        }
    }

    return results;
}

std::string normalizePath(const std::string& path) {
    std::string result = path;
    for (char& c : result) {
        if (c == '\\') {
            c = '/';
        }
    }
    // Remove leading "./" if present
    if (result.rfind("./", 0) == 0) {
        result = result.substr(2);
    }
    return result;
}

std::string findRepoRoot(const std::string& startPath) {
    std::error_code ec;
    fs::path current = fs::absolute(startPath, ec);

    while (true) {
        if (fs::exists(current / ".minigit", ec) && fs::is_directory(current / ".minigit", ec)) {
            return current.generic_string();
        }
        if (!current.has_parent_path() || current == current.parent_path()) {
            break;
        }
        current = current.parent_path();
    }
    return "";
}

std::string getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    std::ostringstream ss;
    ss << nowTime << " +0000";
    return ss.str();
}

std::string getFormattedDate() {
    auto now = std::chrono::system_clock::now();
    std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm* localTime = std::localtime(&nowTime);
    char buf[64];
    if (localTime && std::strftime(buf, sizeof(buf), "%a %b %d %H:%M:%S %Y %z", localTime)) {
        return std::string(buf);
    }
    return getCurrentTimestamp();
}

} // namespace FileUtils
