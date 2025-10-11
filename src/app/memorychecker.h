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

#ifndef MEMORYCHECKER_H
#define MEMORYCHECKER_H

#include <QObject>
#include <QTimer>
#include "Private/memorystructs.h"

enum MemoryState {
    NORMAL_MEMORY_STATE,
    LOW_MEMORY_STATE = 5,
    VERY_LOW_MEMORY_STATE = 15,
    CRITICAL_MEMORY_STATE = 30
};

class MemoryChecker : public QObject {
    Q_OBJECT
public:
    explicit MemoryChecker(QObject * const parent = nullptr);
    static MemoryChecker *getInstance() { return mInstance; }

    void checkMemory();
private:
    void sGetFreeKB(intKB& procFreeKB,
                    intKB& sysFreeKB,
                    intKB& usedKB);
    static char sLine[256];

    MemoryState mLastMemoryState = NORMAL_MEMORY_STATE;

    intKB mLowFreeKB = intKB(0);
    intKB mVeryLowFreeKB = intKB(0);
    intKB mCriticalFreeKB = intKB(0);

    static MemoryChecker *mInstance;
signals:
    void memoryCheckedKB(intKB, intKB, intKB);
    void handleMemoryState(MemoryState,
                           longB bytesToFree);
};

#endif // MEMORYCHECKER_H
