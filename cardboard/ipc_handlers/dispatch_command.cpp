//
// Created by alex on 3/28/20.
//

#include "commands.h"
#include "../Command.h"

#include <variant>
#include <numeric>

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
            static const std::unordered_map<std::string, uint32_t> mod_table = {
                  { "shift", WLR_MODIFIER_SHIFT },
                  { "ctrl", WLR_MODIFIER_CTRL },
                  { "alt", WLR_MODIFIER_ALT },
                  { "super", WLR_MODIFIER_LOGO },
                  { "caps", WLR_MODIFIER_CAPS },
                  { "mod2", WLR_MODIFIER_MOD2 },
                  { "mod3", WLR_MODIFIER_MOD3 },
                  { "mod5", WLR_MODIFIER_MOD5 }
            };

            uint32_t modifiers = 0;
            for(auto mod: bind_data.modifiers)
                modifiers |= mod_table.at(mod);

            xkb_keysym_t sym = xkb_keysym_from_name(bind_data.key.c_str(), XKB_KEYSYM_CASE_INSENSITIVE);

            if(sym == XKB_KEY_NoSymbol)
                return {[bind_data](Server*) -> CommandResult
                    { return {std::string("Invalid keysym: ") + bind_data.key}; }};

            Command command = dispatch_command(*(bind_data.command));
            return {[modifiers, sym, command](Server* server) {
                return commands::bind(server, modifiers, sym, command);
            }};
        },
        [](CommandArguments::exec exec_data) -> Command {
            return [exec_data](Server* server) {
                return commands::exec(server, exec_data.argv);
            };
        },
    }, command_data);
}