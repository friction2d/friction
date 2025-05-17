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

#include "mainwindow.h"

#include "appsupport.h"
#include "widgets/vlabel.h"
#include "widgets/toolbar.h"

using namespace Friction;

void MainWindow::setupToolBox()
{
    setupToolBoxMain();
    setupToolBoxNodes();
    setupToolBoxDraw();

    // set default
    mDocument.setCanvasMode(CanvasMode::boxTransform);
}

void MainWindow::setupToolBoxMain()
{
    mToolBoxMain = new Ui::ToolBar(tr("ToolBox"),
                                   "ToolBoxMain",
                                   this,
                                   true);

    mToolBoxGroupMain = new QActionGroup(this);

    // boxTransform
    QAction *boxTransformAct = new QAction(QIcon::fromTheme("boxTransform"),
                                           tr("Object Mode"),
                                           this);
    boxTransformAct->setCheckable(true);
    boxTransformAct->setShortcut(QKeySequence(AppSupport::getSettings("shortcuts",
                                                                      "boxTransform",
                                                                      "F1").toString()));
    connect(boxTransformAct,
            &QAction::triggered,
            this,
            [boxTransformAct, this]() {
        if (boxTransformAct->isChecked()) { mActions.setMovePathMode(); }
    });
    connect(&mDocument,
            &Document::canvasModeSet,
            this,
            [this, boxTransformAct]() {
        if (mDocument.fCanvasMode == CanvasMode::boxTransform) {
            boxTransformAct->setChecked(true);
        }
    });
    boxTransformAct->setChecked(true); // default
    mToolBoxGroupMain->addAction(boxTransformAct);

    // pointTransform
    QAction *pointTransformAct = new QAction(QIcon::fromTheme("pointTransform"),
                                             tr("Point Mode"),
                                             this);
    pointTransformAct->setCheckable(true);
    pointTransformAct->setShortcut(QKeySequence(AppSupport::getSettings("shortcuts",
                                                                        "pointTransform",
                                                                        "F2").toString()));
    connect(pointTransformAct,
            &QAction::triggered,
            this,
            [pointTransformAct, this]() {
        if (pointTransformAct->isChecked()) { mActions.setMovePointMode(); }
    });
    connect(&mDocument,
            &Document::canvasModeSet,
            this,
            [this, pointTransformAct]() {
        if (mDocument.fCanvasMode == CanvasMode::pointTransform) {
            pointTransformAct->setChecked(true);
        }
    });
    mToolBoxGroupMain->addAction(pointTransformAct);

    // addPointMode
    QAction *addPointModeAct = new QAction(QIcon::fromTheme("pathCreate"),
                                           tr("Add Path"),
                                           this);
    addPointModeAct->setCheckable(true);
    addPointModeAct->setShortcut(QKeySequence(AppSupport::getSettings("shortcuts",
                                                                      "pathCreate",
                                                                      "F3").toString()));
    connect(addPointModeAct,
            &QAction::triggered,
            this,
            [addPointModeAct, this]() {
        if (addPointModeAct->isChecked()) { mActions.setAddPointMode(); }
    });
    connect(&mDocument,
            &Document::canvasModeSet,
            this,
            [this, addPointModeAct]() {
        if (mDocument.fCanvasMode == CanvasMode::pathCreate) {
            addPointModeAct->setChecked(true);
        }
    });
    mToolBoxGroupMain->addAction(addPointModeAct);

    // drawPathMode
    QAction *drawPathModeAct = new QAction(QIcon::fromTheme("drawPath"),
                                           tr("Draw Path"),
                                           this);
    drawPathModeAct->setCheckable(true);
    drawPathModeAct->setShortcut(QKeySequence(AppSupport::getSettings("shortcuts",
                                                                      "drawPath",
                                                                      "F4").toString()));
    connect(drawPathModeAct,
            &QAction::triggered,
            this,
            [drawPathModeAct, this]() {
        if (drawPathModeAct->isChecked()) { mActions.setDrawPathMode(); }
    });
    connect(&mDocument,
            &Document::canvasModeSet,
            this,
            [this, drawPathModeAct]() {
        if (mDocument.fCanvasMode == CanvasMode::drawPath) {
            drawPathModeAct->setChecked(true);
        }
    });
    mToolBoxGroupMain->addAction(drawPathModeAct);

    // circleMode
    QAction *circleModeAct = new QAction(QIcon::fromTheme("circleCreate"),
                                         tr("Add Circle"),
                                         this);
    circleModeAct->setCheckable(true);
    circleModeAct->setShortcut(QKeySequence(AppSupport::getSettings("shortcuts",
                                                                    "circleMode",
                                                                    "F5").toString()));
    connect(circleModeAct,
            &QAction::triggered,
            this,
            [circleModeAct, this]() {
        if (circleModeAct->isChecked()) { mActions.setCircleMode(); }
    });
    connect(&mDocument,
            &Document::canvasModeSet,
            this,
            [this, circleModeAct]() {
        if (mDocument.fCanvasMode == CanvasMode::circleCreate) {
            circleModeAct->setChecked(true);
        }
    });
    mToolBoxGroupMain->addAction(circleModeAct);

    // rectangleMode
    QAction *rectModeAct = new QAction(QIcon::fromTheme("rectCreate"),
                                       tr("Add Rectangle"),
                                       this);
    rectModeAct->setCheckable(true);
    rectModeAct->setShortcut(QKeySequence(AppSupport::getSettings("shortcuts",
                                                                  "rectMode",
                                                                  "F6").toString()));
    connect(rectModeAct,
            &QAction::triggered,
            this,
            [rectModeAct, this]() {
        if (rectModeAct->isChecked()) { mActions.setRectangleMode(); }
    });
    connect(&mDocument,
            &Document::canvasModeSet,
            this,
            [this, rectModeAct]() {
        if (mDocument.fCanvasMode == CanvasMode::rectCreate) {
            rectModeAct->setChecked(true);
        }
    });
    mToolBoxGroupMain->addAction(rectModeAct);

    // textMode
    QAction *textModeAct = new QAction(QIcon::fromTheme("textCreate"),
                                       tr("Add Text"),
                                       this);
    textModeAct->setCheckable(true);
    textModeAct->setShortcut(QKeySequence(AppSupport::getSettings("shortcuts",
                                                                  "textMode",
                                                                  "F7").toString()));
    connect(textModeAct,
            &QAction::triggered,
            this,
            [textModeAct, this]() {
        if (textModeAct->isChecked()) { mActions.setTextMode(); }
    });
    connect(&mDocument,
            &Document::canvasModeSet,
            this,
            [this, textModeAct]() {
        if (mDocument.fCanvasMode == CanvasMode::textCreate) {
            focusFontWidget(true);
            textModeAct->setChecked(true);
        }
    });
    mToolBoxGroupMain->addAction(textModeAct);

    // nullMode
    QAction *nullModeAct = new QAction(QIcon::fromTheme("nullCreate"),
                                       tr("Add Null Object"),
                                       this);
    nullModeAct->setCheckable(true);
    nullModeAct->setShortcut(QKeySequence(AppSupport::getSettings("shortcuts",
                                                                  "nullMode",
                                                                  "F8").toString()));
    connect(nullModeAct,
            &QAction::triggered,
            this,
            [nullModeAct, this]() {
        if (nullModeAct->isChecked()) { mActions.setNullMode(); }
    });
    connect(&mDocument,
            &Document::canvasModeSet,
            this,
            [this, nullModeAct]() {
        if (mDocument.fCanvasMode == CanvasMode::nullCreate) {
            nullModeAct->setChecked(true);
        }
    });
    mToolBoxGroupMain->addAction(nullModeAct);

    // pickMode
    QAction *pickModeAct = new QAction(QIcon::fromTheme("pick"),
                                       tr("Color Pick Mode"),
                                       this);
    pickModeAct->setCheckable(true);
    pickModeAct->setShortcut(QKeySequence(AppSupport::getSettings("shortcuts",
                                                                  "pickMode",
                                                                  "F9").toString()));
    connect(pickModeAct,
            &QAction::triggered,
            this,
            [pickModeAct, this]() {
        if (pickModeAct->isChecked()) { mActions.setPickPaintSettingsMode(); }
    });
    connect(&mDocument,
            &Document::canvasModeSet,
            this,
            [this, pickModeAct]() {
        if (mDocument.fCanvasMode == CanvasMode::pickFillStroke ||
            mDocument.fCanvasMode == CanvasMode::pickFillStrokeEvent) {
            pickModeAct->setChecked(true);
        }
    });

    mToolBoxGroupMain->addAction(pickModeAct);

    // pivot
    mLocalPivotAct = new QAction(mDocument.fLocalPivot ?
                                     QIcon::fromTheme("pivotLocal") :
                                     QIcon::fromTheme("pivotGlobal"),
                                 tr("Pivot Global / Local"),
                                 this);
    mLocalPivotAct->setShortcut(QKeySequence(AppSupport::getSettings("shortcuts",
                                                                     "localPivot",
                                                                     "P").toString()));
    connect(mLocalPivotAct, &QAction::triggered,
            this, [this]() {
        mDocument.fLocalPivot = !mDocument.fLocalPivot;
        for (const auto& scene : mDocument.fScenes) { scene->updatePivot(); }
        Document::sInstance->actionFinished();
        mLocalPivotAct->setIcon(mDocument.fLocalPivot ?
                                    QIcon::fromTheme("pivotLocal") :
                                    QIcon::fromTheme("pivotGlobal"));
    });

    mToolBoxGroupMain->addAction(mLocalPivotAct);
    mToolBoxMain->addActions(mToolBoxGroupMain->actions());

    addToolBar(Qt::LeftToolBarArea, mToolBoxMain);
}

