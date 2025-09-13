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

#include "colorlabel.h"
#include "GUI/global.h"
#include "colorhelpers.h"
#include <QPainter>
#include <QMouseEvent>
#include <QWidget>
#include <QMenu>
#include "Private/document.h"
#include "widgets/colorwidgetshaders.h"

ColorLabel::ColorLabel(QWidget *parent,
                       const bool &bookmark)
    : ColorWidget(parent)
    , mBookmark(bookmark)
{

}

void ColorLabel::mousePressEvent(QMouseEvent *e)
{
    if (!mBookmark) {
        ColorWidget::mousePressEvent(e);
        return;
    }
    QMenu menu(this);
    menu.addAction(QIcon::fromTheme("color"), tr("Bookmark Color"));
    const auto act = menu.exec(e->globalPos());
    if (act) {
        if (act->text() == tr("Bookmark Color")) {
            const QColor col = QColor::fromHsvF(qreal(mHue),
                                                qreal(mSaturation),
                                                qreal(mValue),
                                                mAlpha);
            Document::sInstance->addBookmarkColor(col);
        }
    }
}

void ColorLabel::setAlpha(const qreal alpha_t)
{
    mAlpha = alpha_t;
    update();
}

void ColorLabel::addBookmark()
{
    if (!mBookmark) { return; }
    const QColor col = QColor::fromHsvF(qreal(mHue),
                                        qreal(mSaturation),
                                        qreal(mValue),
                                        mAlpha);
    Document::sInstance->addBookmarkColor(col);
}

void ColorLabel::setGradient(Gradient *gradient)
{
    if (mGradient == gradient) { return; }
    mGradient = gradient;
    update();
}

void ColorLabel::paintGL()
{
    if (!mGradient) {
        qreal pixelRatio = devicePixelRatioF();
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(PLAIN_PROGRAM.fID);
        float r = mHue;
        float g = mSaturation;
        float b = mValue;
        hsv_to_rgb_float(r, g, b);

        qreal scaledWidth = pixelRatio*width();
        qreal scaledHeight = pixelRatio*height();
        int halfScaledWidth = scaledWidth/2;
        glViewport(0, 0, halfScaledWidth, scaledHeight);
        glUniform4f(PLAIN_PROGRAM.fRGBAColorLoc, r, g, b,
                    static_cast<float>(mAlpha));
        glUniform2f(PLAIN_PROGRAM.fMeshSizeLoc, scaledHeight/(1.5f*scaledWidth), 1.f/3);
        glBindVertexArray(mPlainSquareVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glViewport(halfScaledWidth, 0, scaledWidth - halfScaledWidth, scaledHeight);
        glUniform4f(PLAIN_PROGRAM.fRGBAColorLoc, r, g, b, 1.f);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    } else {
        int mDisplayedTop = 0;
        int mTopGradientId = 0;
        int mMaxVisibleGradients = 6;
        qreal pixelRatio = devicePixelRatioF();
        const int nVisible = qMin(/*Gradients.count()*/1 - mTopGradientId,
                                  mMaxVisibleGradients);
        int gradY = mDisplayedTop;
        glClearColor(0.075f, 0.075f, 0.082f, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(GRADIENT_PROGRAM.fID);
        glBindVertexArray(mPlainSquareVAO);
        for (int i = mTopGradientId; i < mTopGradientId + nVisible; i++) {
            const int yInverted = height() - gradY - eSizesUI::widget;
            const int nColors = mGradient->ca_getNumberOfChildren();
            QColor lastColor = mGradient->getColorAt(0);
            int xT = 0;
            const float xInc = static_cast<float>(width())/qMax(1, nColors - 1);
            glUniform2f(GRADIENT_PROGRAM.fMeshSizeLoc,
                        eSizesUI::widget/(3.f*xInc), 1.f/3);
            for (int j = (nColors == 1 ? 0 : 1); j < nColors; j++) {
                const QColor color = mGradient->getColorAt(j);
                glViewport(xT * pixelRatio,
                           yInverted * pixelRatio,
                           qRound(xInc * pixelRatio),
                           eSizesUI::widget * pixelRatio);

                glUniform4f(GRADIENT_PROGRAM.fRGBAColor1Loc,
                            lastColor.redF(), lastColor.greenF(),
                            lastColor.blueF(), lastColor.alphaF());
                glUniform4f(GRADIENT_PROGRAM.fRGBAColor2Loc,
                            color.redF(), color.greenF(),
                            color.blueF(), color.alphaF());

                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                xT += qRound(xInc);
                lastColor = color;
            }
            glViewport(0,
                       yInverted * pixelRatio,
                       width() * pixelRatio,
                       eSizesUI::widget * pixelRatio);
            gradY += eSizesUI::widget;
        }
    }
}
