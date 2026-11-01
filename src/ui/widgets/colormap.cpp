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

#include <QMouseEvent>
#include <QApplication>

#include "colormap.h"
#include "Private/document.h"

using namespace Friction::Ui;

ColorMap::ColorMap(QWidget *parent_t)
    : ColorWidget(parent_t)
{
    setSizePolicy(QSizePolicy::Expanding,
                  QSizePolicy::Expanding);
    setAttribute(Qt::WA_TranslucentBackground);
    setMinimumSize(180, 180);
}

void ColorMap::setTarget(ColorAnimator * const target)
{
    mTarget.assign(target);
}

void ColorMap::setColor(qreal h,
                        qreal s,
                        qreal v)
{
    if (mIsDragging) { return; }

    mHue = h;
    mSaturation = s;
    mValue = v;
    update();
}

void ColorMap::applyToTarget(bool isStart,
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
        if (a2) { a2->setCurrentBaseValue(mSaturation); }
        if (a3) { a3->setCurrentBaseValue(mValue); }
    } else if (mTarget->getColorMode() == ColorMode::rgb) {
        if (a1) { a1->setCurrentBaseValue(c.redF()); }
        if (a2) { a2->setCurrentBaseValue(c.greenF()); }
        if (a3) { a3->setCurrentBaseValue(c.blueF()); }
    } else if (mTarget->getColorMode() == ColorMode::hsl) {
        qreal h = c.hslHueF();
        if (h < 0.0) h = mHue;
        if (a1) { a1->setCurrentBaseValue(h); }
        if (a2) { a2->setCurrentBaseValue(c.hslSaturationF()); }
        if (a3) { a3->setCurrentBaseValue(c.lightnessF()); }
    }
}

void ColorMap::mapInteraction(int x, int y)
{
    mSaturation = clamp((float)x / width(), 0.0f, 1.0f);
    mValue = clamp(1.0f - ((float)y / height()), 0.0f, 1.0f);

    update();

    if (mTarget) { // HSV
        applyToTarget(false, false);
    } else { // RGB/HSL
        emit colorChanged(mHue, mSaturation, mValue);
    }
    Document::sInstance->updateScenes();
}

void ColorMap::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::RightButton) { return; }
    mIsDragging = true;
    grabMouse();

    if (mTarget) { // HSV
        Actions::sInstance->startSmoothChange();
        applyToTarget(true, false);
    } else { // RGB/HSL
        emit editingStarted();
    }
    mapInteraction(e->x(), e->y());
}

void ColorMap::mouseMoveEvent(QMouseEvent *e)
{
    if (!mIsDragging) { return; }
    mapInteraction(e->x(), e->y());
}

void ColorMap::mouseReleaseEvent(QMouseEvent *e)
{
    Q_UNUSED(e)
    if (!mIsDragging) { return; }
    mIsDragging = false;
    releaseMouse();

    if (mTarget) { // HSV
        applyToTarget(false, true);
        Actions::sInstance->finishSmoothChange();
        Document::sInstance->actionFinished();
    } else { // RGB/HSL
        emit editingFinished();
    }
}

void ColorMap::initializeGL()
{
    QGL33 *gl = QOpenGLContext::currentContext()->versionFunctions<QGL33>();
    if (!gl) { return; }

    gIniProgram(gl,
                mProgram,
                GL_PLAIN_VERT,
                ":/shaders/map.frag");

    iniPlainVShaderVBO(gl);
    iniPlainVShaderVAO(gl, mVAO);
}

void ColorMap::paintGL()
{
    QGL33 *gl = QOpenGLContext::currentContext()->versionFunctions<QGL33>();
    if (!gl) { return; }

    gl->glClear(GL_COLOR_BUFFER_BIT);
    gl->glUseProgram(mProgram);

    gl->glUniform2f(gl->glGetUniformLocation(mProgram,
                                             "u_resolution"),
                    width() * devicePixelRatioF(), height() * devicePixelRatioF());
    gl->glUniform1f(gl->glGetUniformLocation(mProgram,
                                             "u_hue"),
                    mHue);
    gl->glUniform1f(gl->glGetUniformLocation(mProgram,
                                             "u_sat"),
                    mSaturation);
    gl->glUniform1f(gl->glGetUniformLocation(mProgram,
                                             "u_val"),
                    mValue);
    gl->glUniform1f(gl->glGetUniformLocation(mProgram,
                                             "u_dpr"),
                    devicePixelRatioF());

    gl->glBindVertexArray(mVAO);
    gl->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void ColorMap::resizeGL(int w, int h)
{
    QGL33 *gl = QOpenGLContext::currentContext()->versionFunctions<QGL33>();
    if (!gl) { return; }

    gl->glViewport(0,
                   0,
                   w * devicePixelRatioF(),
                   h * devicePixelRatioF());
}
