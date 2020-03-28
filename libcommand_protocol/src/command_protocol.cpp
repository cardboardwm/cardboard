#include <command_protocol.h>

#include "command.capnp.h"
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

void write_command_data(int fd, const CommandData&)
{

}