#ifndef IGNORE_HPP
#define IGNORE_HPP

#include <string>
#include <vector>

namespace Repository {

class Ignore {
public:
    Ignore();

    // Load patterns from .minigitignore and .gitignore in the working directory
    void load(const std::string& rootDir = ".");

    // Add a single ignore rule
    void addPattern(const std::string& pattern);

    // Check if a relative file path matches any ignore rule
    bool isIgnored(const std::string& relativePath) const;

private:
    std::vector<std::string> patterns;

    // Helper: wildcard match (* and ?)
    static bool matchPattern(const std::string& pattern, const std::string& path);
};

} // namespace Repository

#endif // IGNORE_HPP
