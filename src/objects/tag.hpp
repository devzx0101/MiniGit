#ifndef TAG_OBJECT_HPP
#define TAG_OBJECT_HPP

#include <string>

namespace Objects {

class Tag {
public:
    Tag();

    // Setters
    void setObjectHash(const std::string& hash);
    void setObjectType(const std::string& type);
    void setTagName(const std::string& name);
    void setTagger(const std::string& tagger);
    void setTimestamp(const std::string& timestamp);
    void setMessage(const std::string& message);

    // Getters
    const std::string& getObjectHash() const;
    const std::string& getObjectType() const;
    const std::string& getTagName() const;
    const std::string& getTagger() const;
    const std::string& getTimestamp() const;
    const std::string& getMessage() const;

    // Serialization & Deserialization
    std::string serialize() const;
    bool deserialize(const std::string& content);

    // Storage
    std::string save(const std::string& repoDir = ".minigit") const;
    static bool load(const std::string& hash, Tag& outTag, const std::string& repoDir = ".minigit");

private:
    std::string objectHash;
    std::string objectType; // e.g. "commit"
    std::string tagName;
    std::string tagger;
    std::string timestamp;
    std::string message;
};

} // namespace Objects

#endif // TAG_OBJECT_HPP