void MainWindow::setupToolBoxNodes()
{
    mToolBoxGroupNodes = new QActionGroup(this);

    // nodeConnect
    mActionConnectPointsAct = new QAction(QIcon::fromTheme("nodeConnect"),
                                          tr("Connect Nodes"),
                                          this);
    cmdAddAction(mActionConnectPointsAct);
    connect(mActionConnectPointsAct, &QAction::triggered,
            this, [this]() { mActions.connectPointsSlot(); });
    mToolBoxGroupNodes->addAction(mActionConnectPointsAct);

    // nodeDisconnect
    mActionDisconnectPointsAct = new QAction(QIcon::fromTheme("nodeDisconnect"),
                                             tr("Disconnect Nodes"),
                                             this);
    cmdAddAction(mActionDisconnectPointsAct);
    connect(mActionDisconnectPointsAct, &QAction::triggered,
            this, [this]() { mActions.disconnectPointsSlot(); });
    mToolBoxGroupNodes->addAction(mActionDisconnectPointsAct);

    // nodeMerge
    mActionMergePointsAct = new QAction(QIcon::fromTheme("nodeMerge"),
                                        tr("Merge Nodes"),
                                        this);
    cmdAddAction(mActionMergePointsAct);
    connect(mActionMergePointsAct, &QAction::triggered,
            this, [this]() { mActions.mergePointsSlot(); });
    mToolBoxGroupNodes->addAction(mActionMergePointsAct);

    // nodeNew
    mActionNewNodeAct = new QAction(QIcon::fromTheme("nodeNew"),
                                    tr("New Node"),
                                    this);
    cmdAddAction(mActionNewNodeAct);
    connect(mActionNewNodeAct, &QAction::triggered,
            this, [this]() { mActions.subdivideSegments(); });
    mToolBoxGroupNodes->addAction(mActionNewNodeAct);

    // nodeSymmetric
    mActionSymmetricPointCtrlsAct = new QAction(QIcon::fromTheme("nodeSymmetric"),
                                                tr("Symmetric Nodes"),
                                                this);
    cmdAddAction(mActionSymmetricPointCtrlsAct);
    connect(mActionSymmetricPointCtrlsAct, &QAction::triggered,
            this, [this]() { mActions.makePointCtrlsSymmetric(); });
    mToolBoxGroupNodes->addAction(mActionSymmetricPointCtrlsAct);

    // nodeSmooth
    mActionSmoothPointCtrlsAct = new QAction(QIcon::fromTheme("nodeSmooth"),
                                             tr("Smooth Nodes"),
                                             this);
    cmdAddAction(mActionSmoothPointCtrlsAct);
    connect(mActionSmoothPointCtrlsAct, &QAction::triggered,
            this, [this]() { mActions.makePointCtrlsSmooth(); });
    mToolBoxGroupNodes->addAction(mActionSmoothPointCtrlsAct);

    // nodeCorner
    mActionCornerPointCtrlsAct = new QAction(QIcon::fromTheme("nodeCorner"),
                                             tr("Corner Nodes"),
                                             this);
    cmdAddAction(mActionCornerPointCtrlsAct);
    connect(mActionCornerPointCtrlsAct, &QAction::triggered,
            this, [this]() { mActions.makePointCtrlsCorner(); });
    mToolBoxGroupNodes->addAction(mActionCornerPointCtrlsAct);

    // segmentLine
    mActionLineAct = new QAction(QIcon::fromTheme("segmentLine"),
                                 tr("Make Segment Line"),
                                 this);
    cmdAddAction(mActionLineAct);
    connect(mActionLineAct, &QAction::triggered,
            this, [this]() { mActions.makeSegmentLine(); });
    mToolBoxGroupNodes->addAction(mActionLineAct);

    // segmentCurve
    mActionCurveAct = new QAction(QIcon::fromTheme("segmentCurve"),
                                  tr("Make Segment Curve"),
                                  this);
    cmdAddAction(mActionCurveAct);
    connect(mActionCurveAct, &QAction::triggered,
            this, [this]() { mActions.makeSegmentCurve(); });
    mToolBoxGroupNodes->addAction(mActionCurveAct);

    // nodeVisibility
    mNodeVisibility = new QToolButton(this);
    mNodeVisibility->setObjectName(QString::fromUtf8("ToolButton"));
    mNodeVisibility->setPopupMode(QToolButton::InstantPopup);
    mNodeVisibility->setFocusPolicy(Qt::NoFocus);
    QAction *nodeVisibilityAction1 = new QAction(QIcon::fromTheme("dissolvedAndNormalNodes"),
                                                 tr("Dissolved and normal nodes"),
                                                 this);
    nodeVisibilityAction1->setData(0);
    QAction *nodeVisibilityAction2 = new QAction(QIcon::fromTheme("dissolvedNodesOnly"),
                                                 tr("Dissolved nodes only"),
                                                 this);
    nodeVisibilityAction2->setData(1);
    QAction *nodeVisibilityAction3 = new QAction(QIcon::fromTheme("normalNodesOnly"),
                                                 tr("Normal nodes only"),
                                                 this);
    nodeVisibilityAction3->setData(2);
    mNodeVisibility->addAction(nodeVisibilityAction1);
    mNodeVisibility->addAction(nodeVisibilityAction2);
    mNodeVisibility->addAction(nodeVisibilityAction3);
    mNodeVisibility->setDefaultAction(nodeVisibilityAction1);
    connect(mNodeVisibility, &QToolButton::triggered,
            this, [this](QAction *act) {
        mNodeVisibility->setDefaultAction(act);
        mDocument.fNodeVisibility = static_cast<NodeVisiblity>(act->data().toInt());
        Document::sInstance->actionFinished();
    });

    mTransformToolBar->getRightToolBar()->addActions(mToolBoxGroupNodes->actions());
    mNodeVisibilityAct = mTransformToolBar->getRightToolBar()->addWidget(mNodeVisibility);

    setEnableToolBoxNodes(false);
}

