#ifndef BUILD_VIEWANIMATION_H
#define BUILD_VIEWANIMATION_H

extern "C" {
#include <wayland-server.h>
};

#include <memory>
#include <deque>
#include <chrono>
#include "View.h"

struct Server;

class ViewAnimation;
using ViewAnimationInstance = std::unique_ptr<ViewAnimation>;

struct AnimationTask {
    View* view;
    int targetx;
    int targety;
    std::function<void()> animation_finished_callback = nullptr;
};

struct AnimationSettings {
    int ms_per_frame;
    int animation_duration; //ms
};

class ViewAnimation {
    ViewAnimation(AnimationSettings);

public:
    void enqueue_task(const AnimationTask&);

private:
    struct Task
    {
        View* view;
        int startx, starty;
        int targetx, targety;
        std::chrono::time_point<std::chrono::high_resolution_clock> begin;
        std::function<void()> animation_finished_callback;
    };

    std::deque<Task> tasks;

    wl_event_source* event_source;
    AnimationSettings settings;
    static int timer_callback(void* data);
    friend ViewAnimationInstance create_view_animation(Server* server, AnimationSettings);
};

ViewAnimationInstance create_view_animation(Server* server, AnimationSettings);

#endif //BUILD_VIEWANIMATION_H
