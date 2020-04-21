#ifndef __CUTTER_PARSE_ARGUMENTS_H_
#define __CUTTER_PARSE_ARGUMENTS_H_

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
    return CommandArguments::quit { code };
}

tl::expected<CommandData, std::string> parse_focus(const std::vector<std::string>& args)
{
    if (args.empty())
        return tl::unexpected("not enough arguments"s);

    if (args[0] == "left") {
        return CommandArguments::focus { CommandArguments::focus::Direction::Left };
    }
    else if (args[0] == "right") {
        return CommandArguments::focus { CommandArguments::focus::Direction::Right };
    }

    return tl::unexpected("unrecognized word '"s + args[0] + "'");
}

tl::expected<CommandData, std::string> parse_exec(const std::vector<std::string>& args)
{
    return CommandArguments::exec { args };
}

tl::expected<CommandData, std::string> parse_bind(const std::vector<std::string>& args)
{
    static const auto find_mod_key = [](const std::string& key) {
        static const std::unordered_set<std::string> mod_keys = { "shift", "ctrl", "alt", "super", "caps", "mod2", "mod3", "mod5" };
        return mod_keys.find(key) != mod_keys.end();
    };

    std::vector<std::string> modifiers;
    std::string key;

    auto locale = std::locale("");

    if (args.size() < 2){
        return tl::unexpected("not enough arguments"s);
    }

    size_t pos = 0;
    while (pos < args[0].size()) {
        auto plus_index = args[0].find('+', pos);
        auto token = args[0].substr(pos, plus_index - pos);

        if (find_mod_key(token)) {
            modifiers.push_back(token);
        }
        else {
            for (char& c : token)
                c = std::tolower(c, locale);
            key = token;
        }

        if (plus_index == args[1].npos) {
            pos = args[0].size();
        }
        else {
            pos = plus_index + 1;
        }
    }

    auto sub_command_args = std::vector(args.begin() + 1, args.end());
    auto command_data = parse_arguments(sub_command_args);

    if (!command_data.has_value())
        return tl::unexpected("could not parse sub command: \n"s + command_data.error());

    return CommandArguments::bind {
        std::move(modifiers),
        std::move(key),
        std::move(*command_data)
    };
}

using parse_f = tl::expected<CommandData, std::string> (*)(const std::vector<std::string>&);
static std::unordered_map<std::string, parse_f> parse_table = {
    { "quit", parse_quit },
    { "focus", parse_focus },
    { "exec", parse_exec },
    { "bind", parse_bind },
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

#endif //__CUTTER_PARSE_ARGUMENTS_H_
