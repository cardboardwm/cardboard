#include "ViewAnimation.h"

#include "Server.h"

ViewAnimation::ViewAnimation(Server *server, AnimationSettings settings):
    server{server},
    settings{settings}
{
}

void ViewAnimation::enqueue_task(const AnimationTask& task)
{
    tasks.emplace_back(
        task.view,
        task.view->x, task.view->y,
        task.targetx, task.targety,
        std::chrono::high_resolution_clock::now(),
        task.animation_finished_callback
    );
}

static float beziere_blend(float t)
{
    t = std::min(1.0f, t);
    return t * t * (3.0f - 2.0f * t);
}

int ViewAnimation::timer_callback(void *data)
{
    auto* view_animation = static_cast<ViewAnimation*>(data);

    auto current_time = std::chrono::high_resolution_clock::now();
    auto tasks_number = view_animation->tasks.size();
    for(size_t i = 0; i < tasks_number; i++)
    {
        auto task = view_animation->tasks.front();
        view_animation->tasks.pop_front();

        auto interval = std::chrono::duration_cast<std::chrono::milliseconds>(
                current_time - task.begin);
        float completeness = static_cast<float>(interval.count()) / view_animation->settings.animation_duration;

        float multiplier = beziere_blend(completeness);
        task.view->move(
            task.startx - multiplier * (task.startx - task.targetx),
            task.starty - multiplier * (task.starty - task.targety)
        );

        if(completeness < 0.999) { // animation incomplete;
            view_animation->tasks.push_back(task);
        } else {
            if(task.animation_finished_callback) {
                task.animation_finished_callback();
            }
        }
    }

    wl_event_source_timer_update(view_animation->event_source, view_animation->settings.ms_per_frame);
    return 0;
}

ViewAnimationInstance create_view_animation(Server* server, AnimationSettings settings)
{
    auto view_animation = std::make_unique<ViewAnimation>(ViewAnimation{server, settings});
    view_animation->event_source =
            wl_event_loop_add_timer(server->event_loop, ViewAnimation::timer_callback, view_animation.get());

    wl_event_source_timer_update(view_animation->event_source, settings.ms_per_frame);

    return view_animation;
}
