#ifndef __CARDBOARD_HELPERS_H_
#define __CARDBOARD_HELPERS_H_

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

#endif // __CARDBOARD_HELPERS_H_
