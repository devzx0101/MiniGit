#include "objects/packfile.hpp"
#include "objects/object_store.hpp"
#include "filesystem/file_utils.hpp"
#include "hashing/sha1.hpp"
#include "compression/zlib_utils.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <zlib.h>

namespace fs = std::filesystem;

namespace Objects {

static void writeBe32(std::string& buf, uint32_t val) {
    buf.push_back((char)((val >> 24) & 0xFF));
    buf.push_back((char)((val >> 16) & 0xFF));
    buf.push_back((char)((val >> 8) & 0xFF));
    buf.push_back((char)(val & 0xFF));
}

static uint32_t readBe32(const unsigned char* p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static std::vector<uint8_t> hexToBytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    bytes.reserve(20);
    for (size_t i = 0; i < 40 && i + 1 < hex.size(); i += 2) {
        uint8_t byte = (uint8_t)std::stoul(hex.substr(i, 2), nullptr, 16);
        bytes.push_back(byte);
    }
    return bytes;
}

std::vector<std::string> Packfile::findLooseObjects(const std::string& repoDir) {
    std::vector<std::string> hashes;
    std::string objectsDir = repoDir + "/objects";
    if (!FileUtils::directoryExists(objectsDir)) return hashes;

    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(objectsDir, ec)) {
        if (!entry.is_directory()) continue;
        std::string dirName = entry.path().filename().string();
        if (dirName.length() != 2 || dirName == "pack" || dirName == "info") continue;

        for (const auto& objFile : fs::directory_iterator(entry.path(), ec)) {
            if (objFile.is_regular_file()) {
                std::string fname = objFile.path().filename().string();
                if (fname.length() == 38) {
                    hashes.push_back(dirName + fname);
                }
            }
        }
    }
    std::sort(hashes.begin(), hashes.end());
    return hashes;
}

std::string Packfile::createPack(const std::vector<std::string>& objectHashes, const std::string& repoDir) {
    if (objectHashes.empty()) return "";

    std::string packDir = repoDir + "/objects/pack";
    FileUtils::createDirectories(packDir);

    // Collect valid objects
    std::vector<PackEntry> entries;
    for (const auto& h : objectHashes) {
        std::string type, content;
        if (ObjectStore::readObject(h, type, content, repoDir)) {
            PackEntry pe;
            pe.hash = h;
            pe.type = type;
            pe.content = content;
            pe.offset = 0;
            pe.crc32 = 0;
            entries.push_back(pe);
        }
    }

    if (entries.empty()) return "";

    // Sort entries lexicographically by 40-char SHA-1 hash (Git spec standard)
    std::sort(entries.begin(), entries.end(), [](const PackEntry& a, const PackEntry& b) {
        return a.hash < b.hash;
    });

    // 1. Build .pack buffer
    // Header: 'PACK' (4 bytes), Version 2 (4 bytes), Number of objects (4 bytes)
    std::string packBytes;
    packBytes.append("PACK", 4);
    writeBe32(packBytes, 2);                      // Version 2
    writeBe32(packBytes, (uint32_t)entries.size()); // Object count

    for (auto& pe : entries) {
        pe.offset = (uint32_t)packBytes.size();

        // Encode entry payload: "<type> <size>\0<content>"
        std::string rawData = pe.type + " " + std::to_string(pe.content.size()) + '\0' + pe.content;

        // Compress object with zlib DEFLATE
        std::string compressed;
        if (!Compression::compressString(rawData, compressed)) {
            compressed = rawData;
        }

        // Compute entry CRC32
        uLong crc = crc32(0L, Z_NULL, 0);
        crc = crc32(crc, (const Bytef*)compressed.data(), (uInt)compressed.size());
        pe.crc32 = (uint32_t)crc;

        // Append 4-byte uncompressed size header followed by compressed bytes
        writeBe32(packBytes, (uint32_t)rawData.size());
        packBytes.append(compressed);
    }

    // 20-byte SHA-1 trailer of the packfile
    std::string packSha = Hashing::SHA1::hashString(packBytes);
    std::vector<uint8_t> packShaBytes = hexToBytes(packSha);
    packBytes.append((const char*)packShaBytes.data(), 20);

    // 2. Build .idx (Version 2) fanout index
    // Magic: \xFF t O c (4 bytes)
    // Version: 2 (4 bytes)
    // 256-entry first-level fanout table (4 bytes each, cumulative counts)
    std::string idxBytes;
    idxBytes.push_back('\xff');
    idxBytes.push_back('t');
    idxBytes.push_back('O');
    idxBytes.push_back('c');
    writeBe32(idxBytes, 2); // Version 2

    // Compute 256 fanout table
    uint32_t fanout[256] = {0};
    for (const auto& pe : entries) {
        uint8_t firstByte = (uint8_t)std::stoul(pe.hash.substr(0, 2), nullptr, 16);
        fanout[firstByte]++;
    }
    // Cumulative sum
    uint32_t runningCount = 0;
    for (int i = 0; i < 256; ++i) {
        runningCount += fanout[i];
        writeBe32(idxBytes, runningCount);
    }

    // SHA-1 table: 20 bytes for each object in order
    for (const auto& pe : entries) {
        std::vector<uint8_t> b = hexToBytes(pe.hash);
        idxBytes.append((const char*)b.data(), 20);
    }

    // CRC32 table: 4 bytes for each object
    for (const auto& pe : entries) {
        writeBe32(idxBytes, pe.crc32);
    }

    // Offset table: 4 bytes for each object (offsets in .pack file)
    for (const auto& pe : entries) {
        writeBe32(idxBytes, pe.offset);
    }

    // Pack checksum (20 bytes)
    idxBytes.append((const char*)packShaBytes.data(), 20);

    // Index checksum (20-byte SHA-1 of all preceding idx data)
    std::string idxSha = Hashing::SHA1::hashString(idxBytes);
    std::vector<uint8_t> idxShaBytes = hexToBytes(idxSha);
    idxBytes.append((const char*)idxShaBytes.data(), 20);

    // Write .pack and .idx files to disk
    std::string packFilename = packDir + "/pack-" + packSha + ".pack";
    std::string idxFilename = packDir + "/pack-" + packSha + ".idx";

    FileUtils::writeFile(packFilename, packBytes);
    FileUtils::writeFile(idxFilename, idxBytes);

    return packSha;
}

