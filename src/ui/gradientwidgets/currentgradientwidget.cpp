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

#include "currentgradientwidget.h"
#include "GUI/global.h"
#include "widgets/colorwidgetshaders.h"
#include "Animators/gradient.h"
#include "Private/document.h"

#include <QMouseEvent>

CurrentGradientWidget::CurrentGradientWidget(QWidget *parent) :
    GLWidget(parent) {
    setMouseTracking(true);
    eSizesUI::widget.add(this, [this](const int size) {
        setFixedHeight(size);
    });
}

void CurrentGradientWidget::setCurrentGradient(Gradient * const gradient) {
    auto& conn = mGradient.assign(gradient);
    if(gradient) {
        conn << connect(gradient, &Gradient::prp_currentFrameChanged,
                        this, qOverload<>(&QWidget::update));
        conn << connect(gradient, &Gradient::ca_childRemoved,
                        this, &CurrentGradientWidget::updateCurrentColor);
        conn << connect(gradient, &Gradient::ca_childAdded,
                        this, &CurrentGradientWidget::updateCurrentColor);
    }
    setCurrentColorId(0);
    update();
}

void CurrentGradientWidget::paintGL() {
    qreal pixelRatio = devicePixelRatioF();
    glClearColor(0.075f, 0.075f, 0.082f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    if(!mGradient) return;
    const int nColors = mGradient->ca_getNumberOfChildren();
    if(nColors == 0) return;
    glUseProgram(PLAIN_PROGRAM.fID);
    int colX = 0;
    const int xInc = width()/nColors;
    const int hoveredColorId = mHoveredX < 0 ? -1 : mHoveredX/xInc;

    glUniform2f(PLAIN_PROGRAM.fMeshSizeLoc,
                height()/static_cast<float>(3*xInc), 1.f/3);
    for(int j = 0; j < nColors; j++) {
        const QColor color = mGradient->getColorAt(j);
        const int cWidth = j == nColors - 1 ? width() - colX : xInc;
        glViewport(colX * pixelRatio,
                   0,
                   cWidth * pixelRatio,
                   height() * pixelRatio);

        const bool lightBorder = shouldValPointerBeLightHSV(color.hueF(),
                                                            color.hsvSaturationF(),
                                                            color.valueF());

        glUniform4f(PLAIN_PROGRAM.fRGBAColorLoc,
                    color.redF(), color.greenF(),
                    color.blueF(), color.alphaF());

        glBindVertexArray(mPlainSquareVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        if(j == mColorId) {
            glUseProgram(DOUBLE_BORDER_PROGRAM.fID);
            glUniform2f(DOUBLE_BORDER_PROGRAM.fInnerBorderSizeLoc,
                        1.f/xInc, 1.f/height());
            glUniform2f(DOUBLE_BORDER_PROGRAM.fOuterBorderSizeLoc,
                        1.f/xInc, 1.f/height());
            if(lightBorder) {
                glUniform4f(DOUBLE_BORDER_PROGRAM.fInnerBorderColorLoc,
                            1, 1, 1, 1);
                glUniform4f(DOUBLE_BORDER_PROGRAM.fOuterBorderColorLoc,
                            0, 0, 0, 1);
            } else {
                glUniform4f(DOUBLE_BORDER_PROGRAM.fInnerBorderColorLoc,
                            0, 0, 0, 1);
                glUniform4f(DOUBLE_BORDER_PROGRAM.fOuterBorderColorLoc,
                            1, 1, 1, 1);
            }
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            glUseProgram(PLAIN_PROGRAM.fID);
        }
        if(j == hoveredColorId) {
            glUseProgram(BORDER_PROGRAM.fID);
            glUniform2f(BORDER_PROGRAM.fBorderSizeLoc,
                        1.f/xInc, 1.f/height());
            if(lightBorder) {
                glUniform4f(BORDER_PROGRAM.fBorderColorLoc, 1, 1, 1, 1);
            } else {
                glUniform4f(BORDER_PROGRAM.fBorderColorLoc, 0, 0, 0, 1);
            }
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            glUseProgram(PLAIN_PROGRAM.fID);
        }
        colX += xInc;
    }
}

void CurrentGradientWidget::colorRightPress(const int x, const QPoint &point) {
    if(!mGradient) return;
    if(mReordering) {
        mGradient->restoreOrder();
    } else {
        colorLeftPress(x);
        QMenu menu(this);
        menu.addAction(QIcon::fromTheme("plus"), tr("Add Color"));
        menu.addAction(QIcon::fromTheme("minus"), tr("Delete Color"));
        menu.addAction(QIcon::fromTheme("color"), tr("Bookmark Color"));
        const auto selectedAction = menu.exec(point);
        if(selectedAction) {
            if(selectedAction->text() == tr("Delete Color")) {
                if(mGradient->ca_getNumberOfChildren() < 2) {
                    mColor->setColor(Qt::black);
                } else {
                    mGradient->removeChild(mColor->ref<ColorAnimator>());
                }
            } else if(selectedAction->text() == tr("Add Color")) {
                mGradient->addColor(Qt::black);
            } else if(selectedAction->text() == tr("Bookmark Color")) {
                if (mColor) {
                    const QColor col = mColor->getColor();
                    Document::sInstance->addBookmarkColor(col);
                }
            }
            Document::sInstance->actionFinished();
        }
    }
}

void CurrentGradientWidget::colorLeftPress(const int x) {
    if(!mGradient) return;
    mFirstMove = true;
    setCurrentColorId(getColorIdAtX(x));
}

void CurrentGradientWidget::mousePressEvent(QMouseEvent *event) {
    if(event->button() == Qt::RightButton) {
        colorRightPress(event->x(), event->globalPos());
    } else if(event->button() == Qt::LeftButton) {
        colorLeftPress(event->x());
    }
}

int CurrentGradientWidget::getColorIdAtX(const int x) {
    const int nCols = mGradient->ca_getNumberOfChildren();
    return qBound(0, x*nCols/width(), nCols - 1);
}

void CurrentGradientWidget::setCurrentColorId(const int id) {
    mColorId = id;
    if(!mGradient || !mGradient->ca_hasChildren()) {
        mColor = nullptr;
        return;
    }
    const int nColors = mGradient->ca_getNumberOfChildren();
    mColorId = qBound(0, id, nColors - 1);

    mColor = mGradient->getChild(mColorId);
    emit selectedColorChanged(mColor);
    update();
}

void CurrentGradientWidget::colorAdd()
{
    if (!mGradient) { return; }
    mGradient->addColor(Qt::black);
}

void CurrentGradientWidget::colorRemove()
{
    if (!mGradient) { return; }
    if (mGradient->ca_getNumberOfChildren() < 2) {
        mColor->setColor(Qt::black);
    } else {
        mGradient->removeChild(mColor->ref<ColorAnimator>());
    }
}

ColorAnimator *CurrentGradientWidget::getColorAnimator() {
    return mColor;
}

void CurrentGradientWidget::mouseMoveEvent(QMouseEvent *event) {
    if(event->buttons() & Qt::LeftButton && mGradient) {
        if(mFirstMove) {
            mFirstMove = false;
            mReordering = true;
            mGradient->saveOrder();
        }
        const int nCols = mGradient->ca_getNumberOfChildren();
        const int colorId = clampInt(event->x()*nCols/width(), 0, nCols - 1);
        if(colorId != mColorId) {
            mGradient->swapChildrenTemporary(mColorId, colorId);
            setCurrentColorId(colorId);
            Document::sInstance->updateScenes();
        }
    }
    mHoveredX = event->x();
    update();
}

void CurrentGradientWidget::mouseReleaseEvent(QMouseEvent *event) {
    if(event->button() != Qt::LeftButton) return;
    mReordering = false;
    if(mFirstMove || !mGradient) return;
    mGradient->finishOrder();
    Document::sInstance->actionFinished();
}

void CurrentGradientWidget::leaveEvent(QEvent *) {
    mHoveredX = -1;
    update();
}

void CurrentGradientWidget::updateCurrentColor() {
    if(!mGradient) return setCurrentColorId(0);
    const int nColors = mGradient->ca_getNumberOfChildren();
    setCurrentColorId(qBound(0, mColorId, nColors));
}
