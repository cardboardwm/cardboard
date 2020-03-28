#ifndef __CARDBOARD_IPC_HANDLERS_HANDLERS_H_
#define __CARDBOARD_IPC_HANDLERS_HANDLERS_H_

#include <csignal>
#include <cstdint>
#include <locale>
#include <string>
#include <string_view>

#include "../IPC.h"
#include "../Server.h"
#include "../Spawn.h"
#include "../Command.h"

extern char** environ;

namespace commands {

inline CommandResult focus(Server* server, int focus_direction)
{
    server->focus_by_offset(focus_direction);
    return {""};
}

inline CommandResult quit(Server* server)
{
    server->teardown(0);
    return { "" };
}

[[maybe_unused]] static std::array<std::pair<std::string_view, uint32_t>, 8> mod_table = { { { "shift", WLR_MODIFIER_SHIFT },
                                                                            { "ctrl", WLR_MODIFIER_CTRL },
                                                                            { "alt", WLR_MODIFIER_ALT },
                                                                            { "super", WLR_MODIFIER_LOGO },
                                                                            { "caps", WLR_MODIFIER_CAPS },
                                                                            { "mod2", WLR_MODIFIER_MOD2 },
                                                                            { "mod3", WLR_MODIFIER_MOD3 },
                                                                            { "mod5", WLR_MODIFIER_MOD5 } } };


inline CommandResult bind(Server* /*server*/, std::string /*key_binding*/, Command /*command*/)
{
    /*
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
    auto handler = ipc_find_command_handler(command[0]);
    if (!handler) {
        return { "Invalid command.\n" };
    }
    server->keybindings_config.map[modifier].insert({ sym, { *handler, command } });
*/
    return { "" };
}

inline CommandResult exec(Server* /*server*/, std::string /*command*/)
{
    /*
         std::vector<char*> argv;
    argv.reserve(cmd.size() - 1 + 1);
    std::transform(std::next(cmd.begin()), cmd.end(), std::back_inserter(argv), [](auto& s) { return const_cast<char*>(s.c_str()); });
    argv.push_back(nullptr);

    if (fork() == 0) {
        setsid();
        execvpe(argv[0], argv.data(), environ);
        exit(0);
    }*/
    return { "" };
}

};


#endif // __CARDBOARD_IPC_HANDLERS_HANDLERS_H_
