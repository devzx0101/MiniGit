#ifndef SHA1_HPP
#define SHA1_HPP

#include <string>
#include <cstdint>

namespace Hashing {

class SHA1 {
public:
    SHA1();
    void update(const std::string& data);
    void update(const uint8_t* data, size_t length);
    std::string final();

    // Convenience static function to compute SHA-1 of a string
    static std::string hashString(const std::string& input);

private:
    void transform(const uint8_t block[64]);

    uint32_t state[5];
    uint8_t buffer[64];
    size_t bufferSize;
    uint64_t bitCount;
};

} // namespace Hashing

#endif // SHA1_HPP
