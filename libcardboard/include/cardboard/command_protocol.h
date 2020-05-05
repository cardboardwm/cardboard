#ifndef __LIBCARDBOARD_COMMAND_PROTOCOL_H_
#define __LIBCARDBOARD_COMMAND_PROTOCOL_H_

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <tl/expected.hpp>

/// Structs representing arguments for IPC commands.
namespace command_arguments {
struct quit {
    int code;
};

struct focus {
    enum class Direction {
        Left,
        Right
    } direction;

    bool cycle;
};

struct bind;

struct exec {
    std::vector<std::string> argv;
};

struct close {
};

struct workspace {
    struct switch_ {
        int n;
    };

    struct move {
        int n;
    };

    std::variant<switch_, move> workspace;
};

struct config_mouse_mod {
    std::vector<std::string> modifiers;
};
}

/**
 * \brief Variant that contains the arguments of each accepted command, as a different type.
 */
using CommandData = std::variant<
    command_arguments::quit,
    command_arguments::focus,
    command_arguments::exec,
    command_arguments::bind,
    command_arguments::close,
    command_arguments::workspace,
    command_arguments::config_mouse_mod>;

namespace command_arguments {
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
 * \brief Deserializes data from a region of memory
 */
tl::expected<CommandData, std::string> read_command_data(void* data, size_t);

/**
 * \brief Serializes CommandData type data into a std::string buffer
 */
tl::expected<std::string, std::string> write_command_data(const CommandData&);

#endif //__LIBCARDBOARD_COMMAND_PROTOCOL_H_
