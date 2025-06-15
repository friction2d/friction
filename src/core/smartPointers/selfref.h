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

#ifndef SELFREF_H
#define SELFREF_H

#include <QPointer>
#include <QSharedPointer>
#include <QObject>

#include "../exceptions.h"
#include "eobject.h"


template <class T> using qsptr = QSharedPointer<T>;
template <class T> using qptr = QPointer<T>;

// Basically everywhere you see enve::make_shared requires this object or StdSelfRef
// The difference being that SelfRef is a QObject (has signals, etc...) while StdSelfRef is more pure.
class CORE_EXPORT SelfRef : public QObject {
    e_PROHIBIT_HEAP
public:
    template <class T, typename... Args>
    static inline QSharedPointer<T> sCreate(Args && ...args) {
        return (new T(std::forward<Args>(args)...))->template iniRef<T>();
    }

    template<class T>
    inline QSharedPointer<T> ref() {
        if(mThisWeak.isNull()) RuntimeThrow("Not initialized, or already deleting");
        return qSharedPointerCast<T>(mThisWeak);
    }
private:
    template<class T>
    QSharedPointer<T> iniRef() {
        Q_ASSERT_X(mThisWeak.isNull(), "SelfRef::iniRef", "reinitialization");
        QSharedPointer<T> thisRef = QSharedPointer<T>(static_cast<T*>(this));
        this->mThisWeak = qSharedPointerCast<SelfRef>(thisRef).toWeakRef();
        return thisRef;
    }

    template<class T>
    inline QWeakPointer<T> weakRef() {
        if(mThisWeak.isNull()) return QWeakPointer<T>();
        return qWeakPointerCast<T>(mThisWeak);
    }
private:
    QWeakPointer<SelfRef> mThisWeak;
};

#endif // SELFREF_H
