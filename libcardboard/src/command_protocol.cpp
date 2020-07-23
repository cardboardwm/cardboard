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

/// \cond IGNORE
namespace cereal {

template <typename Archive>
void serialize(Archive& ar, command_arguments::quit& quit)
{
    ar(quit.code);
}

template <typename Archive>
void serialize(Archive& ar, command_arguments::focus& focus)
{
    ar(focus.direction, focus.cycle);
}

template <typename Archive>
void serialize(Archive& ar, command_arguments::exec& exec)
{
    ar(exec.argv);
}

template <typename Archive>
void serialize(Archive& ar, command_arguments::bind& bind)
{
    ar(bind.modifiers, bind.key, bind.command);
}

template <typename Archive>
void serialize(Archive&, command_arguments::close&)
{
}

template <typename Archive>
void serialize(Archive& ar, command_arguments::workspace::switch_& switch_)
{
    ar(switch_.n);
}

template <typename Archive>
void serialize(Archive& ar, command_arguments::workspace::move& move)
{
    ar(move.n);
}

template <typename Archive>
void serialize(Archive& ar, command_arguments::workspace& workspace)
{
    ar(workspace.workspace);
}

template <typename Archive>
void serialize(Archive&, command_arguments::toggle_floating&)
{
}

template <typename Archive>
void serialize(Archive& ar, command_arguments::move& move)
{
    ar(move.dx, move.dy);
}

template <typename Archive>
void serialize(Archive& ar, command_arguments::resize& resize)
{
    ar(resize.width, resize.height);
}

template <typename Archive>
void serialize(Archive&, command_arguments::insert_into_column&)
{
}

template <typename Archive>
void serialize(Archive&, command_arguments::pop_from_column&)
{
}

template <typename Archive>
void serialize(Archive& ar, command_arguments::config::mouse_mod& mouse_mod)
{
    ar(mouse_mod.modifiers);
}

template <typename Archive>
void serialize(Archive& ar, command_arguments::config::gap& gap)
{
    ar(gap.gap);
}

template <typename Archive>
void serialize(Archive& ar, command_arguments::config::focus_color& focus_color)
{
    ar(focus_color.r, focus_color.g, focus_color.b, focus_color.a);
}

template <typename Archive>
void serialize(Archive& ar, command_arguments::config& config)
{
    ar(config.config);
}

template <typename Archive>
void serialize(Archive&, command_arguments::cycle_width&)
{
}
}
/// \endcond

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

tl::expected<std::string, std::string> write_command_data(const CommandData& command_data)
{
    using namespace std::string_literals;

    std::stringstream buffer_stream;

    {
        cereal::PortableBinaryOutputArchive archive { buffer_stream };
        archive(command_data);
    }

    return buffer_stream.str();
}
