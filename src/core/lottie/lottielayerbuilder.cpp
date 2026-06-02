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
*/

#include "lottie/lottielayerbuilder.h"

#include "Animators/coloranimator.h"
#include "Animators/paintsettingsanimator.h"
#include "Animators/qpointfanimator.h"
#include "Animators/qrealanimator.h"
#include "Animators/transformanimator.h"
#include "Boxes/boundingbox.h"
#include "Boxes/containerbox.h"
#include "Boxes/rectangle.h"
#include "canvas.h"
#include "paintsettings.h"

#include <QColor>
#include <QPointF>

LottieLayerBuilder::LottieLayerBuilder(Canvas* const scene,
                                       const FrameRange& frameRange,
                                       const qreal fps)
    : mScene(scene)
    , mFrameRange(frameRange)
    , mFps(fps)
{

}

QJsonArray LottieLayerBuilder::buildLayers(const bool background) const
{
    QJsonArray layers;
    if (!mScene) { return layers; }

    int nextId = background ? 2 : 1;
    appendContainerLayers(mScene, layers, nextId);
    if (background) { layers.append(buildBackgroundLayer()); }
    return layers;
}

QJsonObject LottieLayerBuilder::buildBackgroundLayer() const
{
    auto layer = baseLayer(QStringLiteral("Background"), 1, 1);
    layer.insert(QStringLiteral("sw"), mScene->getCanvasWidth());
    layer.insert(QStringLiteral("sh"), mScene->getCanvasHeight());

    QColor color(0, 0, 0);
    if (mScene->getBgColorAnimator()) {
        color = mScene->getBgColorAnimator()->getColor(mFrameRange.fMin);
    }
    layer.insert(QStringLiteral("sc"), color.name(QColor::HexRgb));
    return layer;
}

void LottieLayerBuilder::appendContainerLayers(const ContainerBox* const container,
                                               QJsonArray& layers,
                                               int& nextId) const
{
    const auto& boxes = container->getContainedBoxes();
    for (auto it = boxes.crbegin(); it != boxes.crend(); ++it) {
        const auto box = *it;
        if (!box) { continue; }

        const auto childContainer = dynamic_cast<const ContainerBox*>(box);
        if (childContainer) {
            appendContainerLayers(childContainer, layers, nextId);
            continue;
        }

        const auto rectangle = dynamic_cast<RectangleBox*>(box);
        if (rectangle) {
            layers.append(buildRectangleLayer(rectangle, nextId));
            nextId++;
            continue;
        }

        layers.append(buildUnsupportedLayer(box, nextId));
        nextId++;
    }
}

