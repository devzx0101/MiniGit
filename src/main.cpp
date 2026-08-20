#include <iostream>
#include <string>
#include <vector>
#include "commands/init.hpp"

void printUsage() {
    std::cout << "Usage: minigit <command> [<args>]\n\n"
              << "Available commands:\n"
              << "   init       Create an empty Mini-Git repository\n"
              << "   help       Show available commands\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Error: No command specified.\n\n";
        printUsage();
        return 1;
    }

    std::string command = argv[1];

    if (command == "help" || command == "--help" || command == "-h") {
        printUsage();
        return 0;
    } else if (command == "init") {
        return handleInit(argc, argv);
    } else {
        std::cerr << "minigit: '" << command << "' is not a minigit command. See 'minigit help'.\n";
        return 1;
    }
}
