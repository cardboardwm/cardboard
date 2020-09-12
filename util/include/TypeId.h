#ifndef UTIL_TYPEID_H
#define UTIL_TYPEID_H

#include <cstdint>
#include <functional>

struct InvalidId {

};

const std::int32_t InvalidIdNumeric = -1;

template <typename Tag>
struct TypeId
{
    std::int32_t id;

    explicit TypeId(std::int32_t id_) : id {id_} {}
    explicit TypeId(InvalidId): id{ InvalidIdNumeric } {}

    auto& operator=(const TypeId<Tag>&) = default;
    auto& operator=(TypeId<Tag>&&)  noexcept = default;

    explicit operator std::int32_t ()
    {
        return id;
    }

    bool operator==(TypeId other)
    {
        return other.id == id;
    }

    auto& operator=(const std::int32_t id_)
    {
        id = id_;
        return *this;
    }

    auto& operator=(InvalidId)
    {
        id = InvalidIdNumeric;
        return *this;
    }
};

namespace std
{
    template<typename Tag>
    struct hash<TypeId<Tag>>
    {
        std::size_t operator()(TypeId<Tag> id)
        {
            return std::hash<std::int32_t>{}(id);
        }
    };
}

#endif //UTIL_TYPEID_H
