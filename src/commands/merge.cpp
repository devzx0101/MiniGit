#include "merge.hpp"
#include "../repository/merge_engine.hpp"
#include "../filesystem/file_utils.hpp"
#include <iostream>
#include <string>

int handleMerge(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "fatal: specify a branch or commit to merge\n"
                  << "usage: minigit merge <branch>\n";
        return 1;
    }

    if (!FileUtils::directoryExists(".minigit")) {
        std::cerr << "fatal: not a minigit repository (or any of the parent directories): .minigit\n";
        return 1;
    }

    std::string target = argv[2];
    auto result = Repository::MergeEngine::merge(target, ".minigit");

    if (result.isAlreadyUpToDate) {
        std::cout << "Already up to date.\n";
        return 0;
    }

    if (result.isFastForward) {
        std::cout << "Updating " << result.baseCommitHash.substr(0, 7) << ".." << result.targetCommitHash.substr(0, 7) << "\n"
                  << "Fast-forward\n";
        return 0;
    }

    for (const auto& file : result.autoMergedFiles) {
        std::cout << "Auto-merging " << file << "\n";
    }

    if (result.hasConflicts) {
        for (const auto& file : result.conflictFiles) {
            std::cout << "CONFLICT (content): Merge conflict in " << file << "\n";
        }
        std::cout << "Automatic merge failed; fix conflicts and then commit the result.\n";
        return 1;
    }

    if (!result.newCommitHash.empty()) {
        std::cout << "Merge made by the 'ort' strategy.\n";
    }

    return 0;
}
