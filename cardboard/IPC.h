#ifndef __CARDBOARD_IPC_H_
#define __CARDBOARD_IPC_H_

#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include "Listener.h"

#include <sys/un.h>

/**
 * \file
 * \brief This file contains functions and structures related to the IPC mechanism of Cardboard.
 *
 * Cardboard uses socket-based IPC to communicate with client programs that can send commands and modify
 * the configuration of the running compositor. Settings are loaded by running a script (usually a plain shell script)
 * that invokes such commands.
 *
 * The bundled, CLI client is named \c cutter.
 */

struct Server;
class IPC;
using IPCInstance = std::unique_ptr<IPC>;

class IPC {
    enum class ClientState {
        READING_HEADER,
        READING_PAYLOAD,
        WRITING
    };

    struct Client {
        IPC* ipc;
        int client_fd;
        ClientState state;
        int payload_size;
    };

private:
    explicit IPC(
        Server* server,
        int socket_fd,
        std::unique_ptr<sockaddr_un>&& socket_address,
        std::function<std::string(CommandData)>&& command_callback
    ):
        server{server},
        socket_fd{socket_fd},
        socket_address{std::move(socket_address)},
        command_callback{std::move(command_callback)}
    {}

public:
    IPC() = delete;
    IPC(const IPC &) = delete;
    IPC(IPC&&) = default;

private:
    static int handle_client_connection(int fd, [[maybe_unused]] uint32_t mask, void* data);
    static int handle_client_readable(int fd, [[maybe_unused]] uint32_t mask, void* data);
    static int handle_client_writeable(int fd, [[maybe_unused]] uint32_t mask, void* data);

private:
    ListenerList ipc_listeners;
    Server* server;
    int socket_fd;
    std::unique_ptr<sockaddr_un> socket_address;
    std::function<std::string(CommandData)> command_callback;

    friend std::optional<IPCInstance> create_ipc(Server* server, const std::string& socket_path, std::function<std::string(CommandData)> command_callback);
};

std::optional<IPCInstance> create_ipc(Server* server, const std::string& socket_path, std::function<std::string(CommandData)> command_callback);

/**
 * \brief Handler function for \c Server::event_loop that reads the raw
 * command from the IPC socket (\c Server::ipc_socket_fd).
 */
int ipc_read_command(int fd, uint32_t mask, void* data);

#endif // __CARDBOARD_IPC_H_
