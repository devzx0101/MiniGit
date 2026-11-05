#include "commands/gc.hpp"
#include "objects/packfile.hpp"
#include "filesystem/file_utils.hpp"
#include "terminal.hpp"
#include <iostream>
#include <vector>
#include <string>

int handleGc(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    if (!FileUtils::directoryExists(".minigit")) {
        std::cerr << "fatal: not a minigit repository (or any of the parent directories): .minigit\n";
        return 1;
    }

    std::cout << "Enumerating objects...\n";
    auto looseObjects = Objects::Packfile::findLooseObjects(".minigit");

    if (looseObjects.empty()) {
        std::cout << "No loose objects found to pack.\n";
        return 0;
    }

    std::cout << "Counting objects: " << looseObjects.size() << ", done.\n"
              << "Compressing objects: 100% (" << looseObjects.size() << "/" << looseObjects.size() << "), done.\n"
              << "Writing objects: 100% (" << looseObjects.size() << "/" << looseObjects.size() << "), done.\n";

    std::string packSha = Objects::Packfile::createPack(looseObjects, ".minigit");
    if (packSha.empty()) {
        std::cerr << "fatal: failed to create packfile\n";
        return 1;
    }

    // Prune loose files that have been packed
    for (const auto& h : looseObjects) {
        std::string objPath = ".minigit/objects/" + h.substr(0, 2) + "/" + h.substr(2);
        FileUtils::removeFile(objPath);
    }

    std::cout << Color::BoldGreen << "Built pack-" << packSha 
              << ".pack (" << looseObjects.size() << " objects compacted)" << Color::Reset << "\n";

    return 0;
}

