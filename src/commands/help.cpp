#include "commands/help.hpp"
#include "terminal.hpp"
#include <iostream>

int handleHelp(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    std::cout << Color::BoldCyan << "Mini-Git" << Color::Reset 
              << ": A serious, functional educational Git implementation in C++\n\n"
              << Color::BoldWhite << "Usage:" << Color::Reset << " minigit <command> [<args>]\n\n"
              << Color::BoldYellow << "Start a working area:\n" << Color::Reset
              << "   " << Color::BoldGreen << "init" << Color::Reset << "                   Create an empty Mini-Git repository\n"
              << "   " << Color::BoldGreen << "clone" << Color::Reset << " <repo> [<dir>]   Clone a repository into a new directory\n\n"
              << Color::BoldYellow << "Work on the current change:\n" << Color::Reset
              << "   " << Color::BoldGreen << "add" << Color::Reset << " <file>...          Add file contents to the staging area\n"
              << "   " << Color::BoldGreen << "restore" << Color::Reset << " <file>...      Restore working tree files or unstage (--staged)\n"
              << "   " << Color::BoldGreen << "reset" << Color::Reset << " [<file>...]      Reset current HEAD to the specified state\n"
              << "   " << Color::BoldGreen << "status" << Color::Reset << "                 Show the working tree status\n"
              << "   " << Color::BoldGreen << "diff" << Color::Reset << " [--staged]        Show changes between commits, commit and working tree\n"
              << "   " << Color::BoldGreen << "stash" << Color::Reset << " [push|pop|list]  Stash changes in a dirty working directory away\n"
              << "   " << Color::BoldGreen << "clean" << Color::Reset << " [-f] [-d]        Remove untracked files from the working tree\n\n"
              << Color::BoldYellow << "Examine the history and state:\n" << Color::Reset
              << "   " << Color::BoldGreen << "log" << Color::Reset << " [--oneline] [--graph] Show commit logs with graph and decorations\n"
              << "   " << Color::BoldGreen << "reflog" << Color::Reset << "                 Manage and display reflog information\n"
              << "   " << Color::BoldGreen << "cat-file" << Color::Reset << " (-p|-t|-s)    Provide content or type and size information for objects\n"
              << "   " << Color::BoldGreen << "ls-files" << Color::Reset << " [--stage]     Show information about files in the index\n"
              << "   " << Color::BoldGreen << "hash-object" << Color::Reset << " [-w]       Compute object ID and optionally create a blob\n\n"
              << Color::BoldYellow << "Grow, mark and tweak your common history:\n" << Color::Reset
              << "   " << Color::BoldGreen << "branch" << Color::Reset << " [-d] [<name>]   List, create, or delete branches\n"
              << "   " << Color::BoldGreen << "checkout" << Color::Reset << " [-b] <name>   Switch branches or restore working tree files\n"
              << "   " << Color::BoldGreen << "commit" << Color::Reset << " -m <msg>        Record changes to the repository\n"
              << "   " << Color::BoldGreen << "merge" << Color::Reset << " <branch>         Join two or more development histories together\n"
              << "   " << Color::BoldGreen << "rebase" << Color::Reset << " <upstream>      Reapply commits on top of another base tip\n"
              << "   " << Color::BoldGreen << "cherry-pick" << Color::Reset << " <commit>   Apply the changes introduced by an existing commit\n"
              << "   " << Color::BoldGreen << "tag" << Color::Reset << " [-d] [<name>]      Create, list, or delete tag objects\n\n"
              << Color::BoldYellow << "Collaborate with remotes:\n" << Color::Reset
              << "   " << Color::BoldGreen << "remote" << Color::Reset << " [add|remove]    Manage set of tracked repositories\n"
              << "   " << Color::BoldGreen << "fetch" << Color::Reset << " [<remote>]       Download objects and refs from another repository\n"
              << "   " << Color::BoldGreen << "push" << Color::Reset << " [<remote>]        Update remote refs along with associated objects\n"
              << "   " << Color::BoldGreen << "pull" << Color::Reset << " [<remote>]        Fetch from and integrate with another repository\n";
    return 0;
}
