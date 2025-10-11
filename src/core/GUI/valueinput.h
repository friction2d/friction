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

#ifndef VALUEINPUT_H
#define VALUEINPUT_H

#include "../core_global.h"
#include "skia/skiaincludes.h"

#include <QString>
#include <QPointF>
#include <QLocale>

class QPainter;
class SkCanvas;
class QKeyEvent;

enum class TransformMode { move, scale, rotate, shear, none };
enum class DirectionMode { x, y, xy };

class CORE_EXPORT ValueInput {
public:
    ValueInput();

    void draw(SkCanvas *canvas, const int y);
    void draw(QPainter *p, const int y);

    qreal getValue() const { return mInputValue; }
    QPointF getPtValue() const {
        if(mTransMode == TransformMode::rotate ||
               mXYMode == DirectionMode::xy)
            return {mInputValue, mInputValue};
        if(mTransMode == TransformMode::scale) {
            if(mXYMode == DirectionMode::x) return {mInputValue, 1};
            else return {1, mInputValue};
        }
        if(mTransMode == TransformMode::shear) {
            if(mXYMode == DirectionMode::x) return {mInputValue, 0};
            else return {0, mInputValue};
        }
        if(mXYMode == DirectionMode::x) return {mInputValue, 0};
        else return {0, mInputValue};
    }

    void clearAndDisableInput();

    bool handleTransormationInputKeyEvent(const int key);

    bool inputEnabled() const { return mInputEnabled; }

    void setupRotate() {
        mTransMode = TransformMode::rotate;
        setDisplayedValue(0);
    }

    void setupScale() {
        mTransMode = TransformMode::scale;
        setDisplayedValue(1);
    }

    void setupShear() {
        mTransMode = TransformMode::shear;
        setDisplayedValue(0);
    }

    void setupMove() {
        mTransMode = TransformMode::move;
        setDisplayedValue(0);
    }

    void setXOnlyMode() {
        mXYMode = DirectionMode::x;
    }

    void setYOnlyMode() {
        mXYMode = DirectionMode::y;
    }

    void setXYMode() {
        mXYMode = DirectionMode::xy;
    }

    void switchXOnlyMode() {
        if(mXYMode == DirectionMode::x) setXYMode();
        else setXOnlyMode();
    }

    void switchYOnlyMode() {
        if(mXYMode == DirectionMode::y) setXYMode();
        else setYOnlyMode();
    }

    bool xOnlyMode() const {
        return mXYMode == DirectionMode::x;
    }

    bool yOnlyMode() const {
        return mXYMode == DirectionMode::y;
    }

    void setDisplayedValue(const qreal value) {
        setDisplayedValue({value, value});
    }

    void setDisplayedValue(const QPointF& value) {
        mDisplayValue = value;
    }

    void setForce1D(const bool d) {
        mForce1D = d;
    }
protected:
    void updateInputValue();

    QString getText() const {
        QString transStr;
        if(mInputEnabled) {
            transStr = getNameWithXY() + ": " + mInputText + "|";
        } else if(mTransMode == TransformMode::rotate) {
            const auto xVal = QLocale().toString(mDisplayValue.x(), 'f', 3);
            transStr = getName() + ": " + xVal;
        } else if(xOnlyMode()) {
            const auto xVal = QLocale().toString(mDisplayValue.x(), 'f', 3);
            transStr = getNameWithXY() + ": " + xVal;
        } else if(yOnlyMode()) {
            const auto yVal = QLocale().toString(mDisplayValue.y(), 'f', 3);
            transStr = getNameWithXY() + ": " + yVal;
        } else { // xy mode
            const auto xVal = QLocale().toString(mDisplayValue.x(), 'f', 3);
            const auto yVal = QLocale().toString(mDisplayValue.y(), 'f', 3);
            transStr = getName() + " x, y: " + xVal + ", " + yVal;
        }
        return transStr;
    }

    QString getName() const {
        if(mTransMode == TransformMode::move) {
            return "move";
        } else if(mTransMode == TransformMode::scale) {
            return "scale";
        } else if(mTransMode == TransformMode::shear) {
            return "shear";
        } else {
            return "rotate";
        }
    }

    QString getNameWithXY() const {
        if(mForce1D) return getName();
        if(mTransMode == TransformMode::move) {
            if(mXYMode == DirectionMode::xy) return "move x, y";
            else if(mXYMode == DirectionMode::x) return "move x";
            else return "move y";
        } else if(mTransMode == TransformMode::scale) {
            if(mXYMode == DirectionMode::xy) return "scale x, y";
            else if(mXYMode == DirectionMode::x) return "scale x";
            else return "scale y";
        } else if(mTransMode == TransformMode::shear) {
            if(mXYMode == DirectionMode::xy) return "shear x, y";
            else if(mXYMode == DirectionMode::x) return "shear x";
            else return "shear y";
        } else {
            return "rotate";
        }
    }

    SkFont mFont;
    TransformMode mTransMode = TransformMode::move;
    DirectionMode mXYMode = DirectionMode::xy;
    QString mInputText;
    qreal mInputValue = 0;
    QPointF mDisplayValue;
    bool mInputEnabled = false;
    bool mForce1D = false;
};

#endif // VALUEINPUT_H
