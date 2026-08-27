#include "commands/hash_object.hpp"
#include "objects/blob.hpp"
#include <iostream>
#include <string>

int handleHashObject(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage: minigit hash-object [-w] <file>\n";
        return 1;
    }

    bool write = false;
    std::string filePath;

    if (std::string(argv[2]) == "-w") {
        if (argc < 4) {
            std::cerr << "Error: No file specified.\n";
            return 1;
        }
        write = true;
        filePath = argv[3];
    } else {
        filePath = argv[2];
    }

    Objects::Blob blob;
    if (!Objects::Blob::fromFile(filePath, blob)) {
        std::cerr << "fatal: Cannot open '" << filePath << "': No such file\n";
        return 1;
    }

    if (write) {
        std::string hash = blob.save();
        if (hash.empty()) {
            return 1;
        }
        std::cout << hash << "\n";
    } else {
        std::cout << blob.getHash() << "\n";
    }

    return 0;
}
