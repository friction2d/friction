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

#ifndef QDOUBLESLIDER_H
#define QDOUBLESLIDER_H

#include "ui_global.h"

#include <QWidget>
#include <QLineEdit>
#include <QDoubleValidator>

#ifdef Q_OS_MAC
#include <QWheelEvent>
#endif

class UI_EXPORT SliderEdit : public QLineEdit {
    Q_OBJECT
public:
    SliderEdit(QWidget* const parent);
signals:
    void valueSet(const qreal value);
    void hoverChanged();
    void tabPressed();
protected:
    void mousePressEvent(QMouseEvent* e) override;
    void keyPressEvent(QKeyEvent *e) override;
    void hideEvent(QHideEvent *e) override;
    void showEvent(QShowEvent *e) override;
    void leaveEvent(QEvent *e) override;
private:
    void lineEditingFinished();

    bool mCanceled = false;
};

class UI_EXPORT QDoubleSlider : public QWidget {
    Q_OBJECT
public:
    QDoubleSlider(const qreal minVal,
                  const qreal maxVal,
                  const qreal prefferedStep,
                  QWidget * const parent = nullptr,
                  bool autoAdjust = true);
    QDoubleSlider(const QString& name,
                  const qreal minVal,
                  const qreal maxVal,
                  const qreal prefferedStep,
                  QWidget * const parent = nullptr);
    QDoubleSlider(QWidget *parent = nullptr);
    ~QDoubleSlider();

    void setValueSliderVisible(const bool valueSliderVisible);
    void setNameVisible(const bool nameVisible);
    void setName(const QString& name);
    void setNumberDecimals(const int decimals);
    void setLineEditFocus();

    void fitWidthToContent();
    QString getValueString();

    void setValueRange(const qreal min, const qreal max);

    void setPrefferedValueStep(const qreal step);

    qreal minimum() { return mMinValue; }
    qreal value() { return mValue; }
    qreal maximum() { return mMaxValue; }

    void setIsRightSlider(const bool value);
    void setIsLeftSlider(const bool value);

    void setDisplayedValue(const qreal value);

    qreal clamped(const qreal value) const
    { return qBound(mMinValue, value, mMaxValue); }
protected:
    virtual void paint(QPainter *p);
    virtual void openContextMenu(const QPoint &globalPos)
    { Q_UNUSED(globalPos) }
    virtual QString getEditText() const
    { return mValueString; }

    virtual void startTransform(const qreal value);
    virtual void setValue(const qreal value);
    virtual void finishTransform(const qreal value);
    virtual void cancelTransform();
    virtual qreal startSlideValue() const { return mValue; }

    void paintEvent(QPaintEvent *);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

#ifdef Q_OS_MAC
    void wheelEvent(QWheelEvent *event);
#endif

    void enterEvent(QEvent *);
    void leaveEvent(QEvent *);

    void paint(QPainter *p,
               const QColor &allFill,
               const QColor &sliderFill,
               const QColor &stroke,
               const QColor &text);
    void paint(QPainter * const p, const bool enabled);
signals:
    void editingStarted(qreal);
    void valueEdited(qreal);
    void editingFinished(qreal);
    void editingCanceled();
    void tabPressed();
protected:
    QString valueToText(const qreal value) const;
    qreal getDValueForMouseMove(const int mouseX) const;
    bool mouseMoved() const { return mMouseMoved; }
    bool textEditing() const;
    void updateValueString();
private:
    void updateLineEditFromValue();
    bool cancelMove();

    bool mMouseMoved = false;
    bool mLeftNeighbour = false;
    bool mRightNeighbour = false;
    int mDecimals = 3;

    QString mName = "";
    bool mShowName = false;

    SliderEdit *mLineEdit = nullptr;
    QString mValueString;
    qreal mValue = 0;
    bool mTextEdit = false;
    int mLastX;
    int mMovesCount = 0;
    bool mCanceled = false;
    qreal mMinValue = 0;
    qreal mMaxValue = 0;
    qreal mPrefferedValueStep = 1;

    QPoint mGlobalPressPos;
    qreal mLastValue;
    bool mShowValueSlider = true;
    bool mAutoAdjustWidth = true;

    bool mHovered = false;
};

#endif // QDOUBLESLIDER_H
