#ifndef UTIL_UPDATEHANDLER_H
#define UTIL_UPDATEHANDLER_H

template <typename T>
struct UpdateHandler
{
    UpdateHandler(T* t, std::function<void(T*)> update_) : data{t}, update {update} {}

    ~UpdateHandler()
    {
        update(data);
    }

    T* operator->()
    {
        return data;
    }
private:
    T* data;
    std::function<void()> update;
};

#endif //UTIL_UPDATEHANDLER_H
