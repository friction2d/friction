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

#include "gputaskexecutor.h"

#include <iostream>

QAtomicList<stdsptr<eTask>> GpuTaskExecutor::sTasks;
QAtomicInt GpuTaskExecutor::sUseCount = 0;

GpuTaskExecutor::GpuTaskExecutor() :
    TaskExecutor(sUseCount, sTasks) {}

void GpuTaskExecutor::sAddTask(const stdsptr<eTask>& ready) {
    sTasks.appendAndNotifyAll(ready);
}

void GpuTaskExecutor::sAddTasks(const QList<stdsptr<eTask>>& ready) {
    sTasks.appendAndNotifyAll(ready);
}

int GpuTaskExecutor::sUsageCount() {
    return sUseCount;
}

int GpuTaskExecutor::sWaitingTasks() {
    return sTasks.count();
}

bool GpuTaskExecutor::unhandledException() const {
    return static_cast<bool>(mProcessException);
}

std::exception_ptr GpuTaskExecutor::handleException() {
    std::exception_ptr exc;
    std::swap(mProcessException, exc);
    return exc;
}

void GpuTaskExecutor::initialize(QThread* const thread) {
    OffscreenQGL33c::initialize();
    moveContextToThread(thread);
}

void GpuTaskExecutor::initializeContext() {
    //std::cout << "Entered GpuTaskExecutor::initializeContext" << std::endl;
    mInterface = GrGLMakeNativeInterface();
    if(!mInterface) RuntimeThrow("Failed to make native interface.");
    //std::cout << "Created GrGLInterface" << std::endl;
    const auto grContext = GrContext::MakeGL(mInterface);
    if(!grContext) RuntimeThrow("Failed to make GrContext.");
    //std::cout << "Created GrContext" << std::endl;
    GLuint textureSquareVAO;
    iniTexturedVShaderVAO(this, textureSquareVAO);
    //std::cout << "iniTexturedVShaderVAO" << std::endl;
    mContext.setContext(grContext, textureSquareVAO);
    //std::cout << "SwitchableContext set" << std::endl;
    glClearColor(0, 0, 0, 0);
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    checkGLErrors(this, "Error initializing GPU context.");

    //std::cout << "Done GpuTaskExecutor::initializeContext" << std::endl;
}

void GpuTaskExecutor::processTask(eTask& task) {
    task.processGpu(this, mContext);
}

void GpuTaskExecutor::start() {
    makeCurrent();
    if(!mInitialized) {
        try {
            initializeContext();
        } catch(...) {
            setException(std::current_exception());
            doneCurrent();
            QThread::currentThread()->quit();
            return;
        }
        mInitialized = true;
    }
    processLoop();
    doneCurrent();
}

void GpuTaskExecutor::setException(const std::exception_ptr& exception) {
    mProcessException = exception;
}
