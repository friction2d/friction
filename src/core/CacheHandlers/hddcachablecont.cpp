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

#include "hddcachablecont.h"

HddCachableCont::HddCachableCont() {}

HddCachableCont::~HddCachableCont() {
    if(mTmpFile) scheduleDeleteTmpFile();
}

int HddCachableCont::free_RAM_k() {
    const int bytes = clearMemory();
    setDataInMemory(false);
    if(!mTmpFile && !mTmpSaveTask) noDataLeft_k();
    return bytes;
}

eTask *HddCachableCont::scheduleDeleteTmpFile() {
    if(!mTmpFile) return nullptr;
    const auto updatable = enve::make_shared<TmpDeleter>(mTmpFile);
    mTmpFile.reset();
    updatable->queTask();
    return updatable.get();
}

eTask *HddCachableCont::scheduleSaveToTmpFile() {
    if(mTmpSaveTask || mTmpFile) return nullptr;
    mTmpSaveTask = createTmpFileDataSaver();
    if(!mTmpSaveTask) {
        tmpFileSaveFailed();
        return nullptr;
    }
    mTmpSaveTask->queTask();
    return mTmpSaveTask.get();
}

eTask *HddCachableCont::scheduleLoadFromTmpFile() {
    if(storesDataInMemory()) return nullptr;
    if(mTmpLoadTask) return mTmpLoadTask.get();
    if(!mTmpSaveTask && !mTmpFile) return nullptr;

    mTmpLoadTask = createTmpFileDataLoader();
    if(!mTmpLoadTask) {
        tmpFileLoadFailed();
        return nullptr;
    }
    if(mTmpSaveTask)
        mTmpSaveTask->addDependent(mTmpLoadTask.get());
    mTmpLoadTask->queTask();
    return mTmpLoadTask.get();
}

void HddCachableCont::setDataSavedToTmpFile(const qsptr<QTemporaryFile> &tmpFile) {
    mTmpSaveTask.reset();
    mTmpFile = tmpFile;
}

void HddCachableCont::tmpFileSaveFailed() {
    mTmpSaveTask.reset();
    if(!mDataInMemory && !mTmpFile) noDataLeft_k();
}

void HddCachableCont::tmpFileLoadFailed() {
    mTmpLoadTask.reset();
    if(!mDataInMemory && !mTmpFile && !mTmpSaveTask) noDataLeft_k();
}

void HddCachableCont::afterDataLoadedFromTmpFile() {
    setDataInMemory(true);
    mTmpLoadTask.reset();
    if(!inUse()) addToMemoryManagment();
}

void HddCachableCont::afterDataReplaced() {
    setDataInMemory(true);
    updateInMemoryManagment();
    if(mTmpFile) scheduleDeleteTmpFile();
}

void HddCachableCont::setDataInMemory(const bool dataInMemory) {
    mDataInMemory = dataInMemory;
}
