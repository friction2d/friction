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

#ifndef FILEDATACACHEHANDLER_H
#define FILEDATACACHEHANDLER_H
#include "smartPointers/selfref.h"
#include "smartPointers/ememory.h"

class CORE_EXPORT FileDataCacheHandler : public SelfRef {
    Q_OBJECT
protected:
    FileDataCacheHandler();
    virtual void afterSourceChanged() = 0;
public:
    ~FileDataCacheHandler();
    virtual void clearCache() = 0;

    void reload() {
        clearCache();
        mFileMissing = !QFile(mFilePath).exists();
        afterSourceChanged();
        emit sourceChanged();
    }

    void setFilePath(const QString &path);

    const QString &getFilePath() const {
        return mFilePath;
    }

    bool isFileMissing() { return mFileMissing; }

    template<typename T>
    static T *sGetDataHandler(const QString &filePath);
    template<typename T>
    static qsptr<T> sGetCreateDataHandler(const QString &filePath);
    template<typename T>
    static qsptr<T> sCreateDataHandler(const QString &filePath);
signals:
    void sourceChanged();
protected:
    bool mFileMissing = false;
    QString mFilePath;
private:
    static QList<FileDataCacheHandler*> sDataHandlers;
};

template<typename T>
T *FileDataCacheHandler::sGetDataHandler(const QString &filePath) {
    for(const auto &handler : qAsConst(sDataHandlers)) {
        if(handler->getFilePath() == filePath) {
            const auto handlerT = enve_cast<T*>(handler);
            if(handlerT) return handlerT;
        }
    }
    return nullptr;
}

template<typename T>
qsptr<T> FileDataCacheHandler::sCreateDataHandler(const QString &filePath) {
    const auto handler = enve::make_shared<T>();
    try {
        handler->setFilePath(filePath);
    } catch(const std::exception& e) {
        gPrintExceptionCritical(e);
        return nullptr;
    }
    return handler;
}

template<typename T>
qsptr<T> FileDataCacheHandler::sGetCreateDataHandler(const QString &filePath) {
    const auto get = sGetDataHandler<T>(filePath);
    if(get) return get->template ref<T>();
    return sCreateDataHandler<T>(filePath);
}

#endif // FILEDATACACHEHANDLER_H
