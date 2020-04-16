#include <numeric>
#include <sstream>

#include <command_protocol.h>

#include <ipc.h>
#include <unistd.h>

#include <cereal/archives/binary.hpp>
#include <cereal/types/variant.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/memory.hpp>

namespace cereal {
    template<typename Archive>
    void serialize(Archive& ar, CommandArguments::quit&)
    {
        ar(bool(true));
    }

    template<typename Archive>
    void serialize(Archive& ar, CommandArguments::focus& focus)
    {
        ar(focus.direction);
    }

    template<typename Archive>
    void serialize(Archive& ar, CommandArguments::exec& exec)
    {
        ar(exec.argv);
    }

    template<typename Archive>
    void serialize(Archive& ar, CommandArguments::bind& bind)
    {
        ar(bind.modifiers, bind.key, bind.command);
    }
}


std::optional<CommandData> read_command_data(void* data, size_t size)
{
    try{
        std::string buffer(static_cast<char*>(data), size);
        std::stringstream buffer_stream{buffer};
        cereal::BinaryInputArchive archive{buffer_stream};

        CommandData command_data;
        archive(command_data);

        return command_data;
    }
    catch(const cereal::Exception& e)
    {
        std::cerr << e.what() << std::endl;
        return std::nullopt;
    }
}


bool write_command_data(int fd, const CommandData& command_data)
{
    std::stringstream buffer_stream;
    cereal::BinaryOutputArchive archive{buffer_stream};

    archive(command_data);
    std::string buffer = buffer_stream.str();

    libcardboard::ipc::AlignedHeaderBuffer header_buffer =
        libcardboard::ipc::create_header_buffer({static_cast<int>(buffer.size() + 1)});

    if(write(fd, header_buffer.data(), libcardboard::ipc::HEADER_SIZE) == -1)
        return false;

    return write(fd, buffer.c_str(), buffer.size() + 1) != -1;
}