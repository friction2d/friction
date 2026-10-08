/*
#
# Friction - https://friction.graphics
#
# Copyright (c) Friction contributors
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3.
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

#include "colorwheel.h"

#include "Private/document.h"
#include "colorhelpers.h"
#include "glhelpers.h"
#include "actions.h"

#include <QDebug>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QWindow>
#include <QApplication>

using namespace Friction::Ui;

qreal sign(qreal x,
           qreal y,
           QPointF p2,
           QPointF p3)
{
    return (x - p3.x()) * (p2.y() - p3.y()) - (p2.x() - p3.x()) * (y - p3.y());
}

bool pointInTriangle(qreal x,
                     qreal y,
                     QPointF v1,
                     QPointF v2,
                     QPointF v3)
{
    bool b1, b2, b3;

    qreal q0 = 0;
    b1 = sign(x, y, v1, v2) < q0;
    b2 = sign(x, y, v2, v3) < q0;
    b3 = sign(x, y, v3, v1) < q0;

    return ((b1 == b2) && (b2 == b3));
}

bool insideCircle(int r,
                  int x_t,
                  int y_t)
{
    return x_t*x_t + y_t*y_t < r*r;
}

bool outsideCircle(int r,
                   int x_t,
                   int y_t)
{
    return !insideCircle(r, x_t, y_t);
}

ColorWheel::ColorWheel(QWidget *parent_t)
    : ColorWidget(parent_t)
{
    setSizePolicy(QSizePolicy::Expanding,
                  QSizePolicy::Expanding);
    setAttribute(Qt::WA_TranslucentBackground);
    setMinimumSize(180, 180);
}

void ColorWheel::setTarget(ColorAnimator * const target)
{
    mTarget.assign(target);
}

void ColorWheel::setColor(qreal h, qreal s, qreal v)
{
    if (mIsDragging) { return; }

    mHue = h;
    mSaturation = s;
    mValue = v;
    update();
}

void ColorWheel::initializeGL()
{
    QGL33 *gl = QOpenGLContext::currentContext()->versionFunctions<QGL33>();
    if (!gl) { return; }

    gIniProgram(gl,
                mWheelProgram,
                GL_PLAIN_VERT,
                ":/shaders/wheel.frag");
    gIniProgram(gl,
                mTriangleProgram,
                GL_PLAIN_VERT,
                ":/shaders/triangle.frag");

    iniPlainVShaderVBO(gl);
    iniPlainVShaderVAO(gl, mVAO);
}

void ColorWheel::resizeGL(int w, int h)
{
    mWheelDim = static_cast<uint>(std::min(width(),
                                           height()));
    mOuterCircleR = mWheelDim * 0.5f - 4.0f;

    mWheelThickness = static_cast<GLuint>(mWheelDim * 0.06f);

    mInnerCircleR = mOuterCircleR - mWheelThickness;

    ColorWidget::resizeGL(w, h);
}

void ColorWheel::drawTriangle()
{
    QGL33 *gl = QOpenGLContext::currentContext()->versionFunctions<QGL33>();
    if (!mTriangleProgram) { return; }

    gl->glUseProgram(mTriangleProgram);
    gl->glBindVertexArray(mVAO);

    float dpr = devicePixelRatioF();
    float tr_r = mInnerCircleR - 8.0f;

    gl->glUniform2f(gl->glGetUniformLocation(mTriangleProgram, "u_resolution"),
                    width() * dpr, height() * dpr);
    gl->glUniform1f(gl->glGetUniformLocation(mTriangleProgram, "u_radius"),
                    tr_r * dpr);
    gl->glUniform1f(gl->glGetUniformLocation(mTriangleProgram, "u_dpr"),
                    dpr);
    gl->glUniform1f(gl->glGetUniformLocation(mTriangleProgram, "u_hue"),
                    mHue);
    gl->glUniform1f(gl->glGetUniformLocation(mTriangleProgram, "u_sat"),
                    mSaturation);
    gl->glUniform1f(gl->glGetUniformLocation(mTriangleProgram, "u_val"),
                    mValue);

    gl->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void ColorWheel::paintGL()
{
    QGL33 *gl = QOpenGLContext::currentContext()->versionFunctions<QGL33>();
    if (!gl) { return; }

    QColor bg = parentWidget() ? parentWidget()->palette().color(QPalette::Window)
                               : palette().color(QPalette::Window);

    gl->glClearColor(bg.redF(),
                     bg.greenF(),
                     bg.blueF(),
                     1.0f);
    gl->glClear(GL_COLOR_BUFFER_BIT);

    gl->glEnable(GL_BLEND);
    gl->glBlendFunc(GL_SRC_ALPHA,
                    GL_ONE_MINUS_SRC_ALPHA);

    drawWheel();
    drawTriangle();
}

// TODO
/*void ColorWheel::wheelEvent(QWheelEvent *e)
{
    Actions::sInstance->startSmoothChange();
    applyToTarget(true, false);

    if (e->angleDelta().y() > 0) {
        mHue += 0.01;
        if (mHue > 1) { mHue -= 1; }
    } else {
        mHue -= 0.01;
        if (mHue < 0) { mHue += 1; }
    }

    applyToTarget(false, false);
    applyToTarget(false, true);

    Actions::sInstance->finishSmoothChange();
    Document::sInstance->actionFinished();

    update();

}*/

