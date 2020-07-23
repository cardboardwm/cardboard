#ifndef BUILD_VIEWANIMATION_H
#define BUILD_VIEWANIMATION_H

extern "C" {
#include <wayland-server.h>
};

#include <chrono>
#include <deque>
#include <memory>

#include "View.h"

struct Server;

class ViewAnimation;
using ViewAnimationInstance = std::unique_ptr<ViewAnimation>;

struct AnimationTask {
    View* view;
    int target_x;
    int target_y;
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
    /// Cancel animation tasks for the given view and warp it to its target coords.
    void cancel_tasks(View&);

private:
    struct Task {
        View* view;
        int startx, starty;
        int targetx, targety;
        std::chrono::time_point<std::chrono::high_resolution_clock> begin;
        std::function<void()> animation_finished_callback;
        bool cancelled;

        // aggregate initialization not working wtf
        Task(View* view, int startx, int starty, int targetx, int targety, std::chrono::time_point<std::chrono::high_resolution_clock> begin, std::function<void()> animation_finished_callback)
            : view(view)
            , startx(startx)
            , starty(starty)
            , targetx(targetx)
            , targety(targety)
            , begin(begin)
            , animation_finished_callback(animation_finished_callback)
            , cancelled(false)
        {
        }
    };

    std::deque<Task> tasks;

    wl_event_source* event_source;
    AnimationSettings settings;
    static int timer_callback(void* data);
    friend ViewAnimationInstance create_view_animation(Server* server, AnimationSettings);
};

ViewAnimationInstance create_view_animation(Server* server, AnimationSettings);

#endif //BUILD_VIEWANIMATION_H
