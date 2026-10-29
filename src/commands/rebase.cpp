#include "commands/rebase.hpp"
#include "repository/reference.hpp"
#include "repository/index.hpp"
#include "repository/working_tree.hpp"
#include "repository/merge_engine.hpp"
#include "objects/commit.hpp"
#include "objects/tree.hpp"
#include "objects/blob.hpp"
#include "filesystem/file_utils.hpp"
#include "terminal.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <sstream>

static bool replaySingleCommit(const std::string& targetCommitHash, 
                               std::string& currentHeadHash, 
                               std::string& outConflictFile,
                               const std::string& overrideMessage = "") {
    Objects::Commit targetCommit;
    if (!Objects::Commit::load(targetCommitHash, targetCommit, ".minigit")) {
        return false;
    }

    Objects::Commit headCommit;
    if (!Objects::Commit::load(currentHeadHash, headCommit, ".minigit")) {
        return false;
    }

    std::map<std::string, Repository::IndexEntry> baseFiles;
    std::string parentHash = targetCommit.getFirstParentHash();
    if (!parentHash.empty()) {
        Objects::Commit parentCommit;
        if (Objects::Commit::load(parentHash, parentCommit, ".minigit")) {
            Objects::Tree::flatten(parentCommit.getTreeHash(), baseFiles, ".minigit");
        }
    }

    std::map<std::string, Repository::IndexEntry> headFiles;
    Objects::Tree::flatten(headCommit.getTreeHash(), headFiles, ".minigit");

    std::map<std::string, Repository::IndexEntry> targetFiles;
    Objects::Tree::flatten(targetCommit.getTreeHash(), targetFiles, ".minigit");

    std::set<std::string> allFiles;
    for (const auto& p : baseFiles) allFiles.insert(p.first);
    for (const auto& p : headFiles) allFiles.insert(p.first);
    for (const auto& p : targetFiles) allFiles.insert(p.first);

    Repository::Index index;
    index.load(".minigit");

    for (const auto& file : allFiles) {
        bool inBase = baseFiles.count(file) > 0;
        bool inHead = headFiles.count(file) > 0;
        bool inTarget = targetFiles.count(file) > 0;

        std::string baseHash = inBase ? baseFiles[file].hash : "";
        std::string headHash = inHead ? headFiles[file].hash : "";
        std::string targetHash = inTarget ? targetFiles[file].hash : "";

        if (inBase && inTarget && baseHash == targetHash) {
            continue;
        }

        if (!inBase && inTarget) {
            if (!inHead) {
                Objects::Blob b;
                if (Objects::Blob::load(targetHash, b, ".minigit")) {
                    FileUtils::writeFile(file, b.getContent());
                    index.addEntry(file, targetHash);
                }
            } else if (headHash != targetHash) {
                outConflictFile = file;
                return false;
            }
            continue;
        }

        if (inBase && !inTarget) {
            if (inHead) {
                if (headHash == baseHash) {
                    FileUtils::removeFile(file);
                    index.removeEntry(file);
                } else {
                    outConflictFile = file;
                    return false;
                }
            }
            continue;
        }

        if (inBase && inTarget && baseHash != targetHash) {
            if (!inHead) {
                outConflictFile = file;
                return false;
            } else if (headHash == baseHash) {
                Objects::Blob b;
                if (Objects::Blob::load(targetHash, b, ".minigit")) {
                    FileUtils::writeFile(file, b.getContent());
                    index.addEntry(file, targetHash);
                }
            } else if (headHash != targetHash) {
                Objects::Blob baseB, headB, targetB;
                Objects::Blob::load(baseHash, baseB, ".minigit");
                Objects::Blob::load(headHash, headB, ".minigit");
                Objects::Blob::load(targetHash, targetB, ".minigit");

                std::string mergedContent;
                bool clean = Repository::MergeEngine::merge3WayLines(
                    baseB.getContent(), headB.getContent(), targetB.getContent(),
                    "HEAD", targetCommitHash.substr(0, 7), mergedContent
                );

                FileUtils::writeFile(file, mergedContent);
                if (clean) {
                    Objects::Blob newBlob(mergedContent);
                    std::string newBlobHash = newBlob.save(".minigit");
                    index.addEntry(file, newBlobHash);
                } else {
                    outConflictFile = file;
                    return false;
                }
            }
        }
    }

    index.save(".minigit");

    std::string newTreeHash = Objects::Tree::buildFromIndex(index, ".minigit");
    Objects::Commit newCommit;
    newCommit.setTreeHash(newTreeHash);
    newCommit.addParentHash(currentHeadHash);
    newCommit.setAuthor(targetCommit.getAuthor());
    newCommit.setMessage(overrideMessage.empty() ? targetCommit.getMessage() : overrideMessage);
    currentHeadHash = newCommit.save(".minigit");

    return true;
}

