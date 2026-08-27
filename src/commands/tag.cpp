#include "commands/tag.hpp"
#include "repository/reference.hpp"
#include "filesystem/file_utils.hpp"
#include <iostream>
#include <string>

int handleTag(int argc, char* argv[]) {
    if (!FileUtils::directoryExists(".minigit")) {
        std::cerr << "fatal: not a minigit repository (or any of the parent directories): .minigit\n";
        return 1;
    }

    if (argc == 2) {
        // List tags
        auto tags = Repository::Reference::listTags(".minigit");
        for (const auto& tag : tags) {
            std::cout << tag << "\n";
        }
        return 0;
    }

    std::string arg1 = argv[2];

    if (arg1 == "-d" || arg1 == "--delete") {
        if (argc < 4) {
            std::cerr << "fatal: tag name required for deletion\n";
            return 1;
        }
        std::string tagName = argv[3];
        if (!Repository::Reference::tagExists(tagName, ".minigit")) {
            std::cerr << "error: tag '" << tagName << "' not found.\n";
            return 1;
        }
        if (Repository::Reference::deleteTag(tagName, ".minigit")) {
            std::cout << "Deleted tag '" << tagName << "'\n";
            return 0;
        }
        return 1;
    } else {
        // Create tag
        std::string tagName = arg1;
        if (Repository::Reference::tagExists(tagName, ".minigit")) {
            std::cerr << "fatal: tag '" << tagName << "' already exists\n";
            return 1;
        }

        std::string targetCommit;
        if (argc >= 4) {
            if (!Repository::Reference::resolveRef(argv[3], targetCommit, ".minigit")) {
                std::cerr << "fatal: Failed to resolve '" << argv[3] << "' as a valid commit.\n";
                return 1;
            }
        } else {
            if (!Repository::Reference::resolveHead(targetCommit, ".minigit")) {
                std::cerr << "fatal: Failed to resolve 'HEAD' as a valid commit.\n";
                return 1;
            }
        }

        if (Repository::Reference::setTag(tagName, targetCommit, ".minigit")) {
            return 0;
        } else {
            std::cerr << "fatal: failed to create tag '" << tagName << "'\n";
            return 1;
        }
    }
}
