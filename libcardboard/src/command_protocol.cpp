#include <unistd.h>

#include <cereal/archives/portable_binary.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/variant.hpp>
#include <cereal/types/vector.hpp>

#include <numeric>
#include <sstream>

#include <cardboard/command_protocol.h>
#include <cardboard/ipc.h>

namespace cereal {
template <typename Archive>
void serialize(Archive& ar, CommandArguments::quit& quit)
{
    ar(quit.code);
}

template <typename Archive>
void serialize(Archive& ar, CommandArguments::focus& focus)
{
    ar(focus.direction);
}

template <typename Archive>
void serialize(Archive& ar, CommandArguments::exec& exec)
{
    ar(exec.argv);
}

template <typename Archive>
void serialize(Archive& ar, CommandArguments::bind& bind)
{
    ar(bind.modifiers, bind.key, bind.command);
}
}

tl::expected<CommandData, std::string> read_command_data(void* data, size_t size)
{
    try {
        std::string buffer(static_cast<char*>(data), size);
        std::istringstream buffer_stream { buffer };
        cereal::PortableBinaryInputArchive archive { buffer_stream };

        CommandData command_data;
        archive(command_data);

        return command_data;
    } catch (const cereal::Exception& e) {
        return tl::unexpected(std::string { e.what() });
    }
}

tl::expected<void, std::string> write_command_data(int fd, const CommandData& command_data)
{
    using namespace std::string_literals;

    std::stringstream buffer_stream;

    {
        cereal::PortableBinaryOutputArchive archive { buffer_stream };
        archive(command_data);
    }

    std::string buffer = buffer_stream.str();

    libcardboard::ipc::AlignedHeaderBuffer header_buffer = libcardboard::ipc::create_header_buffer({ static_cast<int>(buffer.size() + 1) });

    if (write(fd, header_buffer.data(), libcardboard::ipc::HEADER_SIZE) == -1) {
        int err = errno;
        return tl::unexpected("unable to write payload: "s + strerror(err));
    }

    if (write(fd, buffer.c_str(), buffer.size() + 1) == -1) {
        int err = errno;
        return tl::unexpected("unable to write payload: "s + strerror(err));
    }

    return {};
}