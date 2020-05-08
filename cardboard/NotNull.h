#ifndef BUILD_CARDBOARD_NOTNULL_H
#define BUILD_CARDBOARD_NOTNULL_H

#include <type_traits>

template<typename Type>
class NotNullPointer
{
public:
    using pointer_type = Type*;

    NotNullPointer(Type* t):
        pointer(t)
    {
    }

    template<typename T>
    explicit NotNullPointer(T* t):
        pointer{t}
    {
        static_assert(!std::is_same_v<std::remove_cvref_t<T>, std::nullptr_t>, "Trying to assign nullptr to NotNullPointer");
        assert(t != nullptr && "Trying to assign null value to NotNullPointer");
    }

    NotNullPointer(const NotNullPointer&) noexcept = default;
    NotNullPointer(NotNullPointer&&) noexcept = default;

    NotNullPointer& operator=(const NotNullPointer&) noexcept = default;
    NotNullPointer& operator=(NotNullPointer&&) noexcept = default;

    Type* operator->()
    {
        return pointer;
    }

    explicit operator pointer_type()
    {
        return pointer;
    }

    pointer_type get() const
    {
        return pointer;
    }

private:
    pointer_type pointer;
};

#endif //BUILD_CARDBOARD_NOTNULL_H
