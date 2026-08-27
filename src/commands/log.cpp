#include "log.hpp"
#include "../repository/reference.hpp"
#include "../objects/commit.hpp"
#include "../filesystem/file_utils.hpp"
#include <iostream>
#include <string>
#include <sstream>
#include <set>

int handleLog(int argc, char* argv[]) {
    if (!FileUtils::directoryExists(".minigit")) {
        std::cerr << "fatal: not a minigit repository (or any of the parent directories): .minigit\n";
        return 1;
    }

    bool oneline = false;
    int maxCount = -1;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--oneline") {
            oneline = true;
        } else if (arg == "-n" && i + 1 < argc) {
            maxCount = std::stoi(argv[++i]);
        } else if (arg.rfind("-n", 0) == 0 && arg.size() > 2) {
            maxCount = std::stoi(arg.substr(2));
        }
    }

    std::string currentHash;
    if (!Repository::Reference::resolveHead(currentHash, ".minigit")) {
        std::cerr << "fatal: your current branch does not have any commits yet\n";
        return 1;
    }

    std::set<std::string> visited;
    int count = 0;

    while (!currentHash.empty() && (maxCount == -1 || count < maxCount)) {
        if (visited.count(currentHash)) {
            break;
        }
        visited.insert(currentHash);

        Objects::Commit commit;
        if (!Objects::Commit::load(currentHash, commit, ".minigit")) {
            std::cerr << "error: unable to load commit object " << currentHash << "\n";
            break;
        }

        if (oneline) {
            std::string shortHash = currentHash.substr(0, 7);
            std::string firstLine = commit.getMessage();
            size_t nl = firstLine.find('\n');
            if (nl != std::string::npos) {
                firstLine = firstLine.substr(0, nl);
            }
            std::cout << shortHash << " " << firstLine << "\n";
        } else {
            std::cout << "commit " << currentHash << "\n";
            if (commit.getParentHashes().size() > 1) {
                std::cout << "Merge:";
                for (const auto& p : commit.getParentHashes()) {
                    std::cout << " " << p.substr(0, 7);
                }
                std::cout << "\n";
            }
            std::cout << "Author: " << commit.getAuthor() << "\n";
            std::cout << "Date:   " << commit.getTimestamp() << "\n\n";

            // Indent commit message
            std::istringstream msgStream(commit.getMessage());
            std::string msgLine;
            while (std::getline(msgStream, msgLine)) {
                std::cout << "    " << msgLine << "\n";
            }
            std::cout << "\n";
        }

        count++;
        currentHash = commit.getFirstParentHash();
    }

    return 0;
}
