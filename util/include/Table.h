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

    template<typename C1, typename C2, typename... Cs>
    std::tuple<std::reference_wrapper<C1>, std::reference_wrapper<C2>, std::reference_wrapper<Cs>...> query(Id id)
    {
        return std::make_tuple(
            std::ref(std::get<C1>(components.at(id))),
            std::ref(std::get<C2>(components.at(id))),
            std::ref(std::get<Cs>(components.at(id)))...
        );
    }

    template<typename C1, typename C2, typename... Cs>
    std::tuple<std::reference_wrapper<C1>, std::reference_wrapper<C2>, std::reference_wrapper<Cs>...> query(Id id) const
    {
        return std::make_tuple(
            std::cref(std::get<C1>(components.at(id))),
            std::cref(std::get<C2>(components.at(id))),
            std::cref(std::get<Cs>(components.at(id)))...
        );
    }

};

#endif //UTIL_TABLE_H
