#include "commands/cat_file.hpp"
#include "objects/object_store.hpp"
#include "repository/reference.hpp"
#include <iostream>
#include <string>

int handleCatFile(int argc, char* argv[]) {
    if (argc < 4) {
        std::cout << "Usage: minigit cat-file (-p | -t | -s) <object>\n";
        return 1;
    }

    std::string mode = argv[2];
    std::string refInput = argv[3];

    std::string hash;
    if (!Repository::Reference::resolveRef(refInput, hash, ".minigit")) {
        std::cerr << "fatal: Not a valid object name " << refInput << "\n";
        return 1;
    }

    std::string type;
    std::string content;

    if (!Objects::ObjectStore::readObject(hash, type, content, ".minigit")) {
        std::cerr << "fatal: Not a valid object name " << hash << "\n";
        return 1;
    }

    if (mode == "-t") {
        std::cout << type << "\n";
    } else if (mode == "-s") {
        std::cout << content.size() << "\n";
    } else if (mode == "-p") {
        std::cout << content;
        if (!content.empty() && content.back() != '\n') {
            std::cout << "\n";
        }
    } else {
        std::cerr << "error: unknown flag " << mode << "\n";
        std::cout << "Usage: minigit cat-file (-p | -t | -s) <object>\n";
        return 1;
    }

    return 0;
}
