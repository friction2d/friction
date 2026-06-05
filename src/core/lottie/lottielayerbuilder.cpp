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
#include "Boxes/imagebox.h"
#include "Boxes/pathbox.h"
#include "Boxes/rectangle.h"
#include "Boxes/textbox.h"
#include "canvas.h"
#include "lottie/lottieanimatedproperty.h"
#include "lottie/lottieblendmode.h"
#include "lottie/lottiepatheffects.h"
#include "lottie/lottierealkeyframes.h"
#include "paintsettings.h"
#include "simplemath.h"
#include "skia/skiaincludes.h"

#include <QColor>
#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPointF>
#include <QSet>
#include <algorithm>
#include <limits>

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

sk_sp<SkImage> imageForBox(const ImageBox* const box)
{
    if (!box) { return nullptr; }
    if (box->hasImage()) { return box->image(); }

    const QString path = box->filePath();
    if (path.isEmpty() || !QFileInfo::exists(path)) { return nullptr; }

    const sk_sp<SkData> data = SkData::MakeFromFileName(path.toUtf8().constData());
    return data ? SkImage::MakeFromEncoded(data) : nullptr;
}

sk_sp<SkData> pngData(const sk_sp<SkImage>& image)
{
    if (!image) { return nullptr; }
    return image->encodeToData(SkEncodedImageFormat::kPNG, 100);
}

QString pngDataUri(const sk_sp<SkImage>& image)
{
    const sk_sp<SkData> png = pngData(image);
    if (!png) { return QString(); }

    const QByteArray bytes(static_cast<const char*>(png->data()), png->size());
    return QStringLiteral("data:image/png;base64,%1")
            .arg(QString::fromLatin1(bytes.toBase64()));
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

QJsonObject contourPathObject(const LottieContour& contour)
{
    return QJsonObject{
        {QStringLiteral("i"), contour.inTangents},
        {QStringLiteral("o"), contour.outTangents},
        {QStringLiteral("v"), contour.vertices},
        {QStringLiteral("c"), contour.closed}
    };
}

QList<LottieContour> pathContours(const SkPath& path)
{
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
            return contours;
        }
    }
}

bool compatibleContours(const QList<LottieContour>& a,
                        const QList<LottieContour>& b)
{
    if (a.size() != b.size()) { return false; }
    for (int i = 0; i < a.size(); i++) {
        if (a.at(i).closed != b.at(i).closed) { return false; }
        if (a.at(i).vertices.size() != b.at(i).vertices.size()) { return false; }
        if (a.at(i).inTangents.size() != b.at(i).inTangents.size()) { return false; }
        if (a.at(i).outTangents.size() != b.at(i).outTangents.size()) { return false; }
    }
    return true;
}

qreal lottiePointValue(const QJsonArray& point,
                       const int component)
{
    return point.at(component).toDouble();
}

qreal interpolated(const qreal start,
                   const qreal end,
                   const qreal progress)
{
    return start + (end - start)*progress;
}

qreal contourPointArrayError(const QJsonArray& start,
                             const QJsonArray& end,
                             const QJsonArray& value,
                             const qreal progress)
{
    qreal error = 0;
    for (int pointIndex = 0; pointIndex < value.size(); pointIndex++) {
        const auto startPoint = start.at(pointIndex).toArray();
        const auto endPoint = end.at(pointIndex).toArray();
        const auto valuePoint = value.at(pointIndex).toArray();
        for (int component = 0; component < valuePoint.size(); component++) {
            const qreal expected = interpolated(lottiePointValue(startPoint, component),
                                               lottiePointValue(endPoint, component),
                                               progress);
            error = qMax(error,
                         qAbs(lottiePointValue(valuePoint, component) - expected));
        }
    }
    return error;
}

