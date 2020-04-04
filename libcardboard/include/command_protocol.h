#ifndef BUILD_LIBCOMMAND_PROTOCOL_COMMAND_H
#define BUILD_LIBCOMMAND_PROTOCOL_COMMAND_H

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace CommandArguments {
struct quit {
};

struct focus {
    enum class Direction {
        Left,
        Right
    } direction;
};

struct bind;

struct exec {
    std::vector<std::string> argv;
};
}

/**
 * \brief Variant that contains the arguments of each accepted command, as a different type.
 */
using CommandData = std::variant<
    CommandArguments::quit,
    CommandArguments::focus,
    CommandArguments::exec,
    CommandArguments::bind>;

namespace CommandArguments {
struct bind {
    std::vector<std::string> modifiers;
    std::string key;
    std::unique_ptr<CommandData> command;

    bind() = default;

    bind(std::vector<std::string> modifiers, std::string key, CommandData command)
        : modifiers { std::move(modifiers) }
        , key { std::move(key) }
        , command { std::make_unique<CommandData>(std::move(command)) }
    {
    }

    bind(const bind& other)
        : modifiers { other.modifiers }
        , key { other.key }
        , command { std::make_unique<CommandData>(*other.command) }
    {
    }

    bind(bind&&) = default;
};
}

/**
 * \brief Deserializes data from a file descriptor
 */
std::optional<CommandData> read_command_data(int fd);

/**
 * \brief Serializes and write data to a file descriptor.
 */
bool write_command_data(int fd, const CommandData&);

#endif //BUILD_LIBCOMMAND_PROTOCOL_COMMAND_H
