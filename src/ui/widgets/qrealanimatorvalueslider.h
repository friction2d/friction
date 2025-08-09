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

#ifndef QREALANIMATORVALUESLIDER_H
#define QREALANIMATORVALUESLIDER_H

#include "ui_global.h"

#include "widgets/qdoubleslider.h"
#include "Animators/qrealanimator.h"
#include "smartPointers/selfref.h"
#include "conncontextptr.h"

class UI_EXPORT QrealAnimatorValueSlider : public QDoubleSlider
{
    Q_OBJECT
public:
    QrealAnimatorValueSlider(QString name,
                             qreal minVal,
                             qreal maxVal,
                             qreal prefferedStep,
                             QWidget *parent);
    QrealAnimatorValueSlider(qreal minVal,
                             qreal maxVal,
                             qreal prefferedStep,
                             QWidget *parent);
    QrealAnimatorValueSlider(qreal minVal,
                             qreal maxVal,
                             qreal prefferedStep,
                             QWidget *parent,
                             bool autoAdjust);
    QrealAnimatorValueSlider(qreal minVal,
                             qreal maxVal,
                             qreal prefferedStep,
                             QrealAnimator* animator,
                             QWidget *parent = nullptr);
    QrealAnimatorValueSlider(QrealAnimator* animator,
                             QWidget *parent = nullptr);
    QrealAnimatorValueSlider(QWidget *parent = nullptr);

    void setTarget(QrealAnimator * const animator);
    bool hasTarget();
    bool isTargetDisabled();

protected:
    void paint(QPainter *p);
    void openContextMenu(const QPoint &globalPos);
    QString getEditText() const;

    void startTransform(const qreal value);
    void setValue(const qreal value);
    void finishTransform(const qreal value);
    void cancelTransform();
    qreal startSlideValue() const;

#ifdef Q_OS_MAC
    void wheelEvent(QWheelEvent *e);
#endif
    void mouseMoveEvent(QMouseEvent *e);
    void keyPressEvent(QKeyEvent *e);
    void keyReleaseEvent(QKeyEvent *e);
    bool eventFilter(QObject *obj,
                     QEvent *event);
    void handleTabPressed();

private:
    QrealAnimator *getTransformTargetSibling();
    QrealAnimator *getTargetSibling();
    void targetHasExpressionChanged();

    QMetaObject::Connection mExprConn;
    ConnContextQPtr<QrealAnimator> mTarget;
    qptr<QrealAnimator> mTransformTarget;
    qreal mBaseValue;

    bool mUniform = false;
};

#endif // QREALANIMATORVALUESLIDER_H