qreal contourError(const QList<QList<LottieContour>>& frames,
                   const int contourIndex,
                   const int start,
                   const int end,
                   int& worstIndex)
{
    qreal worstError = 0;
    worstIndex = -1;
    const int span = end - start;
    if (span <= 1) { return worstError; }

    const auto& startContour = frames.at(start).at(contourIndex);
    const auto& endContour = frames.at(end).at(contourIndex);
    for (int frameIndex = start + 1; frameIndex < end; frameIndex++) {
        const qreal progress = qreal(frameIndex - start)/span;
        const auto& contour = frames.at(frameIndex).at(contourIndex);
        qreal error = contourPointArrayError(startContour.vertices,
                                             endContour.vertices,
                                             contour.vertices,
                                             progress);
        error = qMax(error, contourPointArrayError(startContour.inTangents,
                                                   endContour.inTangents,
                                                   contour.inTangents,
                                                   progress));
        error = qMax(error, contourPointArrayError(startContour.outTangents,
                                                   endContour.outTangents,
                                                   contour.outTangents,
                                                   progress));
        if (error > worstError) {
            worstError = error;
            worstIndex = frameIndex;
        }
    }
    return worstError;
}

void simplifyContourRange(const QList<QList<LottieContour>>& frames,
                          const int contourIndex,
                          const int start,
                          const int end,
                          const qreal tolerance,
                          QSet<int>& indices)
{
    int worstIndex = -1;
    const qreal error = contourError(frames, contourIndex, start, end, worstIndex);
    if (worstIndex < 0 || error <= tolerance) { return; }

    indices.insert(worstIndex);
    simplifyContourRange(frames, contourIndex, start, worstIndex, tolerance, indices);
    simplifyContourRange(frames, contourIndex, worstIndex, end, tolerance, indices);
}

QList<int> simplifiedContourIndices(const QList<QList<LottieContour>>& frames,
                                    const int contourIndex)
{
    QList<int> result;
    if (frames.isEmpty()) { return result; }
    if (frames.size() == 1) { return QList<int>{0}; }

    constexpr qreal tolerance = 0.25;
    QSet<int> indices{0, frames.size() - 1};
    simplifyContourRange(frames,
                         contourIndex,
                         0,
                         frames.size() - 1,
                         tolerance,
                         indices);
    result = indices.values();
    std::sort(result.begin(), result.end());
    return result;
}

QJsonObject shapeInEase()
{
    return QJsonObject{
        {QStringLiteral("x"), QJsonArray{0.833}},
        {QStringLiteral("y"), QJsonArray{0.833}}
    };
}

QJsonObject shapeOutEase()
{
    return QJsonObject{
        {QStringLiteral("x"), QJsonArray{0.167}},
        {QStringLiteral("y"), QJsonArray{0.167}}
    };
}

QJsonArray pathShapeObjects(const QList<QList<LottieContour>>& frames,
                            const QString& name,
                            const FrameRange& frameRange)
{
    QJsonArray shapes;
    if (frames.isEmpty()) { return shapes; }

    const auto& firstContours = frames.first();
    const bool animated = frames.size() > 1;
    for (int contourIndex = 0; contourIndex < firstContours.size(); contourIndex++) {
        QJsonObject shape;
        shape.insert(QStringLiteral("ty"), QStringLiteral("sh"));
        shape.insert(QStringLiteral("nm"),
                     QStringLiteral("%1 Path %2").arg(name).arg(contourIndex + 1));
        shape.insert(QStringLiteral("ind"), contourIndex + 1);

        if (animated) {
            QJsonArray keys;
            const QList<int> keyFrameIndices = simplifiedContourIndices(frames, contourIndex);

            for (int keyIndex = 0; keyIndex < keyFrameIndices.size(); keyIndex++) {
                const int frameIndex = keyFrameIndices.at(keyIndex);
                const QJsonObject path = contourPathObject(frames.at(frameIndex).at(contourIndex));

                QJsonObject key;
                key.insert(QStringLiteral("t"), frameRange.fMin + frameIndex);
                key.insert(QStringLiteral("s"), QJsonArray{path});
                if (keyIndex + 1 < keyFrameIndices.size()) {
                    const int nextFrameIndex = keyFrameIndices.at(keyIndex + 1);
                    const QJsonObject nextPath =
                            contourPathObject(frames.at(nextFrameIndex).at(contourIndex));
                    key.insert(QStringLiteral("e"), QJsonArray{nextPath});
                    key.insert(QStringLiteral("i"), shapeInEase());
                    key.insert(QStringLiteral("o"), shapeOutEase());
                }
                keys.append(key);
            }

            if (keys.size() > 1) {
                shape.insert(QStringLiteral("ks"), QJsonObject{
                                 {QStringLiteral("a"), 1},
                                 {QStringLiteral("k"), keys}
                             });
            } else {
                const int frameIndex = keyFrameIndices.isEmpty() ? 0 : keyFrameIndices.first();
                shape.insert(QStringLiteral("ks"),
                             lottieStaticProperty(
                                 contourPathObject(frames.at(frameIndex).at(contourIndex))));
            }
        } else {
            shape.insert(QStringLiteral("ks"),
                         lottieStaticProperty(contourPathObject(firstContours.at(contourIndex))));
        }
        shapes.append(shape);
    }
    return shapes;
}

