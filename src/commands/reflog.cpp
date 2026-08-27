#include "reflog.hpp"
#include "../repository/reference.hpp"
#include "../filesystem/file_utils.hpp"
#include <iostream>
#include <string>
#include <algorithm>

int handleReflog(int argc, char* argv[]) {
    if (!FileUtils::directoryExists(".minigit")) {
        std::cerr << "fatal: not a minigit repository (or any of the parent directories): .minigit\n";
        return 1;
    }

    std::string target = (argc >= 3) ? argv[2] : "HEAD";
    auto entries = Repository::Reference::readReflog(target, ".minigit");

    if (entries.empty()) {
        return 0;
    }

    // Most recent entries first
    std::reverse(entries.begin(), entries.end());

    for (size_t i = 0; i < entries.size(); ++i) {
        const auto& e = entries[i];
        std::string shortHash = e.newHash.substr(0, 7);
        std::cout << shortHash << " " << target << "@{" << i << "}: " << e.message << "\n";
    }

    return 0;
}
