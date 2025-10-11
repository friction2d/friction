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

#ifndef MOVABLEPOINT_H
#define MOVABLEPOINT_H
#include <QPointF>
#include <QRectF>
#include <QPainter>
#include "../smartPointers/ememory.h"
#include "../skia/skiaincludes.h"

class BoundingBox;
class NodePoint;
class BasicTransformAnimator;
enum class CanvasMode : short;
class PropertyUpdater;

enum MovablePointType : short {
    TYPE_PATH_POINT,
    TYPE_SMART_PATH_POINT,
    TYPE_CTRL_POINT,
    TYPE_PIVOT_POINT,
    TYPE_GRADIENT_POINT
};
class MovablePoint;
template<typename T> class TypeMenu;
typedef TypeMenu<MovablePoint> PointTypeMenu;

class CORE_EXPORT MovablePoint : public StdSelfRef {
    e_OBJECT
protected:
    MovablePoint(const MovablePointType type);
    MovablePoint(BasicTransformAnimator * const trans,
                 const MovablePointType type);
public:
    typedef std::function<void(MovablePoint*)> PtOp;
    typedef std::function<void()> Op;

    ~MovablePoint() { if(mRemoveFromSelection) mRemoveFromSelection(); }

    virtual QPointF getRelativePos() const = 0;
    virtual void setRelativePos(const QPointF &relPos) = 0;

    virtual void scale(const qreal scaleXBy, const qreal scaleYBy);
    virtual void scaleRelativeToSavedPivot(const qreal sx,
                                           const qreal sy);
    virtual void shearRelativeToSavedPivot(const qreal shearX,
                                           const qreal shearY);
    virtual void rotateRelativeToSavedPivot(const qreal rot);
    virtual void saveTransformPivotAbsPos(const QPointF &absPivot);

    virtual void startTransform();
    virtual void finishTransform() {}
    virtual void cancelTransform() {}

    virtual void drawSk(SkCanvas * const canvas,
                        const CanvasMode mode,
                        const float invScale,
                        const bool keyOnCurrent,
                        const bool ctrlPressed);

    virtual void remove() {}
    virtual bool isVisible(const CanvasMode mode) const;

    virtual void canvasContextMenu(PointTypeMenu * const menu) {
        Q_UNUSED(menu)
    }

    virtual MovablePoint * getPointAtAbsPos(const QPointF &absPos,
                                            const CanvasMode mode,
                                            const qreal invScale) {
        if(isPointAtAbsPos(absPos, mode, invScale)) return this;
        return nullptr;
    }

    virtual void rectPointsSelection(const QRectF &absRect,
                                     const CanvasMode mode,
                                     const PtOp &adder);

    virtual void setTransform(BasicTransformAnimator * const trans);
    virtual void updateRadius() { setRadius(5); }

    void moveToAbs(const QPointF& absPos);
    void moveByAbs(const QPointF &absTrans);
    void moveToRel(const QPointF &relPos);
    void moveByRel(const QPointF &relTranslation);
    QPointF getAbsolutePos() const;
    bool isPointAtAbsPos(const QPointF &absPoint,
                         const CanvasMode mode,
                         const qreal invScale);
    void setAbsolutePos(const QPointF &pos);

    BasicTransformAnimator *getTransform();

    bool isContainedInRect(const QRectF &absRect);

    void setSelected(const bool selected, const Op &deselect = nullptr);
    void deselect();

    bool isSelected() const { return mSelected; }

    bool selectionEnabled() const {
        return mSelectionEnabled;
    }

    void enableSelection() {
        setSelectionEnabled(true);
    }

    void disableSelection() {
        setSelectionEnabled(false);
    }

    void setSelectionEnabled(const bool enabled) {
        mSelectionEnabled = enabled;
    }

    bool isHidden(const CanvasMode mode) const {
        return !isVisible(mode);
    }

    bool isNodePoint();
    bool isSmartNodePoint();
    bool isPivotPoint();
    bool isCtrlPoint();

    void rotateBy(const qreal rot);
    void scale(const qreal scaleBy);
    void applyTransform(const QMatrix &transform) {
        setRelativePos(transform.map(getRelativePos()));
    }
    void setRadius(const qreal radius);
    qreal getRadius();

    void drawHovered(SkCanvas * const canvas,
                     const float invScale);

    QPointF mapRelativeToAbsolute(const QPointF &relPos) const;
    QPointF mapAbsoluteToRelative(const QPointF &absPos) const;

    const QPointF &getSavedRelPos() const;
protected:
    void drawOnAbsPosSk(SkCanvas * const canvas,
                        const SkPoint &absPos,
                        const float invScale,
                        const SkColor &fillColor,
                        const bool keyOnCurrent = false);
private:
    std::function<void()> mRemoveFromSelection;

    bool mSelectionEnabled = true;
    bool mSelected = false;
    const MovablePointType mType;
    qreal mRadius = 5;
    QPointF mPivot;
    QPointF mSavedRelPos;
    BasicTransformAnimator * mTrans_cv = nullptr;
};

#endif // MOVABLEPOINT_H
