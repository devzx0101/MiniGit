#include "commands/stash.hpp"
#include "repository/index.hpp"
#include "repository/reference.hpp"
#include "repository/working_tree.hpp"
#include "objects/commit.hpp"
#include "objects/tree.hpp"
#include "objects/blob.hpp"
#include "filesystem/file_utils.hpp"
#include "terminal.hpp"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <filesystem>

static const std::string STASH_REF = ".minigit/refs/stash";
static const std::string STASH_LOG = ".minigit/logs/refs/stash";

static std::vector<std::pair<std::string, std::string>> loadStashEntries() {
    std::vector<std::pair<std::string, std::string>> entries;
    if (!FileUtils::fileExists(STASH_LOG)) {
        if (FileUtils::fileExists(STASH_REF)) {
            std::string hash;
            FileUtils::readFile(STASH_REF, hash);
            while (!hash.empty() && (hash.back() == '\n' || hash.back() == '\r' || hash.back() == ' ')) {
                hash.pop_back();
            }
            if (!hash.empty()) {
                entries.push_back({hash, "WIP on worktree"});
            }
        }
        return entries;
    }

    std::string content;
    FileUtils::readFile(STASH_LOG, content);
    std::istringstream stream(content);
    std::string line;
    while (std::getline(stream, line)) {
        if (line.empty()) continue;
        size_t tabPos = line.find('\t');
        if (tabPos != std::string::npos) {
            std::string header = line.substr(0, tabPos);
            std::string msg = line.substr(tabPos + 1);
            std::istringstream headerStream(header);
            std::string oldH, newH;
            headerStream >> oldH >> newH;
            entries.push_back({newH, msg});
        }
    }
    return entries;
}

static void saveStashEntries(const std::vector<std::pair<std::string, std::string>>& entries) {
    if (entries.empty()) {
        std::error_code ec;
        std::filesystem::remove(STASH_REF, ec);
        std::filesystem::remove(STASH_LOG, ec);
        return;
    }

    // Write top entry to refs/stash
    FileUtils::createDirectories(".minigit/refs");
    FileUtils::writeFile(STASH_REF, entries.front().first + "\n");

    // Rewrite log
    FileUtils::createDirectories(".minigit/logs/refs");
    std::ostringstream logOut;
    for (const auto& e : entries) {
        logOut << "0000000000000000000000000000000000000000 " 
               << e.first << " MiniGit User <user@minigit.local> 1700000000 +0000\t" 
               << e.second << "\n";
    }
    FileUtils::writeFile(STASH_LOG, logOut.str());
}