void ColorWheel::drawWheel()
{
    QGL33 *gl = QOpenGLContext::currentContext()->versionFunctions<QGL33>();
    if (!mWheelProgram) { return; }

    gl->glUseProgram(mWheelProgram);
    gl->glBindVertexArray(mVAO);

    float dpr = devicePixelRatioF();
    gl->glUniform2f(gl->glGetUniformLocation(mWheelProgram,
                                             "u_resolution"),
                                             width() * dpr, height() * dpr);
    gl->glUniform1f(gl->glGetUniformLocation(mWheelProgram,
                                             "u_dpr"),
                                             dpr);
    gl->glUniform1f(gl->glGetUniformLocation(mWheelProgram,
                                             "u_innerRadius"),
                                             mInnerCircleR * dpr);
    gl->glUniform1f(gl->glGetUniformLocation(mWheelProgram,
                                             "u_outerRadius"),
                                             mOuterCircleR * dpr);
    gl->glUniform1f(gl->glGetUniformLocation(mWheelProgram,
                                             "u_hue"),
                                             mHue);

    gl->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void ColorWheel::applyToTarget(bool isStart,
                               bool isFinish)
{
    if (!mTarget) { return; }

    auto a1 = mTarget->getVal1Animator();
    auto a2 = mTarget->getVal2Animator();
    auto a3 = mTarget->getVal3Animator();

    if (isStart) {
        if (a1) { a1->prp_startTransform(); }
        if (a2) { a2->prp_startTransform(); }
        if (a3) { a3->prp_startTransform(); }
        return;
    }

    if (isFinish) {
        if (a1) { a1->prp_finishTransform(); }
        if (a2) { a2->prp_finishTransform(); }
        if (a3) { a3->prp_finishTransform(); }
        return;
    }

    QColor c = QColor::fromHsvF(mHue, mSaturation, mValue);

    if (mTarget->getColorMode() == ColorMode::hsv) {
        if (a1) { a1->setCurrentBaseValue(mHue); }
        if (a2) a2->setCurrentBaseValue(mSaturation);
        if (a3) { a3->setCurrentBaseValue(mValue); }
    } else if (mTarget->getColorMode() == ColorMode::rgb) {
        if (a1) { a1->setCurrentBaseValue(c.redF()); }
        if (a2) { a2->setCurrentBaseValue(c.greenF()); }
        if (a3) { a3->setCurrentBaseValue(c.blueF()); }
    } else if (mTarget->getColorMode() == ColorMode::hsl) {
        qreal h = c.hslHueF();
        if (h < 0.0) { h = mHue; }
        if (a1) { a1->setCurrentBaseValue(h); }
        if (a2) { a2->setCurrentBaseValue(c.hslSaturationF()); }
        if (a3) { a3->setCurrentBaseValue(c.lightnessF()); }
    }
}

void ColorWheel::wheelInteraction(const int x_t, const int y_t)
{
    double cx = x_t - width() * 0.5;
    double cy = y_t - height() * 0.5;
    mHue = getAngleF(1, 0, -cx, cy);
    update();

    applyToTarget(false, false);

    Document::sInstance->updateScenes();
}

void ColorWheel::triangleInteraction(int x_t, int y_t)
{
    float cx = x_t - width() * 0.5f;
    float cy = y_t - height() * 0.5f;
    float hue_rad = (mHue + 1/12.f) * 2 * PIf;

    float c = cos(hue_rad);
    float s = sin(hue_rad);
    float tr_x_t = cx * c - cy * s;
    float tr_y_t = cx * s + cy * c;

    float tr_r = mInnerCircleR - 8.0f;
    tr_x_t += tr_r;
    tr_y_t += tr_r;

    float row_width_t = tr_y_t * 2 / sqrt_3f;
    float row_x_0 = tr_r - row_width_t * 0.5f;

    mSaturation = clamp((tr_x_t - row_x_0) / row_width_t, 0.f, 1.f);
    mValue = clamp(tr_y_t / (tr_r * 1.5f), 0.f, 1.f);

    update();

    applyToTarget(false, false);

    Document::sInstance->updateScenes();
}

void ColorWheel::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::RightButton) { return; }

    mIsDragging = true;
    grabMouse();

    Actions::sInstance->startSmoothChange();
    applyToTarget(true, false);

    if (isInTriangle(e->pos())) {
        mFocus = SV;
        triangleInteraction(e->x(), e->y());
    } else if (isInWheel(e->pos())) {
        mFocus = H;
        wheelInteraction(e->x(), e->y());
    } else {
        mFocus = NONE;
        mIsDragging = false;
        applyToTarget(false, true);
        releaseMouse();
        return;
    }
}

void ColorWheel::mouseReleaseEvent(QMouseEvent *)
{
    applyToTarget(false, true);

    releaseMouse();
    mFocus = NONE;
    mIsDragging = false;

    Actions::sInstance->finishSmoothChange();
    Document::sInstance->actionFinished();
}

void ColorWheel::mouseMoveEvent(QMouseEvent *e)
{
    if (mFocus == H) {
        wheelInteraction(e->x(), e->y());
    } else if (mFocus == SV) {
        triangleInteraction(e->x(), e->y());
    }
}

bool ColorWheel::isInWheel(const QPoint& pos_t)
{
    int cx = pos_t.x() - width() * 0.5;
    int cy = pos_t.y() - height() * 0.5;

    if (!insideCircle(static_cast<int>(mOuterCircleR), cx, cy)) { return false; }
    return outsideCircle(static_cast<int>(mInnerCircleR), cx, cy);
}

bool ColorWheel::isInTriangle(const QPoint& pos_t)
{
    int cx = pos_t.x() - width() * 0.5;
    int cy = pos_t.y() - height() * 0.5;

    return insideCircle(static_cast<int>(mInnerCircleR), cx, cy);
}