bool Packfile::readPackedObject(const std::string& hash, std::string& outType, std::string& outContent, const std::string& repoDir) {
    if (hash.length() != 40) return false;

    std::string packDir = repoDir + "/objects/pack";
    if (!FileUtils::directoryExists(packDir)) return false;

    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(packDir, ec)) {
        if (!entry.is_regular_file()) continue;
        std::string fname = entry.path().filename().string();
        if (fname.size() < 4 || fname.substr(fname.size() - 4) != ".idx") continue;

        std::string idxPath = entry.path().string();
        std::string packPath = idxPath.substr(0, idxPath.size() - 4) + ".pack";
        if (!FileUtils::fileExists(packPath)) continue;

        std::string idxBytes;
        if (!FileUtils::readFile(idxPath, idxBytes)) continue;
        if (idxBytes.size() < 4 + 4 + 256 * 4) continue;

        const unsigned char* idxPtr = (const unsigned char*)idxBytes.data();
        if (idxPtr[0] != 0xff || idxPtr[1] != 't' || idxPtr[2] != 'O' || idxPtr[3] != 'c') continue;
        uint32_t version = readBe32(idxPtr + 4);
        if (version != 2) continue;

        // Fanout lookup
        uint8_t firstByte = (uint8_t)std::stoul(hash.substr(0, 2), nullptr, 16);
        uint32_t totalObjects = readBe32(idxPtr + 8 + 255 * 4);
        uint32_t low = (firstByte == 0) ? 0 : readBe32(idxPtr + 8 + (firstByte - 1) * 4);
        uint32_t high = readBe32(idxPtr + 8 + firstByte * 4);

        if (low >= high || high > totalObjects) continue;

        // Binary search within [low, high) range of SHA-1 table
        const unsigned char* shaTable = idxPtr + 8 + 256 * 4;
        std::vector<uint8_t> targetBytes = hexToBytes(hash);

        int foundIdx = -1;
        while (low < high) {
            uint32_t mid = low + (high - low) / 2;
            const unsigned char* midSha = shaTable + mid * 20;
            int cmp = std::memcmp(targetBytes.data(), midSha, 20);
            if (cmp == 0) {
                foundIdx = mid;
                break;
            } else if (cmp < 0) {
                high = mid;
            } else {
                low = mid + 1;
            }
        }

        if (foundIdx != -1) {
            // Read offset from offset table
            const unsigned char* offsetTable = shaTable + totalObjects * 20 + totalObjects * 4;
            uint32_t offset = readBe32(offsetTable + foundIdx * 4);

            std::string packBytes;
            if (!FileUtils::readFile(packPath, packBytes)) return false;
            if (offset + 4 > packBytes.size()) return false;

            const unsigned char* pEntry = (const unsigned char*)packBytes.data() + offset;
            uint32_t uncompressedLen = readBe32(pEntry);
            std::string compressed(packBytes.data() + offset + 4);

            std::string rawData;
            if (Compression::isZlibCompressed(compressed)) {
                Compression::decompressString(compressed, rawData);
            } else {
                rawData = compressed.substr(0, uncompressedLen);
            }

            size_t nullPos = rawData.find('\0');
            if (nullPos == std::string::npos) return false;

            std::string header = rawData.substr(0, nullPos);
            outContent = rawData.substr(nullPos + 1);

            size_t spacePos = header.find(' ');
            if (spacePos != std::string::npos) {
                outType = header.substr(0, spacePos);
            } else {
                outType = "blob";
            }
            return true;
        }
    }

    return false;
}

} // namespace Objects
