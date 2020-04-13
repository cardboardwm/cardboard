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
        wl_event_source *readable_event_source = nullptr;
        wl_event_source *writable_event_source = nullptr;
        int payload_size = 0;
        std::string message{};

        ~Client();
    };

private:
    explicit IPC(
        Server* server,
        int socket_fd,
        std::unique_ptr<sockaddr_un>&& socket_address,
        std::function<std::string(const CommandData&)>&& command_callback
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
    void remove_client(Client*);

private:
    ListenerList ipc_listeners;
    Server* server;
    int socket_fd;
    std::unique_ptr<sockaddr_un> socket_address;
    std::function<std::string(CommandData)> command_callback;

    std::list<Client> clients;

    friend std::optional<IPCInstance> create_ipc(Server* server, const std::string& socket_path, std::function<std::string(const CommandData&)> command_callback);
};

std::optional<IPCInstance> create_ipc(Server* server, const std::string& socket_path, std::function<std::string(const CommandData&)> command_callback);

#endif // __CARDBOARD_IPC_H_
