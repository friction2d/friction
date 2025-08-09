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

#include "Boxes/circle.h"
#include "canvas.h"
#include "MovablePoints/movablepoint.h"
#include "Animators/gradientpoints.h"
#include "Animators/transformanimator.h"
#include "MovablePoints/pointshandler.h"
#include "PathEffects/patheffectcollection.h"

Circle::Circle() : PathBox("Circle", eBoxType::circle) {
    setPointsHandler(enve::make_shared<PointsHandler>());

    mCenterAnimator = enve::make_shared<QPointFAnimator>("center");
    mCenterPoint = enve::make_shared<AnimatedPoint>(mCenterAnimator.get(),
                                             mTransformAnimator.get(),
                                             TYPE_PATH_POINT);
    getPointsHandler()->appendPt(mCenterPoint);

    mCenterPoint->disableSelection();
    mCenterPoint->setRelativePos(QPointF(0, 0));
    ca_prependChild(mPathEffectsAnimators.data(),
                            mCenterAnimator);

    mHorizontalRadiusAnimator =
            enve::make_shared<QPointFAnimator>("horizontal radius");
    mHorizontalRadiusPoint = enve::make_shared<CircleRadiusPoint>(
                mHorizontalRadiusAnimator.get(), mTransformAnimator.get(),
                mCenterPoint.get(), TYPE_PATH_POINT, false);
    getPointsHandler()->appendPt(mHorizontalRadiusPoint);
    mHorizontalRadiusPoint->setRelativePos(QPointF(10, 0));
    const auto hXAnimator = mHorizontalRadiusAnimator->getXAnimator();
    ca_prependChild(mPathEffectsAnimators.data(),
                            hXAnimator->ref<QrealAnimator>());
    hXAnimator->prp_setName("horizontal radius");

    mVerticalRadiusAnimator =
            enve::make_shared<QPointFAnimator>("vertical radius");
    mVerticalRadiusPoint = enve::make_shared<CircleRadiusPoint>(
                mVerticalRadiusAnimator.get(), mTransformAnimator.get(),
                mCenterPoint.get(), TYPE_PATH_POINT, true);
    getPointsHandler()->appendPt(mVerticalRadiusPoint);
    mVerticalRadiusPoint->setRelativePos(QPointF(0, 10));
    const auto vYAnimator = mVerticalRadiusAnimator->getYAnimator();
    ca_prependChild(mPathEffectsAnimators.data(),
                            vYAnimator->ref<QrealAnimator>());
    vYAnimator->prp_setName("vertical radius");

    const auto pathUpdater = [this](const UpdateReason reason) {
        setPathsOutdated(reason);
    };
    connect(mCenterAnimator.get(), &Property::prp_currentFrameChanged,
            this, pathUpdater);
    connect(vYAnimator, &Property::prp_currentFrameChanged,
            this, pathUpdater);
    connect(hXAnimator, &Property::prp_currentFrameChanged,
            this, pathUpdater);
}

void Circle::moveRadiusesByAbs(const QPointF &absTrans) {
    mVerticalRadiusPoint->moveByAbs(absTrans);
    mHorizontalRadiusPoint->moveByAbs(absTrans);
}

void Circle::setVerticalRadius(const qreal verticalRadius) {
    const QPointF centerPos = mCenterPoint->getRelativePos();
    mVerticalRadiusPoint->setRelativePos(
                centerPos + QPointF(0, verticalRadius));
}

void Circle::setHorizontalRadius(const qreal horizontalRadius) {
    const QPointF centerPos = mCenterPoint->getRelativePos();
    mHorizontalRadiusPoint->setRelativePos(
                centerPos + QPointF(horizontalRadius, 0));
}

void Circle::setRadius(const qreal radius) {
    setHorizontalRadius(radius);
    setVerticalRadius(radius);
}

SkPath Circle::getRelativePath(const qreal relFrame) const {
    const QPointF center = mCenterAnimator->getEffectiveValue();
    const qreal xRad = mHorizontalRadiusAnimator->getEffectiveXValue(relFrame);
    const qreal yRad = mVerticalRadiusAnimator->getEffectiveYValue(relFrame);
    SkPath path;
    const auto p0 = center + QPointF(0, -yRad);
    const auto p1 = center + QPointF(xRad, 0);
    const auto p2 = center + QPointF(0, yRad);
    const auto p3 = center + QPointF(-xRad, 0);
    const qreal c = 0.551915024494;

    path.moveTo(toSkPoint(p0));
    path.cubicTo(toSkPoint(p0 + QPointF(c*xRad, 0)),
                 toSkPoint(p1 + QPointF(0, -c*yRad)),
                 toSkPoint(p1));
    path.cubicTo(toSkPoint(p1 + QPointF(0, c*yRad)),
                 toSkPoint(p2 + QPointF(c*xRad, 0)),
                 toSkPoint(p2));
    path.cubicTo(toSkPoint(p2 + QPointF(-c*xRad, 0)),
                 toSkPoint(p3 + QPointF(0, c*yRad)),
                 toSkPoint(p3));
    path.cubicTo(toSkPoint(p3 + QPointF(0, -c*yRad)),
                 toSkPoint(p0 + QPointF(-c*xRad, 0)),
                 toSkPoint(p0));
    path.close();
    return path;
}