static bool squashSingleCommit(const std::string& targetCommitHash,
                               std::string& currentHeadHash,
                               std::string& outConflictFile,
                               const std::string& appendMessage) {
    Objects::Commit targetCommit;
    if (!Objects::Commit::load(targetCommitHash, targetCommit, ".minigit")) return false;

    Objects::Commit headCommit;
    if (!Objects::Commit::load(currentHeadHash, headCommit, ".minigit")) return false;

    std::map<std::string, Repository::IndexEntry> baseFiles;
    std::string parentHash = targetCommit.getFirstParentHash();
    if (!parentHash.empty()) {
        Objects::Commit parentCommit;
        if (Objects::Commit::load(parentHash, parentCommit, ".minigit")) {
            Objects::Tree::flatten(parentCommit.getTreeHash(), baseFiles, ".minigit");
        }
    }

    std::map<std::string, Repository::IndexEntry> headFiles;
    Objects::Tree::flatten(headCommit.getTreeHash(), headFiles, ".minigit");

    std::map<std::string, Repository::IndexEntry> targetFiles;
    Objects::Tree::flatten(targetCommit.getTreeHash(), targetFiles, ".minigit");

    std::set<std::string> allFiles;
    for (const auto& p : baseFiles) allFiles.insert(p.first);
    for (const auto& p : headFiles) allFiles.insert(p.first);
    for (const auto& p : targetFiles) allFiles.insert(p.first);

    Repository::Index index;
    index.load(".minigit");

    for (const auto& file : allFiles) {
        bool inBase = baseFiles.count(file) > 0;
        bool inHead = headFiles.count(file) > 0;
        bool inTarget = targetFiles.count(file) > 0;

        std::string baseHash = inBase ? baseFiles[file].hash : "";
        std::string headHash = inHead ? headFiles[file].hash : "";
        std::string targetHash = inTarget ? targetFiles[file].hash : "";

        if (inBase) {
            if (baseHash == headHash && baseHash == targetHash) continue;
            if (baseHash == headHash && baseHash != targetHash) {
                if (inTarget) {
                    Objects::Blob b;
                    if (Objects::Blob::load(targetHash, b, ".minigit")) {
                        FileUtils::writeFile(file, b.getContent());
                        index.addEntry(file, targetHash);
                    }
                } else {
                    FileUtils::removeFile(file);
                    index.removeEntry(file);
                }
            } else if (baseHash != headHash && baseHash == targetHash) {
                continue;
            } else {
                if (!inHead && !inTarget) continue;
                std::string baseContent = "";
                std::string headContent = "";
                std::string targetContent = "";

                if (inBase) {
                    Objects::Blob b;
                    if (Objects::Blob::load(baseHash, b, ".minigit")) baseContent = b.getContent();
                }
                if (inHead) {
                    Objects::Blob b;
                    if (Objects::Blob::load(headHash, b, ".minigit")) headContent = b.getContent();
                }
                if (inTarget) {
                    Objects::Blob b;
                    if (Objects::Blob::load(targetHash, b, ".minigit")) targetContent = b.getContent();
                }

                std::string mergedContent;
                bool clean = Repository::MergeEngine::merge3WayLines(
                    baseContent, headContent, targetContent,
                    "HEAD", targetCommitHash.substr(0, 7), mergedContent
                );

                FileUtils::writeFile(file, mergedContent);
                if (clean) {
                    Objects::Blob newBlob(mergedContent);
                    std::string newBlobHash = newBlob.save(".minigit");
                    index.addEntry(file, newBlobHash);
                } else {
                    outConflictFile = file;
                    return false;
                }
            }
        } else {
            if (!inHead && inTarget) {
                Objects::Blob b;
                if (Objects::Blob::load(targetHash, b, ".minigit")) {
                    FileUtils::writeFile(file, b.getContent());
                    index.addEntry(file, targetHash);
                }
            } else if (headHash != targetHash) {
                Objects::Blob baseB, headB, targetB;
                Objects::Blob::load(baseHash, baseB, ".minigit");
                Objects::Blob::load(headHash, headB, ".minigit");
                Objects::Blob::load(targetHash, targetB, ".minigit");

                std::string mergedContent;
                bool clean = Repository::MergeEngine::merge3WayLines(
                    baseB.getContent(), headB.getContent(), targetB.getContent(),
                    "HEAD", targetCommitHash.substr(0, 7), mergedContent
                );

                FileUtils::writeFile(file, mergedContent);
                if (clean) {
                    Objects::Blob newBlob(mergedContent);
                    std::string newBlobHash = newBlob.save(".minigit");
                    index.addEntry(file, newBlobHash);
                } else {
                    outConflictFile = file;
                    return false;
                }
            }
        }
    }

    index.save(".minigit");

    std::string newTreeHash = Objects::Tree::buildFromIndex(index, ".minigit");
    Objects::Commit squashedCommit;
    squashedCommit.setTreeHash(newTreeHash);
    squashedCommit.setParentHashes(headCommit.getParentHashes());
    squashedCommit.setAuthor(headCommit.getAuthor());

    std::string combinedMessage = headCommit.getMessage() + "\n\n" + 
        (appendMessage.empty() ? targetCommit.getMessage() : appendMessage);
    squashedCommit.setMessage(combinedMessage);

    currentHeadHash = squashedCommit.save(".minigit");
    return true;
}

