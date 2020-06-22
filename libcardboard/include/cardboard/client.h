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
