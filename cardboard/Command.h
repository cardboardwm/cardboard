//
// Created by alex on 3/26/20.
//

#ifndef BUILD_CARDBOARD_COMMAND_H
#define BUILD_CARDBOARD_COMMAND_H

#include <variant>
#include <functional>
#include <command_protocol.h>

struct Server;

/**
 * \brief This struct holds the result of an IPC command, which can be a string message
 * for the moment.
 */
struct CommandResult {
    std::string message;
};

/**
 * \brief
 */
using Command = std::function<CommandResult(Server*)>;

Command dispatch_command(const CommandData&);

#endif //BUILD_CARDBOARD_COMMAND_H