qreal Circle::getCurrentXRadius() {
    return mHorizontalRadiusAnimator->getEffectiveXValue();
}

qreal Circle::getCurrentYRadius() {
    return mVerticalRadiusAnimator->getEffectiveYValue();
}

QPointFAnimator *Circle::getCenterAnimator()
{
    return mCenterAnimator.get();
}

QPointFAnimator *Circle::getHRadiusAnimator()
{
    return mHorizontalRadiusAnimator.get();
}

QPointFAnimator *Circle::getVRadiusAnimator()
{
    return mVerticalRadiusAnimator.get();
}

void Circle::getMotionBlurProperties(QList<Property*> &list) const {
    PathBox::getMotionBlurProperties(list);
    list.append(mHorizontalRadiusAnimator.get());
    list.append(mVerticalRadiusAnimator.get());
}

bool Circle::differenceInEditPathBetweenFrames(
        const int frame1, const int frame2) const {
    if(mCenterAnimator->prp_differencesBetweenRelFrames(frame1, frame2)) return true;
    if(mHorizontalRadiusAnimator->prp_differencesBetweenRelFrames(frame1, frame2)) return true;
    return mVerticalRadiusAnimator->prp_differencesBetweenRelFrames(frame1, frame2);
}

void Circle::setCenter(const QPointF &center) {
    mCenterAnimator->setBaseValue(center);
}


#include "simpletask.h"
#include "Animators/customproperties.h"
#include "Expressions/expression.h"
#include "svgexporter.h"

void Circle::saveSVG(SvgExporter& exp,
                     DomEleTask* const task) const
{
    auto& ele = task->initialize("ellipse");
    const auto cX = mCenterAnimator->getXAnimator();
    const auto cY = mCenterAnimator->getYAnimator();

    const auto rX = mHorizontalRadiusAnimator->getXAnimator();
    const auto rY = mVerticalRadiusAnimator->getYAnimator();

    cX->saveQrealSVG(exp, ele, task->visRange(), "cx");
    cY->saveQrealSVG(exp, ele, task->visRange(), "cy");
    rX->saveQrealSVG(exp, ele, task->visRange(), "rx");
    rY->saveQrealSVG(exp, ele, task->visRange(), "ry");

    {
        bool ok = false;
        const double value = ele.attribute("rx").toDouble(&ok);
        if (ok && value < 0.) { ele.setAttribute("rx",
                                                 QString::number(qAbs(value))); }
    }
    {
        bool ok = false;
        const double value = ele.attribute("ry").toDouble(&ok);
        if (ok && value < 0.) { ele.setAttribute("ry",
                                                 QString::number(qAbs(value))); }
    }
    {
        QDomNodeList children = ele.childNodes();
        for (int i = 0; i < children.count(); ++i) {
            QDomNode childNode = children.at(i);
            if (!childNode.isElement()) { continue; }
            QDomElement childElement = childNode.toElement();
            if (childElement.tagName() != "animate") { continue; }
            QString nameAttribute = childElement.attribute("attributeName");
            if (nameAttribute == "rx" || nameAttribute == "ry") {
                const QString valuesAttribute = childElement.attribute("values");
                const QStringList valuesList = valuesAttribute.split(';');

                QStringList newValuesList;
                for (const QString &valueStr : valuesList) {
                    const double value = valueStr.toDouble();
                    newValuesList.append(QString::number(qAbs(value)));
                }

                const QString newValuesAttribute = newValuesList.join(';');
                if (newValuesAttribute != valuesAttribute) {
                    childElement.setAttribute("values", newValuesAttribute);
                }
            }
        }
    }

    savePathBoxSVG(exp, ele, task->visRange());
}

CircleRadiusPoint::CircleRadiusPoint(QPointFAnimator * const associatedAnimator,
                                     BasicTransformAnimator * const parent,
                                     AnimatedPoint * const centerPoint,
                                     const MovablePointType &type,
                                     const bool blockX) :
    AnimatedPoint(associatedAnimator, type),
    mXBlocked(blockX), mCenterPoint(centerPoint) {
    setTransform(parent);
    disableSelection();
}

QPointF CircleRadiusPoint::getRelativePos() const {
    const QPointF centerPos = mCenterPoint->getRelativePos();
    return AnimatedPoint::getRelativePos() + centerPos;
}

void CircleRadiusPoint::setRelativePos(const QPointF &relPos) {
    const QPointF centerPos = mCenterPoint->getRelativePos();
    if(mXBlocked) {
        setValue(QPointF(0, relPos.y() - centerPos.y()));
    } else {
        setValue(QPointF(relPos.x() - centerPos.x(), 0));
    }
}
