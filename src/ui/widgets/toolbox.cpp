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

#include "toolbox.h"
#include "themesupport.h"

#include <QToolButton>

using namespace Friction::Ui;

ToolBox::ToolBox(Actions &actions,
                 Document &document,
                 QWidget *parent)
    : QObject{parent}
    , mActions(actions)
    , mDocument(document)
    , mMain(nullptr)
    , mControls(nullptr)
    , mExtra(nullptr)
    , mGroupMain(nullptr)
    , mGroupNodes(nullptr)
    , mGroupDraw(nullptr)
    , mGroupColorPicker(nullptr)
    , mDrawPathMaxError(nullptr)
    , mDrawPathSmooth(nullptr)
    , mLocalPivot(nullptr)
    , mColorPickerButton(nullptr)
    , mColorPickerLabel(nullptr)
{
    setupToolBox(parent);
}

QToolBar *ToolBox::getToolBar(const Type &type)
{
    switch (type) {
    case Type::Controls:
        return mControls;
    case Type::Extra:
        return mExtra;
    default:
        return mMain;
    }
    return nullptr;
}

const QList<QAction*> ToolBox::getMainActions()
{
    return mGroupMain->actions();
}

const QList<QAction*> ToolBox::getNodeActions()
{
    return mGroupNodes->actions();
}

void ToolBox::setMovable(const bool movable)
{
    mMain->setMovable(movable);
    mControls->setMovable(movable);
    if (mExtra) { mExtra->setMovable(movable); }
}

void ToolBox::setupToolBox(QWidget *parent)
{
    if (!parent) { return; }

    mMain = new ToolBar(tr("ToolBox"),
                        "ToolBoxMain",
                        parent,
                        true);
    mControls = new ToolControls(parent);
    // disable for now
    /*mExtra = new ToolboxToolBar(tr("Extra Tools"),
                                "ToolBoxExtra",
                                parent);*/

    mGroupMain = new QActionGroup(this);
    mGroupNodes = new QActionGroup(this);
    mGroupDraw = new QActionGroup(this);
    mGroupColorPicker = new QActionGroup(this);

    setupDocument();
    setupMainActions();
    setupNodesActions();
    setupDrawActions();
    setupColorPickerActions();
}

void ToolBox::setupDocument()
{
    connect(&mDocument, &Document::activeSceneSet,
            this, &ToolBox::setCurrentCanvas);
    connect(&mDocument, &Document::canvasModeSet,
            this, &ToolBox::setCanvasMode);
    connect(&mDocument, &Document::currentPixelColor,
            this, &ToolBox::updateColorPicker);
}

void ToolBox::setupMainAction(const QIcon &icon,
                              const QString &title,
                              const QKeySequence &shortcut,
                              const QList<CanvasMode> &modes,
                              const bool checked)
{
    if (modes.isEmpty() ||
        icon.isNull() ||
        title.isEmpty()) { return; }

    const auto act = new QAction(icon,
                                 title,
                                 mMain);
    act->setCheckable(true);
    act->setChecked(checked);
    act->setShortcut(shortcut);
    mGroupMain->addAction(act);

    connect(act,
            &QAction::triggered,
            this,
            [this, modes](bool checked) {
        if (!checked) { return; }
        switch (modes.at(0)) {
        case CanvasMode::boxTransform:
            mActions.setMovePathMode();
            break;
        case CanvasMode::pointTransform:
            mActions.setMovePointMode();
            break;
        case CanvasMode::pathCreate:
            mActions.setAddPointMode();
            break;
        case CanvasMode::drawPath:
            mActions.setDrawPathMode();
            break;
        case CanvasMode::circleCreate:
            mActions.setCircleMode();
            break;
        case CanvasMode::rectCreate:
            mActions.setRectangleMode();
            break;
        case CanvasMode::textCreate:
            mActions.setTextMode();
            break;
        case CanvasMode::nullCreate:
            mActions.setNullMode();
            break;
        case CanvasMode::pickFillStroke:
            mActions.setPickPaintSettingsMode();
            break;
        default:;
        }
    });
    connect(&mDocument,
            &Document::canvasModeSet,
            this,
            [act, modes](CanvasMode mode) {
        if (modes.contains(mode)) { act->setChecked(true); }
    });
}

