#include <cstdlib>
#include <iostream>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cardboard_common/IPC.h>
#include <command_protocol.h>
#include <ipc.h>

#include "parse_arguments.h"

void print_usage(char* argv0)
{
    std::cerr << "Usage: " << argv0 << " <command> [args...]" << std::endl;
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    struct sockaddr_un sock_address = {};
    sock_address.sun_family = AF_UNIX;

    int sock_fd = -1;
    if (sock_fd = socket(AF_UNIX, SOCK_STREAM, 0); sock_fd == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        return EXIT_FAILURE;
    }

    std::string socket_path;
    if (char* socket_env = getenv(IPC_SOCKET_ENV_VAR); socket_env != nullptr) {
        socket_path = socket_env;
    } else {
        socket_path = "/tmp/cardboard-";
        if (char* wayland_env = getenv("WAYLAND_DISPLAY"); wayland_env != nullptr) {
            socket_path += wayland_env;
        } else {
            socket_path += "wayland-0";
        }
    }
    strncpy(sock_address.sun_path, socket_path.c_str(), socket_path.size());

    if (connect(sock_fd, reinterpret_cast<sockaddr*>(&sock_address), sizeof(sock_address)) == -1) {
        std::cerr << "Failed to connect to the socket '" << socket_path << "'" << std::endl;
        close(sock_fd);
        return EXIT_FAILURE;
    }

    tl::expected<CommandData, std::string> command_data = parse_arguments(argc, argv);

    if (!command_data.has_value()) {
        std::cerr << "Unable to parse data: " << command_data.error() << std::endl;
        close(sock_fd);
        return EXIT_FAILURE;
    }

    auto write_result = write_command_data(sock_fd, *command_data);
    if (!write_result) {
        std::cerr << "error: " << write_result.error() << std::endl;
        close(sock_fd);
        return EXIT_FAILURE;
    }

    libcardboard::ipc::AlignedHeaderBuffer buffer;
    if(recv(sock_fd, buffer.data(), libcardboard::ipc::HEADER_SIZE, 0) == libcardboard::ipc::HEADER_SIZE)
    {
        libcardboard::ipc::Header header = libcardboard::ipc::interpret_header(buffer);
        auto* response_buffer = new std::byte[header.incoming_bytes];
        if (recv(sock_fd, response_buffer, sizeof(header.incoming_bytes), 0) == header.incoming_bytes) {
            std::cout << response_buffer << std::flush;
        }

    }

    close(sock_fd);
    return 0;
}
