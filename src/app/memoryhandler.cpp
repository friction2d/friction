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

#include "memoryhandler.h"
#include "Boxes/boxrendercontainer.h"
#include "GUI/mainwindow.h"
#include <QMetaType>

#ifdef Q_OS_MAC
#include <malloc/malloc.h>
#else
#include <malloc.h>
#endif

MemoryHandler *MemoryHandler::sInstance = nullptr;
Q_DECLARE_METATYPE(MemoryState)
Q_DECLARE_METATYPE(longB)
Q_DECLARE_METATYPE(intKB)

MemoryHandler::MemoryHandler(QObject * const parent) : QObject(parent) {
    Q_ASSERT(!sInstance);
    sInstance = this;

    mMemoryChekerThread = new QThread(this);
    mMemoryChecker = new MemoryChecker();
    mMemoryChecker->moveToThread(mMemoryChekerThread);
    qRegisterMetaType<MemoryState>();
    qRegisterMetaType<longB>();
    connect(mMemoryChecker, &MemoryChecker::handleMemoryState,
            this, &MemoryHandler::freeMemory);
    qRegisterMetaType<intKB>();
    connect(mMemoryChecker, &MemoryChecker::memoryCheckedKB,
            this, &MemoryHandler::memoryChecked);

    mTimer = new QTimer(this);
    connect(mTimer, &QTimer::timeout,
            mMemoryChecker, &MemoryChecker::checkMemory);
    mTimer->start(1000);
    mMemoryChekerThread->start();
}

MemoryHandler::~MemoryHandler() {
    mMemoryChekerThread->quit();
    mMemoryChekerThread->wait();
    sInstance = nullptr;
    delete mMemoryChecker;
}

void MemoryHandler::clearMemory() {
    freeMemory(NORMAL_MEMORY_STATE, longB(std::numeric_limits<qint64>::max()));
}

MemoryState MemoryHandler::sMemoryState() {
    return sInstance->mMemoryState;
}

void MemoryHandler::freeMemory(const MemoryState newState,
                               const longB &minFreeBytes) {
    if(newState != mMemoryState) {
        if(newState == NORMAL_MEMORY_STATE) {
            mTimer->setInterval(1000);
        } else if(newState >= VERY_LOW_MEMORY_STATE) {
            if(mMemoryState < VERY_LOW_MEMORY_STATE) {
                mTimer->setInterval(250);
            }
        } else {
            mTimer->setInterval(500);
        }
        if(mMemoryState == CRITICAL_MEMORY_STATE) {
            emit finishedCriticalState();
        } else if(newState == CRITICAL_MEMORY_STATE) {

        }
        mMemoryState = newState;
    }

    if(minFreeBytes.fValue <= 0) return;
    qint64 memToFree = minFreeBytes.fValue;
    while(memToFree > 0 && !mDataHandler.isEmpty()) {
        const auto cont = mDataHandler.takeFirst();
        memToFree -= cont->free_RAM_k();
    }
    if(newState == CRITICAL_MEMORY_STATE ||
       memToFree > 0) {
        mMemoryState = CRITICAL_MEMORY_STATE;
        emit enteredCriticalState();
        emit allMemoryUsed();
    }
    emit memoryFreed();
}

void MemoryHandler::memoryChecked(const intKB memKb,
                                  const intKB totMemKb,
                                  const intKB usedKb)
{
    Q_UNUSED(memKb)
    Q_UNUSED(totMemKb)
    emit memoryUsed(intMB(usedKb));
}
