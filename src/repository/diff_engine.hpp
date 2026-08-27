#ifndef DIFF_ENGINE_HPP
#define DIFF_ENGINE_HPP

#include <string>
#include <vector>

namespace Repository {

class DiffEngine {
public:
    // Generate unified line diff between two text contents for a given file path
    static std::string unifiedDiff(const std::string& filePath,
                                   const std::string& oldContent,
                                   const std::string& newContent,
                                   const std::string& oldLabel = "a",
                                   const std::string& newLabel = "b");

    // Split text by newlines
    static std::vector<std::string> splitLines(const std::string& text);
};

} // namespace Repository

#endif // DIFF_ENGINE_HPP
