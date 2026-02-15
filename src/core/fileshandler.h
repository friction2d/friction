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

#ifndef FILESHANDLER_H
#define FILESHANDLER_H
#include <QObject>
#include "smartPointers/ememory.h"
#include "FileCacheHandlers/filecachehandler.h"

class CORE_EXPORT FilesHandler : public QObject {
    Q_OBJECT
public:
    FilesHandler();

    void clear();

    template <typename T>
    T *getFileHandler(const QString &filePath);
    bool removeFileHandler(const qsptr<FileCacheHandler> &fh);

    static FilesHandler* sInstance;
private:    
    QList<qsptr<FileCacheHandler>> mFileHandlers;
signals:
    void addedCacheHandler(FileCacheHandler*);
    void removedCacheHandler(FileCacheHandler*);
};

template <typename T>
T *FilesHandler::getFileHandler(const QString &filePath) {
    for(const auto& fh : qAsConst(mFileHandlers)) {
        if(filePath == fh->path()) return static_cast<T*>(fh.get());
    }
    const auto fh = enve::make_shared<T>();
    try {
        fh->setPath(filePath);
    } catch(const std::exception& e) {
        gPrintExceptionCritical(e);
        return nullptr;
    }
    mFileHandlers.append(fh);
    connect(fh.get(), &FileCacheHandler::deleteApproved,
            this, &FilesHandler::removeFileHandler);
    emit addedCacheHandler(fh.get());

    return fh.get();
}


#endif // FILESHANDLER_H