void ToolBox::setupMainActions()
{
    setupMainAction(QIcon::fromTheme("boxTransform"),
                    tr("Object Mode"),
                    QKeySequence(AppSupport::getSettings("shortcuts",
                                                         "boxTransform",
                                                         "F1").toString()),
                    {CanvasMode::boxTransform},
                    true);
    setupMainAction(QIcon::fromTheme("pointTransform"),
                    tr("Point Mode"),
                    QKeySequence(AppSupport::getSettings("shortcuts",
                                                         "pointTransform",
                                                         "F2").toString()),
                    {CanvasMode::pointTransform},
                    false);
    setupMainAction(QIcon::fromTheme("pathCreate"),
                    tr("Add Path"),
                    QKeySequence(AppSupport::getSettings("shortcuts",
                                                         "pathCreate",
                                                         "F3").toString()),
                    {CanvasMode::pathCreate},
                    false);
    setupMainAction(QIcon::fromTheme("drawPath"),
                    tr("Draw Path"),
                    QKeySequence(AppSupport::getSettings("shortcuts",
                                                         "drawPath",
                                                         "F4").toString()),
                    {CanvasMode::drawPath},
                    false);
    setupMainAction(QIcon::fromTheme("circleCreate"),
                    tr("Add Circle"),
                    QKeySequence(AppSupport::getSettings("shortcuts",
                                                         "circleMode",
                                                         "F5").toString()),
                    {CanvasMode::circleCreate},
                    false);
    setupMainAction(QIcon::fromTheme("rectCreate"),
                    tr("Add Rectangle"),
                    QKeySequence(AppSupport::getSettings("shortcuts",
                                                         "rectMode",
                                                         "F6").toString()),
                    {CanvasMode::rectCreate},
                    false);
    setupMainAction(QIcon::fromTheme("textCreate"),
                    tr("Add Text"),
                    QKeySequence(AppSupport::getSettings("shortcuts",
                                                         "textMode",
                                                         "F7").toString()),
                    {CanvasMode::textCreate},
                    false);
    setupMainAction(QIcon::fromTheme("nullCreate"),
                    tr("Add Null Object"),
                    QKeySequence(AppSupport::getSettings("shortcuts",
                                                         "nullMode",
                                                         "F8").toString()),
                    {CanvasMode::nullCreate},
                    false);
    setupMainAction(QIcon::fromTheme("pick"),
                    tr("Color Pick Mode"),
                    QKeySequence(AppSupport::getSettings("shortcuts",
                                                         "pickMode",
                                                         "F9").toString()),
                    {CanvasMode::pickFillStroke,
                     CanvasMode::pickFillStrokeEvent},
                    false);

    // local pivot
    mLocalPivot = new QAction(mDocument.fLocalPivot ?
                                  QIcon::fromTheme("pivotLocal") :
                                  QIcon::fromTheme("pivotGlobal"),
                              tr("Pivot Global / Local"),
                              mMain);
    mLocalPivot->setShortcut(QKeySequence(AppSupport::getSettings("shortcuts",
                                                                  "localPivot",
                                                                  "P").toString()));
    connect(mLocalPivot, &QAction::triggered,
            this, [this]() {
        mDocument.fLocalPivot = !mDocument.fLocalPivot;
        for (const auto& scene : mDocument.fScenes) { scene->updatePivot(); }
        Document::sInstance->actionFinished();
        mLocalPivot->setIcon(mDocument.fLocalPivot ?
                         QIcon::fromTheme("pivotLocal") :
                         QIcon::fromTheme("pivotGlobal"));
    });
    mGroupMain->addAction(mLocalPivot);

    mMain->addActions(mGroupMain->actions());
}

void ToolBox::setupNodesAction(const QIcon &icon,
                               const QString &title,
                               const Node &node)
{

    const auto act = new QAction(icon, title, this);
    connect(act, &QAction::triggered,
            this, [this, node]() {
        switch (node) {
        case NodeConnect:
            mActions.connectPointsSlot();
            break;
        case NodeDisconnect:
            mActions.disconnectPointsSlot();
            break;
        case NodeMerge:
            mActions.mergePointsSlot();
            break;
        case NodeNew:
            mActions.subdivideSegments();
            break;
        case NodeSymmetric:
            mActions.makePointCtrlsSymmetric();
            break;
        case NodeSmooth:
            mActions.makePointCtrlsSmooth();
            break;
        case NodeCorner:
            mActions.makePointCtrlsCorner();
            break;
        case NodeSegmentLine:
            mActions.makeSegmentLine();
            break;
        case NodeSegmentCurve:
            mActions.makeSegmentCurve();
            break;
        default:;
        }
    });
    mControls->addAction(mGroupNodes->addAction(act));
    ThemeSupport::setToolbarButtonStyle("ToolBoxButton", mControls, act);
}

