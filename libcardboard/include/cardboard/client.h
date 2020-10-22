// SPDX-License-Identifier: GPL-3.0-only
/*
Copyright (C) 2020 Alexandru-Iulian Magan, Tudor-Ioan Roman, and contributors.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef LIBCARDBOARD_CLIENT_H_INCLUDED
#define LIBCARDBOARD_CLIENT_H_INCLUDED

#include <memory>
#include <string>

#include <tl/expected.hpp>

#include "command_protocol.h"

#include <sys/un.h>

/// Code for sending commands to Cardboard and receiving responses.
namespace libcutter {

/**
 * \brief Manages a connection to the Cardboard IPC server
 */
class Client {
public:
    Client(const Client&) = delete;
    Client(Client&&) noexcept;
    ~Client();

    /**
     * \brief Serializes and sends a CommandData packet to the server
     */
    tl::expected<void, std::string> send_command(const CommandData&);

    /**
     * \brief Waits for a string response from the server
     */
    tl::expected<std::string, int> wait_response();

private:
    Client(int, std::unique_ptr<sockaddr_un>);

    int socket_fd;
    std::unique_ptr<sockaddr_un> socket_address;

    friend tl::expected<Client, std::string> open_client();
};

/**
 * \brief Creates a client connection on the socket path reported by the system
 */
tl::expected<Client, std::string> open_client();

}

#endif //LIBCARDBOARD_CLIENT_H_INCLUDED
