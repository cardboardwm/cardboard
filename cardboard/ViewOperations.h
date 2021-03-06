// SPDX-License-Identifier: GPL-3.0-only
/*
Copyright (C) 2020 Alexandru-Iulian Magan, Tudor-Ioan Roman, and contributors.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef CARDBOARD_VIEW_OPERATIONS_H_INCLUDED
#define CARDBOARD_VIEW_OPERATIONS_H_INCLUDED

#include "Server.h"
#include "View.h"

#include "NotNull.h"
#include "StrongAlias.h"

/**
 * \file
 * \brief ViewOperations hosts high-level View management functions, handling various low-level details.
 *
 * For example, if you want to move a View to a Workspace, you should use change_view_workspace instead of
 * assigning the workspace_id field manually.
 * */

struct Workspace;

using AbsoluteScroll = StrongAlias<int, struct AbsoluteScrollTag>;
using RelativeScroll = StrongAlias<int, struct RelativeScrollTag>;

/// Sends a view to another workspace
void change_view_workspace(Server& server, View& view, Workspace& new_workspace);

/// Moves window while maintaining windows manager integrity
void reconfigure_view_position(Server& server, View& view, int x, int y, bool animate = true);

/// Resizes window while maintaining windows manager integrity
void reconfigure_view_size(Server& server, View& view, int width, int height);

/// Sets the workspace absolute scroll position to `scroll`
void scroll_workspace(OutputManager&, Workspace&, AbsoluteScroll scroll, bool animate = true);

/// Scrolls workspace with as `scroll` as offset.
void scroll_workspace(OutputManager&, Workspace&, RelativeScroll scroll, bool animate = true);

#endif //CARDBOARD_VIEW_OPERATIONS_H_INCLUDED
