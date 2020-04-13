extern "C" {
#include <wlr/util/log.h>
}

#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <cstring>

#include "IPC.h"
#include "Server.h"

#include <command_protocol.h>

std::optional<IPCInstance> create_ipc(
    Server* server,
    const std::string& socket_path,
    std::function<std::string(CommandData)> command_callback)
{
    int ipc_socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    std::unique_ptr<sockaddr_un> socket_address = std::make_unique<sockaddr_un>();

    socket_address->sun_family = AF_UNIX;
    memcpy(socket_address->sun_path, socket_path.c_str(), socket_path.size() + 1);

    unlink(socket_path.c_str());
    if (bind(ipc_socket_fd, reinterpret_cast<sockaddr*>(&socket_address), sizeof(socket_address)) == -1) {
        wlr_log(WLR_ERROR, "Couldn't bind a name ('%s') to the IPC socket.", socket_address->sun_path);
        return std::nullopt;
    }

    if (listen(ipc_socket_fd, SOMAXCONN) == -1) {
        wlr_log(WLR_ERROR, "Couldn't listen to the IPC socket '%s'.", socket_address->sun_path);
        return std::nullopt;
    }

    // add destroy display handling
    IPCInstance ipc = std::make_unique<IPC>(IPC{
        server, ipc_socket_fd, std::move(socket_address), std::move(command_callback)
    });
    wl_event_loop_add_fd(
        server->event_loop,
        ipc_socket_fd,
        WL_EVENT_READABLE,
        IPC::handle_client_connection,
        ipc.get()
    );

    return ipc;
}

// This function implements wl_event_loop_fd_func_t
int IPC::handle_client_connection(int fd, [[maybe_unused]] uint32_t mask, void* data)
{
    auto ipc = static_cast<IPC*>(data);
    const int client_fd = accept(fd, nullptr, nullptr);
    if (client_fd == -1) {
        wlr_log(WLR_ERROR, "Failed to accept on IPC socket %s: %s", ipc->socket_address->sun_path, strerror(errno));
        return 0;
    }

    int flags;
    if ((flags = fcntl(client_fd, F_GETFD)) == -1
        || fcntl(client_fd, F_SETFD, flags | FD_CLOEXEC) == -1) {
        wlr_log(WLR_ERROR, "Unable to set CLOEXEC on IPC client socket: %s", strerror(errno));
        close(client_fd);
        return 0;
    }

    if ((flags = fcntl(client_fd, F_GETFL)) == -1
        || fcntl(client_fd, F_GETFL, flags | O_NONBLOCK) == -1) {
        wlr_log(WLR_ERROR, "Unable to set O_NONBLOCK on IPC client socket: %s", strerror(errno));
        close(client_fd);
        return 0;
    }

    ipc->clients.push_back({
        ipc,
        client_fd,
        IPC::ClientState::READING_HEADER
    });

    ipc->clients.back().readable_event_source =
        wl_event_loop_add_fd(
            ipc->server->event_loop,
            client_fd,
            WL_EVENT_READABLE,
            IPC::handle_client_readable,
            &ipc->clients.back()
        );

    return 0;
}

int IPC::handle_client_readable(int fd, [[maybe_unused]] uint32_t mask, void* data)
{

}

int IPC::handle_client_writeable(int fd, [[maybe_unused]] uint32_t mask, void* data)
{

}