#include <cardboard/ipc.h>
#include <cutter/client.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace libcutter {

libcutter::Client::~Client()
{
    close(socket_fd);
}

tl::expected<void, std::string> libcutter::Client::send_command(const CommandData& command_data)
{
    using namespace std::string_literals;

    auto buffer = write_command_data(command_data);

    if (!buffer) {
        return tl::unexpected(buffer.error());
    }

    libcardboard::ipc::AlignedHeaderBuffer header_buffer = libcardboard::ipc::create_header_buffer({ static_cast<int>(buffer->size() + 1) });

    if (write(socket_fd, header_buffer.data(), libcardboard::ipc::HEADER_SIZE) == -1) {
        int err = errno;
        return tl::unexpected("unable to write payload: "s + strerror(err));
    }

    if (write(socket_fd, buffer->c_str(), buffer->size() + 1) == -1) {
        int err = errno;
        return tl::unexpected("unable to write payload: "s + strerror(err));
    }

    return {};
}

tl::expected<std::string, int> libcutter::Client::wait_response()
{
    libcardboard::ipc::AlignedHeaderBuffer buffer;

    if (recv(socket_fd, buffer.data(), libcardboard::ipc::HEADER_SIZE, 0) == libcardboard::ipc::HEADER_SIZE) {
        libcardboard::ipc::Header header = libcardboard::ipc::interpret_header(buffer);

        if (header.incoming_bytes == 0) {
            return std::string {};
        }

        auto* response_buffer = new std::byte[header.incoming_bytes];
        if (recv(socket_fd, response_buffer, sizeof(header.incoming_bytes), 0) == header.incoming_bytes) {
            return std::string(reinterpret_cast<char*>(response_buffer));
        } else {
            int err = errno;
            return tl::unexpected(err);
        }
    } else {
        int err = errno;

        if (err == EBADF) {
            return std::string {};
        }

        return tl::unexpected(err);
    }
}

Client::Client(int socket_fd, std::unique_ptr<sockaddr_un> socket_address)
    : socket_fd { socket_fd }
    , socket_address { std::move(socket_address) }
{
}

Client::Client(Client&& other) noexcept
    : socket_fd { other.socket_fd }
    , socket_address { std::move(other.socket_address) }
{
    other.socket_fd = -1;
}

tl::expected<Client, std::string> open_client()
{
    using namespace std::string_literals;

    int socket_fd = -1;
    if (socket_fd = socket(AF_UNIX, SOCK_STREAM, 0); socket_fd == -1) {
        return tl::unexpected("Failed to create socket"s);
    }

    std::string socket_path;
    if (char* socket_env = getenv(libcardboard::ipc::SOCKET_ENV_VAR); socket_env != nullptr) {
        socket_path = socket_env;
    } else {
        if (char* wayland_env = getenv("WAYLAND_DISPLAY"); wayland_env != nullptr) {
            socket_path = "/tmp/cardboard-"s + wayland_env;
        } else {
            return tl::unexpected("WAYLAND_DISPLAY not set"s);
        }
    }

    auto socket_address = std::make_unique<sockaddr_un>();
    socket_address->sun_family = AF_UNIX;
    strncpy(socket_address->sun_path, socket_path.c_str(), socket_path.size());

    if (
        connect(
            socket_fd,
            reinterpret_cast<sockaddr*>(socket_address.get()),
            sizeof(sockaddr_un))
        == -1) {
        close(socket_fd);
        return tl::unexpected("Unable to connect"s);
    }

    return Client {
        socket_fd,
        std::move(socket_address)
    };
}
}