QJsonObject LottieLayerBuilder::buildRectangleLayer(RectangleBox* const box,
                                                    const int id) const
{
    auto layer = baseLayer(box->prp_getName(), id, 4);
    layer.insert(QStringLiteral("ks"), transformObject(box));

    const QPointF topLeft = box->getTopLeftAnimator()->getEffectiveValue(mFrameRange.fMin);
    const QPointF bottomRight = box->getBottomRightAnimator()->getEffectiveValue(mFrameRange.fMin);
    const QPointF radius = box->getRadiusAnimator()->getEffectiveValue(mFrameRange.fMin);

    const qreal x = qMin(topLeft.x(), bottomRight.x());
    const qreal y = qMin(topLeft.y(), bottomRight.y());
    const qreal width = qAbs(topLeft.x() - bottomRight.x());
    const qreal height = qAbs(topLeft.y() - bottomRight.y());

    QJsonObject rect;
    rect.insert(QStringLiteral("ty"), QStringLiteral("rc"));
    rect.insert(QStringLiteral("d"), 1);
    rect.insert(QStringLiteral("nm"), box->prp_getName());
    rect.insert(QStringLiteral("p"), staticProperty(QJsonArray{x + width*0.5,
                                                               y + height*0.5}));
    rect.insert(QStringLiteral("s"), staticProperty(QJsonArray{width, height}));
    rect.insert(QStringLiteral("r"), staticProperty(qMin(qAbs(radius.x()),
                                                        qAbs(radius.y()))));

    QColor fillColor(0, 0, 0, 0);
    qreal fillOpacity = 0;
    const auto fill = box->getFillSettings();
    if (fill && fill->getPaintType() == PaintType::FLATPAINT) {
        fillColor = fill->getColor(mFrameRange.fMin);
        fillOpacity = fillColor.alphaF()*100;
    }

    QJsonObject fillObject;
    fillObject.insert(QStringLiteral("ty"), QStringLiteral("fl"));
    fillObject.insert(QStringLiteral("c"), staticProperty(colorArray(fillColor)));
    fillObject.insert(QStringLiteral("o"), staticProperty(fillOpacity));
    fillObject.insert(QStringLiteral("r"), 1);
    fillObject.insert(QStringLiteral("bm"), 0);
    fillObject.insert(QStringLiteral("nm"), QStringLiteral("Fill"));

    QJsonObject shapeTransform;
    shapeTransform.insert(QStringLiteral("ty"), QStringLiteral("tr"));
    shapeTransform.insert(QStringLiteral("p"), staticProperty(QJsonArray{0, 0}));
    shapeTransform.insert(QStringLiteral("a"), staticProperty(QJsonArray{0, 0}));
    shapeTransform.insert(QStringLiteral("s"), staticProperty(QJsonArray{100, 100}));
    shapeTransform.insert(QStringLiteral("r"), staticProperty(0));
    shapeTransform.insert(QStringLiteral("o"), staticProperty(100));
    shapeTransform.insert(QStringLiteral("sk"), staticProperty(0));
    shapeTransform.insert(QStringLiteral("sa"), staticProperty(0));

    layer.insert(QStringLiteral("shapes"), QJsonArray{rect, fillObject, shapeTransform});
    return layer;
}

QJsonObject LottieLayerBuilder::buildUnsupportedLayer(const BoundingBox* const box,
                                                      const int id) const
{
    return baseLayer(box ? box->prp_getName() : QStringLiteral("Layer"),
                     id,
                     3);
}

QJsonObject LottieLayerBuilder::baseLayer(const QString& name,
                                          const int id,
                                          const int type) const
{
    QJsonObject layer;
    layer.insert(QStringLiteral("ddd"), 0);
    layer.insert(QStringLiteral("ind"), id);
    layer.insert(QStringLiteral("ty"), type);
    layer.insert(QStringLiteral("nm"), name);
    layer.insert(QStringLiteral("sr"), 1);
    layer.insert(QStringLiteral("ks"), transformObject());
    layer.insert(QStringLiteral("ip"), mFrameRange.fMin);
    layer.insert(QStringLiteral("op"), mFrameRange.fMax + 1);
    layer.insert(QStringLiteral("st"), 0);
    layer.insert(QStringLiteral("bm"), 0);
    Q_UNUSED(mFps)
    return layer;
}

QJsonObject LottieLayerBuilder::transformObject(const BoundingBox* const box) const
{
    if (box) {
        const auto transform = box->getBoxTransformAnimator();
        if (transform) {
            QList<QJsonArray> positions;
            QList<QJsonArray> scales;
            QList<QJsonArray> anchors;
            QList<qreal> rotations;
            QList<qreal> opacities;

            for (int frame = mFrameRange.fMin; frame <= mFrameRange.fMax; frame++) {
                const QPointF pos = transform->getPosAnimator()->getEffectiveValue(frame);
                const QPointF scale = transform->getScaleAnimator()->getEffectiveValue(frame);
                const QPointF pivot = transform->getPivotAnimator()->getEffectiveValue(frame);
                positions << QJsonArray{pos.x(), pos.y(), 0};
                scales << QJsonArray{scale.x()*100, scale.y()*100, 100};
                anchors << QJsonArray{pivot.x(), pivot.y(), 0};
                rotations << transform->getRotAnimator()->getEffectiveValue(frame);
                opacities << transform->getOpacityAnimator()->getEffectiveValue(frame);
            }

            QJsonObject animated;
            animated.insert(QStringLiteral("o"), animatedScalarProperty(opacities));
            animated.insert(QStringLiteral("r"), animatedScalarProperty(rotations));
            animated.insert(QStringLiteral("p"), animatedPointProperty(positions));
            animated.insert(QStringLiteral("a"), animatedPointProperty(anchors));
            animated.insert(QStringLiteral("s"), animatedPointProperty(scales));
            return animated;
        }
    }

    QJsonObject transform;
    transform.insert(QStringLiteral("o"), staticProperty(100));
    transform.insert(QStringLiteral("r"), staticProperty(0));
    transform.insert(QStringLiteral("p"), staticProperty(QJsonArray{0, 0, 0}));
    transform.insert(QStringLiteral("a"), staticProperty(QJsonArray{0, 0, 0}));
    transform.insert(QStringLiteral("s"), staticProperty(QJsonArray{100, 100, 100}));
    return transform;
}

