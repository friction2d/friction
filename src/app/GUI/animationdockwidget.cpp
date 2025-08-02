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

#include "animationdockwidget.h"
#include "GUI/keysview.h"
#include "appsupport.h"
#include "Private/esettings.h"

#include <QMenu>

AnimationDockWidget::AnimationDockWidget(QWidget *parent,
                                         KeysView *keysView)
    : QToolBar(parent)
    , mLineButton(nullptr)
    , mCurveButton(nullptr)
    , mSymmetricButton(nullptr)
    , mSmoothButton(nullptr)
    , mCornerButton(nullptr)
    , mFitToHeightButton(nullptr)
    , mFitToWidthButton(nullptr)
{
    setObjectName(QString::fromUtf8("animationDockWidget"));
    setSizePolicy(QSizePolicy::Maximum,
                  QSizePolicy::Maximum);

    const auto easingButton = new QPushButton(QIcon::fromTheme("easing"),
                                              QString(), this);
    easingButton->setToolTip(tr("Ease between keyframes"));
    easingButton->setFocusPolicy(Qt::NoFocus);
    easingButton->setSizePolicy(QSizePolicy::Expanding,
                                QSizePolicy::Expanding);
    generateEasingActions(easingButton, keysView);

    mFitToWidthButton = new QAction(QIcon::fromTheme("zoom_horizontal"),
                                    tr("Fit Horizontal"), this);
    connect(mFitToWidthButton, &QAction::triggered,
            keysView, &KeysView::keyframeZoomHorizontalAction);

    mLineButton = new QAction(QIcon::fromTheme("segmentLine"),
                              tr("Make Segment Line"), this);
    connect(mLineButton, &QAction::triggered,
            keysView, &KeysView::graphMakeSegmentsLinearAction);

    mCurveButton = new QAction(QIcon::fromTheme("segmentCurve"),
                               tr("Make Segment Curve"), this);
    connect(mCurveButton, &QAction::triggered,
            keysView, qOverload<>(&KeysView::graphMakeSegmentsSmoothAction));

    mSymmetricButton = new QAction(QIcon::fromTheme("nodeSymmetric"),
                                   tr("Symmetric Nodes"), this);
    connect(mSymmetricButton, &QAction::triggered,
            keysView, &KeysView::graphSetSymmetricCtrlAction);

    mSmoothButton = new QAction(QIcon::fromTheme("nodeSmooth"),
                                tr("Smooth Nodes"), this);
    connect(mSmoothButton, &QAction::triggered,
            keysView, &KeysView::graphSetSmoothCtrlAction);

    mCornerButton = new QAction(QIcon::fromTheme("nodeCorner"),
                                tr("Corner Nodes"), this);
    connect(mCornerButton, &QAction::triggered,
            keysView, &KeysView::graphSetCornerCtrlAction);

    mFitToHeightButton = new QAction(QIcon::fromTheme("zoom_vertical"),
                                     tr("Fit Vertical"), this);
    connect(mFitToHeightButton, &QAction::triggered,
            keysView, &KeysView::graphResetValueScaleAndMinShownAction);

    /*const auto valueLines = SwitchButton::sCreate2Switch(
                                "toolbarButtons/horizontalLinesChecked.png",
                                "toolbarButtons/horizontalLinesUnchecked.png",
                                gSingleLineTooltip("Show/Hide Value Lines"),
                                this);
    connect(valueLines, &SwitchButton::toggled,
            keysView, &KeysView::graphSetValueLinesDisabled);*/

    /*const auto selectedVisible = SwitchButton::sCreate2Switch(
                                     "toolbarButtons/onlySelectedVisibleUnchecked.png",
                                     "toolbarButtons/onlySelectedVisibleChecked.png",
                                     gSingleLineTooltip("View Only Selected Objects' Properties"),
                                     this);
    connect(selectedVisible, &SwitchButton::toggled,
            keysView, &KeysView::graphSetOnlySelectedVisible);*/

    addWidget(easingButton);
    addAction(mLineButton);
    addAction(mCurveButton);
    addAction(mSymmetricButton);
    addAction(mSmoothButton);
    addAction(mCornerButton);
    addAction(mFitToHeightButton);
    addAction(mFitToWidthButton);
    //addWidget(valueLines);

    eSizesUI::widget.add(this, [this](const int size) {
        setIconSize(QSize(size, size));
    });
}

void AnimationDockWidget::showGraph(const bool show)
{
    mLineButton->setVisible(show);
    mCurveButton->setVisible(show);
    mSymmetricButton->setVisible(show);
    mSmoothButton->setVisible(show);
    mCornerButton->setVisible(show);
    mFitToHeightButton->setVisible(show);
}

void AnimationDockWidget::generateEasingActions(QPushButton *button,
                                                KeysView *keysView)
{
    const QIcon easeIcon(QIcon::fromTheme("easing"));
    const QString easeInText = tr("Ease In");
    const QString easeOutText = tr("Ease Out");
    const QString easeInOutText = tr("Ease In/Out");

    const auto menu = new QMenu(this);
    const auto menuIn = new QMenu(easeInText, menu);
    const auto menuOut = new QMenu(easeOutText, menu);
    const auto menuInOut = new QMenu(easeInOutText, menu);

    menuIn->setIcon(easeIcon);
    menuOut->setIcon(easeIcon);
    menuInOut->setIcon(easeIcon);

    const auto presets = eSettings::sInstance->fExpressions.getCore("Easing");
    for (const auto &preset : presets) {
        const auto presetAct = new QAction(easeIcon, QString(), this);
        presetAct->setData(preset.id);
        presetAct->setText(preset.title);
        if (preset.categories.contains(easeInText)) {
            menuIn->addAction(presetAct);
        } else if (preset.categories.contains(easeOutText)) {
            menuOut->addAction(presetAct);
        } else if (preset.categories.contains(easeInOutText)) {
            menuInOut->addAction(presetAct);
        }
        connect(presetAct, &QAction::triggered,
                this, [presetAct, keysView]() {
            keysView->graphEasingAction(presetAct->data().toString());
        });
    }
    menu->addMenu(menuIn);
    menu->addMenu(menuOut);
    menu->addMenu(menuInOut);
    button->setMenu(menu);
}
