#ifndef BUILD_LIBCOMMAND_PROTOCOL_COMMAND_H
#define BUILD_LIBCOMMAND_PROTOCOL_COMMAND_H

#include <string>
#include <variant>
#include <memory>
#include <optional>
#include <vector>

namespace CommandArguments
{
    struct quit {};

    struct focus
    {
        enum class Direction
        {
            Left,
            Right
        } direction;
    };

    struct bind;

    struct exec
    {
        std::vector<std::string> argv;
    };
}

using CommandData = std::variant<
        CommandArguments::quit,
        CommandArguments::focus,
        CommandArguments::exec,
        CommandArguments::bind
    >;

namespace CommandArguments
{
    struct bind
    {
        std::string key_binding;
        std::unique_ptr<CommandData> command;
    };
}

CommandData read_command_data(int fd);
void write_command_data(int fd, const CommandData&);

#endif //BUILD_LIBCOMMAND_PROTOCOL_COMMAND_H
