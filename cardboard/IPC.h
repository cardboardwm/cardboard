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
 * \brief Handler function for \c Server::event_loop that reads the raw
 * command from the IPC socket (\c Server::ipc_socket_fd).
 */
int ipc_read_command(int fd, uint32_t mask, void* data);

#endif // __CARDBOARD_IPC_H_
