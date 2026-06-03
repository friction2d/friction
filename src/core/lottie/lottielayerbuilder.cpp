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
#include "Animators/gradient.h"
#include "Animators/gradientpoints.h"
#include "Animators/paintsettingsanimator.h"
#include "Animators/qpointfanimator.h"
#include "Animators/qrealanimator.h"
#include "Animators/transformanimator.h"
#include "Boxes/boundingbox.h"
#include "Boxes/containerbox.h"
#include "Boxes/pathbox.h"
#include "Boxes/rectangle.h"
#include "Boxes/textbox.h"
#include "canvas.h"
#include "lottie/lottieanimatedproperty.h"
#include "lottie/lottiepatheffects.h"
#include "lottie/lottierealkeyframes.h"
#include "paintsettings.h"
#include "simplemath.h"
#include "skia/skiaincludes.h"

#include <QColor>
#include <QPointF>
#include <QSet>

namespace {

QJsonObject lottieStaticProperty(const QJsonValue& value)
{
    return QJsonObject{
        {QStringLiteral("a"), 0},
        {QStringLiteral("k"), value}
    };
}

QJsonArray pointArray(const SkPoint& point)
{
    return QJsonArray{point.fX, point.fY};
}

QJsonArray tangentArray(const SkPoint& from, const SkPoint& to)
{
    return QJsonArray{to.fX - from.fX, to.fY - from.fY};
}

QJsonArray zeroTangent()
{
    return QJsonArray{0, 0};
}

struct LottieContour {
    QJsonArray vertices;
    QJsonArray inTangents;
    QJsonArray outTangents;
    bool closed = false;
};

void addMove(LottieContour& contour, const SkPoint& point)
{
    contour.vertices.append(pointArray(point));
    contour.inTangents.append(zeroTangent());
    contour.outTangents.append(zeroTangent());
}

void addLine(LottieContour& contour, const SkPoint& point)
{
    contour.vertices.append(pointArray(point));
    contour.inTangents.append(zeroTangent());
    contour.outTangents.append(zeroTangent());
}

void addCubic(LottieContour& contour,
              const SkPoint& cp1,
              const SkPoint& cp2,
              const SkPoint& end)
{
    if (contour.vertices.isEmpty()) { addMove(contour, end); return; }

    const int lastIndex = contour.vertices.size() - 1;
    const auto lastVertex = contour.vertices.at(lastIndex).toArray();
    const SkPoint start = SkPoint::Make(SkScalar(lastVertex.at(0).toDouble()),
                                        SkScalar(lastVertex.at(1).toDouble()));
    contour.outTangents.replace(lastIndex, tangentArray(start, cp1));
    contour.vertices.append(pointArray(end));
    contour.inTangents.append(tangentArray(end, cp2));
    contour.outTangents.append(zeroTangent());
}

void addQuad(LottieContour& contour,
             const SkPoint& control,
             const SkPoint& end)
{
    if (contour.vertices.isEmpty()) { addMove(contour, end); return; }

    const int lastIndex = contour.vertices.size() - 1;
    const auto lastVertex = contour.vertices.at(lastIndex).toArray();
    const SkPoint start = SkPoint::Make(SkScalar(lastVertex.at(0).toDouble()),
                                        SkScalar(lastVertex.at(1).toDouble()));
    const SkPoint cp1 = SkPoint::Make(start.fX + (control.fX - start.fX)*2/3,
                                      start.fY + (control.fY - start.fY)*2/3);
    const SkPoint cp2 = SkPoint::Make(end.fX + (control.fX - end.fX)*2/3,
                                      end.fY + (control.fY - end.fY)*2/3);
    addCubic(contour, cp1, cp2, end);
}

QJsonObject contourObject(const LottieContour& contour,
                          const QString& name,
                          const int index)
{
    QJsonObject path;
    path.insert(QStringLiteral("i"), contour.inTangents);
    path.insert(QStringLiteral("o"), contour.outTangents);
    path.insert(QStringLiteral("v"), contour.vertices);
    path.insert(QStringLiteral("c"), contour.closed);

    QJsonObject shape;
    shape.insert(QStringLiteral("ty"), QStringLiteral("sh"));
    shape.insert(QStringLiteral("ks"), lottieStaticProperty(path));
    shape.insert(QStringLiteral("nm"), QStringLiteral("%1 Path %2").arg(name).arg(index));
    shape.insert(QStringLiteral("ind"), index);
    return shape;
}

QJsonArray pathShapeObjects(const SkPath& path, const QString& name)
{
    QJsonArray shapes;
    QList<LottieContour> contours;
    LottieContour contour;
    SkPath::Iter iter(path, false);
    SkPoint pts[4];

    for (;;) {
        switch(iter.next(pts)) {
        case SkPath::kMove_Verb:
            if (!contour.vertices.isEmpty()) {
                contours.append(contour);
                contour = LottieContour();
            }
            addMove(contour, pts[0]);
            break;
        case SkPath::kLine_Verb:
            addLine(contour, pts[1]);
            break;
        case SkPath::kQuad_Verb:
            addQuad(contour, pts[1], pts[2]);
            break;
        case SkPath::kConic_Verb: {
            const SkScalar tol = SK_Scalar1 / 1024;
            SkAutoConicToQuads quadder;
            const SkPoint* quadPts = quadder.computeQuads(pts, iter.conicWeight(), tol);
            for (int i = 0; i < quadder.countQuads(); i++) {
                addQuad(contour, quadPts[i*2 + 1], quadPts[i*2 + 2]);
            }
            break;
        }
        case SkPath::kCubic_Verb:
            addCubic(contour, pts[1], pts[2], pts[3]);
            break;
        case SkPath::kClose_Verb:
            contour.closed = true;
            break;
        case SkPath::kDone_Verb:
            if (!contour.vertices.isEmpty()) { contours.append(contour); }
            for (int i = 0; i < contours.size(); i++) {
                shapes.append(contourObject(contours.at(i), name, i + 1));
            }
            return shapes;
        }
    }
}

}

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

