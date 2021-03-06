// SPDX-License-Identifier: GPL-3.0-only
/*
Copyright (C) 2020 Alexandru-Iulian Magan, Tudor-Ioan Roman, and contributors.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef CARDBOARD_NOT_NULL_H_INCLUDED
#define CARDBOARD_NOT_NULL_H_INCLUDED

#include <cassert>
#include <type_traits>
#include <unordered_set>
#include <utility>

// Adapted from GSL::not_null from Microsoft's GSL library, MIT licensed
// https://github.com/microsoft/GSL/blob/0843ea444fcdf07868214da171c4fa9d244e7472/include/gsl/pointers
// https://github.com/microsoft/GSL/blob/0843ea444fcdf07868214da171c4fa9d244e7472/LICENSE
template <class Type>
class NotNullPointer {
    using pointer_type = Type*;

public:
    static_assert(std::is_assignable<pointer_type&, std::nullptr_t>::value, "Type cannot be assigned nullptr.");

    template <typename U, typename = std::enable_if_t<std::is_convertible<U, pointer_type>::value>>
    constexpr NotNullPointer(U&& u)
        : ptr_(std::forward<U>(u))
    {
        assert(ptr_ != nullptr);
    }

    template <typename = std::enable_if_t<!std::is_same<std::nullptr_t, pointer_type>::value>>
    constexpr NotNullPointer(pointer_type u)
        : ptr_(u)
    {
        assert(ptr_ != nullptr);
    }

    template <typename U, typename = std::enable_if_t<std::is_convertible<U, pointer_type>::value>>
    constexpr NotNullPointer(const NotNullPointer<U>& other)
        : NotNullPointer(other.get())
    {
    }

    NotNullPointer(const NotNullPointer& other) = default;
    NotNullPointer& operator=(const NotNullPointer& other) = default;

    constexpr pointer_type get() const
    {
        assert(ptr_ != nullptr);
        return ptr_;
    }

    constexpr operator pointer_type() const { return get(); }
    constexpr pointer_type operator->() const { return get(); }
    constexpr decltype(auto) operator*() const { return *get(); }

    // prevents compilation when someone attempts to assign a null pointer constant
    NotNullPointer(std::nullptr_t) = delete;
    NotNullPointer& operator=(std::nullptr_t) = delete;

    // unwanted operators...pointers only point to single objects!
    NotNullPointer& operator++() = delete;
    NotNullPointer& operator--() = delete;
    NotNullPointer operator++(int) = delete;
    NotNullPointer operator--(int) = delete;
    NotNullPointer& operator+=(std::ptrdiff_t) = delete;
    NotNullPointer& operator-=(std::ptrdiff_t) = delete;
    void operator[](std::ptrdiff_t) const = delete;

private:
    pointer_type ptr_;
};

namespace std {
template <class Type>
struct hash<NotNullPointer<Type>> {
    std::size_t operator()(NotNullPointer<Type> const& ptr) const noexcept
    {
        return std::hash<Type*> {}(ptr.get());
    }
};
}

#endif //CARDBOARD_NOT_NULL_H_INCLUDED
