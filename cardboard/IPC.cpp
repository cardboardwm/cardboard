#include <wlr_cpp/util/log.h>

#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <cstdio>

#include "IPC.h"
#include "Server.h"

static IPCCommandResult run_command([[maybe_unused]] char* cmd, [[maybe_unused]] Server* server)
{
    wlr_log(WLR_DEBUG, "got command %s", cmd);
    return {0, ""};
}

// This function implements wl_event_loop_fd_func_t
int ipc_read_command(int fd, [[maybe_unused]] uint32_t mask, void* data)
{
    Server* server = static_cast<Server*>(data);
    const int client_fd = accept(fd, nullptr, nullptr);
    if (client_fd == -1) {
        wlr_log(WLR_ERROR, "Failed to accept on IPC socket %s: %s", server->ipc_sock_address.sun_path, strerror(errno));
        return 1;
    }

    char cmd[BUFSIZ] = {};
    recv(client_fd, cmd, sizeof(cmd) - 1, 0);

    IPCCommandResult result = run_command(cmd, server);
    if (!result.message.empty()) {
        send(client_fd, result.message.c_str(), result.message.size(), 0);
    }
    close(client_fd);

    return result.code;
}
