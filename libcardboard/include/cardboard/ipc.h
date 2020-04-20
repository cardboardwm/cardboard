#ifndef __LIBCARDBOARD_IPC_H_
#define __LIBCARDBOARD_IPC_H_

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

[[maybe_unused]]
static const char* SOCKET_ENV_VAR = "CARDBOARD_SOCKET";
};

#endif //__LIBCARDBOARD_IPC_H_
