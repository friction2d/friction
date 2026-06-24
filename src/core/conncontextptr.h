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

#ifndef CONNCONTEXTPTR_H
#define CONNCONTEXTPTR_H

#include "conncontext.h"

template <typename T>
class ConnContextPtr {
public:
    bool operator==(T* const t) const { return mPtr == t;}
    T* operator->() const { return mPtr; }
    operator T*() const { return mPtr; }
    operator bool() const { return mPtr; }
    T* operator*() const { return mPtr; }
    ConnContext& operator<<(const QMetaObject::Connection& connection)
    { return mConnContext << connection; }

    ConnContext& assign(T* const ptr) {
        mPtr = ptr;
        mConnContext.clear();
        return mConnContext;
    }
private:
    ConnContext mConnContext;
    T *mPtr = nullptr;
};

template <typename T>
class ConnContextQPtr {
public:
    bool operator==(T* const t) const { return mPtr == t; }
    bool operator==(const QPointer<T>& t) const { return mPtr == t; }

    T* operator->() const { return mPtr; }
    operator T*() const { return mPtr; }
    operator bool() const { return mPtr; }
    T* operator*() const { return mPtr; }
    ConnContext& operator<<(const QMetaObject::Connection& connection)
    { return mConnContext << connection; }

    T* data() const { return mPtr.data(); }
    T* get() const { return mPtr.data(); }

    ConnContext& assign(T* const ptr) {
        mPtr = ptr;
        mConnContext.clear();
        return mConnContext;
    }
private:
    ConnContext mConnContext;
    QPointer<T> mPtr;
};

template <typename T>
class ConnContextQSPtr {
public:
    bool operator==(T* const t) const { return mPtr == t;}
    bool operator==(const QSharedPointer<T>& t) const { return mPtr == t; }

    T* operator->() const { return mPtr.get(); }
    operator T*() const { return mPtr.get(); }
    operator bool() const { return !mPtr.isNull(); }
    T* operator*() const { return mPtr.get(); }
    ConnContext& operator<<(const QMetaObject::Connection& connection)
    { return mConnContext << connection; }

    T* data() const { return mPtr.data(); }
    T* get() const { return mPtr.data(); }

    ConnContext& assign(const QSharedPointer<T>& ptr) {
        mPtr = ptr;
        mConnContext.clear();
        return mConnContext;
    }

    const QSharedPointer<T>& sptr() const { return mPtr; }
private:
    ConnContext mConnContext;
    QSharedPointer<T> mPtr;
};

#endif // CONNCONTEXTPTR_H
