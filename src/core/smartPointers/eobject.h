/*
#
# Friction - https://friction.graphics
#
# Copyright (c) Ole-André Rodlie and contributors
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# See 'README.md' for more information.
#
*/

// Fork of enve - Copyright (C) 2016-2020 Maurycy Liebner

#ifndef EOBJECT_H
#define EOBJECT_H

#include "../core_global.h"

#define e_PROHIBIT_HEAP \
public: \
    static void* operator new(size_t, void* ptr) noexcept { return ptr; } \
    private: \
    static void *operator new (size_t sz) { \
        return std::malloc(sz); \
}

#define e_OBJECT \
    friend class SelfRef; \
    friend class StdSelfRef; \
    e_PROHIBIT_HEAP

#define e_DECLARE_TYPE_FUNCTION(className) \
    virtual className* to##className() { return nullptr; }

#define e_DECLARE_TYPE(className) \
    public: \
    template <class U> \
    static inline className* sCast(U& u) { \
        return u.to##className(); \
    } \
    className* to##className() final { return this; }

#endif // EOBJECT_H
