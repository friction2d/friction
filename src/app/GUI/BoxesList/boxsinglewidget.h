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

#ifndef BOXSINGLEWIDGET_H
#define BOXSINGLEWIDGET_H

#include "GUI/boxeslistactionbutton.h"
#include "optimalscrollarena/singlewidget.h"
#include <QLabel>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMimeData>
#include <QComboBox>
#include "skia/skiaincludes.h"
#include "smartPointers/ememory.h"
#include "framerange.h"
#include "Animators/SmartPath/smartpathanimator.h"
class QrealAnimatorValueSlider;
class TimelineMovable;
class Key;
class BoxTargetWidget;
class BoolPropertyWidget;
class ComboBoxProperty;
class ColorAnimator;
class ColorAnimatorButton;
class BoxScroller;
class eComboBox;

class BoxSingleWidget : public SingleWidget {
public:
    explicit BoxSingleWidget(BoxScroller * const parent);

    void setTargetAbstraction(SWT_Abstraction *abs);

    static QPixmap* VISIBLE_ICON;
    static QPixmap* INVISIBLE_ICON;
    static QPixmap* BOX_CHILDREN_VISIBLE_ICON;
    static QPixmap* BOX_CHILDREN_HIDDEN_ICON;
    static QPixmap* ANIMATOR_CHILDREN_VISIBLE_ICON;
    static QPixmap* ANIMATOR_CHILDREN_HIDDEN_ICON;
    static QPixmap* LOCKED_ICON;
    static QPixmap* UNLOCKED_ICON;
    static QPixmap* MUTED_ICON;
    static QPixmap* UNMUTED_ICON;
    static QPixmap* ANIMATOR_RECORDING_ICON;
    static QPixmap* ANIMATOR_NOT_RECORDING_ICON;
    static QPixmap* ANIMATOR_DESCENDANT_RECORDING_ICON;
    static QPixmap* C_ICON;
    static QPixmap* G_ICON;
    static QPixmap* CG_ICON;
    static QPixmap* GRAPH_PROPERTY_ICON;
    static QPixmap* PROMOTE_TO_LAYER_ICON;

    static bool sStaticPixmapsLoaded;
    static void loadStaticPixmaps(int iconSize);
    static void clearStaticPixmaps();

    void prp_drawTimelineControls(QPainter * const p,
                  const qreal pixelsPerFrame,
                  const FrameRange &viewedFrames);
    Key *getKeyAtPos(const int pressX,
                     const qreal pixelsPerFrame,
                     const int minViewedFrame);
    void getKeysInRect(const QRectF &selectionRect,
                       const qreal pixelsPerFrame,
                       QList<Key *> &listKeys);
    TimelineMovable *getRectangleMovableAtPos(
                        const int pressX,
                        const qreal pixelsPerFrame,
                        const int minViewedFrame);

    void setSelected(const bool selected) {
        mSelected = selected;
        update();
    }
protected:
    bool mSelected = false;
    void mousePressEvent(QMouseEvent *e);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

    void enterEvent(QEvent *);
    void leaveEvent(QEvent *);

#ifdef Q_OS_MAC
    void keyPressEvent(QKeyEvent *event);
#endif

    void paintEvent(QPaintEvent *);
    void mouseDoubleClickEvent(QMouseEvent *e);
    void resizeEvent(QResizeEvent *);

    bool mBlendModeVisible = false;
    bool mPathBlendModeVisible = false;
    bool mFillTypeVisible = false;

    void updatePathCompositionBoxVisible();
    void updateCompositionBoxVisible();
    void updateFillTypeBoxVisible();

    void clearAndHideValueAnimators();
    void updateValueSlidersForQPointFAnimator();
private:
    QString getDisplayName(Property* const prop) const;

    ContainerBox *getPromoteTargetGroup();

    void clearSelected() { setSelected(false); }
    void switchContentVisibleAction();
    void switchRecordingAction();
    void switchBoxLockedAction();

    void switchBoxVisibleAction();
    void setCompositionMode(const int id);
    void setPathCompositionMode(const int id);
    void setFillType(const int id);
    ColorAnimator* getColorTarget() const;

    void setComboProperty(ComboBoxProperty * const combo);

    void handlePropertySelectedChanged(const Property *prop);

    BoxScroller* const mParent;

    bool mDragPressPos = false;
    QPoint mDragStartPos;

    bool mHover = false;

    PixmapActionButton *mRecordButton;
    PixmapActionButton *mContentButton;
    PixmapActionButton *mVisibleButton;
    PixmapActionButton *mLockedButton;
    PixmapActionButton *mHwSupportButton;
    ColorAnimatorButton *mColorButton;
    BoxTargetWidget *mBoxTargetWidget;

    QWidget *mFillWidget;
    BoolPropertyWidget *mCheckBox;
    QHBoxLayout *mMainLayout;
    QrealAnimatorValueSlider *mValueSlider;
    QrealAnimatorValueSlider *mSecondValueSlider;

    PixmapActionButton *mPromoteToLayerButton;
    eComboBox *mPropertyComboBox;
    eComboBox *mBlendModeCombo;
    eComboBox *mPathBlendModeCombo;
    eComboBox *mFillTypeCombo;

    ConnContext mTargetConn;
};

#endif // BOXSINGLEWIDGET_H