void MainWindow::setupToolBoxDraw()
{
    mDocument.fDrawPathManual = false;
    mDrawPathAuto = new QAction(mDocument.fDrawPathManual ?
                                    QIcon::fromTheme("drawPathAutoUnchecked") :
                                    QIcon::fromTheme("drawPathAutoChecked"),
                                tr("Automatic/Manual Fitting"),
                                this);
    connect(mDrawPathAuto, &QAction::triggered,
            this, [this]() {
        mDocument.fDrawPathManual = !mDocument.fDrawPathManual;
        mDrawPathMaxError->setDisabled(mDocument.fDrawPathManual);
        mDrawPathAuto->setIcon(mDocument.fDrawPathManual ?
                                   QIcon::fromTheme("drawPathAutoUnchecked") :
                                   QIcon::fromTheme("drawPathAutoChecked"));
    });

    mDrawPathMaxError = new QDoubleSlider(1, 200, 1, this, false);
    mDrawPathMaxError->setNumberDecimals(0);
    mDrawPathMaxError->setMinimumWidth(50);
    mDrawPathMaxError->setDisplayedValue(mDocument.fDrawPathMaxError);
    connect(mDrawPathMaxError, &QDoubleSlider::valueEdited,
            this, [this](const qreal value) {
        mDocument.fDrawPathMaxError = qFloor(value);
    });

    mDrawPathSmooth = new QDoubleSlider(1, 200, 1, this, false);
    mDrawPathSmooth->setNumberDecimals(0);
    mDrawPathSmooth->setMinimumWidth(50);
    mDrawPathSmooth->setDisplayedValue(mDocument.fDrawPathSmooth);
    connect(mDrawPathSmooth, &QDoubleSlider::valueEdited,
            this, [this](const qreal value) {
        mDocument.fDrawPathSmooth = qFloor(value);
    });

    const auto label1 = new VLabel(QString("%1 ").arg(tr("Max Error")),
                                   this,
                                   mTransformToolBar->orientation());
    const auto label2 = new VLabel(QString("%1 ").arg(tr("Smooth")),
                                   this,
                                   mTransformToolBar->orientation());

    mToolBoxDrawActIcon1 = mTransformToolBar->getRightToolBar()->addAction(QIcon::fromTheme("drawPath"),
                                                                           QString());
    mToolBoxDrawActIcon1->setDisabled(true);

    mToolBoxDrawActLabel1 = mTransformToolBar->getRightToolBar()->addWidget(label1);
    mToolBoxDrawActMaxError = mTransformToolBar->getRightToolBar()->addWidget(mDrawPathMaxError);

    mToolBoxDrawActIcon2 = mTransformToolBar->getRightToolBar()->addAction(QIcon::fromTheme("drawPath"),
                                                                           QString());
    mToolBoxDrawActIcon2->setDisabled(true);

    mToolBoxDrawActLabel2 = mTransformToolBar->getRightToolBar()->addWidget(label2);
    mToolBoxDrawActSmooth = mTransformToolBar->getRightToolBar()->addWidget(mDrawPathSmooth);
    mTransformToolBar->getRightToolBar()->addAction(mDrawPathAuto);

    setEnableToolBoxDraw(false);
}

void MainWindow::setEnableToolBoxNodes(const bool &enable)
{
    mToolBoxGroupNodes->setVisible(enable);
    if (mNodeVisibilityAct) {
        mNodeVisibilityAct->setVisible(enable);
    }
}

void MainWindow::setEnableToolBoxDraw(const bool &enable)
{
    if (mToolBoxDrawActSep) { mToolBoxDrawActSep->setVisible(enable); }
    if (mToolBoxDrawActLabel1) { mToolBoxDrawActLabel1->setVisible(enable); }
    if (mToolBoxDrawActLabel2) { mToolBoxDrawActLabel2->setVisible(enable); }
    if (mToolBoxDrawActIcon1) { mToolBoxDrawActIcon1->setVisible(enable); }
    if (mToolBoxDrawActIcon2) { mToolBoxDrawActIcon2->setVisible(enable); }
    if (mToolBoxDrawActMaxError) { mToolBoxDrawActMaxError->setVisible(enable); }
    if (mToolBoxDrawActSmooth) { mToolBoxDrawActSmooth->setVisible(enable); }
    if (mDrawPathAuto) { mDrawPathAuto->setVisible(enable); }
}
