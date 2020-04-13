#ifndef BUILD_LIBCARDBOARD_INCLUDE_IPC_H
#define BUILD_LIBCARDBOARD_INCLUDE_IPC_H

#include <cstdint>
#include <cstddef>
#include <array>

namespace libcardboard::ipc{
    struct Header{
        int incoming_bytes;
    };
    constexpr std::size_t HEADER_SIZE = 4;

    struct alignas(alignof(Header)) AlignedHeaderBuffer: std::array<std::byte, HEADER_SIZE> {};

    Header interpret_header(const AlignedHeaderBuffer&);
};

#endif //BUILD_LIBCARDBOARD_INCLUDE_IPC_H
