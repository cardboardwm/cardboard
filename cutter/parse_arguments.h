#ifndef CUTTER_PARSE_ARGUMENTS_H_INCLUDED
#define CUTTER_PARSE_ARGUMENTS_H_INCLUDED

#include <locale>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <cardboard/command_protocol.h>

namespace detail {
using namespace std::string_literals;

static auto find_mod_key(const std::string& key)
{
    static const std::unordered_set<std::string> mod_keys = { "shift", "ctrl", "alt", "super", "caps", "mod2", "mod3", "mod5" };
    return mod_keys.find(key) != mod_keys.end();
};

tl::expected<CommandData, std::string> parse_config_mouse_mod(const std::vector<std::string>& args)
{
    if (args.size() != 1) {
        return tl::unexpected("malformed config value"s);
    }

    std::vector<std::string> modifiers;
    auto locale = std::locale("");

    size_t pos = 0;
    while (pos < args[0].size()) {
        auto plus_index = args[0].find('+', pos);
        auto token = args[0].substr(pos, plus_index - pos);

        if (!find_mod_key(token)) {
            return tl::unexpected("token '"s + token + "' is not a modifier");
        }
        modifiers.push_back(token);
        if (plus_index == args[0].npos) {
            pos = args[0].size();
        } else {
            pos = plus_index + 1;
        }
    }

    return command_arguments::config { command_arguments::config::mouse_mod { std::move(modifiers) } };
}

tl::expected<CommandData, std::string> parse_arguments(std::vector<std::string> arguments);

tl::expected<CommandData, std::string> parse_quit(const std::vector<std::string>& args)
{
    int code = 0;
    if (!args.empty()) {
        std::stringstream oss(args[0]);
        if (!(oss >> code)) {
            return tl::unexpected("malformed exit code '"s + args[0] + "'");
        }
    }
    return command_arguments::quit { code };
}

tl::expected<CommandData, std::string> parse_focus(const std::vector<std::string>& args)
{
    if (args.empty())
        return tl::unexpected("not enough arguments"s);

    if (args[0] == "left") {
        return command_arguments::focus { command_arguments::focus::Direction::Left, false };
    } else if (args[0] == "right") {
        return command_arguments::focus { command_arguments::focus::Direction::Right, false };
    } else if (args[0] == "cycle") {
        return command_arguments::focus { {}, true };
    }

    return tl::unexpected("invalid direction '"s + args[0] + "'");
}

tl::expected<CommandData, std::string> parse_exec(const std::vector<std::string>& args)
{
    return command_arguments::exec { args };
}

tl::expected<CommandData, std::string> parse_bind(const std::vector<std::string>& args)
{
    std::vector<std::string> modifiers;
    std::string key;

    auto locale = std::locale("");

    if (args.size() < 2) {
        return tl::unexpected("not enough arguments"s);
    }

    size_t pos = 0;
    while (pos < args[0].size()) {
        auto plus_index = args[0].find('+', pos);
        auto token = args[0].substr(pos, plus_index - pos);

        if (find_mod_key(token)) {
            modifiers.push_back(token);
        } else {
            for (char& c : token)
                c = std::tolower(c, locale);
            key = token;
        }

        if (plus_index == args[0].npos) {
            pos = args[0].size();
        } else {
            pos = plus_index + 1;
        }
    }

    auto sub_command_args = std::vector(args.begin() + 1, args.end());
    auto command_data = parse_arguments(sub_command_args);

    if (!command_data.has_value())
        return tl::unexpected("could not parse sub command: \n"s + command_data.error());

    return command_arguments::bind {
        std::move(modifiers),
        std::move(key),
        std::move(*command_data)
    };
}

tl::expected<CommandData, std::string> parse_close(const std::vector<std::string>&)
{
    return command_arguments::close {};
}

tl::expected<CommandData, std::string> parse_workspace(const std::vector<std::string>& args)
{
    using namespace command_arguments;

    if (args.empty()) {
        return tl::unexpected("not enough arguments"s);
    }

    if (args[0] == "switch") {
        if (args.size() < 2) {
            return tl::unexpected("not enough arguments"s);
        }

        return workspace { workspace::switch_ { std::stoi(args[1]) } };
    } else if (args[0] == "move") {
        if (args.size() < 2) {
            return tl::unexpected("not enough arguments"s);
        }

        return workspace { workspace::move { std::stoi(args[1]) } };
    } else {
        return tl::unexpected("unknown workspace sub-command"s);
    }
}

tl::expected<CommandData, std::string> parse_toggle_floating(const std::vector<std::string>&)
{
    return command_arguments::toggle_floating {};
}

tl::expected<CommandData, std::string> parse_move(const std::vector<std::string>& args)
{
    if (args.empty()) {
        return tl::unexpected("not enough arguments"s);
    }

    if (args.size() == 1) {
        return command_arguments::move { std::stoi(args[0]), 0 };
    } else {
        return command_arguments::move { std::stoi(args[0]), std::stoi(args[1]) };
    }
}

tl::expected<CommandData, std::string> parse_resize(const std::vector<std::string>& args)
{
    if (args.size() < 2) {
        return tl::unexpected("not enough arguments"s);
    }

    return command_arguments::resize { std::stoi(args[0]), std::stoi(args[1]) };
}

tl::expected<CommandData, std::string> parse_config(const std::vector<std::string>& args)
{
    if (args.empty()) {
        return tl::unexpected("not enough arguments"s);
    }

    std::string key = args[0];
    auto new_args = std::vector<std::string>(std::next(args.begin()), args.end());
    if (key == "mouse_mod") {
        return parse_config_mouse_mod(new_args);
    }

    return tl::unexpected("invalid config key '"s + key + "''");
}

tl::expected<CommandData, std::string> parse_cycle_width(const std::vector<std::string>&)
{
    return command_arguments::cycle_width {};
}

using parse_f = tl::expected<CommandData, std::string> (*)(const std::vector<std::string>&);
static std::unordered_map<std::string, parse_f> parse_table = {
    { "quit", parse_quit },
    { "focus", parse_focus },
    { "exec", parse_exec },
    { "bind", parse_bind },
    { "close", parse_close },
    { "workspace", parse_workspace },
    { "toggle_floating", parse_toggle_floating },
    { "move", parse_move },
    { "resize", parse_resize },
    { "config", parse_config },
    { "cycle_width", parse_cycle_width },
};

tl::expected<CommandData, std::string> parse_arguments(std::vector<std::string> arguments)
{
    if (arguments.empty()) {
        return tl::unexpected("not enough arguments"s);
    }

    if (auto it = detail::parse_table.find(arguments[0]); it != detail::parse_table.end()) {
        arguments.erase(arguments.begin());
        return (*it->second)(arguments);
    }

    return tl::unexpected("unknown command '"s + arguments[0] + "'");
}
}

tl::expected<CommandData, std::string> parse_arguments(int argc, char* argv[])
{
    std::vector<std::string> arguments;

    arguments.reserve(argc - 1);
    for (int i = 1; i < argc; i++) {
        arguments.emplace_back(std::string { argv[i] });
    }

    return detail::parse_arguments(std::move(arguments));
}

#endif //CUTTER_PARSE_ARGUMENTS_H_INCLUDED
