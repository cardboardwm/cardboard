#ifndef __CARDBOARD_IPC_H_
#define __CARDBOARD_IPC_H_

#include <cstdint>
#include <string>

struct IPCCommandResult {
    std::string message;
};

int ipc_read_command(int fd, uint32_t mask, void* data);

#endif // __CARDBOARD_IPC_H_