QJsonObject LottieLayerBuilder::buildFonts() const
{
    QJsonArray fonts;
    QSet<QString> names;
    if (mScene) { appendFonts(mScene, fonts, names); }
    return QJsonObject{{QStringLiteral("list"), fonts}};
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
                                               int& nextId,
                                               const int parentId) const
{
    const auto& boxes = container->getContainedBoxes();
    for (const auto box : boxes) {
        if (!box) { continue; }

        const auto childContainer = dynamic_cast<const ContainerBox*>(box);
        if (childContainer) {
            const int groupId = nextId++;
            layers.append(buildContainerLayer(childContainer, groupId, parentId));
            appendContainerLayers(childContainer, layers, nextId, groupId);
            continue;
        }

        const auto rectangle = dynamic_cast<RectangleBox*>(box);
        if (rectangle) {
            auto layer = buildRectangleLayer(rectangle, nextId);
            assignParent(layer, parentId);
            layers.append(layer);
            nextId++;
            continue;
        }

        const auto text = dynamic_cast<TextBox*>(box);
        if (text && canBuildNativeTextLayer(text)) {
            auto layer = buildTextLayer(text, nextId);
            assignParent(layer, parentId);
            layers.append(layer);
            nextId++;
            continue;
        }

        const auto path = dynamic_cast<PathBox*>(box);
        if (path) {
            auto layer = buildPathLayer(path, nextId);
            assignParent(layer, parentId);
            layers.append(layer);
            nextId++;
            continue;
        }

        auto layer = buildUnsupportedLayer(box, nextId);
        assignParent(layer, parentId);
        layers.append(layer);
        nextId++;
    }
}

QJsonObject LottieLayerBuilder::buildContainerLayer(const ContainerBox* const box,
                                                    const int id,
                                                    const int parentId) const
{
    auto layer = baseLayer(box ? box->prp_getName() : QStringLiteral("Group"),
                           id,
                           3);
    layer.insert(QStringLiteral("ks"), transformObject(box));
    assignParent(layer, parentId);
    return layer;
}

