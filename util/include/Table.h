#ifndef UTIL_TABLE_H
#define UTIL_TABLE_H

#include <unordered_map>
#include <functional>
#include <tuple>

template <typename Id, typename... Components>
struct Table
{
    std::unordered_map<Id, std::tuple<Components...>> components;

    template<typename C>
    C& query(Id id)
    {
        return std::get<C>(components.at(id));
    }

    template<typename C>
    const C& query(Id id) const
    {
        return std::get<C>(components.at(id));
    }

    template<typename... Cs>
    std::tuple<std::reference_wrapper<Cs>...> query(Id id)
    {
        return {
            std::ref(std::get<Cs>(components.at(id)))...
        };
    }

    template<typename... Cs>
    std::tuple<std::reference_wrapper<Cs>...> query(Id id) const
    {
        return {
            std::cref(std::get<Cs>(components.at(id)))...
        };
    }

};

#endif //UTIL_TABLE_H
