#ifndef CARDBOARD_LISTENER_H
#define CARDBOARD_LISTENER_H

#include <wayland-server.h>
#include <wlr_cpp/types/wlr_input_device.h>
#include <wlr_cpp/types/wlr_output.h>
#include <wlr_cpp/types/wlr_output_layout.h>

#include <algorithm>
#include <cassert>
#include <variant>

#include "Keyboard.h"
#include "View.h"

struct NoneT {
};
struct Server;

using ListenerData = std::variant<
    NoneT,
    wlr_output*,
    wlr_output_layout_output*,
    View*,
    wlr_input_device*,
    KeyHandleData>;

struct Listener {
    wl_listener listener;
    Server* server;
    ListenerData listener_data;

    Listener(wl_notify_func_t notify, Server* server, ListenerData listener_data)
        : listener { {}, notify }
        , server { server }
        , listener_data { std::move(listener_data) }
    {
    }
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

class ListenerList {
public:
    ~ListenerList()
    {
        for (auto& listener : listeners)
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

    template <typename T>
    void clear_listeners(T owner)
    {
        bool it_works = false;
        auto next = listeners.end();
        auto it = listeners.begin();
        while (it != listeners.end()) {
            next = std::next(it);

            if (auto pval = std::get_if<T>(&it->listener_data)) {
                if (*pval == owner) {
                    remove_listener(&it->listener);
                    it_works = true;
                }
            }

            it = next;
        }

        assert(it_works && "this object doesn't have listeners");
    }

private:
    std::list<Listener> listeners;
};

#endif //CARDBOARD_LISTENER_H
