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

#ifndef TASKSCHEDULER_H
#define TASKSCHEDULER_H

#include <QObject>

#include "Tasks/etask.h"
#include "taskquehandler.h"
#include "Private/esettings.h"

class Scene;
class CpuExecController;
class HddExecController;
class GpuExecController;
class ComplexTask;

class CORE_EXPORT TaskScheduler : public QObject {
    Q_OBJECT
    using Func = std::function<void()>;
public:
    TaskScheduler();
    ~TaskScheduler();

    static TaskScheduler * instance() { return sInstance; }

    static void sSetTaskUnderflowFunc(const Func& func);
    static void sSetAllTasksFinishedFunc(const Func& func);
    static void sClearAllFinishedFuncs();

    static bool sAllTasksFinished();
    static bool sAllQuedCpuTasksFinished();

    static void sClearTasks();

    void initializeGpu();

    void queTasks();
    void queHddTask(const stdsptr<eTask>& task);
    void queCpuTask(const stdsptr<eTask> &task);

    void clearTasks();

    void afterHddTaskFinished(const stdsptr<eTask>& finishedTask);
    void afterCpuGpuTaskFinished(const stdsptr<eTask>& task);

    void setTaskUnderflowFunc(const Func& func);
    void setAllTasksFinishedFunc(const Func& func);

    bool allQuedTasksFinished() const;

    bool allQuedGpuTasksFinished() const;
    bool allQuedCpuTasksFinished() const;
    bool allQuedHddTasksFinished() const;

    bool cpuTasksBeingProcessed() const;
    bool hddTaskBeingProcessed() const;

    int busyHddThreads() const;
    int busyCpuThreads() const;

    int availableCpuThreads() const;

    void setAlwaysQue(const bool alwaysQue);

    void addComplexTask(const qsptr<ComplexTask>& task);

    void enterCriticalMemoryState();
    void finishCriticalMemoryState();

    void waitTillFinished();
signals:
    void finishedAllQuedTasks() const;
    void hddUsageChanged(bool);
    void gpuUsageChanged(bool);
    void cpuUsageChanged(int);
    void complexTaskAdded(ComplexTask*);
private:
    void queScheduledCpuTasks();

    void processNextQuedHddTask();
    void processNextQuedCpuTask();
    bool processNextQuedGpuTask();
    void processNextTasks();

    bool shouldQueMoreCpuTasks() const;
    bool shouldQueMoreHddTasks() const;
    bool overflowed() const;

    void callAllTasksFinishedFunc() const;

    static TaskScheduler* sInstance;

    bool mCriticalMemoryState = false;

    bool mAlwaysQue = false;
    bool mCpuQueing = false;

    QList<qsptr<ComplexTask>> mComplexTasks;

    TaskQueHandler mQuedCGTasks;
    QList<stdsptr<eTask>> mQuedHddTasks;

    QList<stdsptr<CpuExecController>> mCpuExecs;
    stdsptr<GpuExecController> mGpuExec;
    stdsptr<HddExecController> mHddExec;

    Func mTaskUnderflowFunc;
    Func mAllTasksFinishedFunc;
};

#endif // TASKSCHEDULER_H