QJsonObject LottieLayerBuilder::buildRectangleLayer(RectangleBox* const box,
                                                    const int id) const
{
    auto layer = baseLayer(box->prp_getName(), id, 4);
    layer.insert(QStringLiteral("ks"), transformObject(box));

    QList<QJsonArray> centers;
    QList<QJsonArray> sizes;
    QList<qreal> radii;
    for (int frame = mFrameRange.fMin; frame <= mFrameRange.fMax; frame++) {
        const QPointF topLeft = box->getTopLeftAnimator()->getEffectiveValue(frame);
        const QPointF bottomRight = box->getBottomRightAnimator()->getEffectiveValue(frame);
        const QPointF radius = box->getRadiusAnimator()->getEffectiveValue(frame);

        const qreal x = qMin(topLeft.x(), bottomRight.x());
        const qreal y = qMin(topLeft.y(), bottomRight.y());
        const qreal width = qAbs(topLeft.x() - bottomRight.x());
        const qreal height = qAbs(topLeft.y() - bottomRight.y());

        centers << QJsonArray{x + width*0.5, y + height*0.5};
        sizes << QJsonArray{width, height};
        radii << qMin(qAbs(radius.x()), qAbs(radius.y()));
    }

    QJsonObject rect;
    rect.insert(QStringLiteral("ty"), QStringLiteral("rc"));
    rect.insert(QStringLiteral("d"), 1);
    rect.insert(QStringLiteral("nm"), box->prp_getName());
    rect.insert(QStringLiteral("p"), animatedPointProperty(centers));
    rect.insert(QStringLiteral("s"), animatedPointProperty(sizes));
    rect.insert(QStringLiteral("r"), animatedScalarProperty(radii));

    QJsonArray shapes{rect};
    LottiePathEffects::appendBasePathEffects(box, mFrameRange, shapes);
    appendPaintObjects(box, shapes);
    shapes.append(shapeTransformObject());

    layer.insert(QStringLiteral("shapes"), shapes);
    return layer;
}

QJsonObject LottieLayerBuilder::buildTextLayer(TextBox* const box,
                                               const int id) const
{
    auto layer = baseLayer(box->prp_getName(), id, 5);
    layer.insert(QStringLiteral("ks"), transformObject(box));

    QColor fillColor(0, 0, 0);
    const auto fill = box->getFillSettings();
    if (fill && fill->getPaintType() == PaintType::FLATPAINT) {
        fillColor = fill->getColor(mFrameRange.fMin);
    }

    QJsonObject document;
    document.insert(QStringLiteral("s"), box->getFontSize());
    document.insert(QStringLiteral("f"), fontName(box));
    document.insert(QStringLiteral("t"), box->getCurrentValue());
    document.insert(QStringLiteral("j"), 0);
    document.insert(QStringLiteral("tr"), 0);
    document.insert(QStringLiteral("lh"), box->getFontSize()*1.2);
    document.insert(QStringLiteral("ls"), 0);
    document.insert(QStringLiteral("sz"), QJsonArray{mScene ? mScene->getCanvasWidth() : 0,
                                                     mScene ? mScene->getCanvasHeight() : 0});
    document.insert(QStringLiteral("ps"), QJsonArray{0, -box->getFontSize()*0.75});
    document.insert(QStringLiteral("fc"), QJsonArray{fillColor.redF(),
                                                     fillColor.greenF(),
                                                     fillColor.blueF()});

    const auto stroke = box->getStrokeSettings();
    if (stroke &&
        stroke->getPaintType() == PaintType::FLATPAINT &&
        !isZero4Dec(stroke->getLineWidthAnimator()->getEffectiveValue(mFrameRange.fMin))) {
        const QColor strokeColor = stroke->getColor(mFrameRange.fMin);
        document.insert(QStringLiteral("sc"), QJsonArray{strokeColor.redF(),
                                                         strokeColor.greenF(),
                                                         strokeColor.blueF()});
        document.insert(QStringLiteral("sw"),
                        stroke->getLineWidthAnimator()->getEffectiveValue(mFrameRange.fMin));
    }

    QJsonObject documentKey;
    documentKey.insert(QStringLiteral("s"), document);
    documentKey.insert(QStringLiteral("t"), mFrameRange.fMin);

    QJsonObject textData;
    textData.insert(QStringLiteral("d"), QJsonObject{
                        {QStringLiteral("k"), QJsonArray{documentKey}}
                    });
    textData.insert(QStringLiteral("p"), QJsonObject());
    textData.insert(QStringLiteral("m"), QJsonObject{
                        {QStringLiteral("g"), 1},
                        {QStringLiteral("a"), staticProperty(QJsonArray{0, 0})}
                    });
    textData.insert(QStringLiteral("a"), QJsonArray());

    layer.insert(QStringLiteral("t"), textData);
    return layer;
}

