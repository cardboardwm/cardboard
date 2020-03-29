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

[[maybe_unused]] static std::array<std::pair<std::string_view, uint32_t>, 8> mod_table = {  };


inline CommandResult bind(Server* server, uint32_t modifiers, xkb_keysym_t sym, const Command& command)
{
    server->keybindings_config.map[modifiers].insert({sym, command});
    return { "" };
}

inline CommandResult exec(Server*, std::vector<std::string> arguments)
{
    std::vector<char*> argv;
    argv.reserve(arguments.size() - 1 + 1);
    std::transform(
        std::next(arguments.begin()),
        arguments.end(),
        std::back_inserter(argv),
        [](auto& s) { return const_cast<char*>(s.c_str());
    });
    argv.push_back(nullptr);

    if (fork() == 0) {
        setsid();
        execvpe(argv[0], argv.data(), environ);
        exit(0);
    }

    return { "" };
}

};


#endif // __CARDBOARD_IPC_HANDLERS_HANDLERS_H_
