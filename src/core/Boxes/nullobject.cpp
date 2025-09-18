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

#include "nullobject.h"

#include "Animators/transformanimator.h"
#include "canvas.h"

class NullObjectType : public ComboBoxProperty {
    enum class Type {
        square, circle, triangle, crosshair
    };
public:
    NullObjectType(ColorAnimator* const color,
                   QrealAnimator* const size);

    void prp_drawCanvasControls(SkCanvas * const canvas, const CanvasMode mode,
                                const float invScale, const bool ctrlPressed);

    void updatePath();
    bool relPointInsidePath(const QPointF &relPos);
    QRectF relBoundingRect();
private:
    SkPath mCurrentPath;

    ColorAnimator* const mColor;
    QrealAnimator* const mSize;
};

NullObjectType::NullObjectType(ColorAnimator* const color,
                               QrealAnimator* const size) :
    ComboBoxProperty("type", QStringList()  << "square" <<
                                               "circle" <<
                                               "triangle" <<
                                               "crosshair"),
    mColor(color), mSize(size) {
    prp_setDrawingOnCanvasEnabled(true);

    connect(this, &ComboBoxProperty::prp_currentFrameChanged,
            this, &NullObjectType::updatePath);
    connect(this, &ComboBoxProperty::prp_pathChanged,
            this, &NullObjectType::updatePath);
    connect(mSize, &QrealAnimator::prp_currentFrameChanged,
            this, &NullObjectType::updatePath);
    updatePath();
}

void NullObjectType::prp_drawCanvasControls(
        SkCanvas* const canvas, const CanvasMode mode,
        const float invScale, const bool ctrlPressed) {
    Q_UNUSED(mode)
    Q_UNUSED(ctrlPressed)

    const auto qtrans = getTransform();
    const auto sktrans = toSkMatrix(qtrans);
    SkPath mappedPath;
    mCurrentPath.transform(sktrans, &mappedPath);

    const auto color = mColor->getColor();

    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setStrokeWidth(2.f*invScale);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setColor(SK_ColorLTGRAY);
    canvas->drawPath(mappedPath, paint);
    paint.setStrokeWidth(1.f*invScale);
    paint.setColor(toSkColor(color));
    canvas->drawPath(mappedPath, paint);
}

void NullObjectType::updatePath() {
    mCurrentPath.reset();
    const int val = getCurrentValue();
    const Type type = static_cast<Type>(val);
    const qreal size = mSize->getEffectiveValue();
    const qreal ten = 10*size;
    switch(type) {
    case Type::square:
        mCurrentPath.moveTo(-ten, -ten);
        mCurrentPath.lineTo(ten, -ten);
        mCurrentPath.lineTo(ten, ten);
        mCurrentPath.lineTo(-ten, ten);
        mCurrentPath.lineTo(-ten, -ten);
        mCurrentPath.moveTo(-ten, -ten);
        mCurrentPath.lineTo(ten, ten);
        mCurrentPath.moveTo(ten, -ten);
        mCurrentPath.lineTo(-ten, ten);
        break;
    case Type::circle:
        mCurrentPath.addCircle(0, 0, ten);
        mCurrentPath.moveTo(-ten, -ten);
        mCurrentPath.lineTo(ten, ten);
        mCurrentPath.moveTo(ten, -ten);
        mCurrentPath.lineTo(-ten, ten);
        break;
    case Type::crosshair: {
        const qreal circleRadius = ten * 7/8;
        mCurrentPath.addCircle(0, 0, circleRadius);
        const qreal lineLength = 2 * ten / 8;
        mCurrentPath.moveTo(-circleRadius - lineLength, 0);
        mCurrentPath.lineTo(-circleRadius + lineLength, 0);
        mCurrentPath.moveTo(circleRadius - lineLength, 0);
        mCurrentPath.lineTo(circleRadius + lineLength, 0);
        mCurrentPath.moveTo(0, -circleRadius + lineLength);
        mCurrentPath.lineTo(0, -circleRadius - lineLength);
        mCurrentPath.moveTo(0, circleRadius + lineLength);
        mCurrentPath.lineTo(0, circleRadius - lineLength);
        mCurrentPath.moveTo(lineLength/2, 0);
        mCurrentPath.lineTo(-lineLength/2, 0);
        mCurrentPath.moveTo(0, + lineLength/2);
        mCurrentPath.lineTo(0, - lineLength/2);
        break;
        }
    case Type::triangle:
        const qreal a = 3*ten/sqrt(3);
        const qreal h = ten + a*sqrt(3)/6;
        mCurrentPath.moveTo(0, -ten);
        mCurrentPath.lineTo(0.5*a, -ten + h);
        mCurrentPath.lineTo(-0.5*a, -ten + h);
        mCurrentPath.lineTo(0, -ten);
        mCurrentPath.moveTo(0, -ten);
        mCurrentPath.lineTo(0, 0);
        mCurrentPath.moveTo(0.5*a, -ten + h);
        mCurrentPath.lineTo(0, 0);
        mCurrentPath.moveTo(-0.5*a, -ten + h);
        mCurrentPath.lineTo(0, 0);
        break;
    }
}

bool NullObjectType::relPointInsidePath(const QPointF& relPos) {
    const int val = getCurrentValue();
    const Type type = static_cast<Type>(val);
    const qreal size = mSize->getEffectiveValue();
    const qreal ten = 10*size;
    switch(type) {
    case Type::square: {
        QRectF rect(-ten, -ten, 2*ten, 2*ten);
        return rect.contains(relPos);
    } case Type::circle: {
        QPainterPath path;
        path.addEllipse({0., 0.}, ten, ten);
        return path.contains(relPos);
    } case Type::crosshair: {
        QPainterPath path;
        path.addEllipse({0., 0.}, ten, ten);
        return path.contains(relPos);
    } case Type::triangle:
        const qreal a = 3*ten/sqrt(3);
        const qreal h = ten + a*sqrt(3)/6;
        QPainterPath path;
        path.moveTo(0, -ten);
        path.lineTo(0.5*a, -ten + h);
        path.lineTo(-0.5*a, -ten + h);
        path.closeSubpath();
        return path.contains(relPos);
    }
    return false;
}

QRectF NullObjectType::relBoundingRect() {
    const auto skRect = mCurrentPath.computeTightBounds();
    return toQRectF(skRect);
}

void NullObject::queTasks() {
    setRelBoundingRect(mType->relBoundingRect());
    BoundingBox::queTasks();
}

NullObject::NullObject() : BoundingBox("null object", eBoxType::nullObject) {
    mColor = enve::make_shared<ColorAnimator>();
    mSize = enve::make_shared<QrealAnimator>(1, 1, 100, 0.1, "size");
    mType = enve::make_shared<NullObjectType>(mColor.get(), mSize.get());

    ca_addChild(mType);
    ca_addChild(mColor);
    ca_addChild(mSize);

    connect(this, &BoundingBox::prp_sceneChanged,
            this, [this](Canvas* const oldS, Canvas* const newS) {
        if(oldS) oldS->removeNullObject(this);
        if(newS) newS->addNullObject(this);
    });
}

bool NullObject::relPointInsidePath(const QPointF &relPos) const {
    return mType->relPointInsidePath(relPos);
}

void NullObject::drawNullObject(SkCanvas* const canvas,
                                const CanvasMode mode,
                                const float invScale,
                                const bool ctrlPressed)
{
    if (!isVisible()) { return; }
    mType->prp_drawCanvasControls(canvas,
                                  mode,
                                  invScale,
                                  ctrlPressed);
}
