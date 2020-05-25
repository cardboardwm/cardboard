#ifndef CARDBOARD_WINDOWMANAGER_H
#define CARDBOARD_WINDOWMANAGER_H

#include "Server.h"
#include "View.h"

#include "NotNull.h"
#include "StrongAlias.h"

struct Workspace;

using AbsoluteScroll = StrongAlias<int, struct AbsoluteScrollTag>;
using RelativeScroll = StrongAlias<int, struct RelativeScrollTag>;

/// Sends a view to another workspace
void change_view_workspace(NotNullPointer<Server> server, NotNullPointer<View> view, NotNullPointer<Workspace> new_workspace);

/// Moves window while maintaining windows manager integrity
void reconfigure_view_position(NotNullPointer<Server> server, NotNullPointer<View> view, int x, int y);

/// Resizes window while maintaining windows manager integrity
void reconfigure_view_size(NotNullPointer<Server> server, NotNullPointer<View> view, int width, int height);

/// Sets the workspace absolute scroll position to `scroll`
void scroll_workspace(OutputManager&, NotNullPointer<Workspace>, AbsoluteScroll scroll);

/// Scrolls workspace with as `scroll` as offset.
void scroll_workspace(OutputManager&, NotNullPointer<Workspace>, RelativeScroll scroll);

#endif //CARDBOARD_WINDOWMANAGER_H