QJsonArray pathShapeObjects(const SkPath& path, const QString& name)
{
    return pathShapeObjects(QList<QList<LottieContour>>{pathContours(path)},
                            name,
                            FrameRange{0, 0});
}

}

LottieLayerBuilder::LottieLayerBuilder(Canvas* const scene,
                                       const FrameRange& frameRange,
                                       const qreal fps,
                                       const QString& path,
                                       const bool embedImages,
                                       const bool svgRendererFix,
                                       const bool nativeText)
    : mScene(scene)
    , mFrameRange(frameRange)
    , mFps(fps)
    , mPath(path)
    , mEmbedImages(embedImages)
    , mSvgRendererFix(svgRendererFix)
    , mNativeText(nativeText)
{

}

QJsonArray LottieLayerBuilder::buildLayers(const bool background) const
{
    QJsonArray layers;
    if (!mScene) { return layers; }

    int nextId = background ? 2 : 1;
    appendContainerLayers(mScene, layers, nextId);
    if (background) { layers.append(buildBackgroundLayer()); }
    if (mSvgRendererFix) { layers.append(buildSvgRendererFixLayer(nextId)); }
    return layers;
}

QJsonArray LottieLayerBuilder::buildAssets() const
{
    QJsonArray assets;
    QSet<QString> ids;
    if (mScene) { appendImageAssets(mScene, assets, ids); }
    return assets;
}

QJsonObject LottieLayerBuilder::buildFonts() const
{
    QJsonArray fonts;
    QSet<QString> names;
    if (mScene) { appendFonts(mScene, fonts, names); }
    return QJsonObject{{QStringLiteral("list"), fonts}};
}

