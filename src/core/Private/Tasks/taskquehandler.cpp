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

#include "taskquehandler.h"

int TaskQueHandler::countQues() const { return mQues.count(); }

bool TaskQueHandler::isEmpty() const { return mQues.isEmpty(); }

void TaskQueHandler::clear() {
    mQues.clear();
    mCurrentQue = nullptr;
    mTaskCount = 0;
}

stdsptr<eTask> TaskQueHandler::takeQuedForGpuProcessing() {
    int queId = 0;
    for(const auto& que : qAsConst(mQues)) {
        const auto task = que->takeQuedForGpuProcessing();
        if(task) {
            if(que->allDone()) queDone(que.get(), queId);
            mTaskCount--;
            return task;
        }
        queId++;
    }
    return nullptr;
}

stdsptr<eTask> TaskQueHandler::takeQuedForCpuProcessing() {
    int queId = 0;
    for(const auto& que : qAsConst(mQues)) {
        const auto task = que->takeQuedForCpuProcessing();
        if(task) {
            if(que->allDone()) queDone(que.get(), queId);
            mTaskCount--;
            return task;
        }
        queId++;
    }
    return nullptr;
}

void TaskQueHandler::beginQue() {
    if(mCurrentQue) RuntimeThrow("Previous list not ended");
    mQues << std::make_shared<TaskQue>();
    mCurrentQue = mQues.last().get();
}

void TaskQueHandler::addTask(const stdsptr<eTask> &task) {
    if(mCurrentQue) {
        mCurrentQue->addTask(task);
        mTaskCount++;
    } else {
        if(mQues.isEmpty()) {
            beginQue();
            addTask(task);
            endQue();
        } else {
            const auto que = mQues.first().get();
            que->addTask(task);
            mTaskCount++;
        }
    }
}

void TaskQueHandler::endQue() {
    if(!mCurrentQue) return;
    const int count = mCurrentQue->countQued();
    if(count == 0) mQues.removeLast();
    mCurrentQue = nullptr;
}

void TaskQueHandler::queDone(const TaskQue * const que, const int queId) {
    if(que == mCurrentQue) return;
    mQues.removeAt(queId);
}
