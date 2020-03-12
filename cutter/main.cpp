#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cardboard_common/IPC.h>

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

    std::vector<uint8_t> cmd;
    for (int i = 1; i < argc; i++) {
        size_t len = strlen(argv[i]);
        if (len > 255) {
            std::cerr << "Argument length is longer than 255" << std::endl;
            close(sock_fd);
            return EXIT_FAILURE;
        }
        cmd.reserve(cmd.size() + len + 1); // 1 for the length byte

        cmd.push_back(len);
        for (size_t j = 0; j < len; j++) {
            cmd.push_back(argv[i][j]);
        }
    }
    cmd.push_back(0);

    if (send(sock_fd, cmd.data(), cmd.size(), 0) == -1) {
        std::cerr << "Failed to send the command" << std::endl;
        close(sock_fd);
        return EXIT_FAILURE;
    }

    char answer[BUFSIZ];
    if (recv(sock_fd, answer, sizeof(answer), 0) > 0) {
        std::cout << answer << std::flush;
    }

    close(sock_fd);
    return 0;
}