QJsonObject LottieLayerBuilder::buildPathLayer(PathBox* const box,
                                               const int id) const
{
    auto layer = baseLayer(box->prp_getName(), id, 4);
    layer.insert(QStringLiteral("ks"), transformObject(box));

    QJsonArray shapes = pathShapeObjects(box->getRelativePath(mFrameRange.fMin),
                                         box->prp_getName());
    LottiePathEffects::appendBasePathEffects(box, mFrameRange, shapes);
    appendPaintObjects(box, shapes);
    shapes.append(shapeTransformObject());

    layer.insert(QStringLiteral("shapes"), shapes);
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

void LottieLayerBuilder::assignParent(QJsonObject& layer, const int parentId) const
{
    if (parentId > 0) {
        layer.insert(QStringLiteral("parent"), parentId);
    }
}

QJsonObject LottieLayerBuilder::transformObject(const BoundingBox* const box) const
{
    if (box) {
        const auto transform = box->getBoxTransformAnimator();
        if (transform) {
            const auto pivot = transform->getPivotAnimator();
            const auto pivotStatic = pivot &&
                    !pivot->getXAnimator()->anim_hasKeys() &&
                    !pivot->getXAnimator()->hasExpression() &&
                    !pivot->getYAnimator()->anim_hasKeys() &&
                    !pivot->getYAnimator()->hasExpression();
            const QPointF staticPivot = pivotStatic ?
                        pivot->getEffectiveValue(mFrameRange.fMin) :
                        QPointF(0, 0);

            QList<QJsonArray> positions;
            QList<QJsonArray> scales;
            QList<QJsonArray> anchors;
            QList<qreal> rotations;
            QList<qreal> opacities;

            for (int frame = mFrameRange.fMin; frame <= mFrameRange.fMax; frame++) {
                const QPointF pos = transform->getPosAnimator()->getEffectiveValue(frame);
                const QPointF scale = transform->getScaleAnimator()->getEffectiveValue(frame);
                const QPointF pivot = transform->getPivotAnimator()->getEffectiveValue(frame);
                positions << QJsonArray{pos.x() + pivot.x(),
                                         pos.y() + pivot.y(),
                                         0};
                scales << QJsonArray{scale.x()*100, scale.y()*100, 100};
                anchors << QJsonArray{pivot.x(), pivot.y(), 0};
                rotations << transform->getRotAnimator()->getEffectiveValue(frame);
                opacities << transform->getOpacityAnimator()->getEffectiveValue(frame);
            }

            QJsonObject animated;
            const auto opacityReal = LottieRealKeyframes::scalar(
                        transform->getOpacityAnimator(), mFrameRange);
            animated.insert(QStringLiteral("o"),
                            opacityReal.isEmpty() ? animatedScalarProperty(opacities) : opacityReal);

            const auto rotationReal = LottieRealKeyframes::scalar(
                        transform->getRotAnimator(), mFrameRange);
            animated.insert(QStringLiteral("r"),
                            rotationReal.isEmpty() ? animatedScalarProperty(rotations) : rotationReal);

            QJsonObject positionReal;
            if (pivotStatic) {
                positionReal = LottieRealKeyframes::point(
                            transform->getPosAnimator(), mFrameRange,
                            [staticPivot](const QPointF& point, const qreal) {
                                return QJsonArray{point.x() + staticPivot.x(),
                                                  point.y() + staticPivot.y(),
                                                  0};
                            });
            }
            animated.insert(QStringLiteral("p"),
                            positionReal.isEmpty() ? animatedPointProperty(positions) : positionReal);

            const auto anchorReal = LottieRealKeyframes::point(
                        transform->getPivotAnimator(), mFrameRange,
                        [](const QPointF& point, const qreal) {
                            return QJsonArray{point.x(), point.y(), 0};
                        });
            animated.insert(QStringLiteral("a"),
                            anchorReal.isEmpty() ? animatedPointProperty(anchors) : anchorReal);

            const auto scaleReal = LottieRealKeyframes::point(
                        transform->getScaleAnimator(), mFrameRange,
                        [](const QPointF& point, const qreal) {
                            return QJsonArray{point.x()*100, point.y()*100, 100};
                        });
            animated.insert(QStringLiteral("s"),
                            scaleReal.isEmpty() ? animatedPointProperty(scales) : scaleReal);
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
    return LottieAnimatedProperty::staticProperty(value);
}

QJsonObject LottieLayerBuilder::animatedScalarProperty(const QList<qreal>& values) const
{
    return LottieAnimatedProperty::scalar(values, mFrameRange);
}

QJsonObject LottieLayerBuilder::animatedPointProperty(const QList<QJsonArray>& values) const
{
    return LottieAnimatedProperty::point(values, mFrameRange);
}

void LottieLayerBuilder::appendPaintObjects(const PathBox* const box,
                                            QJsonArray& shapes) const
{
    const auto fill = box->getFillSettings();
    if (fill && fill->getPaintType() == PaintType::FLATPAINT) {
        shapes.append(fillObject(box));
    } else if (fill && fill->getPaintType() == PaintType::GRADIENTPAINT) {
        shapes.append(gradientFillObject(box));
    }

    const auto stroke = box->getStrokeSettings();
    if (stroke &&
        !isZero4Dec(stroke->getLineWidthAnimator()->getEffectiveValue(mFrameRange.fMin))) {
        if (stroke->getPaintType() == PaintType::FLATPAINT) {
            shapes.append(strokeObject(box));
        } else if (stroke->getPaintType() == PaintType::GRADIENTPAINT) {
            shapes.append(gradientStrokeObject(box));
        }
    }
}

QJsonObject LottieLayerBuilder::fillObject(const PathBox* const box) const
{
    QList<QJsonArray> fillColors;
    QList<qreal> fillOpacities;
    const auto fill = box->getFillSettings();
    for (int frame = mFrameRange.fMin; frame <= mFrameRange.fMax; frame++) {
        QColor fillColor(0, 0, 0, 0);
        if (fill) { fillColor = fill->getColor(frame); }
        fillColors << colorArray(fillColor);
        fillOpacities << fillColor.alphaF()*100;
    }

    QJsonObject object;
    object.insert(QStringLiteral("ty"), QStringLiteral("fl"));
    const auto colorReal = fill ?
                LottieRealKeyframes::color(fill->getColorAnimator(), mFrameRange, true) :
                QJsonObject();
    object.insert(QStringLiteral("c"),
                  colorReal.isEmpty() ? animatedPointProperty(fillColors) : colorReal);

    const auto opacityReal = fill && fill->getColorAnimator() ?
                LottieRealKeyframes::scalar(
                    fill->getColorAnimator()->getAlphaAnimator(),
                    mFrameRange,
                    [](const qreal value) { return value*100; }) :
                QJsonObject();
    object.insert(QStringLiteral("o"),
                  opacityReal.isEmpty() ? animatedScalarProperty(fillOpacities) : opacityReal);
    object.insert(QStringLiteral("r"), 1);
    object.insert(QStringLiteral("bm"), 0);
    object.insert(QStringLiteral("nm"), QStringLiteral("Fill"));
    return object;
}

QJsonObject LottieLayerBuilder::gradientFillObject(const PathBox* const box) const
{
    return gradientObject(box ? box->getFillSettings() : nullptr,
                          QStringLiteral("Gradient Fill"),
                          false);
}

QJsonObject LottieLayerBuilder::strokeObject(const PathBox* const box) const
{
    QList<QJsonArray> strokeColors;
    QList<qreal> strokeOpacities;
    QList<qreal> strokeWidths;
    int lineCap = 2;
    int lineJoin = 2;

    const auto stroke = box->getStrokeSettings();
    if (stroke) {
        switch(stroke->getCapStyle()) {
        case SkPaint::kButt_Cap: lineCap = 1; break;
        case SkPaint::kSquare_Cap: lineCap = 3; break;
        default: lineCap = 2; break;
        }

        switch(stroke->getJoinStyle()) {
        case SkPaint::kMiter_Join: lineJoin = 1; break;
        case SkPaint::kBevel_Join: lineJoin = 3; break;
        default: lineJoin = 2; break;
        }
    }

    for (int frame = mFrameRange.fMin; frame <= mFrameRange.fMax; frame++) {
        QColor strokeColor(0, 0, 0, 0);
        qreal strokeWidth = 0;
        if (stroke) {
            strokeColor = stroke->getColor(frame);
            strokeWidth = stroke->getLineWidthAnimator()->getEffectiveValue(frame);
        }
        strokeColors << colorArray(strokeColor);
        strokeOpacities << strokeColor.alphaF()*100;
        strokeWidths << strokeWidth;
    }

    QJsonObject object;
    object.insert(QStringLiteral("ty"), QStringLiteral("st"));
    const auto colorReal = stroke ?
                LottieRealKeyframes::color(stroke->getColorAnimator(), mFrameRange, true) :
                QJsonObject();
    object.insert(QStringLiteral("c"),
                  colorReal.isEmpty() ? animatedPointProperty(strokeColors) : colorReal);

    const auto opacityReal = stroke && stroke->getColorAnimator() ?
                LottieRealKeyframes::scalar(
                    stroke->getColorAnimator()->getAlphaAnimator(),
                    mFrameRange,
                    [](const qreal value) { return value*100; }) :
                QJsonObject();
    object.insert(QStringLiteral("o"),
                  opacityReal.isEmpty() ? animatedScalarProperty(strokeOpacities) : opacityReal);

    const auto widthReal = stroke ?
                LottieRealKeyframes::scalar(stroke->getLineWidthAnimator(), mFrameRange) :
                QJsonObject();
    object.insert(QStringLiteral("w"),
                  widthReal.isEmpty() ? animatedScalarProperty(strokeWidths) : widthReal);
    object.insert(QStringLiteral("lc"), lineCap);
    object.insert(QStringLiteral("lj"), lineJoin);
    object.insert(QStringLiteral("ml"), 4);
    object.insert(QStringLiteral("bm"), 0);
    object.insert(QStringLiteral("nm"), QStringLiteral("Stroke"));
    LottiePathEffects::appendStrokeDash(box, mFrameRange, object);
    return object;
}

QJsonObject LottieLayerBuilder::gradientStrokeObject(const PathBox* const box) const
{
    auto object = gradientObject(box ? box->getStrokeSettings() : nullptr,
                                 QStringLiteral("Gradient Stroke"),
                                 true);
    LottiePathEffects::appendStrokeDash(box, mFrameRange, object);
    return object;
}

QJsonObject LottieLayerBuilder::gradientObject(PaintSettingsAnimator* const settings,
                                               const QString& name,
                                               const bool stroke) const
{
    const int stopCount = gradientStopCount(settings);
    const auto outline = dynamic_cast<OutlineSettingsAnimator*>(settings);
    QList<QJsonArray> colors;
    QList<QJsonArray> starts;
    QList<QJsonArray> ends;
    QList<qreal> opacities;
    QList<qreal> widths;
    int lineCap = 2;
    int lineJoin = 2;

    if (outline) {
        switch(outline->getCapStyle()) {
        case SkPaint::kButt_Cap: lineCap = 1; break;
        case SkPaint::kSquare_Cap: lineCap = 3; break;
        default: lineCap = 2; break;
        }

        switch(outline->getJoinStyle()) {
        case SkPaint::kMiter_Join: lineJoin = 1; break;
        case SkPaint::kBevel_Join: lineJoin = 3; break;
        default: lineJoin = 2; break;
        }
    }

    for (int frame = mFrameRange.fMin; frame <= mFrameRange.fMax; frame++) {
        colors << gradientColorArray(settings, frame, stopCount);

        QPointF startPoint(0, 0);
        QPointF endPoint(0, 0);
        if (settings && settings->getGradientPoints()) {
            startPoint = settings->getGradientPoints()->getStartPoint(frame);
            endPoint = settings->getGradientPoints()->getEndPoint(frame);
        }
        starts << QJsonArray{startPoint.x(), startPoint.y()};
        ends << QJsonArray{endPoint.x(), endPoint.y()};
        opacities << 100;
        widths << (outline ? outline->getLineWidthAnimator()->getEffectiveValue(frame) : 0);
    }

    QJsonObject object;
    object.insert(QStringLiteral("ty"), stroke ? QStringLiteral("gs") : QStringLiteral("gf"));
    object.insert(QStringLiteral("o"), animatedScalarProperty(opacities));
    object.insert(QStringLiteral("r"), 1);
    object.insert(QStringLiteral("bm"), 0);
    const auto gradientReal = settings ?
                LottieRealKeyframes::gradientColors(
                    settings->getGradient(),
                    stopCount,
                    mFrameRange) :
                QJsonObject();
    object.insert(QStringLiteral("g"), QJsonObject{
                      {QStringLiteral("p"), stopCount},
                      {QStringLiteral("k"),
                       gradientReal.isEmpty() ?
                           animatedPointProperty(colors) :
                           gradientReal}
                  });
    const auto startReal = settings && settings->getGradientPoints() ?
                LottieRealKeyframes::point(
                    settings->getGradientPoints()->startAnimator(),
                    mFrameRange,
                    [](const QPointF& point, const qreal) {
                        return QJsonArray{point.x(), point.y()};
                    }) :
                QJsonObject();
    object.insert(QStringLiteral("s"),
                  startReal.isEmpty() ? animatedPointProperty(starts) : startReal);

    const auto endReal = settings && settings->getGradientPoints() ?
                LottieRealKeyframes::point(
                    settings->getGradientPoints()->endAnimator(),
                    mFrameRange,
                    [](const QPointF& point, const qreal) {
                        return QJsonArray{point.x(), point.y()};
                    }) :
                QJsonObject();
    object.insert(QStringLiteral("e"),
                  endReal.isEmpty() ? animatedPointProperty(ends) : endReal);
    object.insert(QStringLiteral("t"),
                  settings &&
                  settings->getGradientType() == GradientType::RADIAL ? 2 : 1);
    object.insert(QStringLiteral("nm"), name);

    if (stroke) {
        const auto widthReal = outline ?
                    LottieRealKeyframes::scalar(outline->getLineWidthAnimator(), mFrameRange) :
                    QJsonObject();
        object.insert(QStringLiteral("w"),
                      widthReal.isEmpty() ? animatedScalarProperty(widths) : widthReal);
        object.insert(QStringLiteral("lc"), lineCap);
        object.insert(QStringLiteral("lj"), lineJoin);
        object.insert(QStringLiteral("ml"), 4);
    }

    return object;
}

QJsonArray LottieLayerBuilder::gradientColorArray(PaintSettingsAnimator* const settings,
                                                  const int frame,
                                                  const int stopCount) const
{
    QJsonArray values;
    const auto gradient = settings ? settings->getGradient() : nullptr;
    QGradientStops stops = gradient ?
                gradient->getQGradientStops(settings->prp_relFrameToAbsFrameF(frame)) :
                QGradientStops{{0, QColor(0, 0, 0)}};

    if (stops.isEmpty()) { stops << QGradientStop(0, QColor(0, 0, 0)); }
    while (stops.size() < stopCount) {
        const qreal position = stopCount <= 1 ? 0 : qreal(stops.size())/(stopCount - 1);
        stops << QGradientStop(position, stops.last().second);
    }
    while (stops.size() > stopCount) { stops.removeLast(); }

    for (const auto& stop : stops) {
        values.append(stop.first);
        values.append(stop.second.redF());
        values.append(stop.second.greenF());
        values.append(stop.second.blueF());
    }

    for (const auto& stop : stops) {
        values.append(stop.first);
        values.append(stop.second.alphaF());
    }
    return values;
}

int LottieLayerBuilder::gradientStopCount(PaintSettingsAnimator* const settings) const
{
    const auto gradient = settings ? settings->getGradient() : nullptr;
    const int stops = gradient ? gradient->getQGradientStops().size() : 0;
    return qMax(2, stops);
}

QJsonObject LottieLayerBuilder::shapeTransformObject() const
{
    QJsonObject shapeTransform;
    shapeTransform.insert(QStringLiteral("ty"), QStringLiteral("tr"));
    shapeTransform.insert(QStringLiteral("p"), staticProperty(QJsonArray{0, 0}));
    shapeTransform.insert(QStringLiteral("a"), staticProperty(QJsonArray{0, 0}));
    shapeTransform.insert(QStringLiteral("s"), staticProperty(QJsonArray{100, 100}));
    shapeTransform.insert(QStringLiteral("r"), staticProperty(0));
    shapeTransform.insert(QStringLiteral("o"), staticProperty(100));
    shapeTransform.insert(QStringLiteral("sk"), staticProperty(0));
    shapeTransform.insert(QStringLiteral("sa"), staticProperty(0));
    return shapeTransform;
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

void LottieLayerBuilder::appendFonts(const ContainerBox* const container,
                                     QJsonArray& fonts,
                                     QSet<QString>& names) const
{
    if (!container) { return; }

    const auto& boxes = container->getContainedBoxes();
    for (const auto box : boxes) {
        if (!box) { continue; }

        const auto text = dynamic_cast<const TextBox*>(box);
        if (text && canBuildNativeTextLayer(text)) {
            const QString name = fontName(text);
            if (!names.contains(name)) {
                names.insert(name);
                fonts.append(fontObject(text));
            }
        }

        const auto childContainer = dynamic_cast<const ContainerBox*>(box);
        if (childContainer) {
            appendFonts(childContainer, fonts, names);
        }
    }
}

QJsonObject LottieLayerBuilder::fontObject(const TextBox* const box) const
{
    return QJsonObject{
        {QStringLiteral("fName"), fontName(box)},
        {QStringLiteral("fFamily"), box ? box->getFontFamily() : QStringLiteral("Sans")},
        {QStringLiteral("fStyle"), fontStyleName(box)},
        {QStringLiteral("ascent"), 75}
    };
}

QString LottieLayerBuilder::fontName(const TextBox* const box) const
{
    const QString family = box ? box->getFontFamily() : QStringLiteral("Sans");
    const QString style = fontStyleName(box);
    QString name = style == QStringLiteral("Regular") ?
                family :
                QStringLiteral("%1-%2").arg(family, style);
    return name.replace(QLatin1Char(' '), QLatin1Char('_'));
}

QString LottieLayerBuilder::fontStyleName(const TextBox* const box) const
{
    if (!box) { return QStringLiteral("Regular"); }

    const auto& style = box->getFontStyle();
    const bool bold = style.weight() >= SkFontStyle::kSemiBold_Weight;
    const bool italic = style.slant() != SkFontStyle::kUpright_Slant;

    if (bold && italic) { return QStringLiteral("BoldItalic"); }
    if (bold) { return QStringLiteral("Bold"); }
    if (italic) { return QStringLiteral("Italic"); }
    return QStringLiteral("Regular");
}

bool LottieLayerBuilder::canBuildNativeTextLayer(const TextBox* const box) const
{
    return box &&
           !box->hasTextEffects() &&
           !box->hasBasePathEffects() &&
           !box->hasFillEffects() &&
           !box->hasOutlineBaseEffects() &&
           !box->hasOutlineEffects();
}
