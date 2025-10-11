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

#ifndef TRANSFORMANIMATOR_H
#define TRANSFORMANIMATOR_H
#include "Properties/comboboxproperty.h"
#include "staticcomplexanimator.h"
#include "../skia/skiaincludes.h"
#include "transformvalues.h"

#include <QMatrix>

class TransformUpdater;
class BoxPathPoint;
class MovablePoint;
class QPointFAnimator;

class CORE_EXPORT BasicTransformAnimator : public StaticComplexAnimator {
    e_OBJECT
    Q_OBJECT
protected:
    BasicTransformAnimator();
public:
    virtual void reset();
    virtual QMatrix getRelativeTransformAtFrame(
            const qreal relFrame, QMatrix* postTransform = nullptr) const;
    virtual QMatrix getInheritedTransformAtFrame(const qreal relFrame) const;
    virtual QMatrix getTotalTransformAtFrame(const qreal relFrame) const;

    FrameRange prp_getIdenticalRelRange(const int relFrame) const;

    void resetScale();
    void resetTranslation();
    void resetRotation();

    void setScale(const qreal sx, const qreal sy);
    void setPosition(const qreal x, const qreal y);
    void setRotation(const qreal rot);

    void startRotTransform();
    void startPosTransform();
    void startScaleTransform();

    void setRelativePos(const QPointF &relPos);
    void moveByAbs(const QPointF &absTrans);

    void rotateRelativeToSavedValue(const qreal rotRel);
    void translate(const qreal dX, const qreal dY);
    void scale(const qreal sx, const qreal sy);
    void moveRelativeToSavedValue(const qreal dX,
                                  const qreal dY);

    qreal dx();
    qreal dy();
    qreal rot();
    qreal xScale();
    qreal yScale();
    QPointF pos();

    QPointF mapAbsPosToRel(const QPointF &absPos) const;
    QPointF mapRelPosToAbs(const QPointF &relPos) const;

    QPointF mapFromParent(const QPointF &parentRelPos) const;
    SkPoint mapAbsPosToRel(const SkPoint &absPos) const;
    SkPoint mapRelPosToAbs(const SkPoint &relPos) const;
    SkPoint mapFromParent(const SkPoint &parentRelPos) const;

    void scaleRelativeToSavedValue(const qreal sx,
                                   const qreal sy,
                                   const QPointF &pivot);
    void rotateRelativeToSavedValue(const qreal rotRel,
                                    const QPointF &pivot);

    void updateRelativeTransform(const UpdateReason reason);
    void updateInheritedTransform(const UpdateReason reason);

    const QMatrix &getInheritedTransform() const;
    const QMatrix &getTotalTransform() const;
    const QMatrix &getRelativeTransform() const;

    void setParentTransformAnimator(BasicTransformAnimator *parent);

    QPointFAnimator *getPosAnimator() const;
    QPointFAnimator *getScaleAnimator() const;
    QrealAnimator *getRotAnimator() const;

    void updateTotalTransform(const UpdateReason reason);
    void setSVGEventsVisibility(const bool visible);

protected:
    QList<qsptr<BasicTransformAnimator>> mChildBoxes;

    QMatrix mRelTransform;
    QMatrix mPostTransform;
    QMatrix mInheritedTransform;
    QMatrix mTotalTransform;

    ConnContextQPtr<BasicTransformAnimator> mParentTransform;

    qsptr<QPointFAnimator> mPosAnimator;
    qsptr<QPointFAnimator> mScaleAnimator;
    qsptr<QrealAnimator> mRotAnimator;
    qsptr<ComboBoxProperty> mSVGBeginProperty;
    qsptr<ComboBoxProperty> mSVGEndProperty;

private:
    bool rotationFlipped() const;
signals:
    void totalTransformChanged(const UpdateReason);
    void inheritedTransformChanged(const UpdateReason);
};

class CORE_EXPORT AdvancedTransformAnimator : public BasicTransformAnimator {
    e_OBJECT
protected:
    AdvancedTransformAnimator();
public:
    void reset();
    QMatrix getRelativeTransformAtFrame(
            const qreal relFrame, QMatrix* postTransform = nullptr) const;

    void applyTransformEffects(const qreal relFrame,
                               qreal& pivotX, qreal& pivotY,
                               qreal& posX, qreal& posY,
                               qreal& rot,
                               qreal& scaleX, qreal& scaleY,
                               qreal& shearX, qreal& shearY,
                               QMatrix& postTransform) const;

    void setValues(const TransformValues& values);

    QMatrix getRotScaleShearTransform();
    void startRotScaleShearTransform();
    void resetRotScaleShear();

    void resetShear();
    void resetPivot();
    void setPivotFixedTransform(const QPointF &newPivot);

    QPointF getPivot(const qreal relFrame);
    QPointF getPivot();
    qreal getPivotX();
    qreal getPivotY();

    void startShearTransform();
    void setShear(const qreal shearX,
                  const qreal shearY);
    void shear(const qreal shearXBy,
               const qreal shearYBy);
    void shearRelativeToSavedValue(const qreal shearXBy,
                                   const qreal shearYBy,
                                   const QPointF &pivot);

    qreal getOpacity();

    void startTransformSkipOpacity();

    void startOpacityTransform();
    void setOpacity(const qreal newOpacity);
    void setPivot(const qreal x, const qreal y);
    void startPivotTransform();

    void finishPivotTransform();
    QPointF getPivotAbs();
    QPointF getPivotAbs(const qreal relFrame);

    qreal getOpacity(const qreal relFrame);

    bool posOrPivotRecording() const;
    bool rotOrScaleOrPivotRecording() const;

    QPointFAnimator *getShearAnimator() const {
        return mShearAnimator.get();
    }

    QPointFAnimator *getPivotAnimator() const {
        return mPivotAnimator.get();
    }

    QrealAnimator *getOpacityAnimator() const {
        return mOpacityAnimator.get();
    }
private:
    qsptr<QPointFAnimator> mPivotAnimator;
    qsptr<QPointFAnimator> mShearAnimator;
    qsptr<QrealAnimator> mOpacityAnimator;
};

class CORE_EXPORT BoxTransformAnimator : public AdvancedTransformAnimator {
    e_OBJECT
protected:
    BoxTransformAnimator();
public:
    static const QString getSVGPropertyAction(const int value);
    QDomElement saveSVG(SvgExporter& exp, const FrameRange& visRange,
                        const QDomElement& child) const;
};

#endif // TRANSFORMANIMATOR_H
