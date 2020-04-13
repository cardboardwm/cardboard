#include <ipc.h>

#include <bit>

using namespace libcardboard::ipc;

Header interpret_header(const AlignedHeaderBuffer& buffer)
{
    if constexpr(std::endian::native == std::endian::little)
        return {*reinterpret_cast<const int32_t*>(buffer.data())};
    else
    {
        uint32_t r = *reinterpret_cast<const uint32_t*>(buffer.data());
        r = (r & 0x000000ffu) << 24u |
            (r & 0x0000ff00u) << 8u  |
            (r & 0x00ff0000u) >> 8u  |
            (r & 0xff000000u) >> 24u;

        return {static_cast<int>(r)};
    }
}