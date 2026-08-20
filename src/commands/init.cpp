#include "init.hpp"
#include "../filesystem/file_utils.hpp"
#include <iostream>

int handleInit(int argc, char* argv[]) {
    const std::string repoDir = ".minigit";

    bool alreadyExists = FileUtils::directoryExists(repoDir);

    // Create directory tree
    if (!FileUtils::createDirectories(repoDir + "/objects") ||
        !FileUtils::createDirectories(repoDir + "/refs/heads")) {
        std::cerr << "Error: Failed to create repository directories.\n";
        return 1;
    }

    // Create HEAD pointing to default branch (refs/heads/main)
    const std::string headPath = repoDir + "/HEAD";
    if (!FileUtils::fileExists(headPath)) {
        if (!FileUtils::writeFile(headPath, "ref: refs/heads/main\n")) {
            std::cerr << "Error: Failed to create HEAD file.\n";
            return 1;
        }
    }

    // Create default config file
    const std::string configPath = repoDir + "/config";
    if (!FileUtils::fileExists(configPath)) {
        std::string defaultConfig = 
            "[core]\n"
            "\trepositoryformatversion = 0\n"
            "\tfilemode = false\n"
            "\tbare = false\n";
        if (!FileUtils::writeFile(configPath, defaultConfig)) {
            std::cerr << "Error: Failed to create config file.\n";
            return 1;
        }
    }

    // Create empty index (staging area) file if it doesn't exist
    const std::string indexPath = repoDir + "/index";
    if (!FileUtils::fileExists(indexPath)) {
        if (!FileUtils::writeFile(indexPath, "")) {
            std::cerr << "Error: Failed to create index file.\n";
            return 1;
        }
    }

    if (alreadyExists) {
        std::cout << "Reinitialized existing Mini-Git repository in " << repoDir << "/\n";
    } else {
        std::cout << "Initialized empty Mini-Git repository in " << repoDir << "/\n";
    }

    return 0;
}
