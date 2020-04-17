#ifndef BUILD_LIBCARDBOARD_INCLUDE_IPC_H
#define BUILD_LIBCARDBOARD_INCLUDE_IPC_H

#include <array>
#include <cstddef>
#include <cstdint>

namespace libcardboard::ipc {
struct Header {
    int incoming_bytes;
};
constexpr std::size_t HEADER_SIZE = 4;

struct alignas(alignof(Header)) AlignedHeaderBuffer : std::array<std::byte, HEADER_SIZE> {
};

Header interpret_header(const AlignedHeaderBuffer&);
AlignedHeaderBuffer create_header_buffer(const Header&);
};

#endif //BUILD_LIBCARDBOARD_INCLUDE_IPC_H
