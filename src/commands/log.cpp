#include "commands/log.hpp"
#include "repository/reference.hpp"
#include "objects/commit.hpp"
#include "filesystem/file_utils.hpp"
#include "terminal.hpp"
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <set>

static std::string formatDecorations(const std::string& commitHash, 
                                     const std::map<std::string, std::vector<std::string>>& decorations) {
    auto it = decorations.find(commitHash);
    if (it == decorations.end() || it->second.empty()) {
        return "";
    }
    std::string result = " (";
    for (size_t i = 0; i < it->second.size(); ++i) {
        if (i > 0) result += ", ";
        result += it->second[i];
    }
    result += ")";
    return result;
}

int handleLog(int argc, char* argv[]) {
    if (!FileUtils::directoryExists(".minigit")) {
        std::cerr << "fatal: not a minigit repository (or any of the parent directories): .minigit\n";
        return 1;
    }

    bool oneline = false;
    bool graph = false;
    int maxCount = -1;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--oneline") {
            oneline = true;
        } else if (arg == "--graph") {
            graph = true;
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

    // Collect branch and tag decorations
    std::map<std::string, std::vector<std::string>> decorations;
    std::string activeBranch = Repository::Reference::getCurrentBranchName(".minigit");
    auto branches = Repository::Reference::listBranches(".minigit");
    for (const auto& b : branches) {
        std::string bHash;
        if (Repository::Reference::getBranchCommit(b.first, bHash, ".minigit")) {
            if (b.second || b.first == activeBranch) {
                decorations[bHash].push_back(std::string(Color::BoldCyan) + "HEAD -> " + Color::BoldGreen + b.first + Color::Reset);
            } else {
                decorations[bHash].push_back(std::string(Color::BoldGreen) + b.first + Color::Reset);
            }
        }
    }

    if (activeBranch.empty() && !currentHash.empty()) {
        decorations[currentHash].insert(decorations[currentHash].begin(), std::string(Color::BoldCyan) + "HEAD" + Color::Reset);
    }

    auto tags = Repository::Reference::listTags(".minigit");
    for (const auto& tag : tags) {
        std::string tHash;
        if (Repository::Reference::getTagCommit(tag, tHash, ".minigit")) {
            decorations[tHash].push_back(std::string(Color::BoldYellow) + "tag: " + tag + Color::Reset);
        }
    }

    std::set<std::string> visited;
    int count = 0;

    const std::string graphNode = std::string(Color::BoldCyan) + "* " + Color::Reset;
    const std::string graphLine = std::string(Color::BoldCyan) + "| " + Color::Reset;
    const std::string graphPipe = std::string(Color::BoldCyan) + "|" + Color::Reset;

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

        std::string dec = formatDecorations(currentHash, decorations);

        if (oneline) {
            std::string shortHash = currentHash.substr(0, 7);
            std::string firstLine = commit.getMessage();
            size_t nl = firstLine.find('\n');
            if (nl != std::string::npos) {
                firstLine = firstLine.substr(0, nl);
            }

            if (graph) {
                std::cout << graphNode;
            }
            std::cout << Color::BoldYellow << shortHash << Color::Reset 
                      << dec << " " << firstLine << "\n";
        } else {
            if (graph) {
                std::cout << graphNode;
            }
            std::cout << Color::Yellow << "commit " << currentHash << Color::Reset << dec << "\n";

            if (commit.getParentHashes().size() > 1) {
                if (graph) std::cout << graphLine;
                std::cout << "Merge:";
                for (const auto& p : commit.getParentHashes()) {
                    std::cout << " " << p.substr(0, 7);
                }
                std::cout << "\n";
            }

            if (graph) std::cout << graphLine;
            std::cout << "Author: " << commit.getAuthor() << "\n";

            if (graph) std::cout << graphLine;
            std::cout << "Date:   " << commit.getTimestamp() << "\n";

            if (graph) {
                std::cout << graphLine << "\n";
            } else {
                std::cout << "\n";
            }

            // Indent commit message
            std::istringstream msgStream(commit.getMessage());
            std::string msgLine;
            while (std::getline(msgStream, msgLine)) {
                if (graph) std::cout << graphLine;
                std::cout << "    " << msgLine << "\n";
            }

            if (graph) {
                std::cout << graphPipe << "\n";
            } else {
                std::cout << "\n";
            }
        }

        count++;
        currentHash = commit.getFirstParentHash();
    }

    return 0;
}
