#include <wlr_cpp/util/log.h>

#include <sys/socket.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <optional>
#include <string>
#include <vector>

#include "IPC.h"
#include "Server.h"

using ParsedCommand = std::vector<std::string>;
using IPCCommandHandler = IPCCommandResult(ParsedCommand, Server*);

template <typename... T>
constexpr auto make_array(T&&... values) -> std::array<typename std::decay<typename std::common_type<T...>::type>::type, sizeof...(T)>
{
    return std::array<typename std::decay<typename std::common_type<T...>::type>::type, sizeof...(T)> { std::forward<T>(values)... };
}

static IPCCommandResult command_quit([[maybe_unused]] ParsedCommand cmd, Server* server)
{
    server->teardown();
    return { 0, "" };
}

static auto table = make_array<std::pair<std::string_view, IPCCommandHandler*>>({ "quit", command_quit });

static IPCCommandResult run_command(ParsedCommand cmd, Server* server)
{
    std::string stringified;
    for (auto& arg : cmd) {
        stringified += arg + ' ';
    }
    wlr_log(WLR_DEBUG, "ipc: got command %s", stringified.c_str());

    std::pair<std::string_view, IPCCommandHandler*> to_search = { std::string_view(cmd[0]), nullptr };
    auto handler = std::lower_bound(table.begin(), table.end(), to_search);
    if (handler == table.end() || handler->first != cmd[0]) {
        return { 1, "No such command.\n" };
    }

    return handler->second(cmd, server);
}

static std::optional<ParsedCommand> parse_command(uint8_t* cmd, ssize_t len)
{
    // The wire format is the following: commands are sent as words, the first word
    // being the command name, the others being the arguments. Each word is preceded
    // by its length in one byte. The final byte on the wire is 0.
    ParsedCommand r;
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
        return 1;
    }

    uint8_t cmd[BUFSIZ] = {};
    ssize_t len = recv(client_fd, cmd, sizeof(cmd) - 1, 0);
    std::optional<ParsedCommand> parsed = parse_command(cmd, len);

    if (!parsed) {
        constexpr std::string_view error = "Malformed command.\n";
        send(client_fd, error.data(), error.size(), 0);
        close(client_fd);

        return 1;
    }

    IPCCommandResult result = run_command(*parsed, server);
    if (!result.message.empty()) {
        send(client_fd, result.message.c_str(), result.message.size(), 0);
    }
    close(client_fd);

    return result.code;
}
