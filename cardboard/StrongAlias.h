// SPDX-License-Identifier: GPL-3.0-only
/*
Copyright (C) 2020 Alexandru-Iulian Magan, Tudor-Ioan Roman, and contributors.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef CARDBOARD_STRONG_ALIAS_H_INCLUDED
#define CARDBOARD_STRONG_ALIAS_H_INCLUDED

#include <utility>

template <typename T, typename Tag>
class StrongAlias {
public:
    explicit StrongAlias(const T& value)
        : m_value { value } {};
    explicit StrongAlias(T&& value)
        : m_value { std::move(value) } {};
    explicit StrongAlias()
        : m_value {}
    {
    }

    T& get() { return m_value; }
    const T& get() const { return m_value; }

    explicit operator T() { return m_value; }
    explicit operator T() const { return m_value; }

    auto& operator=(const T& t)
    {
        m_value = t;

        return *this;
    }

    auto& operator=(T&& t)
    {
        m_value = std::move(t);

        return *this;
    }

private:
    T m_value;
};

#endif //CARDBOARD_STRONG_ALIAS_H_INCLUDED
