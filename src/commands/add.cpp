#include "add.hpp"
#include "../repository/index.hpp"
#include "../repository/ignore.hpp"
#include "../objects/blob.hpp"
#include "../filesystem/file_utils.hpp"
#include <iostream>
#include <vector>

static void addSingleFile(const std::string& path, Repository::Index& index, bool& hasErrors) {
    std::string normPath = FileUtils::normalizePath(path);

    if (!FileUtils::fileExists(normPath)) {
        std::cerr << "fatal: pathspec '" << normPath << "' did not match any files\n";
        hasErrors = true;
        return;
    }

    Objects::Blob blob;
    if (!Objects::Blob::fromFile(normPath, blob)) {
        std::cerr << "error: unable to read file '" << normPath << "'\n";
        hasErrors = true;
        return;
    }

    std::string hash = blob.save(".minigit");
    if (hash.empty()) {
        std::cerr << "error: unable to store blob object for '" << normPath << "'\n";
        hasErrors = true;
        return;
    }

    index.addEntry(normPath, hash, "100644");
}

int handleAdd(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Nothing specified, nothing added.\n"
                  << "usage: minigit add <file>...\n";
        return 1;
    }

    if (!FileUtils::directoryExists(".minigit")) {
        std::cerr << "fatal: not a minigit repository (or any of the parent directories): .minigit\n";
        return 1;
    }

    Repository::Index index;
    if (!index.load(".minigit")) {
        std::cerr << "fatal: unable to read index file\n";
        return 1;
    }

    Repository::Ignore ignore;
    ignore.load(".");

    bool hasErrors = false;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        std::string norm = FileUtils::normalizePath(arg);

        if (norm == "." || FileUtils::directoryExists(norm)) {
            // Recursive directory add
            std::string scanDir = (norm == ".") ? "." : norm;
            auto files = FileUtils::listFiles(scanDir, true);
            for (const auto& file : files) {
                std::string fullRel = (scanDir == ".") ? file : norm + "/" + file;
                fullRel = FileUtils::normalizePath(fullRel);
                if (!ignore.isIgnored(fullRel)) {
                    addSingleFile(fullRel, index, hasErrors);
                }
            }
        } else {
            // Single file add
            if (ignore.isIgnored(norm)) {
                // If explicitly adding an ignored file or regular file
                // Git typically warns or adds; let's allow or skip if not existing
            }
            addSingleFile(norm, index, hasErrors);
        }
    }

    if (!index.save(".minigit")) {
        std::cerr << "fatal: unable to write index file\n";
        return 1;
    }

    return hasErrors ? 1 : 0;
}
