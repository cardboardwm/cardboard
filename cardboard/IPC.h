#ifndef __CARDBOARD_IPC_H_
#define __CARDBOARD_IPC_H_

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

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

/**
 * \brief This struct holds the result of an IPC command, which can be a string message
 * for the moment.
 */
struct IPCCommandResult {
    std::string message;
};

/**
 * \brief An array of strings, similar to \c argv.
 */
using IPCParsedCommand = std::vector<std::string>;
/**
 * \brief The function prototype for IPC command handlers.
 */
using IPCCommandHandler = IPCCommandResult(IPCParsedCommand, Server*);

/**
 * \brief Finds the appropiate handler for a command named \a name within the handler table.
 *
 * \returns \c std::nullopt if there is no command with that \a name.
 */
std::optional<IPCCommandHandler*> ipc_find_command_handler(std::string_view name);

/**
 * \brief Handler function for \c Server::event_loop that reads the raw
 * command from the IPC socket (\c Server::ipc_socket_fd).
 */
int ipc_read_command(int fd, uint32_t mask, void* data);

#endif // __CARDBOARD_IPC_H_
