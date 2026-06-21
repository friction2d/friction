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

#ifndef OFFSCREENQGL33C_H
#define OFFSCREENQGL33C_H

#include "glhelpers.h"
#include "exceptions.h"
#include <QOffscreenSurface>

class CORE_EXPORT OffscreenQGL33c : public QGL33 {
public:
    OffscreenQGL33c();
    ~OffscreenQGL33c();

    void initialize() {
        mOffscreenSurface = new QOffscreenSurface(nullptr, nullptr);
        mOffscreenSurface->create();
        mContext = new QOpenGLContext();
        mContext->setShareContext(QOpenGLContext::globalShareContext());
        if(!mContext->create())
            PrettyRuntimeThrow("Creating OpenGL context failed.\n "
                               "Make sure your GPU drivers support OpenGL 3.3 core.");
    }

    void makeCurrent() {
        if (!mContext->makeCurrent(mOffscreenSurface)) {
#ifdef USE_GLES
            PrettyRuntimeThrow("Making OpenGL ES context current failed.\n"
                               "Make sure your GPU drivers support OpenGL ES 3.0.");
#else
            PrettyRuntimeThrow("Making OpenGL context current failed.\n"
                               "Make sure your GPU drivers support OpenGL 3.3 core.");
#endif
        }
        if (!mInitialized) {
#ifdef USE_GLES
            initializeOpenGLFunctions();
#else
            if (!initializeOpenGLFunctions()) {
                PrettyRuntimeThrow("Initializing OpenGL 3.3 core functions failed.\n"
                                   "Make sure your GPU drivers support OpenGL 3.3 core.");
            }
#endif
            mInitialized = true;
        }
    }

    void doneCurrent() {
        mContext->doneCurrent();
    }

    void moveContextToThread(QThread * const thread) {
        mContext->moveToThread(thread);
    }
private:
    bool mInitialized = false;
    QOpenGLContext* mContext = nullptr;
    QOffscreenSurface *mOffscreenSurface = nullptr;
};

#endif // OFFSCREENQGL33C_H
