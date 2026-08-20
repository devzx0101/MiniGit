#include "sha1.hpp"
#include <sstream>
#include <iomanip>
#include <cstring>

namespace Hashing {

namespace {
    inline uint32_t leftRotate(uint32_t value, size_t count) {
        return (value << count) | (value >> (32 - count));
    }
}

SHA1::SHA1() {
    state[0] = 0x67452301;
    state[1] = 0xEFCDAB89;
    state[2] = 0x98BADCFE;
    state[3] = 0x10325476;
    state[4] = 0xC3D2E1F0;
    bufferSize = 0;
    bitCount = 0;
}

void SHA1::transform(const uint8_t block[64]) {
    uint32_t w[80];

    for (size_t i = 0; i < 16; ++i) {
        w[i] = (static_cast<uint32_t>(block[i * 4 + 0]) << 24)
             | (static_cast<uint32_t>(block[i * 4 + 1]) << 16)
             | (static_cast<uint32_t>(block[i * 4 + 2]) << 8)
             | (static_cast<uint32_t>(block[i * 4 + 3]));
    }

    for (size_t i = 16; i < 80; ++i) {
        w[i] = leftRotate(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
    }

    uint32_t a = state[0];
    uint32_t b = state[1];
    uint32_t c = state[2];
    uint32_t d = state[3];
    uint32_t e = state[4];

    for (size_t i = 0; i < 80; ++i) {
        uint32_t f = 0;
        uint32_t k = 0;

        if (i < 20) {
            f = (b & c) | ((~b) & d);
            k = 0x5A827999;
        } else if (i < 40) {
            f = b ^ c ^ d;
            k = 0x6ED9EBA1;
        } else if (i < 60) {
            f = (b & c) | (b & d) | (c & d);
            k = 0x8F1BBCDC;
        } else {
            f = b ^ c ^ d;
            k = 0xCA62C1D6;
        }

        uint32_t temp = leftRotate(a, 5) + f + e + k + w[i];
        e = d;
        d = c;
        c = leftRotate(b, 30);
        b = a;
        a = temp;
    }

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
}

void SHA1::update(const uint8_t* data, size_t length) {
    for (size_t i = 0; i < length; ++i) {
        buffer[bufferSize++] = data[i];
        if (bufferSize == 64) {
            transform(buffer);
            bitCount += 512;
            bufferSize = 0;
        }
    }
}

void SHA1::update(const std::string& data) {
    update(reinterpret_cast<const uint8_t*>(data.data()), data.size());
}

std::string SHA1::final() {
    uint64_t totalBits = bitCount + (bufferSize * 8);

    // Padding: append single 0x80 byte
    buffer[bufferSize++] = 0x80;

    if (bufferSize > 56) {
        while (bufferSize < 64) {
            buffer[bufferSize++] = 0x00;
        }
        transform(buffer);
        bufferSize = 0;
    }

    while (bufferSize < 56) {
        buffer[bufferSize++] = 0x00;
    }

    // Append 64-bit length in big-endian
    for (int i = 7; i >= 0; --i) {
        buffer[bufferSize++] = static_cast<uint8_t>((totalBits >> (i * 8)) & 0xFF);
    }
    transform(buffer);

    // Convert state words to 40-character hexadecimal string
    std::ostringstream result;
    result << std::hex << std::setfill('0');
    for (size_t i = 0; i < 5; ++i) {
        result << std::setw(8) << state[i];
    }

    return result.str();
}

std::string SHA1::hashString(const std::string& input) {
    SHA1 hasher;
    hasher.update(input);
    return hasher.final();
}

} // namespace Hashing
