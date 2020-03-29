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


inline CommandResult bind(Server* /*server*/, std::vector<std::string> /*modifiers*/, std::string /*key*/, Command /*command*/)
{
    /*
    auto command = static_cast<IPCParsedCommand>(std::vector(std::next(cmd.begin(), 2), cmd.end()));
    auto handler = ipc_find_command_handler(command[0]);
    if (!handler) {
        return { "Invalid command.\n" };
    }
    server->keybindings_config.map[modifier].insert({ sym, { *handler, command } });
*/
    return { "" };
}

inline CommandResult exec(Server* /*server*/, std::vector<std::string> /*command*/)
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
