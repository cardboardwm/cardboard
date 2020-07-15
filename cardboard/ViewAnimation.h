#ifndef BUILD_VIEWANIMATION_H
#define BUILD_VIEWANIMATION_H

extern "C" {
#include <wayland-server.h>
};

#include <memory>

class Server;

class ViewAnimation;
using ViewAnimationInstance = std::unique_ptr<ViewAnimation>;

class ViewAnimation {
    ViewAnimation(Server* server, int ms_per_frame);

public:

private:
    Server* server;
    wl_event_source* event_source;
    int ms_per_frame;
    static int timer_callback(void* data);
    friend ViewAnimationInstance create_view_animation(Server* server, int ms_per_frame);
};

ViewAnimationInstance create_view_animation(Server* server, int ms_per_frame);

#endif //BUILD_VIEWANIMATION_H
