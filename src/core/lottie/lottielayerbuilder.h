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

#ifndef LOTTIELAYERBUILDER_H
#define LOTTIELAYERBUILDER_H

#include "framerange.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QSet>
#include <Qt>

class BoundingBox;
class Canvas;
class QColor;
class ContainerBox;
class ImageBox;
class PaintSettingsAnimator;
class PathBox;
class RectangleBox;
class TextBox;

class CORE_EXPORT LottieLayerBuilder
{
public:
    LottieLayerBuilder(Canvas* const scene,
                       const FrameRange& frameRange,
                       const qreal fps,
                       const QString& path = QString(),
                       const bool embedImages = true,
                       const bool svgRendererFix = false,
                       const bool nativeText = false);

    QJsonArray buildLayers(const bool background) const;
    QJsonArray buildAssets() const;
    QJsonObject buildFonts() const;

private:
    QJsonObject buildSvgRendererFixLayer(const int id) const;
    QJsonObject buildBackgroundLayer() const;
    void appendContainerLayers(const ContainerBox* const container,
                               QJsonArray& layers,
                               int& nextId,
                               const int parentId = 0) const;
    QJsonObject buildMatteLayer(BoundingBox* const box,
                                const int id) const;
    QJsonObject buildBoxLayer(BoundingBox* const box,
                              const int id) const;
    bool canBuildBoxLayer(const BoundingBox* const box) const;
    bool canBuildMatteLayer(const BoundingBox* const box) const;
    bool isAlphaMatteLayer(const BoundingBox* const box) const;
    int alphaMatteType(const BoundingBox* const box) const;
    QJsonObject buildContainerLayer(const ContainerBox* const box,
                                    const int id,
                                    const int parentId) const;
    QString appendPrecompAsset(const ContainerBox* const box) const;
    QJsonObject buildRectangleLayer(RectangleBox* const box,
                                    const int id) const;
    QJsonObject buildTextLayer(TextBox* const box,
                               const int id) const;
    QJsonObject textDocumentObject(TextBox* const box,
                                   const int frame) const;
    QJsonArray textDocumentKeyframes(TextBox* const box) const;
    int textJustification(const Qt::Alignment alignment) const;
    QJsonObject buildImageLayer(ImageBox* const box,
                                const int id) const;
    QJsonObject buildPathLayer(PathBox* const box,
                               const int id) const;
    QJsonObject buildUnsupportedLayer(const BoundingBox* const box,
                                      const int id) const;

    QJsonObject baseLayer(const QString& name,
                          const int id,
                          const int type,
                          const BoundingBox* const box = nullptr) const;
    void assignParent(QJsonObject& layer, const int parentId) const;
    QJsonObject transformObject(const BoundingBox* const box = nullptr) const;
    QJsonObject staticProperty(const QJsonValue& value) const;
    QJsonObject animatedScalarProperty(const QList<qreal>& values) const;
    QJsonObject animatedPointProperty(const QList<QJsonArray>& values) const;
    void appendPaintObjects(const PathBox* const box,
                            QJsonArray& shapes) const;
    QJsonObject fillObject(const PathBox* const box) const;
    QJsonObject gradientFillObject(const PathBox* const box) const;
    QJsonObject strokeObject(const PathBox* const box) const;
    QJsonObject gradientStrokeObject(const PathBox* const box) const;
    QJsonObject gradientObject(PaintSettingsAnimator* const settings,
                               const QString& name,
                               const bool stroke) const;
    QJsonArray gradientColorArray(PaintSettingsAnimator* const settings,
                                  const int frame,
                                  const int stopCount) const;
    int gradientStopCount(PaintSettingsAnimator* const settings) const;
    QJsonObject shapeTransformObject() const;
    QJsonArray colorArray(const QColor& color) const;
    void appendFonts(const ContainerBox* const container,
                     QJsonArray& fonts,
                     QSet<QString>& names) const;
    void appendImageAssets(const ContainerBox* const container,
                           QJsonArray& assets,
                           QSet<QString>& ids) const;
    QJsonObject imageAsset(const ImageBox* const box) const;
    QString imageAssetId(const ImageBox* const box) const;
    QString imageAssetFileName(const ImageBox* const box) const;
    QString imageAssetsDirName() const;
    QString imageAssetsDirPath() const;
    QJsonObject fontObject(const TextBox* const box) const;
    QString fontName(const TextBox* const box) const;
    QString fontStyleName(const TextBox* const box) const;
    bool canBuildNativeTextLayer(const TextBox* const box) const;
    bool canBuildNativeTextValue(const QString& value) const;

    Canvas* const mScene;
    const FrameRange mFrameRange;
    const qreal mFps;
    const QString mPath;
    const bool mEmbedImages;
    const bool mSvgRendererFix;
    const bool mNativeText;
    mutable QJsonArray mPrecompAssets;
    mutable int mNextPrecompId = 1;
};

#endif // LOTTIELAYERBUILDER_H
