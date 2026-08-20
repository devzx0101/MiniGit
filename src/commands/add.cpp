#include "add.hpp"
#include "../repository/index.hpp"
#include "../objects/blob.hpp"
#include "../filesystem/file_utils.hpp"
#include <iostream>
#include <vector>

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

    bool hasErrors = false;

    for (int i = 2; i < argc; ++i) {
        std::string filePath = argv[i];

        if (!FileUtils::fileExists(filePath)) {
            std::cerr << "fatal: pathspec '" << filePath << "' did not match any files\n";
            hasErrors = true;
            continue;
        }

        Objects::Blob blob;
        if (!Objects::Blob::fromFile(filePath, blob)) {
            std::cerr << "error: unable to read file '" << filePath << "'\n";
            hasErrors = true;
            continue;
        }

        // Store blob into .minigit/objects/
        std::string hash = blob.save(".minigit");
        if (hash.empty()) {
            std::cerr << "error: unable to store blob object for '" << filePath << "'\n";
            hasErrors = true;
            continue;
        }

        // Record in the staging area / index
        index.addEntry(filePath, hash, "100644");
    }

    if (!index.save(".minigit")) {
        std::cerr << "fatal: unable to write index file\n";
        return 1;
    }

    return hasErrors ? 1 : 0;
}
