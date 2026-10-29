#include <iostream>
#include <string>
#include <vector>
#include "terminal.hpp"
#include "commands/init.hpp"
#include "commands/add.hpp"
#include "commands/commit.hpp"
#include "commands/status.hpp"
#include "commands/log.hpp"
#include "commands/branch.hpp"
#include "commands/checkout.hpp"
#include "commands/diff.hpp"
#include "commands/reset.hpp"
#include "commands/restore.hpp"
#include "commands/tag.hpp"
#include "commands/merge.hpp"
#include "commands/reflog.hpp"
#include "commands/cat_file.hpp"
#include "commands/ls_files.hpp"
#include "commands/hash_object.hpp"
#include "commands/remote.hpp"
#include "commands/stash.hpp"
#include "commands/clean.hpp"
#include "commands/cherry_pick.hpp"
#include "commands/rebase.hpp"
#include "commands/help.hpp"

int main(int argc, char* argv[]) {
    Color::init();

    if (argc < 2) {
        std::cerr << Color::BoldRed << "Error: " << Color::Reset 
                  << "No command specified.\n\n";
        handleHelp(argc, argv);
        return 1;
    }

    std::string command = argv[1];

    if (command == "help" || command == "--help" || command == "-h") {
        return handleHelp(argc, argv);
    } else if (command == "init") {
        return handleInit(argc, argv);
    } else if (command == "add") {
        return handleAdd(argc, argv);
    } else if (command == "commit") {
        return handleCommit(argc, argv);
    } else if (command == "status") {
        return handleStatus(argc, argv);
    } else if (command == "log") {
        return handleLog(argc, argv);
    } else if (command == "branch") {
        return handleBranch(argc, argv);
    } else if (command == "checkout") {
        return handleCheckout(argc, argv);
    } else if (command == "diff") {
        return handleDiff(argc, argv);
    } else if (command == "reset") {
        return handleReset(argc, argv);
    } else if (command == "restore") {
        return handleRestore(argc, argv);
    } else if (command == "tag") {
        return handleTag(argc, argv);
    } else if (command == "merge") {
        return handleMerge(argc, argv);
    } else if (command == "reflog") {
        return handleReflog(argc, argv);
    } else if (command == "cat-file") {
        return handleCatFile(argc, argv);
    } else if (command == "ls-files") {
        return handleLsFiles(argc, argv);
    } else if (command == "hash-object") {
        return handleHashObject(argc, argv);
    } else if (command == "remote") {
        return handleRemote(argc, argv);
    } else if (command == "clone") {
        return handleClone(argc, argv);
    } else if (command == "fetch") {
        return handleFetch(argc, argv);
    } else if (command == "push") {
        return handlePush(argc, argv);
    } else if (command == "pull") {
        return handlePull(argc, argv);
    } else if (command == "stash") {
        return handleStash(argc, argv);
    } else if (command == "clean") {
        return handleClean(argc, argv);
    } else if (command == "cherry-pick") {
        return handleCherryPick(argc, argv);
    } else if (command == "rebase") {
        return handleRebase(argc, argv);
    } else {
        std::cerr << Color::BoldRed << "minigit: " << Color::Reset 
                  << "'" << command << "' is not a minigit command. See '" 
                  << Color::BoldYellow << "minigit help" << Color::Reset << "'.\n";
        return 1;
    }
}
