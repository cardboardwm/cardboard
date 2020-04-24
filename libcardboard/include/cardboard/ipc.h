#ifndef __LIBCARDBOARD_IPC_H_
#define __LIBCARDBOARD_IPC_H_

#include <array>
#include <cstddef>
#include <cstdint>

/// Utility code for connecting to cardboard's socket and reading and writing the IPC header.
namespace libcardboard::ipc {

/**
 * \brief The IPC header that describes the payload
 */
struct Header {
    int incoming_bytes;
};

/**
 * \brief The size of the IPC header in bytes
 */
constexpr std::size_t HEADER_SIZE = 4;

/**
 * \brief Buffer type where the IPC header can be stored for fast serialization and deserialization
 */
struct alignas(alignof(Header)) AlignedHeaderBuffer : std::array<std::byte, HEADER_SIZE> {
};

/**
 * \brief Deserializes the data in the buffer into a Header value
 */
Header interpret_header(const AlignedHeaderBuffer&);

/**
 * \brief Serializes the Header value into the returned buffer
 */
AlignedHeaderBuffer create_header_buffer(const Header&);

/**
 * \brief The environment variable name where CARDBOARD_SOCKET path will be stored
 */
[[maybe_unused]]
static const char* SOCKET_ENV_VAR = "CARDBOARD_SOCKET";

};

#endif //__LIBCARDBOARD_IPC_H_
