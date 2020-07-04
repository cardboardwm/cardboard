extern "C" {
#include <wlr/util/log.h>
}

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <cstring>

#include "IPC.h"
#include "Server.h"

#include <cardboard/command_protocol.h>
#include <cardboard/ipc.h>

std::optional<IPCInstance> create_ipc(
    Server& server,
    const std::string& socket_path,
    std::function<std::string(const CommandData&)> command_callback)
{
    int ipc_socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (ipc_socket_fd == -1) {
        wlr_log(WLR_ERROR, "Couldn't create ipc socket");
        return std::nullopt;
    }

    if (fcntl(ipc_socket_fd, F_SETFD, FD_CLOEXEC) == -1) {
        wlr_log(WLR_ERROR, "Unable to set CLOEXEC on IPC client socket: %s", strerror(errno));
        close(ipc_socket_fd);
        return std::nullopt;
    }

    if (fcntl(ipc_socket_fd, F_SETFL, O_NONBLOCK) == -1) {
        wlr_log(WLR_ERROR, "Unable to set O_NONBLOCK on IPC client socket: %s", strerror(errno));
        close(ipc_socket_fd);
        return std::nullopt;
    }

    std::unique_ptr<sockaddr_un> socket_address = std::make_unique<sockaddr_un>();

    socket_address->sun_family = AF_UNIX;
    memcpy(socket_address->sun_path, socket_path.c_str(), socket_path.size() + 1);

    unlink(socket_path.c_str());
    if (bind(ipc_socket_fd, reinterpret_cast<sockaddr*>(socket_address.get()), sizeof(*socket_address)) == -1) {
        int err = errno;
        wlr_log(WLR_ERROR, "Couldn't bind a name ('%s') to the IPC socket. (%d)", socket_address->sun_path, err);
        return std::nullopt;
    }

    if (listen(ipc_socket_fd, SOMAXCONN) == -1) {
        wlr_log(WLR_ERROR, "Couldn't listen to the IPC socket '%s'.", socket_address->sun_path);
        return std::nullopt;
    }

    // add destroy display handling
    IPCInstance ipc = std::make_unique<IPC>(IPC {
        &server, ipc_socket_fd, std::move(socket_address), std::move(command_callback) });
    wl_event_loop_add_fd(
        server.event_loop,
        ipc_socket_fd,
        WL_EVENT_READABLE,
        IPC::handle_client_connection,
        ipc.get());

    return ipc;
}

// This function implements wl_event_loop_fd_func_t
int IPC::handle_client_connection(int /*fd*/, uint32_t mask, void* data)
{
    assert(mask == WL_EVENT_READABLE);

    auto ipc = static_cast<IPC*>(data);
    const int client_fd = accept(ipc->socket_fd, nullptr, nullptr);
    if (client_fd == -1) {
        wlr_log(WLR_ERROR, "Failed to accept on IPC socket %s: %s", ipc->socket_address->sun_path, strerror(errno));
        return 0;
    }

    int flags;
    if ((flags = fcntl(client_fd, F_GETFD)) == -1
        || fcntl(client_fd, F_SETFD, flags | FD_CLOEXEC) == -1) {
        wlr_log(WLR_ERROR, "Unable to set CLOEXEC on IPC client socket: %s", strerror(errno));
        close(client_fd);
        return 0;
    }

    if ((flags = fcntl(client_fd, F_GETFL)) == -1
        || fcntl(client_fd, F_GETFL, flags | O_NONBLOCK) == -1) {
        wlr_log(WLR_ERROR, "Unable to set O_NONBLOCK on IPC client socket: %s", strerror(errno));
        close(client_fd);
        return 0;
    }

    ipc->clients.emplace_back(
        ipc,
        client_fd,
        IPC::ClientState::READING_HEADER);

    ipc->clients.back().readable_event_source = wl_event_loop_add_fd(
        ipc->server->event_loop,
        client_fd,
        WL_EVENT_READABLE,
        IPC::handle_client_readable,
        &ipc->clients.back());

    return 0;
}