void ToolBox::setupNodesActions()
{
    mGroupNodes->addAction(mControls->addSpacer(true, true));
    mControls->addAction(mGroupNodes->addAction(QIcon::fromTheme("pointTransform"),
                                                tr("Nodes")));

    setupNodesAction(QIcon::fromTheme("nodeConnect"),
                     tr("Connect Nodes"), NodeConnect);
    setupNodesAction(QIcon::fromTheme("nodeDisconnect"),
                     tr("Disconnect Nodes"), NodeDisconnect);
    setupNodesAction(QIcon::fromTheme("nodeMerge"),
                     tr("Merge Nodes"), NodeMerge);
    setupNodesAction(QIcon::fromTheme("nodeNew"),
                     tr("New Node"), NodeNew);
    setupNodesAction(QIcon::fromTheme("nodeSymmetric"),
                     tr("Symmetric Nodes"), NodeSymmetric);
    setupNodesAction(QIcon::fromTheme("nodeSmooth"),
                     tr("Smooth Nodes"), NodeSmooth);
    setupNodesAction(QIcon::fromTheme("nodeCorner"),
                     tr("Corner Nodes"), NodeCorner);
    setupNodesAction(QIcon::fromTheme("segmentLine"),
                     tr("Make Segment Line"), NodeSegmentLine);
    setupNodesAction(QIcon::fromTheme("segmentCurve"),
                     tr("Make Segment Curve"), NodeSegmentCurve);

    {
        // node visibility tool button
        const auto button = new QToolButton(mControls);
        button->setObjectName("ToolBoxButton");
        button->setPopupMode(QToolButton::InstantPopup);
        button->setFocusPolicy(Qt::NoFocus);
        const auto act1 = new QAction(QIcon::fromTheme("dissolvedAndNormalNodes"),
                                      tr("Dissolved and normal nodes"),
                                      this);
        act1->setData(0);
        const auto act2 = new QAction(QIcon::fromTheme("dissolvedNodesOnly"),
                                      tr("Dissolved nodes only"),
                                      this);
        act2->setData(1);
        const auto act3 = new QAction(QIcon::fromTheme("normalNodesOnly"),
                                      tr("Normal nodes only"),
                                      this);
        act3->setData(2);
        button->addAction(act1);
        button->addAction(act2);
        button->addAction(act3);
        button->setDefaultAction(act1);
        connect(button, &QToolButton::triggered,
                this, [this, button](QAction *act) {
            button->setDefaultAction(act);
            mDocument.fNodeVisibility = static_cast<NodeVisiblity>(act->data().toInt());
            Document::sInstance->actionFinished();
        });
        mGroupNodes->addAction(mControls->addWidget(button));
    }

    mGroupNodes->setEnabled(false);
    mGroupNodes->setVisible(false);
}

