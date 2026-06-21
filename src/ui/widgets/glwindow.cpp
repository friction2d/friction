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

#include "glwindow.h"
#include "Private/esettings.h"
#include "colorhelpers.h"
#include <QPainter>
#include <QDebug>
#include "exceptions.h"

GLWindow::GLWindow(QWidget * const parent)
    : QOpenGLWidget(parent) {
    setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);
}

void GLWindow::bindSkia(const int w, const int h) {
    qreal pixelRatio = devicePixelRatioF();
    int scaledWidth = pixelRatio*w;
    int scaledHeight = pixelRatio*h;
    GrGLFramebufferInfo fbInfo;
    fbInfo.fFBOID = context()->defaultFramebufferObject();//buffer;
    fbInfo.fFormat = GR_GL_RGBA8;//buffer;
    GrBackendRenderTarget backendRT = GrBackendRenderTarget(
                                        scaledWidth, scaledHeight,
                                        0, 8, // (optional) 4, 8,
                                        fbInfo
                                        /*kRGBA_half_GrPixelConfig*/
                                        /*kSkia8888_GrPixelConfig*/);

    // setup SkSurface
    // To use distance field text, use commented out SkSurfaceProps instead
    // SkSurfaceProps props(SkSurfaceProps::kUseDeviceIndependentFonts_Flag,
    //                      SkSurfaceProps::kLegacyFontHost_InitType);
    SkSurfaceProps props(SkSurfaceProps::kLegacyFontHost_InitType);

//    sk_sp<SkColorSpace> colorSpace = SkColorSpace::MakeSRGB();
    mSurface = SkSurface::MakeFromBackendRenderTarget(
                                    mGrContext.get(),
                                    backendRT,
                                    kBottomLeft_GrSurfaceOrigin,
                                    SkColorType::kRGBA_8888_SkColorType,
                                    nullptr/*colorSpace*/,
                                    &props);
    if(!mSurface) RuntimeThrow("Failed to wrap buffer into SkSurface.");
    mCanvas = mSurface->getCanvas();
    mGrContext->resetContext();
}

void GLWindow::resizeGL(int, int) {
    try {
        mRebind = true;
        update();
    } catch(const std::exception& e) {
        gPrintExceptionCritical(e);
    }
}

void GLWindow::initializeGL() {
    try {
        const auto globalCtx = QOpenGLContext::globalShareContext();
        if(!globalCtx)
            RuntimeThrow("Application-wide shared OpenGL context not found");
        if(context()->shareContext() != globalCtx)
            context()->setShareContext(globalCtx);

#ifdef USE_GLES
        if (!context() || !context()->isValid()) {
            RuntimeThrow("Initializing OpenGL ES failed. Context is invalid. "
                         "Make sure your GPU supports OpenGL ES 3.0.");
        }
        initializeOpenGLFunctions();
#else
        if (!initializeOpenGLFunctions()) {
            RuntimeThrow("Initializing OpenGL 3.3 functions failed. "
                         "Make sure your GPU supports OpenGL 3.3.");
        }
#endif

        initialize();
    } catch(const std::exception& e) {
        gPrintExceptionFatal(e);
    }
}

void GLWindow::initialize()
{
    glClearColor(0, 0, 0, 1);

    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const auto iface = GrGLMakeNativeInterface();
    if (!iface) { RuntimeThrow("Failed to make native interface."); }

    GrContextOptions options;
    options.fInternalMultisampleCount = eSettings::instance().fInternalMultisampleCount;

    mGrContext = GrContext::MakeGL(iface, options);
    if (!mGrContext) { RuntimeThrow("Failed to make GrContext."); }

    try {
        bindSkia(width(), height());
    } catch(...) {
        RuntimeThrow("Failed to bind skia.");
    }
}

void GLWindow::paintGL() {
    if(mRebind) {
        mRebind = false;
        try {
            bindSkia(width(), height());
        } catch(const std::exception& e) {
            gPrintExceptionCritical(e);
        }
    }
    // cleared by Canvas
    // glClear(GL_COLOR_BUFFER_BIT);
    renderSk(mCanvas);
    mCanvas->flush();
}

void GLWindow::showEvent(QShowEvent *e) {
    resizeGL(width(), height());
    updateFix();
    QOpenGLWidget::showEvent(e);
}

#include <QTimer>
void GLWindow::updateFix() {
    QTimer::singleShot(1, this, [this]() { update(); });
}