int IPC::handle_client_readable(int /*fd*/, uint32_t mask, void* data)
{
    auto client = static_cast<IPC::Client*>(data);

    if (mask & WL_EVENT_ERROR) {
        wlr_log(WLR_ERROR, "IPC Client socket error, disconnecting");
        client->ipc->remove_client(client);
        return 0;
    }

    if (mask & WL_EVENT_HANGUP) {
        wlr_log(WLR_DEBUG, "IPC Client on fd %d hung up, disconnecting", client->client_fd);
        client->ipc->remove_client(client);
        return 0;
    }

    int available_bytes;
    if (ioctl(client->client_fd, FIONREAD, &available_bytes) == -1) {
        wlr_log(WLR_INFO, "Unable to read pending available data");
        client->ipc->remove_client(client);
        return 0;
    }

    switch (client->state) {
    case IPC::ClientState::READING_HEADER: {
        if (available_bytes < static_cast<int>(libcardboard::ipc::HEADER_SIZE)) {
            break;
        }

        libcardboard::ipc::AlignedHeaderBuffer buffer;
        if (ssize_t received = recv(
                client->client_fd,
                buffer.data(),
                libcardboard::ipc::HEADER_SIZE,
                0);
            received != -1) {
            libcardboard::ipc::Header header = libcardboard::ipc::interpret_header(buffer);
            client->payload_size = header.incoming_bytes;
            client->state = IPC::ClientState::READING_PAYLOAD;

            available_bytes -= received;

            if (client->payload_size <= available_bytes) {
                // do nothing jumps to the IPC::ClientState::READING_PAYLOAD case
            } else {
                break;
            }
        } else {
            wlr_log(WLR_INFO, "recv failed on header");
            client->ipc->remove_client(client);
            return 0;
        }
        [[fallthrough]];
    }
    case IPC::ClientState::READING_PAYLOAD: {
        if (client->payload_size > available_bytes)
            break;

        auto* buffer = new std::byte[client->payload_size];
        if (ssize_t received = recv(
                client->client_fd,
                buffer,
                client->payload_size,
                0);
            received == -1) {
            wlr_log(WLR_INFO, "couldn't read payload");
            client->ipc->remove_client(client);
            return 0;
        }
        read_command_data(buffer, client->payload_size)
            .map([client](const CommandData& command_data) {
                client->message = client->ipc->command_callback(command_data);
            })
            .map_error([client](const std::string& error) {
                using namespace std::string_literals;
                wlr_log(WLR_INFO, "unable to parse command: %s", error.c_str());
                client->message = "Unable to parse command: " + error;
            });
        delete[] buffer;

        client->writable_event_source = wl_event_loop_add_fd(
            client->ipc->server->event_loop,
            client->client_fd,
            WL_EVENT_WRITABLE,
            IPC::handle_client_writeable,
            client);

        client->state = IPC::ClientState::WRITING;
        break;
    }
    case IPC::ClientState::WRITING:
        break; // do nothing
    }

    return 0;
}

int IPC::handle_client_writeable(int /*fd*/, uint32_t mask, void* data)
{
    auto client = static_cast<IPC::Client*>(data);

    if (mask & WL_EVENT_ERROR) {
        wlr_log(WLR_ERROR, "IPC Client socket error, disconnecting");
        client->ipc->remove_client(client);
        return 0;
    }

    if (mask & WL_EVENT_HANGUP) {
        wlr_log(WLR_DEBUG, "IPC Client on fd %d hung up, disconnecting", client->client_fd);
        client->ipc->remove_client(client);
        return 0;
    }

    if (!client->message.empty()) {
        libcardboard::ipc::AlignedHeaderBuffer header = libcardboard::ipc::create_header_buffer({ static_cast<int>(client->message.size()) });

        auto* buffer = new std::byte[client->message.empty() + header.size()];
        std::copy(
            header.begin(),
            header.end(),
            buffer);

        std::copy(
            client->message.begin(),
            client->message.end(),
            reinterpret_cast<char*>(buffer));

        ssize_t written = write(client->client_fd, buffer, client->message.empty() + header.size());
        if (written == -1 && errno == EAGAIN) {
            return 0;
        }

        if (written == -1) {
            wlr_log(WLR_INFO, "Unable to send data to IPC client");
        }
    }

    client->ipc->remove_client(client);
    return 0;
}

void IPC::remove_client(IPC::Client* client)
{
    clients.remove_if(
        [client](auto& x) { return client == &x; });
}

IPC::Client::~Client()
{
    // shutdown routine for ipc client
    shutdown(client_fd, SHUT_RDWR);

    if (readable_event_source) {
        wl_event_source_remove(readable_event_source);
    }

    if (writable_event_source) {
        wl_event_source_remove(writable_event_source);
    }
}

IPC::Client::Client(IPC::Client&& other) noexcept
    : ipc { other.ipc }
    , client_fd { other.client_fd }
    , state { other.state }
    , readable_event_source { other.readable_event_source }
    , writable_event_source { other.writable_event_source }
    , payload_size { other.payload_size }
    , message { std::move(other.message) }
{
    other.ipc = nullptr;
    other.client_fd = 0;
    other.readable_event_source = nullptr;
    other.writable_event_source = nullptr;
    other.payload_size = 0;
}
