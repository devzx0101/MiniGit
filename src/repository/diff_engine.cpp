#include "diff_engine.hpp"
#include <sstream>
#include <vector>
#include <algorithm>

namespace Repository {

std::vector<std::string> DiffEngine::splitLines(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(line);
    }
    return lines;
}

std::string DiffEngine::unifiedDiff(const std::string& filePath,
                                   const std::string& oldContent,
                                   const std::string& newContent,
                                   const std::string& oldLabel,
                                   const std::string& newLabel) {
    if (oldContent == newContent) {
        return "";
    }

    std::vector<std::string> a = splitLines(oldContent);
    std::vector<std::string> b = splitLines(newContent);

    size_t n = a.size();
    size_t m = b.size();

    // LCS dynamic programming table
    std::vector<std::vector<int>> dp(n + 1, std::vector<int>(m + 1, 0));
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < m; ++j) {
            if (a[i] == b[j]) {
                dp[i + 1][j + 1] = dp[i][j] + 1;
            } else {
                dp[i + 1][j + 1] = std::max(dp[i + 1][j], dp[i][j + 1]);
            }
        }
    }

    // Backtrack to find edit operations
    struct DiffLine {
        char op; // ' ', '-', '+'
        std::string line;
    };
    std::vector<DiffLine> diff;

    int i = (int)n;
    int j = (int)m;
    while (i > 0 || j > 0) {
        if (i > 0 && j > 0 && a[i - 1] == b[j - 1]) {
            diff.push_back({' ', a[i - 1]});
            i--;
            j--;
        } else if (j > 0 && (i == 0 || dp[i][j - 1] >= dp[i - 1][j])) {
            diff.push_back({'+', b[j - 1]});
            j--;
        } else if (i > 0 && (j == 0 || dp[i][j - 1] < dp[i - 1][j])) {
            diff.push_back({'-', a[i - 1]});
            i--;
        }
    }

    std::reverse(diff.begin(), diff.end());

    std::ostringstream out;
    out << "diff --minigit " << oldLabel << "/" << filePath << " " << newLabel << "/" << filePath << "\n";
    out << "--- " << oldLabel << "/" << filePath << "\n";
    out << "+++ " << newLabel << "/" << filePath << "\n";
    out << "@@ -1," << n << " +1," << m << " @@\n";

    for (const auto& dl : diff) {
        out << dl.op << dl.line << "\n";
    }

    return out.str();
}

} // namespace Repository
