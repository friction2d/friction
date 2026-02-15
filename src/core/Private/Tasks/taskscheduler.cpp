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

#include "taskscheduler.h"

#include "canvas.h"
#include "execcontroller.h"
#include "gputaskexecutor.h"
#include "taskexecutor.h"
#include "complextask.h"
#include "Private/document.h"
#include "Boxes/boxrenderdata.h"

TaskScheduler *TaskScheduler::sInstance = nullptr;

TaskScheduler::TaskScheduler() {
    Q_ASSERT(!sInstance);
    sInstance = this;
    qRegisterMetaType<stdsptr<eTask>>();
    const int numberThreads = qMax(1, QThread::idealThreadCount());
    for(int i = 0; i < numberThreads; i++) {
        const auto taskExecutor = std::make_shared<CpuExecController>(this);
        connect(taskExecutor.get(), &ExecController::finishedTaskSignal,
                this, &TaskScheduler::afterCpuGpuTaskFinished);

        mCpuExecs << taskExecutor;
    }

    mHddExec = std::make_shared<HddExecController>(this);
    connect(mHddExec.get(), &ExecController::finishedTaskSignal,
            this, &TaskScheduler::afterHddTaskFinished);

    mGpuExec = std::make_shared<GpuExecController>(this);
    connect(mGpuExec.get(), &ExecController::finishedTaskSignal,
            this, &TaskScheduler::afterCpuGpuTaskFinished);
}

TaskScheduler::~TaskScheduler() {
    for(const auto& exec : qAsConst(mCpuExecs)) {
        exec->stopAndWait();
    }
    mHddExec->stopAndWait();
    mGpuExec->stopAndWait();
}

void TaskScheduler::sSetTaskUnderflowFunc(const Func& func) {
    sInstance->setTaskUnderflowFunc(func);
}

void TaskScheduler::sSetAllTasksFinishedFunc(const Func& func) {
    sInstance->setAllTasksFinishedFunc(func);
}

void TaskScheduler::sClearAllFinishedFuncs() {
    sSetTaskUnderflowFunc(nullptr);
    sSetAllTasksFinishedFunc(nullptr);
}

bool TaskScheduler::sAllTasksFinished() {
    return sInstance->allQuedTasksFinished();
}

bool TaskScheduler::sAllQuedCpuTasksFinished() {
    return sInstance->allQuedCpuTasksFinished();
}

void TaskScheduler::sClearTasks() {
    sInstance->clearTasks();
}

void TaskScheduler::initializeGpu() {
    try {
        mGpuExec->initialize();
    } catch(...) {
        RuntimeThrow("Failed to initialize GPU execution controller.");
    }
}

void TaskScheduler::queHddTask(const stdsptr<eTask>& task) {
    mQuedHddTasks << task;
    processNextQuedHddTask();
}

void TaskScheduler::queCpuTask(const stdsptr<eTask>& task) {
    mQuedCGTasks.addTask(task);
    if(task->readyToBeProcessed()) {
        if(task->hardwareSupport() == HardwareSupport::cpuOnly ||
           !processNextQuedGpuTask()) {
            processNextQuedCpuTask();
        }
    }
}

void TaskScheduler::clearTasks() {
    mQuedCGTasks.clear();

    for(const auto& hddTask : qAsConst(mQuedHddTasks))
        hddTask->cancel();
    mQuedHddTasks.clear();

    callAllTasksFinishedFunc();
}

bool TaskScheduler::overflowed() const {
    const int nQues = mQuedCGTasks.countQues();
    const int maxQues = mAlwaysQue ? mCpuExecs.count() : 1;
    return nQues >= maxQues;
}

void TaskScheduler::callAllTasksFinishedFunc() const {
    if(allQuedTasksFinished()) {
        if(mAllTasksFinishedFunc) mAllTasksFinishedFunc();
        emit finishedAllQuedTasks();
    }
}

bool TaskScheduler::shouldQueMoreCpuTasks() const {
    return !mCpuQueing && !overflowed() &&
            availableCpuThreads() > 0 &&
            (mAlwaysQue || GpuTaskExecutor::sUsageCount() == 0);
}

bool TaskScheduler::shouldQueMoreHddTasks() const {
    return !mCpuQueing && !overflowed() &&
            mQuedHddTasks.count() + HddTaskExecutor::sWaitingTasks() < 2;
}

void TaskScheduler::queTasks() {
    queScheduledCpuTasks();
    processNextQuedHddTask();
}

void TaskScheduler::queScheduledCpuTasks() {
    if(!mAlwaysQue && !shouldQueMoreCpuTasks()) return;
    mCpuQueing = true;
    mQuedCGTasks.beginQue();
    for(const auto& it : Document::sInstance->fVisibleScenes) {
        const auto scene = it.first;
        scene->queTasks();
    }
    mQuedCGTasks.endQue();
    mCpuQueing = false;

    if(!mQuedCGTasks.isEmpty()) processNextTasks();
}

void TaskScheduler::afterHddTaskFinished(const stdsptr<eTask>& finishedTask) {
    TaskExecutor::sTaskFinishSignals--;
    finishedTask->finishedProcessing();
    processNextTasks();
    if(!hddTaskBeingProcessed()) queTasks();
    callAllTasksFinishedFunc();
}

