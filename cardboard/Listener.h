#ifndef CARDBOARD_LISTENER_H
#define CARDBOARD_LISTENER_H

#include <wayland-server.h>
#include "Server.h"

struct Listener
{
    wl_listener* listener;
    Server* server;
    void* user_pointer;
};

Server* get_server(wl_listener* listener)
{
    Listener* l = wl_container_of(listener, l, listener);
    return l->server;
}

void* get_user_pointer(wl_listener* listener)
{
    Listener* l = wl_container_of(listener, l, listener);
    return l->user_pointer;
}

#endif //CARDBOARD_LISTENER_H
