#include <wlr_cpp/util/log.h>

#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <string>
#include <vector>

#include "IPC.h"
#include "Server.h"

#include "ipc_handlers/handlers.h"

// Keep this sorted!!!
static std::array<std::pair<std::string_view, IPCCommandHandler*>, 5> table = { { { "bind", ipc_handlers::command_bind },
                                                                                  { "exec", ipc_handlers::command_exec },
                                                                                  { "focus_left", ipc_handlers::command_focus_left },
                                                                                  { "focus_right", ipc_handlers::command_focus_right },
                                                                                  { "quit", ipc_handlers::command_quit } } };

std::optional<IPCCommandHandler*> ipc_find_command_handler(std::string_view name)
{
    std::pair<std::string_view, IPCCommandHandler*> to_search = { name, nullptr };
    auto handler = std::lower_bound(table.begin(), table.end(), to_search);
    if (handler == table.end() || handler->first != name) {
        return std::nullopt;
    }

    return handler->second;
}

IPCCommandResult run_command(IPCParsedCommand cmd, Server* server)
{
    std::string stringified;
    for (auto& arg : cmd) {
        stringified += arg + ' ';
    }
    wlr_log(WLR_DEBUG, "ipc: got command %s", stringified.c_str());

    auto handler = ipc_find_command_handler(cmd[0]);
    if (!handler) {
        return { "No such command.\n" };
    }

    return (*handler)(cmd, server);
}

static std::optional<IPCParsedCommand> parse_command(uint8_t* cmd, ssize_t len)
{
    // The wire format is the following: commands are sent as words, the first word
    // being the command name, the others being the arguments. Each word is preceded
    // by its length in one byte. The final byte on the wire is 0.
    IPCParsedCommand r;
    int i = 0;
    while (i < len && cmd[i] != 0) {
        uint8_t segment_size = cmd[i];
        i++;
        r.push_back({});

        std::string& arg = r.back();
        arg.reserve(segment_size);

        if (i + segment_size >= len) {
            return std::nullopt;
        }
        for (int j = 0; j < segment_size; j++) {
            arg.push_back(cmd[i + j]);
        }

        i += segment_size;
    }

    if (i >= len || (i < len && cmd[i] != 0)) {
        return std::nullopt;
    }

    return r;
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
        || fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        wlr_log(WLR_ERROR, "Unable to set NONBLOCK on IPC client socket: %s", strerror(errno));
        close(client_fd);
        return 0;
    }

    uint8_t cmd[BUFSIZ] = {};
    ssize_t len = recv(client_fd, cmd, sizeof(cmd) - 1, 0);
    std::optional<IPCParsedCommand> parsed = parse_command(cmd, len);

    if (!parsed) {
        constexpr std::string_view error = "Malformed command.\n";
        send(client_fd, error.data(), error.size(), 0);
        close(client_fd);

        return 0;
    }

    IPCCommandResult result = run_command(*parsed, server);
    if (!result.message.empty()) {
        send(client_fd, result.message.c_str(), result.message.size(), 0);
    }
    close(client_fd);

    return 0;
}
