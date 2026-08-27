#ifndef COMMIT_HPP
#define COMMIT_HPP

#include <string>
#include <vector>

namespace Objects {

class Commit {
public:
    Commit();

    // Setters
    void setTreeHash(const std::string& hash);
    void addParentHash(const std::string& hash);
    void setParentHashes(const std::vector<std::string>& parents);
    void setAuthor(const std::string& author);
    void setCommitter(const std::string& committer);
    void setTimestamp(const std::string& timestamp);
    void setMessage(const std::string& message);

    // Getters
    const std::string& getTreeHash() const;
    const std::vector<std::string>& getParentHashes() const;
    std::string getFirstParentHash() const;
    const std::string& getAuthor() const;
    const std::string& getCommitter() const;
    const std::string& getTimestamp() const;
    const std::string& getMessage() const;

    // Serialization & Deserialization
    std::string serialize() const;
    bool deserialize(const std::string& content);

    // Storage
    std::string save(const std::string& repoDir = ".minigit") const;
    static bool load(const std::string& hash, Commit& outCommit, const std::string& repoDir = ".minigit");

private:
    std::string treeHash;
    std::vector<std::string> parentHashes;
    std::string author;
    std::string committer;
    std::string timestamp;
    std::string message;
};

} // namespace Objects

#endif // COMMIT_HPP
