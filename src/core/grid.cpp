/*
#
# Friction - https://friction.graphics
#
# Copyright (c) Ole-André Rodlie and contributors
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3.
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

#include "grid.h"

#include "simplemath.h"
#include "skia/skqtconversions.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPoint.h"

#include "Private/esettings.h"
#include "appsupport.h"

#include <QPainter>
#include <QPen>
#include <QLineF>

#include <cmath>

#include <algorithm>
#include <limits>
#include <vector>

using namespace Friction::Core;

Grid::Grid(QObject *parent)
    : QObject(parent)
{

}

void Grid::drawGrid(QPainter* painter,
                    const QRectF& worldViewport,
                    const QTransform& worldToScreen,
                    const qreal devicePixelRatio) const
{
    if (!painter || !mSettings.show) { return; }

    auto drawLine = [&](const QPointF& a,
                        const QPointF& b,
                        const bool major,
                        const Orientation orientation,
                        const double alphaFactor)
    {
        QPen pen(major ? mSettings.colorMajor : mSettings.color);
        pen.setCosmetic(true);
        pen.setColor(scaledAlpha(pen.color(), alphaFactor));
        painter->setPen(pen);
        painter->drawLine(a, b);
        Q_UNUSED(orientation)
    };

    forEachGridLine(worldViewport,
                    worldToScreen,
                    devicePixelRatio,
                    drawLine);
}

void Grid::drawGrid(SkCanvas* canvas,
                    const QRectF& worldViewport,
                    const QTransform& worldToScreen,
                    const qreal devicePixelRatio) const
{
    if (!canvas || !mSettings.show) { return; }

    const float strokeWidth = static_cast<float>(devicePixelRatio / effectiveScale(worldToScreen));

    auto drawLine = [&](const QPointF& a,
                        const QPointF& b,
                        const bool major,
                        const Orientation orientation,
                        const double alphaFactor)
    {
        SkPaint paint;
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(strokeWidth);
        paint.setAntiAlias(false);
        const QColor base = major ? mSettings.colorMajor : mSettings.color;
        paint.setColor(toSkColor(scaledAlpha(base, alphaFactor)));
        canvas->drawLine(SkPoint::Make(static_cast<SkScalar>(a.x()),
                                       static_cast<SkScalar>(a.y())),
                         SkPoint::Make(static_cast<SkScalar>(b.x()),
                                       static_cast<SkScalar>(b.y())),
                         paint);
        Q_UNUSED(orientation)
    };

    forEachGridLine(worldViewport,
                    worldToScreen,
                    devicePixelRatio,
                    drawLine);
}

QPointF Grid::maybeSnapPivot(const QPointF& pivotWorld,
                             const QTransform& worldToScreen,
                             const bool forceSnap,
                             const bool bypassSnap,
                             const QRectF* canvasRectWorld,
                             const std::vector<QPointF>* anchorOffsets,
                             const std::vector<QPointF>* pivotTargets,
                             const std::vector<QPointF>* boxTargets,
                             const std::vector<QPointF>* nodeTargets) const
{
    const bool hasPivotTargets = mSettings.snapToPivots &&
                                 pivotTargets && !pivotTargets->empty();
    const bool hasBoxTargets = mSettings.snapToBoxes &&
                               boxTargets && !boxTargets->empty();
    const bool hasNodeTargets = mSettings.snapToNodes &&
                                nodeTargets && !nodeTargets->empty();

    const bool snapSourcesEnabled = (mSettings.snapToGrid && mSettings.show) ||
                                    (mSettings.snapToCanvas && canvasRectWorld) ||
                                    hasPivotTargets || hasBoxTargets || hasNodeTargets;

    if ((!snapSourcesEnabled && !forceSnap) || bypassSnap) { return pivotWorld; }

    const double sizeX = mSettings.sizeX;
    const double sizeY = mSettings.sizeY;
    const bool hasGrid = sizeX > 0.0 && sizeY > 0.0 && mSettings.show;

    const bool canUseCanvas = mSettings.snapToCanvas && canvasRectWorld;
    QRectF normalizedCanvas;
    if (canUseCanvas) { normalizedCanvas = canvasRectWorld->normalized(); }
    const bool hasCanvasTargets = canUseCanvas && !normalizedCanvas.isEmpty();

    if (!hasGrid && !hasCanvasTargets &&
        !hasPivotTargets && !hasBoxTargets &&
        !hasNodeTargets) { return pivotWorld; }

    const std::vector<QPointF>* offsetsPtr = anchorOffsets;
    std::vector<QPointF> fallbackOffsets;
    if (!offsetsPtr) {
        fallbackOffsets.emplace_back(QPointF(0.0, 0.0));
        offsetsPtr = &fallbackOffsets;
    }

    const auto& offsets = *offsetsPtr;
    if (offsets.empty()) { return pivotWorld; }

    struct AnchorContext {
        QPointF offset;
        QPointF world;
        QPointF screen;
    };

    std::vector<AnchorContext> anchors;
    anchors.reserve(offsets.size());
    for (const auto& offset : offsets) {
        const QPointF worldPoint = pivotWorld + offset;
        anchors.push_back({offset,
                           worldPoint,
                           worldToScreen.map(worldPoint)});
    }

    if (anchors.empty()) { return pivotWorld; }

    QPointF bestPivot = pivotWorld;
    double bestDistance = std::numeric_limits<double>::infinity();
    bool foundCandidate = false;

    auto considerCandidate = [&](const AnchorContext& anchor,
                                 const QPointF& candidateAnchorWorld)
    {
        const QPointF candidatePivot = candidateAnchorWorld - anchor.offset;
        const QPointF screenCandidate = worldToScreen.map(candidateAnchorWorld);
        const double candidateDistance = QLineF(anchor.screen, screenCandidate).length();
        if (candidateDistance < bestDistance) {
            bestDistance = candidateDistance;
            bestPivot = candidatePivot;
            foundCandidate = true;
        }
    };

    if (hasGrid && (mSettings.snapToGrid || forceSnap)) {
        for (const auto& anchor : anchors) {
            const double gx = mSettings.originX + std::round((anchor.world.x() - mSettings.originX) / sizeX) * sizeX;
            const double gy = mSettings.originY + std::round((anchor.world.y() - mSettings.originY) / sizeY) * sizeY;
            considerCandidate(anchor, QPointF(gx, gy));
        }
    }

    if (hasCanvasTargets) {
        const double left = normalizedCanvas.left();
        const double right = normalizedCanvas.right();
        const double top = normalizedCanvas.top();
        const double bottom = normalizedCanvas.bottom();
        const double midX = (left + right) * 0.5;
        const double midY = (top + bottom) * 0.5;

        const QPointF canvasTargets[] = {
            QPointF(left, top),
            QPointF(right, top),
            QPointF(left, bottom),
            QPointF(right, bottom),
            QPointF(midX, top),
            QPointF(midX, bottom),
            QPointF(left, midY),
            QPointF(right, midY),
            QPointF(midX, midY)
        };

        for (const auto& anchor : anchors) {
            for (const auto& target : canvasTargets) { considerCandidate(anchor, target); }
        }
    }

    if (hasPivotTargets) {
        for (const auto& anchor : anchors) {
            for (const auto& target : *pivotTargets) { considerCandidate(anchor, target); }
        }
    }

    if (hasBoxTargets) {
        for (const auto& anchor : anchors) {
            for (const auto& target : *boxTargets) { considerCandidate(anchor, target); }
        }
    }

    if (hasNodeTargets) {
        for (const auto& anchor : anchors) {
            for (const auto& target : *nodeTargets) { considerCandidate(anchor, target); }
        }
    }

    if (!foundCandidate) { return pivotWorld; }
    if (forceSnap) { return bestPivot; }

    if (bestDistance <= mSettings.snapThresholdPx) { return bestPivot; }
    return pivotWorld;
}

QColor Grid::scaledAlpha(const QColor &base,
                         double factor)
{
    QColor c = base;
    factor = clamp(factor, 0.0, 1.0);
    c.setAlphaF(c.alphaF() * factor);
    return c;
}

double Grid::lineSpacingPx(const QTransform &worldToScreen,
                           qreal devicePixelRatio,
                           const QPointF &delta)
{
    const QPointF origin = worldToScreen.map(QPointF(0.0, 0.0));
    const QPointF mapped = worldToScreen.map(delta);
    return QLineF(origin, mapped).length() * devicePixelRatio;
}

double Grid::effectiveScale(const QTransform &worldToScreen)
{
    const double sx = std::hypot(worldToScreen.m11(), worldToScreen.m12());
    const double sy = std::hypot(worldToScreen.m21(), worldToScreen.m22());
    const double avg = (sx + sy) * 0.5;
    return avg > 0.0 ? avg : 1.0;
}

double Grid::fadeFactor(double spacingPx)
{
    constexpr double minVisible = 4.0;
    constexpr double fullVisible = 16.0;
    if (spacingPx <= minVisible) { return 0.0; }
    if (spacingPx >= fullVisible) { return 1.0; }
    return (spacingPx - minVisible) / (fullVisible - minVisible);
}

void Grid::setSettings(const Settings &settings,
                       const bool global)
{
    if (!differSettings(mSettings, settings)) { return; }
    mSettings = settings;
    if (global) { eSettings::sInstance->fGrid = settings; }
    emit changed(settings);
}

Grid::Settings Grid::getSettings()
{
    return mSettings;
}

const Grid::Settings Grid::loadSettings()
{
    Settings settings;
    {
        const QVariant var = AppSupport::getSettings("grid", "sizeX");
        if (var.isValid()) { settings.sizeX = var.toDouble(); }
    }
    {
        const QVariant var = AppSupport::getSettings("grid", "sizeY");
        if (var.isValid()) { settings.sizeY = var.toDouble(); }
    }
    {
        const QVariant var = AppSupport::getSettings("grid", "originX");
        if (var.isValid()) { settings.originX = var.toDouble(); }
    }
    {
        const QVariant var = AppSupport::getSettings("grid", "originY");
        if (var.isValid()) { settings.originY = var.toDouble(); }
    }
    {
        const QVariant var = AppSupport::getSettings("grid", "snapThresholdPx");
        if (var.isValid()) { settings.snapThresholdPx = var.toInt(); }
    }
    {
        const QVariant var = AppSupport::getSettings("grid", "show");
        if (var.isValid()) { settings.show = var.toBool(); }
    }
    {
        const QVariant var = AppSupport::getSettings("grid", "drawOnTop");
        if (var.isValid()) { settings.drawOnTop = var.toBool(); }
    }
    {
        const QVariant var = AppSupport::getSettings("grid", "snapEnabled");
        if (var.isValid()) { settings.snapEnabled = var.toBool(); }
    }
    {
        const QVariant var = AppSupport::getSettings("grid", "snapToCanvas");
        if (var.isValid()) { settings.snapToCanvas = var.toBool(); }
    }
    {
        const QVariant var = AppSupport::getSettings("grid", "snapToBoxes");
        if (var.isValid()) { settings.snapToBoxes = var.toBool(); }
    }
    {
        const QVariant var = AppSupport::getSettings("grid", "snapToNodes");
        if (var.isValid()) { settings.snapToNodes = var.toBool(); }
    }
    {
        const QVariant var = AppSupport::getSettings("grid", "snapToPivots");
        if (var.isValid()) { settings.snapToPivots = var.toBool(); }
    }
    {
        const QVariant var = AppSupport::getSettings("grid", "snapToGrid");
        if (var.isValid()) { settings.snapToGrid = var.toBool(); }
    }
    {
        const QVariant var = AppSupport::getSettings("grid", "snapAnchorPivot");
        if (var.isValid()) { settings.snapAnchorPivot = var.toBool(); }
    }
    {
        const QVariant var = AppSupport::getSettings("grid", "snapAnchorBounds");
        if (var.isValid()) { settings.snapAnchorBounds = var.toBool(); }
    }
    {
        const QVariant var = AppSupport::getSettings("grid", "snapAnchorNodes");
        if (var.isValid()) { settings.snapAnchorNodes = var.toBool(); }
    }
    {
        const QVariant var = AppSupport::getSettings("grid", "majorEveryX");
        if (var.isValid()) { settings.majorEveryX = var.toInt(); }
    }
    {
        const QVariant var = AppSupport::getSettings("grid", "majorEveryY");
        if (var.isValid()) { settings.majorEveryY = var.toInt(); }
    }
    {
        const QVariant var = AppSupport::getSettings("grid", "color");
        if (var.isValid()) { settings.color = var.value<QColor>(); }
    }
    {
        const QVariant var = AppSupport::getSettings("grid", "colorMajor");
        if (var.isValid()) { settings.colorMajor = var.value<QColor>(); }
    }
    qDebug() << "Load Grid Settings";
    debugSettings(settings);
    return settings;
}

void Grid::saveSettings(const Settings &settings)
{
    qDebug() << "Save Grid Settings";
    debugSettings(settings);
    AppSupport::setSettings("grid", "sizeX", settings.sizeX);
    AppSupport::setSettings("grid", "sizeY", settings.sizeY);
    AppSupport::setSettings("grid", "originX", settings.originX);
    AppSupport::setSettings("grid", "originY", settings.originY);
    AppSupport::setSettings("grid", "snapThresholdPx", settings.snapThresholdPx);
    AppSupport::setSettings("grid", "show", settings.show);
    AppSupport::setSettings("grid", "drawOnTop", settings.drawOnTop);
    AppSupport::setSettings("grid", "snapEnabled", settings.snapEnabled);
    AppSupport::setSettings("grid", "snapToCanvas", settings.snapToCanvas);
    AppSupport::setSettings("grid", "snapToBoxes", settings.snapToBoxes);
    AppSupport::setSettings("grid", "snapToNodes", settings.snapToNodes);
    AppSupport::setSettings("grid", "snapToPivots", settings.snapToPivots);
    AppSupport::setSettings("grid", "snapToGrid", settings.snapToGrid);
    AppSupport::setSettings("grid", "snapAnchorPivot", settings.snapAnchorPivot);
    AppSupport::setSettings("grid", "snapAnchorBounds", settings.snapAnchorBounds);
    AppSupport::setSettings("grid", "snapAnchorNodes", settings.snapAnchorNodes);
    AppSupport::setSettings("grid", "majorEveryX", settings.majorEveryX);
    AppSupport::setSettings("grid", "majorEveryY", settings.majorEveryY);
    AppSupport::setSettings("grid", "color", settings.color);
    AppSupport::setSettings("grid", "colorMajor", settings.colorMajor);
}

void Grid::debugSettings(const Settings &settings)
{
    qDebug() << "Grid Settings:"
             << "sizeX" << settings.sizeX
             << "sizeY" << settings.sizeY
             << "originX" << settings.originX
             << "originY" << settings.originY
             << "snapThresholdPx" << settings.snapThresholdPx
             << "show" << settings.show
             << "drawOnTop" << settings.drawOnTop
             << "snapEnabled" << settings.snapEnabled
             << "snapToCanvas" << settings.snapToCanvas
             << "snapToBoxes" << settings.snapToBoxes
             << "snapToNodes" << settings.snapToNodes
             << "snapToPivots" << settings.snapToPivots
             << "snapToGrid" << settings.snapToGrid
             << "snapAnchorPivot" << settings.snapAnchorPivot
             << "snapAnchorBounds" << settings.snapAnchorBounds
             << "snapAnchorNodes" << settings.snapAnchorNodes
             << "majorEveryX" << settings.majorEveryX
             << "majorEveryY" << settings.majorEveryY
             << "color" << settings.color
             << "colorMajor" << settings.colorMajor;
}

bool Grid::differSettings(const Settings &orig,
                          const Settings &diff)
{
    if (orig.sizeX != diff.sizeX ||
        orig.sizeY != diff.sizeY ||
        orig.originX != diff.originX ||
        orig.originY != diff.originY ||
        orig.snapThresholdPx != diff.snapThresholdPx ||
        orig.show != diff.show ||
        orig.drawOnTop != diff.drawOnTop ||
        orig.snapEnabled != diff.snapEnabled ||
        orig.snapToCanvas != diff.snapToCanvas ||
        orig.snapToBoxes != diff.snapToBoxes ||
        orig.snapToNodes != diff.snapToNodes ||
        orig.snapToPivots != diff.snapToPivots ||
        orig.snapToGrid != diff.snapToGrid ||
        orig.snapAnchorPivot != diff.snapAnchorPivot ||
        orig.snapAnchorBounds != diff.snapAnchorBounds ||
        orig.snapAnchorNodes != diff.snapAnchorNodes ||
        orig.majorEveryX != diff.majorEveryX ||
        orig.majorEveryY != diff.majorEveryY ||
        orig.color != diff.color ||
        orig.colorMajor != diff.colorMajor) { return true; }
    return false;
}

void Grid::setOption(const Option &option,
                     const QVariant &value,
                     const bool global)
{
    qDebug() << "Grid::setOption" << (int)option << value << global;
    QString key;
    switch(option){
    case Option::SizeX:
        if (mSettings.sizeX == value.toInt()) { return; }
        mSettings.sizeX = value.toInt();
        if (global) {
            eSettings::sInstance->fGrid.sizeX = value.toInt();
            key = "sizeX";
        }
        break;
    case Option::SizeY:
        if (mSettings.sizeY == value.toInt()) { return; }
        mSettings.sizeY = value.toInt();
        if (global) {
            eSettings::sInstance->fGrid.sizeY = value.toInt();
            key = "sizeY";
        }
        break;
    case Option::OriginX:
        if (mSettings.originX == value.toInt()) { return; }
        mSettings.originX = value.toInt();
        if (global) {
            eSettings::sInstance->fGrid.originX = value.toInt();
            key = "originX";
        }
        break;
    case Option::OriginY:
        if (mSettings.originY == value.toInt()) { return; }
        mSettings.originY = value.toInt();
        if (global) {
            eSettings::sInstance->fGrid.originY = value.toInt();
            key = "originY";
        }
        break;
    case Option::SnapThresholdPx:
        if (mSettings.snapThresholdPx == value.toInt()) { return; }
        mSettings.snapThresholdPx = value.toInt();
        if (global) {
            eSettings::sInstance->fGrid.snapThresholdPx = value.toInt();
            key = "snapThresholdPx";
        }
        break;
    case Option::Show:
        if (mSettings.show == value.toBool()) { return; }
        mSettings.show = value.toBool();
        if (global) {
            eSettings::sInstance->fGrid.show = value.toBool();
            key = "show";
        }
        break;
    case Option::DrawOnTop:
        if (mSettings.drawOnTop == value.toBool()) { return; }
        mSettings.drawOnTop = value.toBool();
        if (global) {
            eSettings::sInstance->fGrid.drawOnTop = value.toBool();
            key = "drawOnTop";
        }
        break;
    case Option::SnapEnabled:
        if (mSettings.snapEnabled == value.toBool()) { return; }
        mSettings.snapEnabled = value.toBool();
        if (global) {
            eSettings::sInstance->fGrid.snapEnabled = value.toBool();
            key = "snapEnabled";
        }
        break;
    case Option::SnapToCanvas:
        if (mSettings.snapToCanvas == value.toBool()) { return; }
        mSettings.snapToCanvas = value.toBool();
        if (global) {
            eSettings::sInstance->fGrid.snapToCanvas = value.toBool();
            key = "snapToCanvas";
        }
        break;
    case Option::SnapToBoxes:
        if (mSettings.snapToBoxes == value.toBool()) { return; }
        mSettings.snapToBoxes = value.toBool();
        if (global) {
            eSettings::sInstance->fGrid.snapToBoxes = value.toBool();
            key = "snapToBoxes";
        }
        break;
    case Option::SnapToNodes:
        if (mSettings.snapToNodes == value.toBool()) { return; }
        mSettings.snapToNodes = value.toBool();
        if (global) {
            eSettings::sInstance->fGrid.snapToNodes = value.toBool();
            key = "snapToNodes";
        }
        break;
    case Option::SnapToPivots:
        if (mSettings.snapToPivots == value.toBool()) { return; }
        mSettings.snapToPivots = value.toBool();
        if (global) {
            eSettings::sInstance->fGrid.snapToPivots = value.toBool();
            key = "snapToPivots";
        }
        break;
    case Option::SnapToGrid:
        if (mSettings.snapToGrid == value.toBool()) { return; }
        mSettings.snapToGrid = value.toBool();
        if (global) {
            eSettings::sInstance->fGrid.snapToGrid = value.toBool();
            key = "snapToGrid";
        }
        break;
    case Option::AnchorPivot:
        if (mSettings.snapAnchorPivot == value.toBool()) { return; }
        mSettings.snapAnchorPivot = value.toBool();
        if (global) {
            eSettings::sInstance->fGrid.snapAnchorPivot = value.toBool();
            key = "snapAnchorPivot";
        }
        break;
    case Option::AnchorBounds:
        if (mSettings.snapAnchorBounds == value.toBool()) { return; }
        mSettings.snapAnchorBounds = value.toBool();
        if (global) {
            eSettings::sInstance->fGrid.snapAnchorBounds = value.toBool();
            key = "snapAnchorBounds";
        }
        break;
    case Option::AnchorNodes:
        if (mSettings.snapAnchorNodes == value.toBool()) { return; }
        mSettings.snapAnchorNodes = value.toBool();
        if (global) {
            eSettings::sInstance->fGrid.snapAnchorNodes = value.toBool();
            key = "snapAnchorNodes";
        }
        break;
    case Option::MajorEveryX:
        if (mSettings.majorEveryX == value.toInt()) { return; }
        mSettings.majorEveryX = value.toInt();
        if (global) {
            eSettings::sInstance->fGrid.majorEveryX = value.toInt();
            key = "majorEveryX";
        }
        break;
    case Option::MajorEveryY:
        if (mSettings.majorEveryY == value.toInt()) { return; }
        mSettings.majorEveryY = value.toInt();
        if (global) {
            eSettings::sInstance->fGrid.majorEveryY = value.toInt();
            key = "majorEveryY";
        }
        break;
    case Option::Color:
        if (mSettings.color == value.value<QColor>()) { return; }
        mSettings.color = value.value<QColor>();
        if (global) {
            eSettings::sInstance->fGrid.color = value.value<QColor>();
            key = "color";
        }
        break;
    case Option::ColorMajor:
        if (mSettings.colorMajor == value.value<QColor>()) { return; }
        mSettings.colorMajor = value.value<QColor>();
        if (global) {
            eSettings::sInstance->fGrid.colorMajor = value.value<QColor>();
            key = "colorMajor";
        }
        break;
    default: return;
    }

    if (global && !key.isEmpty()) {
        AppSupport::setSettings("grid", key, value);
    }

    emit changed(mSettings);
}

QVariant Grid::getOption(const Option &option)
{
    switch(option){
    case Option::SizeX:
        return mSettings.sizeX;
    case Option::SizeY:
        return mSettings.sizeY;
    case Option::OriginX:
        return mSettings.originX;
    case Option::OriginY:
        return mSettings.originY;
    case Option::SnapThresholdPx:
        return mSettings.snapThresholdPx;
    case Option::Show:
        return mSettings.show;
    case Option::DrawOnTop:
        return mSettings.drawOnTop;
    case Option::SnapEnabled:
        return mSettings.snapEnabled;
    case Option::SnapToCanvas:
        return mSettings.snapToCanvas;
    case Option::SnapToBoxes:
        return mSettings.snapToBoxes;
    case Option::SnapToNodes:
        return mSettings.snapToNodes;
    case Option::SnapToPivots:
        return mSettings.snapToPivots;
    case Option::SnapToGrid:
        return mSettings.snapToGrid;
    case Option::AnchorPivot:
        return mSettings.snapAnchorPivot;
    case Option::AnchorBounds:
        return mSettings.snapAnchorBounds;
    case Option::AnchorNodes:
        return mSettings.snapAnchorNodes;
    case Option::MajorEveryX:
        return mSettings.majorEveryX;
    case Option::MajorEveryY:
        return mSettings.majorEveryY;
    case Option::Color:
        return mSettings.color;
    case Option::ColorMajor:
        return mSettings.colorMajor;
    default: return QVariant();
    }
}

void Grid::writeDocument(eWriteStream &dst) const
{
    qDebug() << "write grid settings to document";
    debugSettings(mSettings);

    dst << mSettings.sizeX;
    dst << mSettings.sizeY;
    dst << mSettings.originX;
    dst << mSettings.originY;
    dst << mSettings.snapThresholdPx;
    dst << mSettings.show;
    dst << mSettings.drawOnTop;
    dst << mSettings.snapEnabled;
    dst << mSettings.snapToCanvas;
    dst << mSettings.snapToBoxes;
    dst << mSettings.snapToNodes;
    dst << mSettings.snapToPivots;
    dst << mSettings.snapToGrid;
    dst << mSettings.snapAnchorPivot;
    dst << mSettings.snapAnchorBounds;
    dst << mSettings.snapAnchorNodes;
    dst << mSettings.majorEveryX;
    dst << mSettings.majorEveryY;
    dst << mSettings.color;
    dst << mSettings.colorMajor;
}

void Grid::readDocument(eReadStream &src)
{
    Settings settings = mSettings;
    src >> settings.sizeX;
    src >> settings.sizeY;
    src >> settings.originX;
    src >> settings.originY;
    src >> settings.snapThresholdPx;
    src >> settings.show;
    src >> settings.drawOnTop;
    src >> settings.snapEnabled;
    src >> settings.snapToCanvas;
    src >> settings.snapToBoxes;
    src >> settings.snapToNodes;
    src >> settings.snapToPivots;
    src >> settings.snapToGrid;
    src >> settings.snapAnchorPivot;
    src >> settings.snapAnchorBounds;
    src >> settings.snapAnchorNodes;
    src >> settings.majorEveryX;
    src >> settings.majorEveryY;
    src >> settings.color;
    src >> settings.colorMajor;

    qDebug() << "read grid settings from document";
    debugSettings(settings);

    // we only restore some settings (we can change this in the future)
    mSettings.sizeX = settings.sizeX;
    mSettings.sizeY = settings.sizeY;
    mSettings.originX = settings.originX;
    mSettings.originY = settings.originY;
    mSettings.snapThresholdPx = settings.snapThresholdPx;
    mSettings.majorEveryX = settings.majorEveryX;
    mSettings.majorEveryY = settings.majorEveryY;
    mSettings.color = settings.color;
    mSettings.colorMajor = settings.colorMajor;
    mSettings.drawOnTop = settings.drawOnTop;

    emit changed(mSettings);
}

void Grid::writeXML(QDomDocument &doc) const
{
    qDebug() << "write grid settings to document";
    debugSettings(mSettings);

    auto element = doc.createElement("Grid");
    element.setAttribute("sizeX", QString::number(mSettings.sizeX));
    element.setAttribute("sizeY", QString::number(mSettings.sizeY));
    element.setAttribute("originX", QString::number(mSettings.originX));
    element.setAttribute("originY", QString::number(mSettings.originY));
    element.setAttribute("snapThresholdPx", QString::number(mSettings.snapThresholdPx));
    element.setAttribute("show", mSettings.show ? "true" : "false");
    element.setAttribute("drawOnTop", mSettings.drawOnTop ? "true" : "false");
    element.setAttribute("snapEnabled", mSettings.snapEnabled ? "true" : "false");
    element.setAttribute("snapToCanvas", mSettings.snapToCanvas ? "true" : "false");
    element.setAttribute("snapToBoxes", mSettings.snapToBoxes ? "true" : "false");
    element.setAttribute("snapToNodes", mSettings.snapToNodes ? "true" : "false");
    element.setAttribute("snapToPivots", mSettings.snapToPivots ? "true" : "false");
    element.setAttribute("snapToGrid", mSettings.snapToGrid ? "true" : "false");
    element.setAttribute("snapAnchorPivot", mSettings.snapAnchorPivot ? "true" : "false");
    element.setAttribute("snapAnchorBounds", mSettings.snapAnchorBounds ? "true" : "false");
    element.setAttribute("snapAnchorNodes", mSettings.snapAnchorNodes ? "true" : "false");
    element.setAttribute("majorEveryX", QString::number(mSettings.majorEveryX));
    element.setAttribute("majorEveryY", QString::number(mSettings.majorEveryY));
    element.setAttribute("color", mSettings.color.name());
    element.setAttribute("colorMajor", mSettings.colorMajor.name());
    doc.appendChild(element);
}

void Grid::readXML(const QDomElement &element)
{
    if (element.isNull()) { return; }

    qDebug() << "read grid settings from document";
    // we only restore some settings (we can change this in the future)
    if (element.hasAttribute("sizeX")) {
        mSettings.sizeX = element.attribute("sizeX").toDouble();
    }
    if (element.hasAttribute("sizeY")) {
        mSettings.sizeY = element.attribute("sizeY").toDouble();
    }
    if (element.hasAttribute("originX")) {
        mSettings.originX = element.attribute("originX").toDouble();
    }
    if (element.hasAttribute("originY")) {
        mSettings.originY = element.attribute("originY").toDouble();
    }
    if (element.hasAttribute("snapThresholdPx")) {
        mSettings.snapThresholdPx = element.attribute("snapThresholdPx").toInt();
    }
    if (element.hasAttribute("majorEveryX")) {
        mSettings.majorEveryX = element.attribute("majorEveryX").toInt();
    }
    if (element.hasAttribute("majorEveryY")) {
        mSettings.majorEveryY = element.attribute("majorEveryY").toInt();
    }
    if (element.hasAttribute("color")) {
        mSettings.color = QColor(element.attribute("color"));
    }
    if (element.hasAttribute("colorMajor")) {
        mSettings.colorMajor = QColor(element.attribute("colorMajor"));
    }
    if (element.hasAttribute("drawOnTop")) {
        mSettings.drawOnTop = element.attribute("drawOnTop") == "true" ? true : false;
    }

    emit changed(mSettings);
}

template<typename DrawLineFunc>
void Grid::forEachGridLine(const QRectF& viewport,
                           const QTransform& worldToScreen,
                           const qreal devicePixelRatio,
                           DrawLineFunc&& drawLine) const
{
    if (!mSettings.show) { return; }

    const double sizeX = mSettings.sizeX;
    const double sizeY = mSettings.sizeY;
    if (sizeX <= 0.0 || sizeY <= 0.0) { return; }

    QRectF baseView = viewport.normalized();
    if (!baseView.isValid() || baseView.isEmpty()) { return; }
    const double expandX = std::max(baseView.width(), sizeX);
    const double expandY = std::max(baseView.height(), sizeY);
    QRectF view = baseView.adjusted(-expandX, -expandY, expandX, expandY);

    const int majorEveryX = std::max(1, mSettings.majorEveryX);
    const int majorEveryY = std::max(1, mSettings.majorEveryY);

    const double spacingX = lineSpacingPx(worldToScreen, devicePixelRatio, {sizeX, 0.0});
    const double spacingY = lineSpacingPx(worldToScreen, devicePixelRatio, {0.0, sizeY});
    const double majorSpacingX = spacingX * majorEveryX;
    const double majorSpacingY = spacingY * majorEveryY;

    const double majorAlphaX = fadeFactor(majorSpacingX);
    const double majorAlphaY = fadeFactor(majorSpacingY);

    if (majorAlphaX <= 0.0 && majorAlphaY <= 0.0) {
        return;
    }

    const double minorAlphaX = fadeFactor(spacingX);
    const double minorAlphaY = fadeFactor(spacingY);

    auto firstAligned = [](double start,
                           double origin,
                           double spacing)
    {
        const double steps = std::floor((start - origin) / spacing);
        return origin + steps * spacing;
    };

    const double originX = mSettings.originX;
    const double originY = mSettings.originY;

    const double xBegin = firstAligned(view.left(), originX, sizeX);
    const double xEnd = view.right() + sizeX;

    for (double x = xBegin; x <= xEnd; x += sizeX) {
        const long long index = static_cast<long long>(std::llround((x - originX) / sizeX));
        const bool major = (index % majorEveryX) == 0;
        const double alpha = major ? majorAlphaX : minorAlphaX;
        if (!major && alpha <= 0.0) { continue; }
        const QPointF top(x, view.top());
        const QPointF bottom(x, view.bottom());
        drawLine(top, bottom, major, Orientation::Vertical, alpha);
    }

    const double yBegin = firstAligned(view.top(), originY, sizeY);
    const double yEnd = view.bottom() + sizeY;

    for (double y = yBegin; y <= yEnd; y += sizeY) {
        const long long index = static_cast<long long>(std::llround((y - originY) / sizeY));
        const bool major = (index % majorEveryY) == 0;
        const double alpha = major ? majorAlphaY : minorAlphaY;
        if (!major && alpha <= 0.0) { continue; }
        const QPointF left(view.left(), y);
        const QPointF right(view.right(), y);
        drawLine(left, right, major, Orientation::Horizontal, alpha);
    }
}
