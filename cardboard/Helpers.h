// SPDX-License-Identifier: GPL-3.0-only
/*
Copyright (C) 2020 Alexandru-Iulian Magan, Tudor-Ioan Roman, and contributors.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
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