QJsonObject LottieLayerBuilder::staticProperty(const QJsonValue& value) const
{
    return QJsonObject{
        {QStringLiteral("a"), 0},
        {QStringLiteral("k"), value}
    };
}

QJsonObject LottieLayerBuilder::animatedScalarProperty(const QList<qreal>& values) const
{
    if (values.isEmpty()) { return staticProperty(0); }
    if (sameScalarValues(values)) { return staticProperty(values.first()); }
    return QJsonObject{
        {QStringLiteral("a"), 1},
        {QStringLiteral("k"), scalarKeyframes(values)}
    };
}

QJsonObject LottieLayerBuilder::animatedPointProperty(const QList<QJsonArray>& values) const
{
    if (values.isEmpty()) { return staticProperty(QJsonArray()); }
    if (samePointValues(values)) { return staticProperty(values.first()); }
    return QJsonObject{
        {QStringLiteral("a"), 1},
        {QStringLiteral("k"), pointKeyframes(values)}
    };
}

QJsonArray LottieLayerBuilder::scalarKeyframes(const QList<qreal>& values) const
{
    QJsonArray keyframes;
    for (int i = 0; i < values.size(); i++) {
        QJsonObject key;
        key.insert(QStringLiteral("t"), mFrameRange.fMin + i);
        key.insert(QStringLiteral("s"), QJsonArray{values.at(i)});
        if (i + 1 < values.size()) {
            key.insert(QStringLiteral("e"), QJsonArray{values.at(i + 1)});
            key.insert(QStringLiteral("i"), keyframeEase());
            key.insert(QStringLiteral("o"), keyframeEase());
        }
        keyframes.append(key);
    }
    return keyframes;
}

QJsonArray LottieLayerBuilder::pointKeyframes(const QList<QJsonArray>& values) const
{
    QJsonArray keyframes;
    for (int i = 0; i < values.size(); i++) {
        QJsonObject key;
        key.insert(QStringLiteral("t"), mFrameRange.fMin + i);
        key.insert(QStringLiteral("s"), values.at(i));
        if (i + 1 < values.size()) {
            key.insert(QStringLiteral("e"), values.at(i + 1));
            key.insert(QStringLiteral("i"), keyframeEase());
            key.insert(QStringLiteral("o"), keyframeEase());
        }
        keyframes.append(key);
    }
    return keyframes;
}

bool LottieLayerBuilder::sameScalarValues(const QList<qreal>& values) const
{
    if (values.isEmpty()) { return true; }
    const qreal first = values.first();
    for (const qreal value : values) {
        if (!qFuzzyCompare(first + 1, value + 1)) { return false; }
    }
    return true;
}

bool LottieLayerBuilder::samePointValues(const QList<QJsonArray>& values) const
{
    if (values.isEmpty()) { return true; }
    const auto first = values.first();
    for (const auto& value : values) {
        if (value.size() != first.size()) { return false; }
        for (int i = 0; i < value.size(); i++) {
            if (!qFuzzyCompare(first.at(i).toDouble() + 1,
                               value.at(i).toDouble() + 1)) {
                return false;
            }
        }
    }
    return true;
}

QJsonObject LottieLayerBuilder::keyframeEase() const
{
    return QJsonObject{
        {QStringLiteral("x"), QJsonArray{0.667}},
        {QStringLiteral("y"), QJsonArray{1}}
    };
}

QJsonArray LottieLayerBuilder::colorArray(const QColor& color) const
{
    return QJsonArray{
        color.redF(),
        color.greenF(),
        color.blueF(),
        color.alphaF()
    };
}
