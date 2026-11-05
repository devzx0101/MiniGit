#include "commands/tag.hpp"
#include "repository/reference.hpp"
#include "objects/tag.hpp"
#include "filesystem/file_utils.hpp"
#include "terminal.hpp"
#include <iostream>
#include <string>
#include <vector>

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

    bool isDelete = false;
    bool isAnnotated = false;
    std::string tagName;
    std::string message;
    std::string targetCommitInput;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-d" || arg == "--delete") {
            isDelete = true;
        } else if (arg == "-a" || arg == "--annotate") {
            isAnnotated = true;
        } else if ((arg == "-m" || arg == "--message") && i + 1 < argc) {
            isAnnotated = true;
            message = argv[++i];
        } else if (tagName.empty()) {
            tagName = arg;
        } else if (targetCommitInput.empty()) {
            targetCommitInput = arg;
        }
    }

    if (isDelete) {
        if (tagName.empty()) {
            std::cerr << "fatal: tag name required for deletion\n";
            return 1;
        }
        if (!Repository::Reference::tagExists(tagName, ".minigit")) {
            std::cerr << "error: tag '" << tagName << "' not found.\n";
            return 1;
        }
        if (Repository::Reference::deleteTag(tagName, ".minigit")) {
            std::cout << "Deleted tag '" << tagName << "'\n";
            return 0;
        }
        return 1;
    }

    if (tagName.empty()) {
        std::cerr << "fatal: tag name required\n";
        return 1;
    }

    if (Repository::Reference::tagExists(tagName, ".minigit")) {
        std::cerr << "fatal: tag '" << tagName << "' already exists\n";
        return 1;
    }

    std::string targetCommit;
    if (!targetCommitInput.empty()) {
        if (!Repository::Reference::resolveRef(targetCommitInput, targetCommit, ".minigit")) {
            std::cerr << "fatal: Failed to resolve '" << targetCommitInput << "' as a valid commit.\n";
            return 1;
        }
    } else {
        if (!Repository::Reference::resolveHead(targetCommit, ".minigit")) {
            std::cerr << "fatal: Failed to resolve 'HEAD' as a valid commit.\n";
            return 1;
        }
    }

    if (isAnnotated) {
        // Create full Git tag object
        Objects::Tag tagObj;
        tagObj.setTagName(tagName);
        tagObj.setObjectHash(targetCommit);
        tagObj.setObjectType("commit");
        if (message.empty()) {
            message = "Release " + tagName;
        }
        tagObj.setMessage(message);
        std::string tagHash = tagObj.save(".minigit");
        if (tagHash.empty()) {
            std::cerr << "fatal: failed to write tag object\n";
            return 1;
        }
        // Tag ref points to the tag object itself in annotated mode
        if (Repository::Reference::setTag(tagName, tagHash, ".minigit")) {
            std::cout << "Created annotated tag '" << Color::BoldGreen << tagName << Color::Reset 
                      << "' pointing to commit " << Color::BoldYellow << targetCommit.substr(0, 7) << Color::Reset << "\n";
            return 0;
        }
    } else {
        // Lightweight tag: points directly to commit hash
        if (Repository::Reference::setTag(tagName, targetCommit, ".minigit")) {
            return 0;
        }
    }

    return 1;
}
