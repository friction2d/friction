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

#include "execcontroller.h"

#include "gputaskexecutor.h"
#include <iostream>

ExecController::ExecController(TaskExecutor* const executor, QObject* const parent) :
    QObject(parent),
    mExecutor(executor),
    mThread(new QThread(this)) {
    connect(mExecutor, &TaskExecutor::finishedTask,
            this, &ExecController::finishedTask,
            Qt::QueuedConnection);
}

void ExecController::stop() {
    mExecutor->stop();
    mThread->quit();
}

void ExecController::stopAndWait() {
    stop();
    mThread->wait();
}

void ExecController::start() {
    mExecutor->moveToThread(mThread);
    mThread->start();
    QMetaObject::invokeMethod(mExecutor, &TaskExecutor::start,
                              Qt::QueuedConnection);
}

void ExecController::finishedTask(const stdsptr<eTask>& task) {
    emit finishedTaskSignal(task, this);
}

CpuExecController::CpuExecController(QObject* const parent) :
    ExecController(new CpuTaskExecutor, parent) {
    start();
}

GpuExecController::GpuExecController(QObject* const parent) :
    ExecController(new GpuTaskExecutor, parent) {
    const auto gpuExec = static_cast<GpuTaskExecutor*>(mExecutor);
    connect(mThread, &QThread::finished,
            this, [gpuExec]() {
        if(gpuExec->unhandledException())
            gPrintExceptionCritical(gpuExec->handleException());
    });
}

void GpuExecController::initialize() {
    //std::cout << "Entered GpuExecController initialize" << std::endl;

    const auto gpuExec = static_cast<GpuTaskExecutor*>(mExecutor);
    gpuExec->initialize(mThread);

    //std::cout << "GpuTaskExecutor initialized" << std::endl;
    start();
}

HddExecController::HddExecController(QObject* const parent) :
    ExecController(new HddTaskExecutor, parent) {
    start();
}
