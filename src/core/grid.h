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

#ifndef FRICTION_GRID_H
#define FRICTION_GRID_H

#include "core_global.h"
#include "include/core/SkCanvas.h"
#include "ReadWrite/ereadstream.h"
#include "ReadWrite/ewritestream.h"

#include <QObject>
#include <QRectF>
#include <QTransform>
#include <QPointF>
#include <QColor>
#include <QDomDocument>
#include <QDomElement>

#include <vector>

namespace Friction
{
    namespace Core
    {
        class CORE_EXPORT Grid : public QObject
        {
            Q_OBJECT
        public:
            enum class Option {
                SizeX,
                SizeY,
                OriginX,
                OriginY,
                SnapThresholdPx,
                Show,
                DrawOnTop,
                SnapEnabled,
                SnapToCanvas,
                SnapToBoxes,
                SnapToNodes,
                SnapToPivots,
                SnapToGrid,
                AnchorPivot,
                AnchorBounds,
                AnchorNodes,
                MajorEveryX,
                MajorEveryY,
                Color,
                ColorMajor
            };

            struct Settings {
                double sizeX = 40.0;
                double sizeY = 40.0;
                double originX = 960.0;
                double originY = 540.0;
                int snapThresholdPx = 40;
                bool show = false;
                bool drawOnTop = false;
                bool snapEnabled = true;
                bool snapToCanvas = false;
                bool snapToBoxes = true;
                bool snapToNodes = false;
                bool snapToPivots = false;
                bool snapToGrid = true;
                bool snapAnchorPivot = true;
                bool snapAnchorBounds = true;
                bool snapAnchorNodes = false;
                int majorEveryX = 8;
                int majorEveryY = 8;
                QColor color = QColor(128, 127, 255, 75);
                QColor colorMajor = QColor(255, 127, 234, 125);
            };

            explicit Grid(QObject *parent = nullptr);

            void drawGrid(QPainter* painter,
                          const QRectF& worldViewport,
                          const QTransform& worldToScreen,
                          qreal devicePixelRatio = 1.0) const;
            void drawGrid(SkCanvas* canvas,
                          const QRectF& worldViewport,
                          const QTransform& worldToScreen,
                          qreal devicePixelRatio = 1.0) const;

            QPointF maybeSnapPivot(const QPointF& pivotWorld,
                                   const QTransform& worldToScreen,
                                   bool forceSnap,
                                   bool bypassSnap,
                                   const QRectF* canvasRectWorld = nullptr,
                                   const std::vector<QPointF>* anchorOffsets = nullptr,
                                   const std::vector<QPointF>* pivotTargets = nullptr,
                                   const std::vector<QPointF>* boxTargets = nullptr,
                                   const std::vector<QPointF>* nodeTargets = nullptr) const;

            static QColor scaledAlpha(const QColor& base, double factor);
            static double lineSpacingPx(const QTransform& worldToScreen,
                                        qreal devicePixelRatio,
                                        const QPointF& delta);
            static double effectiveScale(const QTransform& worldToScreen);
            static double fadeFactor(double spacingPx);

            Settings getSettings();
            void setSettings(const Settings &settings,
                             const bool global = false);

            static const Settings loadSettings();
            static void saveSettings(const Settings &settings);
            static void debugSettings(const Settings &settings);
            static bool differSettings(const Settings &orig,
                                       const Settings &diff);

            void setOption(const Option &option,
                           const QVariant &value,
                           const bool global = false);
            QVariant getOption(const Option &option);

            void writeDocument(eWriteStream &dst) const;
            void readDocument(eReadStream &src);

            void writeXML(QDomDocument& doc) const;
            void readXML(const QDomElement& element);

        signals:
            void changed(const Friction::Core::Grid::Settings &settings);

        private:
            enum class Orientation {
                Vertical,
                Horizontal
            };

            Settings mSettings;

            template<typename DrawLineFunc>
            void forEachGridLine(const QRectF& worldViewport,
                                 const QTransform& worldToScreen,
                                 qreal devicePixelRatio,
                                 DrawLineFunc&& drawLine) const;
        };
    }
}

#endif // FRICTION_GRID_H
