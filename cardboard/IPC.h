#ifndef __CARDBOARD_IPC_H_
#define __CARDBOARD_IPC_H_

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

struct Server;

struct IPCCommandResult {
    std::string message;
};

using IPCParsedCommand = std::vector<std::string>;
using IPCCommandHandler = IPCCommandResult(IPCParsedCommand, Server*);

std::optional<IPCCommandHandler*> ipc_find_command_handler(std::string_view name);
int ipc_read_command(int fd, uint32_t mask, void* data);

#endif // __CARDBOARD_IPC_H_
