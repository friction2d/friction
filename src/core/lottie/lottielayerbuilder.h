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

class BoundingBox;
class Canvas;
class QColor;
class ContainerBox;
class PathBox;
class RectangleBox;
class TextBox;

class CORE_EXPORT LottieLayerBuilder
{
public:
    LottieLayerBuilder(Canvas* const scene,
                       const FrameRange& frameRange,
                       const qreal fps);

    QJsonArray buildLayers(const bool background) const;
    QJsonObject buildFonts() const;

private:
    QJsonObject buildBackgroundLayer() const;
    void appendContainerLayers(const ContainerBox* const container,
                               QJsonArray& layers,
                               int& nextId,
                               const int parentId = 0) const;
    QJsonObject buildContainerLayer(const ContainerBox* const box,
                                    const int id,
                                    const int parentId) const;
    QJsonObject buildRectangleLayer(RectangleBox* const box,
                                    const int id) const;
    QJsonObject buildTextLayer(TextBox* const box,
                               const int id) const;
    QJsonObject buildPathLayer(PathBox* const box,
                               const int id) const;
    QJsonObject buildUnsupportedLayer(const BoundingBox* const box,
                                      const int id) const;

    QJsonObject baseLayer(const QString& name,
                          const int id,
                          const int type) const;
    void assignParent(QJsonObject& layer, const int parentId) const;
    QJsonObject transformObject(const BoundingBox* const box = nullptr) const;
    QJsonObject staticProperty(const QJsonValue& value) const;
    QJsonObject animatedScalarProperty(const QList<qreal>& values) const;
    QJsonObject animatedPointProperty(const QList<QJsonArray>& values) const;
    QJsonArray scalarKeyframes(const QList<qreal>& values) const;
    QJsonArray pointKeyframes(const QList<QJsonArray>& values) const;
    bool sameScalarValues(const QList<qreal>& values) const;
    bool samePointValues(const QList<QJsonArray>& values) const;
    QJsonObject keyframeEase() const;
    void appendPaintObjects(const PathBox* const box,
                            QJsonArray& shapes) const;
    QJsonObject fillObject(const PathBox* const box) const;
    QJsonObject strokeObject(const PathBox* const box) const;
    QJsonObject shapeTransformObject() const;
    QJsonArray colorArray(const QColor& color) const;
    void appendFonts(const ContainerBox* const container,
                     QJsonArray& fonts,
                     QSet<QString>& names) const;
    QJsonObject fontObject(const TextBox* const box) const;
    QString fontName(const TextBox* const box) const;
    QString fontStyleName(const TextBox* const box) const;
    bool canBuildNativeTextLayer(const TextBox* const box) const;

    Canvas* const mScene;
    const FrameRange mFrameRange;
    const qreal mFps;
};

#endif // LOTTIELAYERBUILDER_H
