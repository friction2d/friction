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

#ifndef FILEHANDLEROBJREF_H
#define FILEHANDLEROBJREF_H
#include "filecachehandler.h"

class CORE_EXPORT FileHandlerObjRefBase : public QObject {
protected:
    FileHandlerObjRefBase() {}
    void increment(FileCacheHandler* const handler) const;
    void decrement(FileCacheHandler* const handler) const;
};

template <class T>
class CORE_EXPORT FileHandlerObjRef : private FileHandlerObjRefBase {
public:
    using Connector = std::function<void(ConnContext& conn, T* obj)>;
    using Getter = std::function<T*(const QString& path)>;
    using AfterAssigned = std::function<void(T* obj)>;

    FileHandlerObjRef(eBoxOrSound* const parent,
                      const Getter& getter,
                      const AfterAssigned& afterAssigned = nullptr,
                      const Connector& connector = nullptr) :
        mParent(parent),
        mGetter(getter),
        mAfterAssigned(afterAssigned),
        mConnector(connector) {
        const auto qThis = static_cast<QObject*>(this);
        connect(parent, &eBoxOrSound::parentChanged,
                qThis, [this](ContainerBox* parent) {
            setHasParent(parent);
        });
    }

    ~FileHandlerObjRef()
    { if(mFileHandler && mHasParent) decrement(mFileHandler); }

    void assign(const QString& path) {
        return assign(mGetter(path));
    }

    T* data() const { return mFileHandler; }
    operator T*() const { return data(); }
    T* operator->() const { return mFileHandler; }

    const QString& path() const {
        const auto obj = data();
        if(obj) return obj->path();
        else return mPath;
    }
private:
    void setHasParent(const bool parent) {
        if(mHasParent == parent) return;
        if(data()) {
            if(parent) increment(data());
            else       decrement(data());
        }
        mHasParent = parent;
    }

    void connectParent(ConnContext &conn) {
        if(!data()) return;
        eBoxOrSound* const obj = mParent;
        conn << connect(data(), &FileCacheHandler::deleteApproved,
                        this, [this, obj]() {
            {
                UndoRedo ur;
                const auto path = this->path();
                ur.fUndo = [this, path]() { assign(path); };
                ur.fRedo = [this]() {
                    const auto fileHandler = data();
                    if(fileHandler && fileHandler->refCount() == 1) {
                        assign(nullptr);
                        fileHandler->deleteAction();
                    }
                };
                obj->prp_addUndoRedo(ur);
            }
            obj->removeFromParent_k();
        });
    }

    void assign(T* const obj) {
        const auto oldTarget = data();
        if(mHasParent && oldTarget) decrement(oldTarget);
        auto& conn = mFileHandler.assign(obj);
        if(obj) {
            mPath = obj->path();
            if(mHasParent) increment(obj);
            const auto qThis = static_cast<QObject*>(this);
            if(mConnector) mConnector(conn, obj);
            connectParent(conn);
            conn << connect(obj, &FileCacheHandler::deleteApproved,
                            qThis, [this]() { assign(nullptr); });
        } else mPath.clear();
        if(mAfterAssigned) mAfterAssigned(obj);
    }

    bool mHasParent = false;
    QString mPath;
    ConnContextQPtr<T> mFileHandler;
    eBoxOrSound* const mParent;
    const Getter mGetter;
    const AfterAssigned mAfterAssigned;
    const Connector mConnector;
};

#endif // FILEHANDLEROBJREF_H
