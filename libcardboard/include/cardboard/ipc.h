// SPDX-License-Identifier: GPL-3.0-only
/*
Copyright (C) 2020 Alexandru-Iulian Magan, Tudor-Ioan Roman, and contributors.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef LIBCARDBOARD_IPC_H_INCLUDED
#define LIBCARDBOARD_IPC_H_INCLUDED

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
[[maybe_unused]] static const char* SOCKET_ENV_VAR = "CARDBOARD_SOCKET";

};

#endif //LIBCARDBOARD_IPC_H_INCLUDED
