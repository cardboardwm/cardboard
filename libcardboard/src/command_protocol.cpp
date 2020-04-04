#include <command_protocol.h>

#include "command.capnp.h"
#include <capnp/message.h>
#include <capnp/serialize-packed.h>

static CommandData deserialize(const generated::Command::Reader& reader)
{
    switch (reader.getCommand()) {
    case generated::Command::Commands::QUIT:
        return CommandArguments::quit {};
    case generated::Command::Commands::FOCUS:
        return CommandArguments::focus {
            reader.getArguments().getFocusDirection() == generated::Command::FocusDirection::LEFT ? CommandArguments::focus::Direction::Left : CommandArguments::focus::Direction::Right
        };
    case generated::Command::Commands::BIND: {
        std::vector<std::string> modifiers;
        for (auto modifier : reader.getArguments().getBind().getModifiers())
            modifiers.emplace_back(modifier.cStr());
        return CommandArguments::bind {
            std::move(modifiers),
            reader.getArguments().getBind().getKey().cStr(),
            deserialize(reader.getArguments().getBind().getCommand())
        };
    }
    case generated::Command::Commands::EXEC:
        std::vector<std::string> argv;
        for (auto arg : reader.getArguments().getExecCommand())
            argv.emplace_back(arg.cStr());
        return CommandArguments::exec { std::move(argv) };
    }

    return {};
}

std::optional<CommandData> read_command_data(int fd)
{
    try {
        ::capnp::PackedFdMessageReader message { fd };
        return deserialize(message.getRoot<generated::Command>());
    } catch (const kj::Exception&) {
        return std::nullopt;
    }
}

template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...)->overloaded<Ts...>;

static void serialize(generated::Command::Builder& builder, const CommandData& command_data)
{
    std::visit(overloaded {
                   [&builder](const CommandArguments::focus& focus_data) {
                       builder.setCommand(generated::Command::Commands::FOCUS);
                       builder.initArguments().setFocusDirection(
                           focus_data.direction == CommandArguments::focus::Direction::Left ? generated::Command::FocusDirection::LEFT : generated::Command::FocusDirection::RIGHT);
                   },
                   [&builder](const CommandArguments::quit&) {
                       builder.setCommand(generated::Command::Commands::QUIT);
                   },
                   [&builder](const CommandArguments::bind& bind_data) {
                       builder.setCommand(generated::Command::Commands::BIND);
                       auto bind_builder = builder.initArguments().initBind();
                       auto modifiers = bind_builder.initModifiers(bind_data.modifiers.size());
                       for (size_t i = 0; i < bind_data.modifiers.size(); i++)
                           modifiers.set(i, bind_data.modifiers[i]);
                       bind_builder.setKey(bind_data.key);
                       auto command_builder = bind_builder.initCommand();
                       serialize(command_builder, *(bind_data.command.get()));
                   },
                   [&builder](const CommandArguments::exec& exec_data) {
                       builder.setCommand(generated::Command::Commands::EXEC);
                       auto argv = builder.initArguments().initExecCommand(exec_data.argv.size());

                       for (size_t i = 0; i < exec_data.argv.size(); i++)
                           argv.set(i, exec_data.argv[i]);
                   },
               },
               command_data);
}

bool write_command_data(int fd, const CommandData& command_data)
{
    try {
        ::capnp::MallocMessageBuilder message;
        auto command = message.initRoot<generated::Command>();
        serialize(command, command_data);

        writePackedMessageToFd(fd, message);
        return true;
    } catch (const kj::Exception&) {
        return false;
    }
}