#include <command_protocol.h>

#include "command.capnp.h"
#include <capnp/message.h>
#include <capnp/serialize-packed.h>

static CommandData deserialize(const generated::Command::Reader& reader)
{
    switch (reader.getCommand()) {
    case generated::Command::Commands::QUIT:
        return CommandArguments::quit{};
    case generated::Command::Commands::FOCUS:
        return CommandArguments::focus{
            reader.getArguments().getFocusDirection() == generated::Command::FocusDirection::LEFT ?
                CommandArguments::focus::Direction::Left : CommandArguments::focus::Direction::Right
        };
    case generated::Command::Commands::BIND:
        return CommandArguments::bind{
            reader.getArguments().getBind().getKeyBinding().cStr(),
            std::make_unique<CommandData>(deserialize(reader.getArguments().getBind().getCommand()))
        };
    case generated::Command::Commands::EXEC:
        return CommandArguments::exec{
            reader.getArguments().getExecCommand().cStr()
        };
    }
}

CommandData read_command_data(int fd)
{
    ::capnp::PackedFdMessageReader message{fd};
    return deserialize(message.getRoot<generated::Command>());
}

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

static void serialize(generated::Command::Builder& builder, const CommandData& command_data)
{
    std::visit(overloaded {
        [&builder](const CommandArguments::focus& focus_data) {
            builder.setCommand(generated::Command::Commands::FOCUS);
            builder.initArguments().setFocusDirection(
                focus_data.direction == CommandArguments::focus::Direction::Left ?
                generated::Command::FocusDirection::LEFT : generated::Command::FocusDirection::RIGHT
            );
        },
        [&builder](const CommandArguments::quit&) {
            builder.setCommand(generated::Command::Commands::QUIT);
        },
        [&builder](const CommandArguments::bind& bind_data) {
            builder.setCommand(generated::Command::Commands::BIND);
            auto bind_builder = builder.initArguments().initBind();
            bind_builder.setKeyBinding(bind_data.key_binding);
            auto command_builder = bind_builder.initCommand();
            serialize(command_builder, *(bind_data.command.get()));
        },
        [&builder](const CommandArguments::exec& exec_data)  {
            builder.setCommand(generated::Command::Commands::EXEC);
            builder.initArguments().setExecCommand(exec_data.command);
        },
    }, command_data);
}

void write_command_data(int fd, const CommandData& command_data)
{
    ::capnp::MallocMessageBuilder message;
    auto command = message.initRoot<generated::Command>();
    serialize(command, command_data);

    writePackedMessageToFd(fd, message);
}