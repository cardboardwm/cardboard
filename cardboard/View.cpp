extern "C" {
#include <wayland-server.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
}

#include <array>
#include <cassert>
#include <cmath>
#include <limits>
#include <utility>

#include "Server.h"
#include "View.h"

OptionalRef<Output> View::get_views_output(Server& server)
{
    if (workspace_id < 0) {
        return NullRef<Output>;
    }
    return server.get_views_workspace(this).output;
}

void View::change_output(OptionalRef<Output> old_output, OptionalRef<Output> new_output)
{
    if (old_output && old_output != new_output) {
        wlr_surface_send_leave(get_surface(), old_output.unwrap().wlr_output);
    }

    if (new_output) {
        wlr_surface_send_enter(get_surface(), new_output.unwrap().wlr_output);
    }
}

void View::save_state(ExpansionSavedState to_save)
{
    assert(!saved_state.has_value() && expansion_state == ExpansionState::NORMAL);

    saved_state = to_save;
    wlr_log(WLR_DEBUG, "saved size (%4d, %4d)", saved_state->width, saved_state->height);
}

void View::recover()
{
    if (expansion_state == ExpansionState::RECOVERING) {
        expansion_state = ExpansionState::NORMAL;
    }
}

void View::cycle_width(int screen_width)
{
    if (screen_width == 0) {
        // shouldn't happen, but better safe than sorry
        return;
    }
    static constexpr double golden_ratio = 0x1.3c6ef372fe94fp-1; // 1 / phi
    constexpr std::array<double, 3> ratios = { 1 - golden_ratio, 0.5, golden_ratio };
    double current_ratio = static_cast<double>(geometry.width) / static_cast<double>(screen_width);

    decltype(ratios)::size_type i = 0;
    while (i < ratios.size() && ratios[i] <= current_ratio) {
        i++;
    }

    if (fabs(current_ratio - ratios[i]) < 0.01) {
        i = (i + 1) % ratios.size();
    }

    if (i == ratios.size()) {
        i = 0;
    }
    resize(static_cast<int>(screen_width * ratios[i]), geometry.height);
}

void View::move(int x_, int y_)
{
    x = x_;
    y = y_;
}

void create_view(Server& server, NotNullPointer<View> view_)
{
    server.views.push_back(view_);
    View* view = server.views.back();

    view->prepare(server);
}