int handleStash(int argc, char* argv[]) {
    if (!FileUtils::directoryExists(".minigit")) {
        std::cerr << "fatal: not a minigit repository (or any of the parent directories): .minigit\n";
        return 1;
    }

    std::string subcmd = "push";
    if (argc >= 3) {
        subcmd = argv[2];
    }

    if (subcmd == "list") {
        auto entries = loadStashEntries();
        for (size_t i = 0; i < entries.size(); ++i) {
            std::cout << Color::BoldYellow << "stash@{" << i << "}: " << Color::Reset 
                      << entries[i].second << "\n";
        }
        return 0;
    }

    if (subcmd == "clear") {
        saveStashEntries({});
        std::cout << "Cleared all stash entries.\n";
        return 0;
    }

    if (subcmd == "drop") {
        auto entries = loadStashEntries();
        if (entries.empty()) {
            std::cerr << "error: No stash entries found.\n";
            return 1;
        }
        std::string droppedHash = entries.front().first;
        entries.erase(entries.begin());
        saveStashEntries(entries);
        std::cout << "Dropped stash@{0} (" << droppedHash.substr(0, 7) << ")\n";
        return 0;
    }

    if (subcmd == "pop" || subcmd == "apply") {
        auto entries = loadStashEntries();
        if (entries.empty()) {
            std::cerr << "error: No stash entries found.\n";
            return 1;
        }

        std::string stashHash = entries.front().first;
        Objects::Commit stashCommit;
        if (!Objects::Commit::load(stashHash, stashCommit, ".minigit")) {
            std::cerr << "error: unable to load stash commit " << stashHash << "\n";
            return 1;
        }

        // Apply stashed files to working tree and index
        if (!Repository::WorkingTree::checkoutTree(stashCommit.getTreeHash(), true, ".minigit")) {
            std::cerr << "error: failed to apply stash tree to working directory\n";
            return 1;
        }

        std::cout << "On branch " << Repository::Reference::getCurrentBranchName(".minigit") << "\n"
                  << "Changes applied from stash@{0}\n";

        if (subcmd == "pop") {
            entries.erase(entries.begin());
            saveStashEntries(entries);
            std::cout << "Dropped stash@{0} (" << stashHash.substr(0, 7) << ")\n";
        }
        return 0;
    }

    // Default: stash push / save
    std::string customMsg;
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "-m" || arg == "--message") && i + 1 < argc) {
            customMsg = argv[++i];
        } else if (arg != "push" && arg != "save" && customMsg.empty()) {
            customMsg = arg;
        }
    }

    auto status = Repository::WorkingTree::getStatus(".minigit");
    if (!status.hasStagedChanges() && !status.hasUnstagedChanges()) {
        std::cout << "No local changes to save\n";
        return 0;
    }

    std::string headCommitHash;
    if (!Repository::Reference::resolveHead(headCommitHash, ".minigit")) {
        std::cerr << "fatal: You do not have the initial commit yet\n";
        return 1;
    }

    std::string branchName = Repository::Reference::getCurrentBranchName(".minigit");
    if (branchName.empty()) branchName = "HEAD";

    Objects::Commit headCommit;
    std::string headMsg;
    if (Objects::Commit::load(headCommitHash, headCommit, ".minigit")) {
        headMsg = headCommit.getMessage();
        size_t nl = headMsg.find('\n');
        if (nl != std::string::npos) headMsg = headMsg.substr(0, nl);
    }

    // 1. Build Index Tree & Commit
    Repository::Index index;
    index.load(".minigit");
    std::string indexTreeHash = Objects::Tree::buildFromIndex(index, ".minigit");

    Objects::Commit indexCommit;
    indexCommit.setTreeHash(indexTreeHash);
    indexCommit.addParentHash(headCommitHash);
    indexCommit.setMessage("index on " + branchName + ": " + headCommitHash.substr(0, 7) + " " + headMsg);
    std::string indexCommitHash = indexCommit.save(".minigit");

    // 2. Build Worktree Snapshot Tree
    // Stage all tracked changes into a temporary snapshot index
    Repository::Index worktreeIndex = index;
    for (const auto& path : status.unstagedModified) {
        Objects::Blob blob;
        if (Objects::Blob::fromFile(path, blob)) {
            std::string h = blob.save(".minigit");
            worktreeIndex.addEntry(path, h);
        }
    }
    for (const auto& path : status.unstagedDeleted) {
        worktreeIndex.removeEntry(path);
    }

    std::string worktreeTreeHash = Objects::Tree::buildFromIndex(worktreeIndex, ".minigit");

    // 3. Create Stash Commit (parents: HEAD commit, index commit)
    std::string stashDesc = customMsg.empty() 
        ? ("WIP on " + branchName + ": " + headCommitHash.substr(0, 7) + " " + headMsg)
        : ("On " + branchName + ": " + customMsg);

    Objects::Commit stashCommit;
    stashCommit.setTreeHash(worktreeTreeHash);
    stashCommit.addParentHash(headCommitHash);
    stashCommit.addParentHash(indexCommitHash);
    stashCommit.setMessage(stashDesc);
    std::string stashCommitHash = stashCommit.save(".minigit");

    // 4. Save to refs/stash and prepend to stash log
    auto entries = loadStashEntries();
    entries.insert(entries.begin(), {stashCommitHash, stashDesc});
    saveStashEntries(entries);

    // 5. Restore working directory to clean HEAD state
    Repository::WorkingTree::checkoutTree(headCommit.getTreeHash(), true, ".minigit");

    std::cout << "Saved working directory and index state " << Color::BoldYellow << stashDesc << Color::Reset << "\n";
    return 0;
}