void TaskScheduler::processNextQuedHddTask() {
    bool finished = false;
    QList<stdsptr<eTask>> tasks;
    for(int i = 0; i < mQuedHddTasks.count(); i++) {
        const auto task = mQuedHddTasks.at(i);
        if(!task->readyToBeProcessed()) continue;
        task->aboutToProcess(Hardware::hdd);
        if(task->getState() > eTaskState::processing)
            finished = true;
        mQuedHddTasks.removeAt(i--);
        tasks << task;
    }
    if(!tasks.isEmpty()) HddTaskExecutor::sAddTasks(tasks);
    if(finished) processNextTasks();

    emit hddUsageChanged(busyHddThreads());
}

void TaskScheduler::processNextTasks() {
    if(mCriticalMemoryState) return;
    processNextQuedHddTask();
    processNextQuedGpuTask();
    processNextQuedCpuTask();
    if(mTaskUnderflowFunc) {
        if(shouldQueMoreCpuTasks() || shouldQueMoreHddTasks()) {
            mTaskUnderflowFunc();
        }
    }
}

bool TaskScheduler::processNextQuedGpuTask() {
    bool finished = false;
    QList<stdsptr<eTask>> tasks;
    const int count = 3 - GpuTaskExecutor::sWaitingTasks();
    for(int i = 0; i < count; i++) {
        const auto task = mQuedCGTasks.takeQuedForGpuProcessing();
        if(!task) break;
        task->aboutToProcess(Hardware::gpu);
        if(task->getState() > eTaskState::processing) {
            finished = true;
            i--; continue;
        }
        tasks << task;
    }
    if(!tasks.isEmpty()) GpuTaskExecutor::sAddTasks(tasks);
    if(finished) processNextTasks();

    emit gpuUsageChanged(GpuTaskExecutor::sUsageCount() > 0);
    return !tasks.isEmpty();
}

void TaskScheduler::afterCpuGpuTaskFinished(const stdsptr<eTask>& task) {
    TaskExecutor::sTaskFinishSignals--;
    task->finishedProcessing();
    processNextTasks();
    if(!cpuTasksBeingProcessed()) queTasks();
    callAllTasksFinishedFunc();
}

void TaskScheduler::setTaskUnderflowFunc(const Func& func) {
    mTaskUnderflowFunc = func;
}

void TaskScheduler::setAllTasksFinishedFunc(const Func& func) {
    mAllTasksFinishedFunc = func;
}

bool TaskScheduler::allQuedTasksFinished() const {
    return allQuedCpuTasksFinished() &&
           allQuedHddTasksFinished() &&
           allQuedGpuTasksFinished() &&
           TaskExecutor::sTaskFinishSignals == 0;
}

bool TaskScheduler::allQuedGpuTasksFinished() const {
    return mQuedCGTasks.isEmpty() && GpuTaskExecutor::sUsageCount() == 0;
}

bool TaskScheduler::allQuedCpuTasksFinished() const {
    return mQuedCGTasks.isEmpty() && !cpuTasksBeingProcessed();
}

bool TaskScheduler::allQuedHddTasksFinished() const {
    return mQuedHddTasks.isEmpty() && !hddTaskBeingProcessed();
}

bool TaskScheduler::cpuTasksBeingProcessed() const {
    return busyCpuThreads() > 0;
}

bool TaskScheduler::hddTaskBeingProcessed() const {
    return busyHddThreads() > 0;
}

int TaskScheduler::busyHddThreads() const {
    return HddTaskExecutor::sUsageCount();
}

int TaskScheduler::busyCpuThreads() const {
    return CpuTaskExecutor::sUsageCount();
}

int TaskScheduler::availableCpuThreads() const {
    const int cap = eSettings::sInstance->fCpuThreadsCap;
    const int free = mCpuExecs.count() - busyCpuThreads();
    if(cap > 0) return qMin(free, cap);
    return free;
}

void TaskScheduler::setAlwaysQue(const bool alwaysQue) {
    mAlwaysQue = alwaysQue;
}

void TaskScheduler::addComplexTask(const qsptr<ComplexTask> &task) {
    if(task->done()) return;
    mComplexTasks << task;
    const QWeakPointer<ComplexTask> taskPtr = task;
    const auto deleter = [this, taskPtr]() {
        mComplexTasks.removeOne(taskPtr);
    };
    emit complexTaskAdded(task.data());
    connect(task.data(), &ComplexTask::canceled, this, deleter);
    connect(task.data(), &ComplexTask::finishedAll, this, deleter);
}

void TaskScheduler::enterCriticalMemoryState() {
    if(mCriticalMemoryState) return;
    mCriticalMemoryState = true;
}

void TaskScheduler::finishCriticalMemoryState() {
    if(!mCriticalMemoryState) return;
    mCriticalMemoryState = false;
    queTasks();
    processNextTasks();
}

void TaskScheduler::waitTillFinished() {
    if(allQuedTasksFinished()) return;
    QEventLoop loop;
    QObject::connect(this, &TaskScheduler::finishedAllQuedTasks,
                     &loop, &QEventLoop::quit, Qt::QueuedConnection);
    loop.exec();
}

void TaskScheduler::processNextQuedCpuTask() {
    bool finished = false;
    QList<stdsptr<eTask>> tasks;
    const int count = 3*mCpuExecs.count() - CpuTaskExecutor::sWaitingTasks();
    for(int i = 0; i < count; i++) {
        const auto task = mQuedCGTasks.takeQuedForCpuProcessing();
        if(!task) break;
        task->aboutToProcess(Hardware::cpu);
        if(task->getState() > eTaskState::processing) {
            finished = true;
            i--; continue;
        }
        tasks << task;
    }
    if(!tasks.isEmpty()) CpuTaskExecutor::sAddTasks(tasks);
    if(finished) processNextTasks();
    emit cpuUsageChanged(busyCpuThreads());
}
