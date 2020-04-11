#ifndef CARDBOARD_LISTENER_H
#define CARDBOARD_LISTENER_H

#include "BuildConfig.h"

extern "C" {
#include <wayland-server.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
}

#include <algorithm>
#include <cassert>
#include <variant>

#include "Cursor.h"
#include "Keyboard.h"
#include "Layers.h"
#include "XDGView.h"
#if HAVE_XWAYLAND
#include "Xwayland.h"
#endif

struct NoneT {
};
struct Server;

/// Constant data for many event listeners.
using ListenerData = std::variant<
    NoneT,
    KeyHandleData,
    LayerSurface*,
    LayerSurfacePopup*,
    Output*,
    Keyboard*,
    SeatCursor*,
    Seat*,
#if HAVE_XWAYLAND
    XwaylandView*,
#endif
    XDGView*,
    XDGPopup*>;

/**
 * \brief The Listener is a wrapper around Wayland's \c wl_listener concept.
 *
 * It holds context information related to an event handler call.
 */
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

/**
 * \brief Returns a pointer to the Server struct from within a \a listener
 * given to a Wayland event handler function.
 */
inline Server* get_server(wl_listener* listener)
{
    Listener* l = wl_container_of(listener, l, listener);
    return l->server;
}

/**
 * \brief Returns the enclosed data from a \a listener.
 */
template <typename T>
T& get_listener_data(wl_listener* listener)
{
    Listener* l = wl_container_of(listener, l, listener);
    return std::get<T>(l->listener_data);
}

/**
 * \brief Holds the event listeners and Listener objects for all the event handlers
 * registered during the lifetime of the compositor.
 */
class ListenerList {
public:
    ~ListenerList()
    {
        for (auto& listener : listeners)
            wl_list_remove(&listener.listener.link);
    }

    /// Registers a \a listener for a given \a signal.
    wl_listener* add_listener(wl_signal* signal, Listener&& listener)
    {
        listeners.push_back(std::move(listener));
        wl_signal_add(signal, &listeners.back().listener);

        return &listeners.back().listener;
    }

    /// Unregisters a Listener object associated with a ray Wayland \a raw_listener.
    void remove_listener(wl_listener* raw_listener)
    {
        wl_list_remove(&raw_listener->link);

        listeners.remove_if([raw_listener](auto& listener) { return raw_listener == &listener.listener; });
    }

    /// Unregisters all event listeners associated with \a owner (i.e. \a owner is the listener data).
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
