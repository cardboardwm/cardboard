#include "ViewAnimation.h"

#include "Server.h"

ViewAnimation::ViewAnimation(Server *server, int ms_per_frame): server{server}, ms_per_frame{ms_per_frame}
{
}

int ViewAnimation::timer_callback(void *data)
{
    auto* view_animation = static_cast<ViewAnimation*>(data);

    wl_event_source_timer_update(view_animation->event_source, view_animation->ms_per_frame);
    return 0;
}

ViewAnimationInstance create_view_animation(Server* server, int ms_per_frame)
{
    auto view_animation = std::make_unique<ViewAnimation>(ViewAnimation{server, ms_per_frame});
    view_animation->event_source =
            wl_event_loop_add_timer(server->event_loop, ViewAnimation::timer_callback, view_animation.get());

    wl_event_source_timer_update(view_animation->event_source, ms_per_frame);

    return view_animation;
}
