//
// Created by alex on 3/28/20.
//

#include "commands.h"
#include "../Command.h"

#include <variant>

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

Command dispatch_command(const CommandData& command_data)
{
    return std::visit(overloaded {
        [](CommandArguments::focus focus_data) -> Command {
            return [focus_data](Server* server) {
                return commands::focus(
                    server,
                    focus_data.direction == CommandArguments::focus::Direction::Left ?
                        -1 : +1
                );
            };
        },
        [](CommandArguments::quit) -> Command {
            return commands::quit;
        },
        [](const CommandArguments::bind& bind_data) -> Command {
            std::string key_binding;
            Command command = dispatch_command(*(bind_data.command));
            return [key_binding, command](Server* server) {
                return commands::bind(server, key_binding, command);
            };
        },
        [](CommandArguments::exec exec_data) -> Command {
            return [exec_data](Server* server) {
                return commands::exec(server, exec_data.command);
            };
        },
    }, command_data);
}