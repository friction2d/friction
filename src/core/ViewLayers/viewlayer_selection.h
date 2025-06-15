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

#ifndef VIEW_LAYER_SELECTION_H
#define VIEW_LAYER_SELECTION_H

#include "viewlayer.h"

#include <QRectF>
#include <QPointF>

#include "Private/document.h"
#include "skia/skiaincludes.h"
#include "MovablePoints/segment.h"
#include "MovablePoints/movablepoint.h"

class BaseCanvas;


class ViewLayerSelection : public ViewLayer {
public:
    ViewLayerSelection(BaseCanvas *canvas);
    ~ViewLayerSelection() = default;

    static ViewLayerSelection *sGetInstance() { return sInstance; }

    void repaint(SkCanvas * const canvas) override;

    bool getPivotLocal() const;

    void isBoxSelectionEmpty();
    void isPointSelectionEmpty();

    void selectAllAction();
    void invertSelectionAction();
    void clearSelectionAction();

    void selectedPathsBreakApart();
    void startSelectedStrokeColorTransform();
    void startSelectedFillColorTransform();

    void cutAction();
    void pasteAction();
    void copyAction();
    void deleteAction();
    void duplicateAction();

    void setSelectedFontText(text);
    void setSelectedFontSize(size);
    void setSelectedFontFamilyAndStyle(family, style);
    void setSelectedTextVAlignment(alignment);
    void setSelectedTextAlignment(alignment);

    void flipSelectedBoxesVertically();
    void flipSelectedBoxesHorizontally();
    void rotateSelectedBoxesStartAndFinish();

    void scaleSelectedBy(const qreal scaleXBy,
                         const qreal scaleYBy,
                         const QPointF &absOrigin,
                         const bool startTrans);

    void raiseSelectedBoxesToTop();
    void lowerSelectedBoxesToBottom();
    void raiseSelectedBoxes();
    void lowerSelectedBoxes();

    void alignSelectedBoxes(const Qt::Alignment align,
                            const AlignPivot pivot,
                            const AlignRelativeTo relativeTo);
    void applyCurrentTransformToSelected();

    // Movable points
    void addPointToSelection(MovablePoint * const point);
    void removePointFromSelection(MovablePoint * const point);

    int getPointsSelectionCount() const;
    void clearPointsSelectionOrDeselect();

    void clearPointsSelection();
    void mergePoints();
    void disconnectPoints();
    bool connectPoints();
    void subdivideSegments();

    QPointF getSelectedPointsAbsPivotPos();
    bool isPointSelectionEmpty() const;
    void scaleSelectedPointsBy(const qreal scaleXBy,
                               const qreal scaleYBy,
                               const QPointF &absOrigin,
                               const bool startTrans);
    void rotateSelectedPointsBy(const qreal rotBy,
                                const QPointF &absOrigin,
                                const bool startTrans);
    int getPointsSelectionCount() const;

    void moveSelectedPointsByAbs(const QPointF &by,
                                 const bool startTransform);
    void finishSelectedPointsTransform();

    void shiftAllPointsForAllKeys(const int by);
    void revertAllPointsForAllKeys();
    void shiftAllPoints(const int by);
    void revertAllPoints();

    void selectAllPointsAction();

    // Mouse events
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseDoubleClickEvent(QMouseEvent *e) override;

signals:
    void pointSelectionChanged();

private:
    BaseCanvas *_baseCanvas;

    static ViewLayerSelection *sInstance;

    bool _isCurrentlySelecting;

    QRectF _selectionRect;
    QRectF _cursorPosition;
};

#endif // VIEW_LAYER_SELECTION_H
