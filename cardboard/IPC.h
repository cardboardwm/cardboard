#ifndef __CARDBOARD_IPC_H_
#define __CARDBOARD_IPC_H_

#include <cstdint>
#include <string>
#include <vector>

struct Server;

struct IPCCommandResult {
    std::string message;
};

using ParsedCommand = std::vector<std::string>;
using IPCCommandHandler = IPCCommandResult(ParsedCommand, Server*);

int ipc_read_command(int fd, uint32_t mask, void* data);

#endif // __CARDBOARD_IPC_H_
