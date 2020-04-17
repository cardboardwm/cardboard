#ifndef __LIBCARDBOARD_COMMAND_PROTOCOL_H_
#define __LIBCARDBOARD_COMMAND_PROTOCOL_H_

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <tl/expected.hpp>

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
    bind& operator=(bind&&) = default;

    bind& operator=(const bind& other)
    {
        if (&other == this)
            return *this;

        modifiers = other.modifiers;
        key = other.key;
        command = std::make_unique<CommandData>(*other.command);

        return *this;
    }
};
}

/**
 * \brief Deserializes data from a pointer
 */
tl::expected<CommandData, std::string> read_command_data(void* data, size_t);

/**
 * \brief Serializes and write data to a file descriptor.
 */
tl::expected<void, std::string> write_command_data(int fd, const CommandData&);

#endif //__LIBCARDBOARD_COMMAND_PROTOCOL_H_