void ToolBox::setupDrawActions()
{
    mDrawPathMaxError = new QDoubleSlider(1, 200, 1, mControls, false);
    mDrawPathMaxError->setNumberDecimals(0);
    mDrawPathMaxError->setMinimumWidth(50);
    mDrawPathMaxError->setDisplayedValue(mDocument.fDrawPathMaxError);
    connect(mDrawPathMaxError, &QDoubleSlider::valueEdited,
            this, [this](const qreal value) {
        mDocument.fDrawPathMaxError = qFloor(value);
    });

    mDrawPathSmooth = new QDoubleSlider(1, 200, 1, mControls, false);
    mDrawPathSmooth->setNumberDecimals(0);
    mDrawPathSmooth->setMinimumWidth(50);
    mDrawPathSmooth->setDisplayedValue(mDocument.fDrawPathSmooth);
    connect(mDrawPathSmooth, &QDoubleSlider::valueEdited,
            this, [this](const qreal value) {
        mDocument.fDrawPathSmooth = qFloor(value);
    });

    const auto labelMax = new QLabel(tr("Max Error"), mControls);
    const auto labelSmooth = new QLabel(tr("Smooth"), mControls);

    mGroupDraw->addAction(mControls->addSpacer(true, true));
    mGroupDraw->addAction(mControls->addAction(QIcon::fromTheme("drawPath"),
                                               QString()));
    mGroupDraw->addAction(mControls->addWidget(labelMax));
    mGroupDraw->addAction(mControls->addSeparator());
    mGroupDraw->addAction(mControls->addWidget(mDrawPathMaxError));

    mGroupDraw->addAction(mControls->addSpacer(true, true));

    mGroupDraw->addAction(mControls->addAction(QIcon::fromTheme("drawPath"),
                                               QString()));
    mGroupDraw->addAction(mControls->addWidget(labelSmooth));
    mGroupDraw->addAction(mControls->addSeparator());
    mGroupDraw->addAction(mControls->addWidget(mDrawPathSmooth));
    mGroupDraw->addAction(mControls->addSpacer(true, true));

    {
        const auto act = new QAction(mDocument.fDrawPathManual ?
                                         QIcon::fromTheme("drawPathAutoUnchecked") :
                                         QIcon::fromTheme("drawPathAutoChecked"),
                                     tr("Automatic/Manual Fitting"),
                                     this);
        connect(act, &QAction::triggered,
                this, [this, act]() {
            mDocument.fDrawPathManual = !mDocument.fDrawPathManual;
            mDrawPathMaxError->setDisabled(mDocument.fDrawPathManual);
            act->setIcon(mDocument.fDrawPathManual ?
                             QIcon::fromTheme("drawPathAutoUnchecked") :
                             QIcon::fromTheme("drawPathAutoChecked"));
        });
        mControls->addAction(act);
        mGroupDraw->addAction(act);
        ThemeSupport::setToolbarButtonStyle("ToolBoxButton", mControls, act);
    }

    mGroupDraw->setEnabled(false);
    mGroupDraw->setVisible(false);
}

void ToolBox::setupColorPickerActions()
{
    mColorPickerButton = new QToolButton(mControls);
    mColorPickerButton->setObjectName("FlatButton");
    mColorPickerButton->setIcon(QIcon::fromTheme("pick"));
    mColorPickerLabel = new QLabel(mControls);

    mGroupColorPicker->addAction(mControls->addSpacer(true, true));
    mGroupColorPicker->addAction(mControls->addSeparator());
    mGroupColorPicker->addAction(mControls->addWidget(mColorPickerButton));
    mGroupColorPicker->addAction(mControls->addWidget(mColorPickerLabel));

    mGroupColorPicker->setVisible(false);
}

void ToolBox::setCurrentCanvas(Canvas * const target)
{
    mControls->setCurrentCanvas(target);
    if (mExtra) { mExtra->setCurrentCanvas(target); }
}

void ToolBox::setCanvasMode(const CanvasMode &mode)
{
    const bool boxMode = mode == CanvasMode::boxTransform;
    const bool pointMode = mode == CanvasMode::pointTransform;
    const bool drawMode = mode == CanvasMode::drawPath;
    const bool pickMode = mode == CanvasMode::pickFillStroke ||
                          mode == CanvasMode::pickFillStrokeEvent;

    mGroupNodes->setEnabled(pointMode);
    mGroupNodes->setVisible(pointMode);

    mGroupDraw->setEnabled(drawMode);
    mGroupDraw->setVisible(drawMode);

    mLocalPivot->setEnabled(boxMode || pointMode);

    if (mExtra) { mExtra->setCanvasMode(mode); }

    mGroupColorPicker->setVisible(pickMode);
    if (pickMode) { updateColorPicker(Qt::black); }
}

void ToolBox::updateColorPicker(const QColor &color)
{
    if (!mColorPickerButton || !mColorPickerLabel) { return; }
    mColorPickerButton->setStyleSheet(QString("background-color: %1;").arg(color.isValid() ?
                                                                               color.name() :
                                                                               "black"));
    mColorPickerLabel->setText(QString("&nbsp;"
                                       "<b>R:</b> %1 "
                                       "<b>G:</b> %2 "
                                       "<b>B:</b> %3")
                                   .arg(QString::number(color.isValid() ? color.redF() : 0., 'f', 3),
                                        QString::number(color.isValid() ? color.greenF() : 0., 'f', 3),
                                        QString::number(color.isValid() ? color.blueF() : 0., 'f', 3)));
}
