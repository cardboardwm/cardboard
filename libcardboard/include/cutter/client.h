#ifndef BUILD_CLIENT_H
#define BUILD_CLIENT_H

#include <string>
#include <memory>
#include <tl/expected.hpp>

#include <cardboard/command_protocol.h>

#include <sys/un.h>

namespace libcutter
{
    class Client
    {
    public:
        Client(const Client&) = delete;
        Client(Client&&) noexcept;
        ~Client();

        tl::expected<void, std::string> send_command(const CommandData&);
        tl::expected<std::string, int> wait_response();
    private:
        Client(int, std::unique_ptr<sockaddr_un>);
    private:
        int socket_fd;
        std::unique_ptr<sockaddr_un> socket_address;

        friend tl::expected<Client, std::string> open_client();
    };

    tl::expected<Client, std::string> open_client();
}

#endif //BUILD_CLIENT_H
