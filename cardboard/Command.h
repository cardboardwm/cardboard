#ifndef CARDBOARD_COMMAND_H_INCLUDED
#define CARDBOARD_COMMAND_H_INCLUDED

#include <functional>
#include <variant>

#include <cardboard/command_protocol.h>

struct Server;

/**
 * \brief This struct holds the result of an IPC command, which can be a string message
 * for the moment.
 */
struct CommandResult {
    std::string message;
};

/**
 * \brief Callable that stores a bound command (internal function pointer and all arguments)
 */
using Command = std::function<CommandResult(Server*)>;

/**
 * \brief Dispatches a the command's arguments (CommandData) and binds the command into the Command type.
 */
Command dispatch_command(const CommandData&);

#endif //CARDBOARD_COMMAND_H_INCLUDED
