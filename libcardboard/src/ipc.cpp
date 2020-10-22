// SPDX-License-Identifier: GPL-3.0-only
/*
Copyright (C) 2020 Alexandru-Iulian Magan, Tudor-Ioan Roman, and contributors.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
#include <cardboard/ipc.h>

#include <bit>

namespace libcardboard::ipc {

Header interpret_header(const AlignedHeaderBuffer& buffer)
{
    if constexpr (std::endian::native == std::endian::little) {
        return { *reinterpret_cast<const int32_t*>(buffer.data()) };
    } else {
        uint32_t r = *reinterpret_cast<const uint32_t*>(buffer.data());
        r = (r & 0x000000ffu) << 24u | (r & 0x0000ff00u) << 8u | (r & 0x00ff0000u) >> 8u | (r & 0xff000000u) >> 24u;

        return { static_cast<int>(r) };
    }
}

AlignedHeaderBuffer create_header_buffer(const Header& header)
{
    AlignedHeaderBuffer buffer;

    *reinterpret_cast<int*>(buffer.data()) = header.incoming_bytes;

    if constexpr (std::endian::native == std::endian::big) {
        uint32_t r = *reinterpret_cast<int*>(buffer.data());

        r = ((r >> 24u) & 0x000000ffu) | ((r << 8u) & 0x00ff0000u) | ((r >> 8u) & 0x0000ff00u) | ((r << 24u) & 0xff000000u);

        *reinterpret_cast<int*>(buffer.data()) = r;
    }

    return buffer;
}

}