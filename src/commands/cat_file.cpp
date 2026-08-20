#include "cat_file.hpp"
#include "../objects/object_store.hpp"
#include <iostream>
#include <string>

int handleCatFile(int argc, char* argv[]) {
    if (argc < 4 || std::string(argv[2]) != "-p") {
        std::cout << "Usage: minigit cat-file -p <object-hash>\n";
        return 1;
    }

    std::string hash = argv[3];
    std::string type;
    std::string content;

    if (!Objects::ObjectStore::readObject(hash, type, content)) {
        std::cerr << "fatal: Not a valid object name " << hash << "\n";
        return 1;
    }

    std::cout << content;
    return 0;
}
