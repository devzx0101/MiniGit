#include "help.hpp"
#include <iostream>

int handleHelp(int argc, char* argv[]) {
    std::cout << "Mini-Git: A serious, functional educational Git implementation in C++\n\n"
              << "Usage: minigit <command> [<args>]\n\n"
              << "Start a working area:\n"
              << "   init                   Create an empty Mini-Git repository\n"
              << "   clone <repo> [<dir>]   Clone a repository into a new directory\n\n"
              << "Work on the current change:\n"
              << "   add <file>...          Add file contents to the staging area\n"
              << "   restore <file>...      Restore working tree files or unstage (--staged)\n"
              << "   reset [<file>...]      Reset current HEAD to the specified state\n"
              << "   status                 Show the working tree status\n"
              << "   diff [--staged]        Show changes between commits, commit and working tree\n\n"
              << "Examine the history and state:\n"
              << "   log [--oneline]        Show commit logs\n"
              << "   reflog                 Manage and display reflog information\n"
              << "   cat-file (-p|-t|-s)    Provide content or type and size information for objects\n"
              << "   ls-files [--stage]     Show information about files in the index\n"
              << "   hash-object [-w]       Compute object ID and optionally create a blob\n\n"
              << "Grow, mark and tweak your common history:\n"
              << "   branch [-d] [<name>]   List, create, or delete branches\n"
              << "   checkout [-b] <name>   Switch branches or restore working tree files\n"
              << "   commit -m <msg>        Record changes to the repository\n"
              << "   merge <branch>         Join two or more development histories together\n"
              << "   tag [-d] [<name>]      Create, list, or delete tag objects\n\n"
              << "Collaborate with remotes:\n"
              << "   remote [add|remove]    Manage set of tracked repositories\n"
              << "   fetch [<remote>]       Download objects and refs from another repository\n"
              << "   push [<remote>]        Update remote refs along with associated objects\n"
              << "   pull [<remote>]        Fetch from and integrate with another repository\n";
    return 0;
}
