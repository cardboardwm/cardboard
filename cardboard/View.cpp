extern "C" {
#include <wayland-server.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
}

#include <cassert>
#include <limits>

#include "Server.h"
#include "View.h"

void create_view(Server* server, View* view_)
{
    server->views.push_back(view_);
    View* view = server->views.back();

    view->prepare(server);
}
