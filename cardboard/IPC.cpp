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
int ipc_read_command(int fd, [[maybe_unused]] uint32_t mask, void* data)
{
    Server* server = static_cast<Server*>(data);
    const int client_fd = accept(fd, nullptr, nullptr);
    if (client_fd == -1) {
        wlr_log(WLR_ERROR, "Failed to accept on IPC socket %s: %s", server->ipc_sock_address.sun_path, strerror(errno));
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

    std::array<long long, MAX_MESSAGE_SIZE / sizeof(long long)> buffer{};

    ssize_t size = recv(
        client_fd,
        buffer.data(),
        sizeof(long long) * buffer.size() - 1,
        0);

    if(size == -1) {
        close(fd);
        wlr_log(WLR_DEBUG, "Couldn't recv\n");
        return 0;
    }

    std::optional<CommandData> command_data = read_command_data(
        buffer.data(),
        static_cast<size_t>(size)
    );

    if (!command_data) {
        static const std::string message = "Unable to receive data";
        send(client_fd, message.data(), message.size(), 0);
        close(client_fd);
    } else {
        CommandResult result = dispatch_command(*command_data)(server);
        if (!result.message.empty()) {
            send(client_fd, result.message.c_str(), result.message.size(), 0);
        }
        close(client_fd);
    }

    return 0;
}
