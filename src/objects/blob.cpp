#include "objects/blob.hpp"
#include "objects/object_store.hpp"
#include "filesystem/file_utils.hpp"
#include <iostream>

namespace Objects {

Blob::Blob() : content(""), hash("") {}

Blob::Blob(const std::string& rawContent)
    : content(rawContent) {
    hash = ObjectStore::computeHash("blob", content);
}

bool Blob::fromFile(const std::string& filePath, Blob& outBlob) {
    std::string fileData;
    if (!FileUtils::readFile(filePath, fileData)) {
        return false;
    }
    outBlob = Blob(fileData);
    return true;
}

bool Blob::load(const std::string& objectHash, Blob& outBlob, const std::string& repoDir) {
    std::string type;
    std::string data;

    if (!ObjectStore::readObject(objectHash, type, data, repoDir)) {
        return false;
    }

    if (type != "blob") {
        std::cerr << "Error: Object " << objectHash << " is of type '" << type << "', expected 'blob'.\n";
        return false;
    }

    outBlob.content = data;
    outBlob.hash = objectHash;
    return true;
}

std::string Blob::save(const std::string& repoDir) {
    hash = ObjectStore::storeObject("blob", content, repoDir);
    return hash;
}

const std::string& Blob::getContent() const {
    return content;
}

const std::string& Blob::getHash() const {
    return hash;
}

size_t Blob::getSize() const {
    return content.size();
}

} // namespace Objects
