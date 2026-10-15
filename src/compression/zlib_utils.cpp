#include "compression/zlib_utils.hpp"
#include <zlib.h>
#include <vector>

namespace Compression {

bool isZlibCompressed(const std::string& data) {
    if (data.size() < 2) return false;
    unsigned char b0 = static_cast<unsigned char>(data[0]);
    unsigned char b1 = static_cast<unsigned char>(data[1]);
    return ((b0 & 0x0F) == 8) && (((b0 << 8) + b1) % 31 == 0);
}

bool compressString(const std::string& input, std::string& output) {
    z_stream defstream;
    defstream.zalloc = Z_NULL;
    defstream.zfree = Z_NULL;
    defstream.opaque = Z_NULL;

    if (deflateInit(&defstream, Z_DEFAULT_COMPRESSION) != Z_OK) {
        return false;
    }

    defstream.avail_in = static_cast<uInt>(input.size());
    defstream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(input.data()));

    char outBuffer[32768];
    output.clear();

    do {
        defstream.avail_out = sizeof(outBuffer);
        defstream.next_out = reinterpret_cast<Bytef*>(outBuffer);

        int ret = deflate(&defstream, Z_FINISH);
        if (ret == Z_STREAM_ERROR) {
            deflateEnd(&defstream);
            return false;
        }

        size_t have = sizeof(outBuffer) - defstream.avail_out;
        output.append(outBuffer, have);
    } while (defstream.avail_out == 0);

    deflateEnd(&defstream);
    return true;
}

bool decompressString(const std::string& input, std::string& output) {
    z_stream infstream;
    infstream.zalloc = Z_NULL;
    infstream.zfree = Z_NULL;
    infstream.opaque = Z_NULL;
    infstream.avail_in = static_cast<uInt>(input.size());
    infstream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(input.data()));

    if (inflateInit(&infstream) != Z_OK) {
        return false;
    }

    char outBuffer[32768];
    output.clear();

    int ret;
    do {
        infstream.avail_out = sizeof(outBuffer);
        infstream.next_out = reinterpret_cast<Bytef*>(outBuffer);

        ret = inflate(&infstream, Z_NO_FLUSH);
        if (ret != Z_OK && ret != Z_STREAM_END && ret != Z_BUF_ERROR) {
            inflateEnd(&infstream);
            return false;
        }

        size_t have = sizeof(outBuffer) - infstream.avail_out;
        output.append(outBuffer, have);
    } while (infstream.avail_out == 0 && ret != Z_STREAM_END);

    inflateEnd(&infstream);
    return (ret == Z_STREAM_END);
}

} // namespace Compression
