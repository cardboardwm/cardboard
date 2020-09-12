#ifndef UTIL_LISTENER_H
#define UTIL_LISTENER_H

#include <variant>
#include <unordered_map>

struct NoneT {};

using ListenerData = std::variant<
        NoneT,
        std::int32_t, // type erased ID
        void*         // type erased pointer
    >;

struct Listener
{
    using notify_f = void(*)(wl_listener*, void* server, ListenerData, void* data);

    wl_listener listener {};
    void* server {};
    ListenerData listener_data {};
    notify_f notify {};

    Listener() = default;

    Listener(notify_f notify_, void* server, ListenerData listener_data)
        : listener { {}, [](wl_listener* listener_, void* data) {
                            Listener* l = wl_container_of(listener_, l, listener);
                            l->notify(listener_, l->server, l->listener_data, data);
                    } }
        , server { server }
        , listener_data { listener_data }
        , notify { notify_ }
    {
    }

    Listener(Listener&&) = default;
    Listener(const Listener&) = default;
};

using ListenerId = TypeId<Listener>;

class ListenerList
{
public:
    ~ListenerList()
    {
        for (auto& listener : listeners)
            wl_list_remove(&listener.second.listener.link);
    }

    ListenerId add_listener(wl_signal* signal, Listener listener)
    {
        ListenerId id = current_id;
        current_id.id++;

        listeners.emplace(id, listener);

        return id;
    }

    void remove_listener(ListenerId id)
    {
        wl_list_remove(&listeners[id].listener.link);
        listeners.erase(id);
    }

private:
    std::unordered_map<ListenerId, Listener> listeners;
    ListenerId current_id {0};
};



#endif //UTIL_LISTENER_H
