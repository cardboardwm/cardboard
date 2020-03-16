#ifndef __CARDBOARD_IPC_HANDLERS_HANDLERS_H_
#define __CARDBOARD_IPC_HANDLERS_HANDLERS_H_

#include <cstdint>
#include <locale>
#include <string>
#include <string_view>

#include "../IPC.h"
#include "../Server.h"

namespace ipc_handlers {

static IPCCommandResult command_quit(IPCParsedCommand cmd, Server* server)
{
    int exit = 0;
    if (cmd.size() >= 2) {
        exit = std::atoi(cmd[1].c_str());
    }
    server->teardown(exit);
    return { "" };
}

static IPCCommandResult command_focus_left([[maybe_unused]] IPCParsedCommand cmd, Server* server)
{
    server->focus_by_offset(-1);
    return { "" };
}

static IPCCommandResult command_focus_right([[maybe_unused]] IPCParsedCommand cmd, Server* server)
{
    server->focus_by_offset(+1);
    return { "" };
}

static std::array<std::pair<std::string_view, uint32_t>, 8> mod_table = { { { "shift", WLR_MODIFIER_SHIFT },
                                                                            { "ctrl", WLR_MODIFIER_CTRL },
                                                                            { "alt", WLR_MODIFIER_ALT },
                                                                            { "super", WLR_MODIFIER_LOGO },
                                                                            { "caps", WLR_MODIFIER_CAPS },
                                                                            { "mod2", WLR_MODIFIER_MOD2 },
                                                                            { "mod3", WLR_MODIFIER_MOD3 },
                                                                            { "mod5", WLR_MODIFIER_MOD5 } } };

static IPCCommandResult command_bind(IPCParsedCommand cmd, Server* server)
{
    uint32_t modifier = 0;
    xkb_keysym_t sym = XKB_KEY_NoSymbol;
    std::string::size_type pos = 0;
    auto locale = std::locale("");

    if (cmd.size() < 3) {
        return { "Not enough arguments" };
    }

    while (pos < cmd[1].size()) {
        auto plus_index = cmd[1].find('+', pos);
        auto token = cmd[1].substr(pos, plus_index - pos);

        if (auto it = std::find_if(mod_table.begin(), mod_table.end(), [&](auto& cmd) { return cmd.first == token; }); it != mod_table.end()) {
            modifier |= it->second;
        } else {
            // make string lowercase
            for (char& c : token) {
                c = std::tolower(c, locale);
            }

            xkb_keysym_t matched_sym = xkb_keysym_from_name(token.c_str(), XKB_KEYSYM_CASE_INSENSITIVE);
            if (matched_sym == XKB_KEY_NoSymbol) {
                return { "Invalid keysym '" + token + '\'' };
            }

            if (sym != XKB_KEY_NoSymbol) {
                return { "Tried to assign more than one keysym '" + token + '\'' };
            }
            sym = matched_sym;
        }

        if (plus_index == cmd[1].npos) {
            pos = cmd[1].size();
        } else {
            pos = plus_index + 1;
        }
    }

    auto command = static_cast<IPCParsedCommand>(std::vector(std::next(cmd.begin(), 2), cmd.end()));
    server->keybindingsConfig.map[modifier].insert({ sym, command });

    return { "" };
}

};

#endif // __CARDBOARD_IPC_HANDLERS_HANDLERS_H_
