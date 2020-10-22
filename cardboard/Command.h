// SPDX-License-Identifier: GPL-3.0-only
/*
Copyright (C) 2020 Alexandru-Iulian Magan, Tudor-Ioan Roman, and contributors.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
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
