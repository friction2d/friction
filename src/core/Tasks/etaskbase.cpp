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

#include "etaskbase.h"

#include "etask.h"

#include "GUI/dialogsinterface.h"

void eTaskBase::finishedProcessing() {
    mState = eTaskState::finished;
    if(mCancel) {
        mCancel = false;
        cancel();
    } else if(unhandledException()) {
        const bool handled = handleException();
        if(!handled) {
            const auto ePtr = takeException();
//            try {
//                if(ePtr) std::rethrow_exception(ePtr);
//            } catch(const std::exception& e) {
//                const auto& inst = DialogsInterface::instance();
//                inst.showStatusMessage(e.what());
//            }
            gPrintExceptionCritical(ePtr);
            cancel();
        }
    } else {
        afterProcessing();
        tellDependentThatFinished();
    }
}

void eTaskBase::addDependent(eTask * const task) {
    Q_ASSERT(task != this);
    if(!task) return;
    if(mState == eTaskState::finished) {
        return;
    } else if(mState == eTaskState::canceled) {
        task->cancel();
    } else {
        if(mDependent.contains(task)) return;
        mDependent << task;
        task->incDependencies();
    }
}

void eTaskBase::addDependent(const Dependent &func) {
    if(mState == eTaskState::finished) {
        if(func.fFinished) func.fFinished();
    } else if(mState == eTaskState::canceled) {
        if(func.fCanceled) func.fCanceled();
    } else mDependentF << func;
}

void eTaskBase::cancel() {
    if(mState == eTaskState::processing) {
        mCancel = true;
        return;
    }
    mState = eTaskState::canceled;
    cancelDependent();
    afterCanceled();
}

void eTaskBase::moveDependent(eTaskBase* const to) {
    for(const auto& dependent : qAsConst(mDependent)) {
        to->mDependent << dependent;
    }
    mDependent.clear();
    for(const auto& dependent : qAsConst(mDependentF)) {
        to->mDependentF << dependent;
    }
    mDependentF.clear();
}

void eTaskBase::setException(const std::exception_ptr& exception) {
    mUpdateException = exception;
}

bool eTaskBase::unhandledException() const {
    return static_cast<bool>(mUpdateException);
}

std::exception_ptr eTaskBase::takeException() {
    std::exception_ptr exc;
    std::swap(mUpdateException, exc);
    return exc;
}

void eTaskBase::tellDependentThatFinished() {
    for(const auto& dependent : qAsConst(mDependent)) {
        if(dependent) dependent->decDependencies();
    }
    mDependent.clear();
    for(const auto& dependent : qAsConst(mDependentF)) {
        if(dependent.fFinished) dependent.fFinished();
    }
    mDependentF.clear();
}

void eTaskBase::cancelDependent() {
    for(const auto& dependent : qAsConst(mDependent)) {
        if(dependent) dependent->cancel();
    }
    mDependent.clear();
    for(const auto& dependent : qAsConst(mDependentF)) {
        if(dependent.fCanceled) dependent.fCanceled();
    }
    mDependentF.clear();
}