QJsonObject LottieLayerBuilder::buildSvgRendererFixLayer(const int id) const
{
    auto layer = baseLayer(QStringLiteral("SVG Renderer Fix"), id, 3);

    QJsonObject transform;
    transform.insert(QStringLiteral("o"), staticProperty(0));
    transform.insert(QStringLiteral("r"), staticProperty(0));
    transform.insert(QStringLiteral("a"), staticProperty(QJsonArray{0, 0, 0}));
    transform.insert(QStringLiteral("s"), staticProperty(QJsonArray{100, 100, 100}));
    transform.insert(QStringLiteral("p"), QJsonObject{
                         {QStringLiteral("a"), 1},
                         {QStringLiteral("k"), QJsonArray{
                              QJsonObject{
                                  {QStringLiteral("t"), mFrameRange.fMin},
                                  {QStringLiteral("s"), QJsonArray{0, 0, 0}},
                                  {QStringLiteral("e"), QJsonArray{0.001, 0, 0}},
                                  {QStringLiteral("i"), QJsonObject{
                                       {QStringLiteral("x"), QJsonArray{0.833}},
                                       {QStringLiteral("y"), QJsonArray{0.833}}
                                   }},
                                  {QStringLiteral("o"), QJsonObject{
                                       {QStringLiteral("x"), QJsonArray{0.167}},
                                       {QStringLiteral("y"), QJsonArray{0.167}}
                                   }}
                              },
                              QJsonObject{
                                  {QStringLiteral("t"), mFrameRange.fMax},
                                  {QStringLiteral("s"), QJsonArray{0.001, 0, 0}}
                              }
                          }}
                     });
    layer.insert(QStringLiteral("ks"), transform);
    return layer;
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
    for (int i = 0; i < boxes.size(); i++) {
        const auto box = boxes.at(i);
        if (!box) { continue; }

        if (isAlphaMatteLayer(box) && i + 1 < boxes.size()) {
            const auto target = boxes.at(i + 1);
            if (const auto targetContainer = dynamic_cast<const ContainerBox*>(target)) {
                if (canBuildMatteLayer(box)) {
                    const int groupId = nextId++;
                    layers.append(buildContainerLayer(targetContainer, groupId, parentId));
                    appendMaskedContainerLayers(box,
                                                targetContainer,
                                                layers,
                                                nextId,
                                                groupId,
                                                parentId,
                                                alphaMatteType(box));
                    i++;
                    continue;
                }
            } else if (canBuildMatteLayer(box) && canBuildBoxLayer(target)) {
                auto matteLayer = buildMatteLayer(box, nextId);
                assignParent(matteLayer, parentId);
                layers.append(matteLayer);
                nextId++;

                auto targetLayer = buildBoxLayer(target, nextId);
                targetLayer.insert(QStringLiteral("tt"), alphaMatteType(box));
                assignParent(targetLayer, parentId);
                layers.append(targetLayer);
                nextId++;
                i++;
                continue;
            }
        }

        const auto childContainer = dynamic_cast<const ContainerBox*>(box);
        if (childContainer) {
            const int groupId = nextId++;
            layers.append(buildContainerLayer(childContainer, groupId, parentId));
            appendContainerLayers(childContainer, layers, nextId, groupId);
            continue;
        }

        if (canBuildBoxLayer(box)) {
            auto layer = buildBoxLayer(box, nextId);
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

void LottieLayerBuilder::appendMaskedContainerLayers(BoundingBox* const matte,
                                                     const ContainerBox* const container,
                                                     QJsonArray& layers,
                                                     int& nextId,
                                                     const int parentId,
                                                     const int matteParentId,
                                                     const int matteType) const
{
    const auto& boxes = container->getContainedBoxes();
    for (const auto box : boxes) {
        if (!box) { continue; }

        if (const auto childContainer = dynamic_cast<const ContainerBox*>(box)) {
            const int groupId = nextId++;
            layers.append(buildContainerLayer(childContainer, groupId, parentId));
            appendMaskedContainerLayers(matte,
                                        childContainer,
                                        layers,
                                        nextId,
                                        groupId,
                                        matteParentId,
                                        matteType);
            continue;
        }

        if (canBuildBoxLayer(box)) {
            auto matteLayer = buildMatteLayer(matte, nextId);
            assignParent(matteLayer, matteParentId);
            layers.append(matteLayer);
            nextId++;

            auto targetLayer = buildBoxLayer(box, nextId);
            targetLayer.insert(QStringLiteral("tt"), matteType);
            assignParent(targetLayer, parentId);
            layers.append(targetLayer);
            nextId++;
            continue;
        }

        auto layer = buildUnsupportedLayer(box, nextId);
        assignParent(layer, parentId);
        layers.append(layer);
        nextId++;
    }
}

QJsonObject LottieLayerBuilder::buildMatteLayer(BoundingBox* const box,
                                                const int id) const
{
    auto layer = buildBoxLayer(box, id);
    layer.insert(QStringLiteral("td"), 1);
    layer.insert(QStringLiteral("bm"), 0);
    return layer;
}

QJsonObject LottieLayerBuilder::buildBoxLayer(BoundingBox* const box,
                                              const int id) const
{
    if (const auto rectangle = dynamic_cast<RectangleBox*>(box)) {
        return buildRectangleLayer(rectangle, id);
    }

    if (const auto text = dynamic_cast<TextBox*>(box)) {
        if (canBuildNativeTextLayer(text)) { return buildTextLayer(text, id); }
    }

    if (const auto image = dynamic_cast<ImageBox*>(box)) {
        if (imageForBox(image)) { return buildImageLayer(image, id); }
    }

    if (const auto path = dynamic_cast<PathBox*>(box)) {
        return buildPathLayer(path, id);
    }

    return QJsonObject();
}

bool LottieLayerBuilder::canBuildBoxLayer(const BoundingBox* const box) const
{
    if (!box) { return false; }
    if (dynamic_cast<const RectangleBox*>(box)) { return true; }
    if (dynamic_cast<const TextBox*>(box)) { return true; }
    if (const auto image = dynamic_cast<const ImageBox*>(box)) {
        return imageForBox(image).get();
    }
    return dynamic_cast<const PathBox*>(box);
}

bool LottieLayerBuilder::canBuildMatteLayer(const BoundingBox* const box) const
{
    return canBuildBoxLayer(box) && !dynamic_cast<const ImageBox*>(box);
}

bool LottieLayerBuilder::isAlphaMatteLayer(const BoundingBox* const box) const
{
    if (!box) { return false; }
    const auto mode = box->getBlendMode();
    return mode == SkBlendMode::kDstIn || mode == SkBlendMode::kDstOut;
}

int LottieLayerBuilder::alphaMatteType(const BoundingBox* const box) const
{
    if (!box) { return 1; }
    return box->getBlendMode() == SkBlendMode::kDstOut ? 2 : 1;
}

QJsonObject LottieLayerBuilder::buildContainerLayer(const ContainerBox* const box,
                                                    const int id,
                                                    const int parentId) const
{
    auto layer = baseLayer(box ? box->prp_getName() : QStringLiteral("Group"),
                           id,
                           3,
                           box);
    layer.insert(QStringLiteral("ks"), transformObject(box));
    assignParent(layer, parentId);
    return layer;
}

QJsonObject LottieLayerBuilder::buildRectangleLayer(RectangleBox* const box,
                                                    const int id) const
{
    auto layer = baseLayer(box->prp_getName(), id, 4, box);
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
    auto layer = baseLayer(box->prp_getName(), id, 5, box);
    layer.insert(QStringLiteral("ks"), transformObject(box));

    QJsonObject textData;
    textData.insert(QStringLiteral("d"), QJsonObject{
                        {QStringLiteral("k"), textDocumentKeyframes(box)}
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

QJsonObject LottieLayerBuilder::textDocumentObject(TextBox* const box,
                                                   const int frame) const
{
    QColor fillColor(0, 0, 0);
    const auto fill = box->getFillSettings();
    if (fill && fill->getPaintType() == PaintType::FLATPAINT) {
        fillColor = fill->getColor(frame);
    }

    const qreal fontSize = box->getFontSize();
    const qreal lineHeight = box->getFontSpacing()*box->getLineSpacing(frame);
    const qreal letterSpacing = box->getLetterSpacing(frame);
    const qreal effectiveLineHeight = isZero4Dec(lineHeight) ? fontSize*1.2 : lineHeight;
    QString normalizedText = box->getValueAtRelFrame(frame);
    normalizedText.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    normalizedText.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    const int lineCount = qMax(1, normalizedText.count(QLatin1Char('\n')) + 1);
    const qreal textHeight = (lineCount - 1)*effectiveLineHeight + fontSize;
    const qreal maxWidth = qMax<qreal>(1, box->getMaxLineWidth(frame));
    qreal horizontalOffset = 0;
    const auto horizontalAlignment = box->getTextHAlignment();
    if (horizontalAlignment & Qt::AlignRight) {
        horizontalOffset = -maxWidth;
    } else if (horizontalAlignment & Qt::AlignHCenter) {
        horizontalOffset = -0.5*maxWidth;
    }

    qreal verticalOffset = 0;
    const auto verticalAlignment = box->getTextVAlignment();
    if (verticalAlignment & Qt::AlignBottom) {
        verticalOffset = -textHeight;
    } else if (verticalAlignment & Qt::AlignVCenter) {
        verticalOffset = -0.5*textHeight;
    }

    QJsonObject document;
    document.insert(QStringLiteral("s"), fontSize);
    document.insert(QStringLiteral("f"), fontName(box));
    document.insert(QStringLiteral("t"), normalizedText);
    document.insert(QStringLiteral("j"), textJustification(box->getTextHAlignment()));
    document.insert(QStringLiteral("tr"), qRound(letterSpacing*1000));
    document.insert(QStringLiteral("lh"), effectiveLineHeight);
    document.insert(QStringLiteral("ls"), 0);
    document.insert(QStringLiteral("sz"), QJsonArray{maxWidth, textHeight});
    document.insert(QStringLiteral("ps"),
                    QJsonArray{horizontalOffset, -fontSize*0.75 + verticalOffset});
    document.insert(QStringLiteral("fc"), QJsonArray{fillColor.redF(),
                                                     fillColor.greenF(),
                                                     fillColor.blueF()});

    const auto stroke = box->getStrokeSettings();
    if (stroke &&
        stroke->getPaintType() == PaintType::FLATPAINT &&
        !isZero4Dec(stroke->getLineWidthAnimator()->getEffectiveValue(frame))) {
        const QColor strokeColor = stroke->getColor(frame);
        document.insert(QStringLiteral("sc"), QJsonArray{strokeColor.redF(),
                                                         strokeColor.greenF(),
                                                         strokeColor.blueF()});
        document.insert(QStringLiteral("sw"),
                        stroke->getLineWidthAnimator()->getEffectiveValue(frame));
    }

    return document;
}

QJsonArray LottieLayerBuilder::textDocumentKeyframes(TextBox* const box) const
{
    QJsonArray keys;
    QJsonObject previousDocument;
    for (int frame = mFrameRange.fMin; frame <= mFrameRange.fMax; frame++) {
        const QJsonObject document = textDocumentObject(box, frame);
        if (frame != mFrameRange.fMin && document == previousDocument) { continue; }

        QJsonObject key;
        key.insert(QStringLiteral("s"), document);
        key.insert(QStringLiteral("t"), frame);
        keys.append(key);
        previousDocument = document;
    }
    return keys;
}

int LottieLayerBuilder::textJustification(const Qt::Alignment alignment) const
{
    if (alignment & Qt::AlignRight) { return 1; }
    if (alignment & Qt::AlignHCenter) { return 2; }
    return 0;
}

QJsonObject LottieLayerBuilder::buildImageLayer(ImageBox* const box,
                                                const int id) const
{
    auto layer = baseLayer(box->prp_getName(), id, 2, box);
    layer.insert(QStringLiteral("ks"), transformObject(box));
    layer.insert(QStringLiteral("refId"), imageAssetId(box));

    const auto image = imageForBox(box);
    if (image) {
        layer.insert(QStringLiteral("w"), image->width());
        layer.insert(QStringLiteral("h"), image->height());
    }
    return layer;
}

QJsonObject LottieLayerBuilder::buildPathLayer(PathBox* const box,
                                               const int id) const
{
    auto layer = baseLayer(box->prp_getName(), id, 4, box);
    layer.insert(QStringLiteral("ks"), transformObject(box));

    QList<QList<LottieContour>> pathFrames;
    bool compatible = true;
    for (int frame = mFrameRange.fMin; frame <= mFrameRange.fMax; frame++) {
        const auto contours = pathContours(box->getRelativePath(frame));
        if (!pathFrames.isEmpty() && !compatibleContours(pathFrames.first(), contours)) {
            compatible = false;
            break;
        }
        pathFrames.append(contours);
    }

    QJsonArray shapes = compatible ?
                pathShapeObjects(pathFrames, box->prp_getName(), mFrameRange) :
                pathShapeObjects(box->getRelativePath(mFrameRange.fMin),
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
                                          const int type,
                                          const BoundingBox* const box) const
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
    layer.insert(QStringLiteral("bm"),
                 box ? LottieBlendMode::value(box->getBlendMode()) : 0);
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

void LottieLayerBuilder::appendImageAssets(const ContainerBox* const container,
                                           QJsonArray& assets,
                                           QSet<QString>& ids) const
{
    if (!container) { return; }

    const auto& boxes = container->getContainedBoxes();
    for (const auto box : boxes) {
        if (!box) { continue; }

        const auto image = dynamic_cast<const ImageBox*>(box);
        if (image) {
            const QString id = imageAssetId(image);
            if (!ids.contains(id)) {
                const auto asset = imageAsset(image);
                if (!asset.isEmpty()) {
                    ids.insert(id);
                    assets.append(asset);
                }
            }
        }

        const auto childContainer = dynamic_cast<const ContainerBox*>(box);
        if (childContainer) {
            appendImageAssets(childContainer, assets, ids);
        }
    }
}

QJsonObject LottieLayerBuilder::imageAsset(const ImageBox* const box) const
{
    const auto image = imageForBox(box);
    if (!image) { return QJsonObject(); }

    QString path;
    QString dir;
    bool embedded = true;
    if (mEmbedImages) {
        path = pngDataUri(image);
        if (path.isEmpty()) { return QJsonObject(); }
    } else {
        const sk_sp<SkData> data = pngData(image);
        if (!data) { return QJsonObject(); }

        const QString dirPath = imageAssetsDirPath();
        if (dirPath.isEmpty()) { return QJsonObject(); }
        QDir().mkpath(dirPath);

        QFile file(QDir(dirPath).filePath(imageAssetFileName(box)));
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            return QJsonObject();
        }
        file.write(static_cast<const char*>(data->data()), data->size());
        file.close();

        path = imageAssetFileName(box);
        dir = imageAssetsDirName() + QStringLiteral("/");
        embedded = false;
    }

    return QJsonObject{
        {QStringLiteral("id"), imageAssetId(box)},
        {QStringLiteral("w"), image->width()},
        {QStringLiteral("h"), image->height()},
        {QStringLiteral("u"), dir},
        {QStringLiteral("p"), path},
        {QStringLiteral("e"), embedded ? 1 : 0}
    };
}

QString LottieLayerBuilder::imageAssetId(const ImageBox* const box) const
{
    return QStringLiteral("image_%1").arg(reinterpret_cast<quintptr>(box), 0, 16);
}

QString LottieLayerBuilder::imageAssetFileName(const ImageBox* const box) const
{
    return imageAssetId(box) + QStringLiteral(".png");
}

QString LottieLayerBuilder::imageAssetsDirName() const
{
    const QFileInfo info(mPath);
    return info.completeBaseName() + QStringLiteral("_assets");
}

QString LottieLayerBuilder::imageAssetsDirPath() const
{
    const QFileInfo info(mPath);
    if (info.absolutePath().isEmpty()) { return QString(); }
    return QDir(info.absolutePath()).filePath(imageAssetsDirName());
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
    if (!mNativeText) { return false; }
    if (!box ||
        box->hasTextEffects() ||
        box->hasBasePathEffects() ||
        box->hasFillEffects() ||
        box->hasOutlineBaseEffects() ||
        box->hasOutlineEffects()) {
        return false;
    }

    for (int frame = mFrameRange.fMin; frame <= mFrameRange.fMax; frame++) {
        if (!isOne4Dec(box->getWordSpacing(frame))) { return false; }
        if (!canBuildNativeTextValue(box->getValueAtRelFrame(frame))) { return false; }
    }
    return true;
}

bool LottieLayerBuilder::canBuildNativeTextValue(const QString& value) const
{
    for (const auto character : value) {
        if (character == QLatin1Char('\n') || character == QLatin1Char('\r')) {
            continue;
        }
        if (character.unicode() > 0x7f) { return false; }
    }
    return true;
}
