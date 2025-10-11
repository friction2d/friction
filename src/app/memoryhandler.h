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

#ifndef MEMORYHANDLER_H
#define MEMORYHANDLER_H
#include <QThread>
#include "memorychecker.h"
#include "memorydatahandler.h"

class MemoryHandler : public QObject {
    Q_OBJECT
public:
    explicit MemoryHandler(QObject * const parent = nullptr);
    ~MemoryHandler();

    void clearMemory();

    static MemoryHandler *sInstance;
    static MemoryState sMemoryState();
signals:
    void allMemoryUsed();
    void memoryFreed();

    void enteredCriticalState();
    void finishedCriticalState();
    void memoryUsed(const intMB &used);
private:
    void freeMemory(const MemoryState newState,
                    const longB &minFreeBytes);
    void memoryChecked(const intKB memKb,
                       const intKB totMemKb,
                       const intKB usedKb);

    MemoryDataHandler mDataHandler;
    MemoryState mMemoryState = NORMAL_MEMORY_STATE;
    QTimer *mTimer;
    QThread *mMemoryChekerThread;
    MemoryChecker *mMemoryChecker;
};

#endif // MEMORYHANDLER_H