int handleRebase(int argc, char* argv[]) {
    if (!FileUtils::directoryExists(".minigit")) {
        std::cerr << "fatal: not a minigit repository (or any of the parent directories): .minigit\n";
        return 1;
    }

    if (argc < 3) {
        std::cerr << "usage: minigit rebase [-i] <upstream>\n";
        return 1;
    }

    bool interactive = false;
    std::string upstreamArg;

    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-i" || arg == "--interactive") {
            interactive = true;
        } else if (upstreamArg.empty()) {
            upstreamArg = arg;
        }
    }

    if (upstreamArg.empty()) {
        std::cerr << "fatal: no upstream specified for rebase\n";
        return 1;
    }
    std::string upstreamCommitHash;
    if (!Repository::Reference::resolveRef(upstreamArg, upstreamCommitHash, ".minigit")) {
        std::cerr << "fatal: invalid upstream '" << upstreamArg << "'\n";
        return 1;
    }

    std::string currentBranch = Repository::Reference::getCurrentBranchName(".minigit");
    if (currentBranch.empty()) {
        std::cerr << "fatal: cannot rebase detached HEAD\n";
        return 1;
    }

    std::string currentHeadHash;
    if (!Repository::Reference::resolveHead(currentHeadHash, ".minigit")) {
        std::cerr << "fatal: current branch has no commits\n";
        return 1;
    }

    auto status = Repository::WorkingTree::getStatus(".minigit");
    if (status.hasStagedChanges() || status.hasUnstagedChanges()) {
        std::cerr << "error: cannot rebase: you have unstaged or uncommitted changes.\n"
                  << "hint: please commit or stash them.\n";
        return 1;
    }

    if (currentHeadHash == upstreamCommitHash) {
        std::cout << "Current branch " << currentBranch << " is up to date.\n";
        return 0;
    }

    std::string lca = Repository::MergeEngine::findLCA(currentHeadHash, upstreamCommitHash, ".minigit");

    if (lca == upstreamCommitHash) {
        std::cout << "Current branch " << currentBranch << " is up to date.\n";
        return 0;
    }

    if (lca == currentHeadHash) {
        // Fast-forward rebase
        Repository::Reference::setBranchCommit(currentBranch, upstreamCommitHash, ".minigit");
        Objects::Commit upCommit;
        Objects::Commit::load(upstreamCommitHash, upCommit, ".minigit");
        Repository::WorkingTree::checkoutTree(upCommit.getTreeHash(), true, ".minigit");
        std::cout << "Fast-forwarded " << Color::BoldGreen << currentBranch << Color::Reset 
                  << " to " << Color::BoldYellow << upstreamArg << Color::Reset << ".\n";
        return 0;
    }

    // Collect commits from currentHead down to LCA
    std::vector<std::string> commitsToReplay;
    std::string curr = currentHeadHash;
    while (!curr.empty() && curr != lca) {
        commitsToReplay.push_back(curr);
        Objects::Commit c;
        if (!Objects::Commit::load(curr, c, ".minigit")) break;
        curr = c.getFirstParentHash();
    }

    // Reverse to apply from oldest to newest
    std::reverse(commitsToReplay.begin(), commitsToReplay.end());

    // Interactive rebase todo list check
    std::string todoPath = ".minigit/rebase-todo";
    std::vector<std::pair<std::string, std::string>> todoInstructions; // {action, commitHash}

    if (interactive) {
        if (!FileUtils::fileExists(todoPath)) {
            // Generate template todo file
            std::ostringstream todoTemplate;
            todoTemplate << "# Commands:\n# p, pick <commit> = use commit\n# r, reword <commit> = use commit, but edit the commit message\n# s, squash <commit> = use commit, but meld into previous commit\n# d, drop <commit> = remove commit\n#\n";
            for (const auto& cHash : commitsToReplay) {
                Objects::Commit c;
                Objects::Commit::load(cHash, c, ".minigit");
                std::string msg = c.getMessage();
                size_t nl = msg.find('\n');
                if (nl != std::string::npos) msg = msg.substr(0, nl);
                todoTemplate << "pick " << cHash.substr(0, 7) << " " << msg << "\n";
            }
            FileUtils::writeFile(todoPath, todoTemplate.str());
            std::cout << Color::BoldYellow << "Interactive rebase todo list created at " 
                      << todoPath << Color::Reset << "\n"
                      << "Edit this file to pick/squash/drop/reword, then re-run 'minigit rebase -i " 
                      << upstreamArg << "'\n";
        }

        // Parse todo list if it exists
        std::string todoContent;
        FileUtils::readFile(todoPath, todoContent);
        std::istringstream todoStream(todoContent);
        std::string line;
        while (std::getline(todoStream, line)) {
            if (line.empty() || line[0] == '#') continue;
            std::istringstream lineStream(line);
            std::string action, shortH;
            lineStream >> action >> shortH;
            if (action.empty() || shortH.empty()) continue;

            for (const auto& fullH : commitsToReplay) {
                if (fullH.rfind(shortH, 0) == 0) {
                    todoInstructions.push_back({action, fullH});
                    break;
                }
            }
        }
    }

    std::cout << "First, rewinding head to replay your work on top of it...\n";

    // Set working tree to upstream
    Objects::Commit upCommit;
    Objects::Commit::load(upstreamCommitHash, upCommit, ".minigit");
    Repository::WorkingTree::checkoutTree(upCommit.getTreeHash(), true, ".minigit");

    std::string newHead = upstreamCommitHash;

    if (interactive && !todoInstructions.empty()) {
        std::cout << "Applying " << todoInstructions.size() << " interactive instruction(s)...\n";
        for (const auto& inst : todoInstructions) {
            std::string action = inst.first;
            std::string cHash = inst.second;
            std::string conflictFile;

            Objects::Commit c;
            Objects::Commit::load(cHash, c, ".minigit");
            std::string msg = c.getMessage();
            size_t nl = msg.find('\n');
            if (nl != std::string::npos) msg = msg.substr(0, nl);

            if (action == "drop" || action == "d") {
                std::cout << Color::BoldRed << "Dropping: " << Color::Reset << cHash.substr(0, 7) << " " << msg << "\n";
                continue;
            } else if (action == "squash" || action == "s") {
                std::cout << Color::BoldMagenta << "Squashing: " << Color::Reset << cHash.substr(0, 7) << " " << msg << "\n";
                if (!squashSingleCommit(cHash, newHead, conflictFile, msg)) {
                    std::cerr << Color::BoldRed << "CONFLICT in " << conflictFile << Color::Reset << "\n";
                    return 1;
                }
            } else if (action == "reword" || action == "r") {
                std::cout << Color::BoldCyan << "Rewording: " << Color::Reset << cHash.substr(0, 7) << " " << msg << "\n";
                std::string rewordedMsg = msg + " (reworded)";
                if (!replaySingleCommit(cHash, newHead, conflictFile, rewordedMsg)) {
                    std::cerr << Color::BoldRed << "CONFLICT in " << conflictFile << Color::Reset << "\n";
                    return 1;
                }
            } else { // pick / default
                std::cout << "Applying (pick): " << cHash.substr(0, 7) << " " << msg << "\n";
                if (!replaySingleCommit(cHash, newHead, conflictFile)) {
                    std::cerr << Color::BoldRed << "CONFLICT in " << conflictFile << Color::Reset << "\n";
                    return 1;
                }
            }
        }
        FileUtils::removeFile(todoPath);
    } else {
        std::cout << "Applying " << commitsToReplay.size() << " commit(s)...\n";
        for (size_t i = 0; i < commitsToReplay.size(); ++i) {
            std::string conflictFile;
            Objects::Commit c;
            Objects::Commit::load(commitsToReplay[i], c, ".minigit");
            std::string msg = c.getMessage();
            size_t nl = msg.find('\n');
            if (nl != std::string::npos) msg = msg.substr(0, nl);

            std::cout << "Applying: " << msg << "\n";

            if (!replaySingleCommit(commitsToReplay[i], newHead, conflictFile)) {
                std::cerr << Color::BoldRed << "CONFLICT in " << conflictFile << Color::Reset << "\n"
                          << "Failed to merge in the changes.\n"
                          << "Resolve all conflicts manually, mark them as resolved with 'minigit add <file>', then commit.\n";
                return 1;
            }
        }
    }

    // Update branch pointer to the newly rebased commit
    Repository::Reference::setBranchCommit(currentBranch, newHead, ".minigit");
    Repository::Reference::setHeadSymbolic("refs/heads/" + currentBranch, ".minigit");

    std::cout << Color::BoldGreen << "Successfully rebased and updated refs/heads/" 
              << currentBranch << Color::Reset << ".\n";

    return 0;
}
