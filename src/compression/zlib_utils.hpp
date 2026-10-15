#ifndef ZLIB_UTILS_HPP
#define ZLIB_UTILS_HPP

#include <string>

namespace Compression {

// Compress data with standard zlib deflate
bool compressString(const std::string& input, std::string& output);

// Decompress zlib-deflated data
bool decompressString(const std::string& input, std::string& output);

// Check if data begins with a valid zlib header
bool isZlibCompressed(const std::string& data);

} // namespace Compression

#endif // ZLIB_UTILS_HPP
