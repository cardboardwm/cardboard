#ifndef __CARDBOARD_IPC_H_
#define __CARDBOARD_IPC_H_

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <sys/un.h>

#include "Listener.h"

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

/**
 * \brief Manages all incoming client connections, communicating with them using the Cardboard IPC protocol
 */
class IPC {
    /**
     * \brief describes the stage of the client-server protocol communication of a client
     */
    enum class ClientState {
        READING_HEADER,
        READING_PAYLOAD,
        WRITING
    };

    /**
     * \brief the state of a single client
     */
    struct Client {
        IPC* ipc;
        int client_fd;
        ClientState state;
        wl_event_source* readable_event_source = nullptr;
        wl_event_source* writable_event_source = nullptr;
        int payload_size = 0;
        std::string message {};

        Client(IPC* ipc, int client_fd, ClientState state)
            : ipc(ipc)
            , client_fd(client_fd)
            , state(state)
        {
        }
        Client(const Client&) = delete;

        /**
         * \brief disconnects the client and removes the event sources from wayland's book keeping
         */
        ~Client();
    };

private:
    explicit IPC(
        Server* server,
        int socket_fd,
        std::unique_ptr<sockaddr_un>&& socket_address,
        std::function<std::string(const CommandData&)>&& command_callback)
        : server { server }
        , socket_fd { socket_fd }
        , socket_address { std::move(socket_address) }
        , command_callback { std::move(command_callback) }
    {
    }

public:
    IPC() = delete;
    IPC(const IPC&) = delete;
    IPC(IPC&&) = default;

private:
    /**
     * \brief the callback wayland calls when a client is trying to connect
     */
    static int handle_client_connection(int fd, uint32_t mask, void* data);

    /**
     * \brief the callback wayland calls when data is available on a client connection file descriptor
     */
    static int handle_client_readable(int fd, uint32_t mask, void* data);

    /**
     * \brief the callback wayland calls a client connection file descriptor can be written to
     */
    static int handle_client_writeable(int fd, uint32_t mask, void* data);

private:
    /**
     * \brief removes a client from the clients list - thus disconnecting it as well
     */
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

/**
 * \brief Creates an IPC instance
 * \param server the server that handles the ipc instance
 * \param socket_path the path of the system where the socket path will be found
 * \param command_callback callable that will be called when a command will be received
 * \return returns the newly created IPC instance
 */
std::optional<IPCInstance> create_ipc(Server* server, const std::string& socket_path, std::function<std::string(const CommandData&)> command_callback);

#endif // __CARDBOARD_IPC_H_
