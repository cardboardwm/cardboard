#ifndef CARDBOARD_LISTENER_H
#define CARDBOARD_LISTENER_H

#include <wayland-server.h>
#include "View.h"

#include <variant>
#include <algorithm>

struct NoneT {};
struct Server;

using ListenerData = std::variant<
        NoneT,
        wlr_output*,
        View*,
        wlr_input_device*
    >;

struct Listener
{
    wl_listener listener;
    Server* server;
    ListenerData listener_data;

    Listener(wl_notify_func_t notify, Server* server, ListenerData listener_data):
        listener {{}, notify},
        server{server},
        listener_data{ std::move(listener_data) }
    {}
};

inline Server* get_server(wl_listener* listener)
{
    Listener* l = wl_container_of(listener, l, listener);
    return l->server;
}

template <typename T>
T& get_listener_data(wl_listener* listener)
{
    Listener* l = wl_container_of(listener, l, listener);
    return std::get<T>(l->listener_data);
}

class ListenerList
{
public:

    ~ListenerList()
    {
        for(auto& listener: listeners)
            wl_list_remove(&listener.listener.link);
    }

    void add_listener(wl_signal* signal, Listener&& listener)
    {
        listeners.push_back(std::move(listener));
        wl_signal_add(signal, &listeners.back().listener);
    }

    void remove_listener(wl_listener* raw_listener)
    {
        wl_list_remove(&raw_listener->link);

        listeners.remove_if([raw_listener](auto& listener) {
            return raw_listener == &listener.listener;
        });
    }

private:
    std::list<Listener> listeners;
};

#endif //CARDBOARD_LISTENER_H
