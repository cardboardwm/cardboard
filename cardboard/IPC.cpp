#include <wlr_cpp/util/log.h>

#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <cstring>


#include "IPC.h"
#include "Server.h"

#include <command_protocol.h>


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
        || fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        wlr_log(WLR_ERROR, "Unable to set NONBLOCK on IPC client socket: %s", strerror(errno));
        close(client_fd);
        return 0;
    }

    std::optional<CommandData> command_data = read_command_data(client_fd);

    if(!command_data)
    {
        static const std::string message = "Unable to receive data";
        send(client_fd, message.data(), message.size(), 0);
        close(client_fd);
    }
    else
    {
        CommandResult result = dispatch_command(*command_data)(server);
        if (!result.message.empty()) {
            send(client_fd, result.message.c_str(), result.message.size(), 0);
        }
        close(client_fd);
    }

    return 0;
}
