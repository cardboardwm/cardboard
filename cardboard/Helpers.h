#ifndef CARDBOARD_HELPERS_H_INCLUDED
#define CARDBOARD_HELPERS_H_INCLUDED

#include <initializer_list>

#include "Listener.h"
#include "Server.h"

constexpr void register_handlers(Server& server, ListenerData subject, std::initializer_list<ListenerPair> pairs)
{
    for (const auto& pair : pairs) {
        server.listeners.add_listener(pair.signal,
                                      Listener { pair.notify, &server, subject });
    }
}

#endif // CARDBOARD_HELPERS_H_INCLUDED
