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

#include "transformtoolbar.h"
#include "Animators/qpointfanimator.h"
#include "Animators/transformanimator.h"
#include "Private/document.h"
#include "Boxes/circle.h"
#include "Boxes/rectangle.h"
#include "widgets/toolbutton.h"

#include <QWidgetAction>
#include <QStandardItemModel>
#include <QPushButton>

#define INDEX_ALIGN_GEOMETRY 0
#define INDEX_ALIGN_GEOMETRY_PIVOT 1
#define INDEX_ALIGN_PIVOT 2

#define INDEX_REL_SCENE 0
#define INDEX_REL_LAST_SELECTED 1
#define INDEX_REL_LAST_SELECTED_PIVOT 2
#define INDEX_REL_BOUNDINGBOX 3

using namespace Friction::Ui;

TransformToolBar::TransformToolBar(QWidget *parent)
    : ToolBar("TransformToolBar", parent, true)
    , mToolBarLeft(nullptr)
    , mToolBarRight(nullptr)
    , mCanvasMode(CanvasMode::boxTransform)
    , mTransformX(nullptr)
    , mTransformY(nullptr)
    , mTransformR(nullptr)
    , mTransformSX(nullptr)
    , mTransformSY(nullptr)
    , mTransformRX(nullptr)
    , mTransformRY(nullptr)
    , mTransformBX(nullptr)
    , mTransformBY(nullptr)
    , mTransformPX(nullptr)
    , mTransformPY(nullptr)
    , mTransformOX(nullptr)
    , mTransformMove(nullptr)
    , mTransformRotate(nullptr)
    , mTransformScale(nullptr)
    , mTransformRadius(nullptr)
    , mTransformBottomRight(nullptr)
    , mTransformPivot(nullptr)
    , mTransformOpacity(nullptr)
    , mTransformAlign(nullptr)
    , mTranformAlignPivot(nullptr)
    , mTransformAlignRelativeTo(nullptr)
    , mColorPicker(nullptr)
    , mColorPickerLabel(nullptr)
    , mAlignEnabled(true)
{
    setToolButtonStyle(Qt::ToolButtonIconOnly);
    setContextMenuPolicy(Qt::NoContextMenu);
    setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    setWindowTitle(tr("Transform Toolbar"));

    setupWidgets();
}

void TransformToolBar::setCurrentCanvas(Canvas * const target)
{
    mCanvas.assign(target);
    if (target) {
        mCanvas << connect(mCanvas, &Canvas::currentBoxChanged,
                           this, &TransformToolBar::setCurrentBox);
        mCanvas << connect(mCanvas, &Canvas::canvasModeSet,
                           this, &TransformToolBar::setCanvasMode);
    }
    setCurrentBox(target ? target->getCurrentBox() : nullptr);

    mToolBarLeft->setCurrentCanvas(target);
    mToolBarRight->setCurrentCanvas(target);
}

void TransformToolBar::setCurrentBox(BoundingBox * const target)
{
    setTransform(target);
    mTransformAlign->setEnabled(target && mAlignEnabled);
}

void TransformToolBar::setCanvasMode(const CanvasMode &mode)
{
    mCanvasMode = mode;

    const bool hasPivot = mTransformPX->hasTarget() && mTransformPY->hasTarget();
    const bool hasOpacity = mTransformOX->hasTarget();
    mTransformPivot->setVisible(hasPivot && mode == CanvasMode::boxTransform);
    mTransformOpacity->setVisible(hasOpacity && mode == CanvasMode::boxTransform);

    const bool hasRadius = mTransformRX->hasTarget() && mTransformRY->hasTarget();
    const bool hasRectangle = mTransformBX->hasTarget() && mTransformBY->hasTarget();
    const bool showRadiusRect = mode == CanvasMode::boxTransform ||
                                mode == CanvasMode::circleCreate ||
                                mode == CanvasMode::rectCreate;
    mTransformRadius->setVisible(hasRadius && showRadiusRect);
    mTransformBottomRight->setVisible(hasRectangle && showRadiusRect);

    const bool canShowAlign = mode == CanvasMode::boxTransform ||
                              mode == CanvasMode::pointTransform ||
                              mode == CanvasMode::circleCreate ||
                              mode == CanvasMode::rectCreate ||
                              mode == CanvasMode::textCreate;
    mTransformAlign->setVisible(canShowAlign && mAlignEnabled);
}

ViewerToolBar *TransformToolBar::getLeftToolBar()
{
    return mToolBarLeft;
}

ViewerToolBar *TransformToolBar::getRightToolBar()
{
    return mToolBarRight;
}

void TransformToolBar::updateColorPicker(const QColor &color)
{
    if (!mColorPicker || !mColorPickerLabel) { return; }
    mColorPicker->setStyleSheet(QString("background-color: %1;").arg(color.isValid() ? color.name() : "black"));
    mColorPickerLabel->setText(QString("<b>R:</b> %1 "
                                       "<b>G:</b> %2 "
                                       "<b>B:</b> %3").arg(QString::number(color.isValid() ? color.redF() : 0., 'f', 3),
                                                           QString::number(color.isValid() ? color.greenF() : 0., 'f', 3),
                                                           QString::number(color.isValid() ? color.blueF() : 0., 'f', 3)));
}

void TransformToolBar::setAlignEnabled(const bool enabled)
{
    mAlignEnabled = enabled;
    mTransformAlign->setEnabled(enabled);
    setCanvasMode(mCanvasMode);
}

bool TransformToolBar::isAlignEnabled()
{
    return mAlignEnabled;
}

void TransformToolBar::setTransform(BoundingBox * const target)
{
    const bool multiple = target ? mCanvas->getSelectedBoxesCount() > 1 : false;
    // TODO: add support for multiple boxes

    if (!target || multiple) {
        resetWidgets();
        return;
    }

    const auto animator = target->getBoxTransformAnimator();
    if (!animator) {
        resetWidgets();
        return;
    }

    const auto pos = animator->getPosAnimator();
    mTransformX->setTarget(pos ? pos->getXAnimator() : nullptr);
    mTransformY->setTarget(pos ? pos->getYAnimator() : nullptr);

    mTransformMove->setEnabled(pos);

    const auto rot = animator->getRotAnimator();
    mTransformR->setTarget(rot);

    mTransformRotate->setEnabled(rot);

    const auto scale = animator->getScaleAnimator();
    mTransformSX->setTarget(scale ? scale->getXAnimator() : nullptr);
    mTransformSY->setTarget(scale ? scale->getYAnimator() : nullptr);

    mTransformScale->setEnabled(scale);

    const auto pivot = animator->getPivotAnimator();
    mTransformPX->setTarget(pivot ? pivot->getXAnimator() : nullptr);
    mTransformPY->setTarget(pivot ? pivot->getYAnimator() : nullptr);

    mTransformPivot->setEnabled(pivot);

    const auto opacity = animator->getOpacityAnimator();
    mTransformOX->setTarget(opacity);

    mTransformOpacity->setEnabled(opacity);

    const auto circle = enve_cast<Circle*>(target);
    const auto rectangle = enve_cast<RectangleBox*>(target);

    mTransformRX->setTarget(circle ?
                                circle->getHRadiusAnimator()->getXAnimator() :
                                (rectangle ? rectangle->getRadiusAnimator()->getXAnimator() : nullptr));
    mTransformRY->setTarget(circle ?
                                circle->getVRadiusAnimator()->getYAnimator() :
                                (rectangle ? rectangle->getRadiusAnimator()->getYAnimator() : nullptr));

    mTransformBX->setTarget(rectangle ? rectangle->getBottomRightAnimator()->getXAnimator() : nullptr);
    mTransformBY->setTarget(rectangle ? rectangle->getBottomRightAnimator()->getYAnimator() : nullptr);

    const bool hasRadius = (circle || rectangle);
    mTransformRadius->setEnabled(hasRadius);
    mTransformBottomRight->setEnabled(rectangle);

    setCanvasMode(mCanvasMode);
}

void TransformToolBar::resetWidgets()
{
    mTransformX->setTarget(nullptr);
    mTransformY->setTarget(nullptr);
    mTransformR->setTarget(nullptr);
    mTransformSX->setTarget(nullptr);
    mTransformSY->setTarget(nullptr);
    mTransformRX->setTarget(nullptr);
    mTransformRY->setTarget(nullptr);
    mTransformBX->setTarget(nullptr);
    mTransformBY->setTarget(nullptr);
    mTransformPX->setTarget(nullptr);
    mTransformPY->setTarget(nullptr);
    mTransformOX->setTarget(nullptr);

    mTransformMove->setEnabled(false);
    mTransformRotate->setEnabled(false);
    mTransformScale->setEnabled(false);
    mTransformRadius->setEnabled(false);
    mTransformBottomRight->setEnabled(false);
    mTransformPivot->setEnabled(false);
    mTransformOpacity->setEnabled(false);
    mTransformAlign->setEnabled(false);

    mTransformRadius->setVisible(false);
    mTransformBottomRight->setVisible(false);
}

void TransformToolBar::setupWidgets()
{
    mTransformMove = new QActionGroup(this);
    mTransformRotate = new QActionGroup(this);
    mTransformScale = new QActionGroup(this);
    mTransformRadius = new QActionGroup(this);
    mTransformBottomRight = new QActionGroup(this);
    mTransformPivot = new QActionGroup(this);
    mTransformOpacity = new QActionGroup(this);
    mTransformAlign = new QActionGroup(this);

    mToolBarLeft = new ViewerToolBar("TransformToolBarLeft",
                                     tr("Left ToolBox"),
                                     this);
    mToolBarRight = new ViewerToolBar("TransformToolBarRight",
                                      tr("Right ToolBox"),
                                      this);

    addWidget(mToolBarLeft);
    setupTransform();
    addWidget(mToolBarRight);

    mColorPicker = new QToolButton(this);
    mColorPicker->setIcon(QIcon::fromTheme("pick"));
    mColorPickerLabel = new QLabel(this);

    mToolBarRight->addCanvasAction(CanvasMode::pickFillStroke,
                                   mToolBarRight->addSeparator());
    mToolBarRight->addCanvasWidget(CanvasMode::pickFillStroke,
                                   mColorPicker);
    mToolBarRight->addCanvasAction(CanvasMode::pickFillStroke,
                                   mToolBarRight->addSeparator());
    mToolBarRight->addCanvasWidget(CanvasMode::pickFillStroke,
                                   mColorPickerLabel);
}

void TransformToolBar::setupTransform()
{
    mTransformX = new QrealAnimatorValueSlider(nullptr, this);
    mTransformY = new QrealAnimatorValueSlider(nullptr, this);
    mTransformR = new QrealAnimatorValueSlider(nullptr, this);
    mTransformSX = new QrealAnimatorValueSlider(nullptr, this);
    mTransformSY = new QrealAnimatorValueSlider(nullptr, this);
    mTransformRX = new QrealAnimatorValueSlider(nullptr, this);
    mTransformRY = new QrealAnimatorValueSlider(nullptr, this);
    mTransformBX = new QrealAnimatorValueSlider(nullptr, this);
    mTransformBY = new QrealAnimatorValueSlider(nullptr, this);
    mTransformPX = new QrealAnimatorValueSlider(nullptr, this);
    mTransformPY = new QrealAnimatorValueSlider(nullptr, this);
    mTransformOX =new QrealAnimatorValueSlider(nullptr, this);

    mTransformMove->addAction(addAction(QIcon::fromTheme("boxTransform"),
                                        tr("Move")));
    mTransformMove->addAction(addWidget(mTransformX));
    mTransformMove->addAction(addSeparator());
    mTransformMove->addAction(addWidget(mTransformY));

    mTransformRotate->addAction(addAction(QIcon::fromTheme("loop3"),
                                          tr("Rotate")));
    mTransformRotate->addAction(addWidget(mTransformR));

    mTransformScale->addAction(addAction(QIcon::fromTheme("fullscreen"),
                                         tr("Scale")));
    mTransformScale->addAction(addWidget(mTransformSX));
    mTransformScale->addAction(addSeparator());
    mTransformScale->addAction(addWidget(mTransformSY));

    mTransformPivot->addAction(addAction(QIcon::fromTheme("pivot"),
                                         tr("Pivot")));
    mTransformPivot->addAction(addWidget(mTransformPX));
    mTransformPivot->addAction(addSeparator());
    mTransformPivot->addAction(addWidget(mTransformPY));

    mTransformOpacity->addAction(addAction(QIcon::fromTheme("alpha"),
                                           tr("Opacity")));
    mTransformOpacity->addAction(addWidget(mTransformOX));

    mTransformBottomRight->addAction(addAction(QIcon::fromTheme("rectCreate"),
                                              tr("Rectangle")));
    mTransformBottomRight->addAction(addWidget(mTransformBX));
    mTransformBottomRight->addAction(addSeparator());
    mTransformBottomRight->addAction(addWidget(mTransformBY));

    mTransformRadius->addAction(addAction(QIcon::fromTheme("circleCreate"),
                                          tr("Radius")));
    mTransformRadius->addAction(addWidget(mTransformRX));
    mTransformRadius->addAction(addSeparator());
    mTransformRadius->addAction(addWidget(mTransformRY));

    setupAlign();
    resetWidgets();

    mTransformX->setValueRange(0, 1);
    mTransformX->setDisplayedValue(0);
    mTransformY->setValueRange(0, 1);
    mTransformY->setDisplayedValue(0);
    mTransformR->setValueRange(0, 1);
    mTransformR->setDisplayedValue(0);
    mTransformSX->setValueRange(0, 1);
    mTransformSX->setDisplayedValue(0);
    mTransformSY->setValueRange(0, 1);
    mTransformSY->setDisplayedValue(0);
    mTransformRX->setValueRange(0, 1);
    mTransformRX->setDisplayedValue(0);
    mTransformRY->setValueRange(0, 1);
    mTransformRY->setDisplayedValue(0);
    mTransformBX->setValueRange(0, 1);
    mTransformBX->setDisplayedValue(0);
    mTransformBY->setValueRange(0, 1);
    mTransformBY->setDisplayedValue(0);
    mTransformPX->setValueRange(0, 1);
    mTransformPX->setDisplayedValue(0);
    mTransformPY->setValueRange(0, 1);
    mTransformPY->setDisplayedValue(0);
    mTransformOX->setValueRange(0, 100);
    mTransformOX->setDisplayedValue(100);
}

void TransformToolBar::setupAlign()
{
    const auto button = new ToolButton(this, false);
    const auto buttonAct = new QWidgetAction(this);

    const auto frame = new QWidget(this);
    const auto frameLayout = new QHBoxLayout(frame);

    buttonAct->setDefaultWidget(frame);
    button->addAction(buttonAct);
    button->setObjectName("AutoPopupButton");
    button->setToolTip(tr("Align (click to expand)"));
    button->setIcon(QIcon::fromTheme("alignCenter"));
    button->setFocusPolicy(Qt::NoFocus);
    button->setPopupMode(ToolButton::InstantPopup);

    mTransformAlign->addAction(addWidget(button));

    mTranformAlignPivot = new QComboBox(this);
    mTranformAlignPivot->setMinimumWidth(20);
    mTranformAlignPivot->setFocusPolicy(Qt::NoFocus);
    mTranformAlignPivot->addItem(tr("Geometry")); // INDEX_ALIGN_GEOMETRY
    mTranformAlignPivot->addItem(tr("Geometry by Pivot")); // INDEX_ALIGN_GEOMETRY_PIVOT
    mTranformAlignPivot->addItem(tr("Pivot")); // INDEX_ALIGN_PIVOT

    mTransformAlignRelativeTo = new QComboBox(this);
    mTransformAlignRelativeTo->setMinimumWidth(20);
    mTransformAlignRelativeTo->setFocusPolicy(Qt::NoFocus);
    mTransformAlignRelativeTo->addItem(tr("Scene")); // INDEX_REL_SCENE
    mTransformAlignRelativeTo->addItem(tr("Last Selected")); // INDEX_REL_LAST_SELECTED
    mTransformAlignRelativeTo->addItem(tr("Last Selected Pivot")); // INDEX_REL_LAST_SELECTED_PIVOT
    mTransformAlignRelativeTo->addItem(tr("Bounding Box")); // INDEX_REL_BOUNDINGBOX

    frameLayout->addWidget(new QLabel(tr("Align"), this));
    frameLayout->addWidget(mTranformAlignPivot);
    frameLayout->addWidget(new QLabel(tr("To"), this));
    frameLayout->addWidget(mTransformAlignRelativeTo);

    setComboBoxItemState(mTransformAlignRelativeTo, INDEX_REL_LAST_SELECTED_PIVOT, false);
    setComboBoxItemState(mTransformAlignRelativeTo, INDEX_REL_BOUNDINGBOX, false);

    const auto leftButton = new QPushButton(QIcon::fromTheme("pivot-align-left"),
                                            tr(""), this);
    leftButton->setFocusPolicy(Qt::NoFocus);
    leftButton->setToolTip(tr("Align Left"));
    connect(leftButton, &QPushButton::clicked,
            this, [this]() { triggerAlign(Qt::AlignLeft); });
    const auto mAlignLeftAct = mTransformAlign->addAction(addWidget(leftButton));
    mTransformAlign->addAction(addSeparator());

    const auto hCenterButton = new QPushButton(QIcon::fromTheme("pivot-align-hcenter"),
                                               tr(""), this);
    hCenterButton->setFocusPolicy(Qt::NoFocus);
    hCenterButton->setToolTip(tr("Align Horizontal Center"));
    connect(hCenterButton, &QPushButton::clicked,
            this, [this]() { triggerAlign(Qt::AlignHCenter); });
    mTransformAlign->addAction(addWidget(hCenterButton));
    mTransformAlign->addAction(addSeparator());

    const auto rightButton = new QPushButton(QIcon::fromTheme("pivot-align-right"),
                                             tr(""), this);
    rightButton->setFocusPolicy(Qt::NoFocus);
    rightButton->setToolTip(tr("Align Right"));
    connect(rightButton, &QPushButton::clicked,
            this, [this]() { triggerAlign(Qt::AlignRight); });
    const auto mAlignRightAct = mTransformAlign->addAction(addWidget(rightButton));
    mTransformAlign->addAction(addSeparator());

    const auto topButton = new QPushButton(QIcon::fromTheme("pivot-align-top"),
                                           tr(""), this);
    topButton->setFocusPolicy(Qt::NoFocus);
    topButton->setToolTip(tr("Align Top"));
    connect(topButton, &QPushButton::clicked,
            this, [this]() { triggerAlign(Qt::AlignTop); });
    const auto mAlignTopAct = mTransformAlign->addAction(addWidget(topButton));
    mTransformAlign->addAction(addSeparator());

    const auto vCenterButton = new QPushButton(QIcon::fromTheme("pivot-align-vcenter"),
                                               tr(""), this);
    vCenterButton->setFocusPolicy(Qt::NoFocus);
    vCenterButton->setToolTip(tr("Align Vertical Center"));
    connect(vCenterButton, &QPushButton::clicked,
            this, [this]() { triggerAlign(Qt::AlignVCenter); });
    mTransformAlign->addAction(addWidget(vCenterButton));
    mTransformAlign->addAction(addSeparator());

    const auto bottomButton = new QPushButton(QIcon::fromTheme("pivot-align-bottom"),
                                              tr(""), this);
    bottomButton->setFocusPolicy(Qt::NoFocus);
    bottomButton->setToolTip(tr("Align Bottom"));
    connect(bottomButton, &QPushButton::clicked,
            this, [this]() { triggerAlign(Qt::AlignBottom); });
    const auto mAlignBottomAct = mTransformAlign->addAction(addWidget(bottomButton));

    connect(mTranformAlignPivot,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        setComboBoxItemState(mTransformAlignRelativeTo,
                             INDEX_REL_LAST_SELECTED_PIVOT,
                             index == INDEX_ALIGN_PIVOT);
        setComboBoxItemState(mTransformAlignRelativeTo,
                             INDEX_REL_BOUNDINGBOX,
                             index == INDEX_ALIGN_PIVOT);
        if (index == INDEX_ALIGN_PIVOT) { mTransformAlignRelativeTo->setCurrentIndex(INDEX_REL_BOUNDINGBOX); }
        else { mTransformAlignRelativeTo->setCurrentIndex(INDEX_REL_SCENE); }
    });

    connect(mTransformAlignRelativeTo,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [mAlignLeftAct,
                   mAlignRightAct,
                   mAlignTopAct,
                   mAlignBottomAct](int index) {
        mAlignLeftAct->setEnabled(index != INDEX_REL_LAST_SELECTED_PIVOT);
        mAlignRightAct->setEnabled(index != INDEX_REL_LAST_SELECTED_PIVOT);
        mAlignTopAct->setEnabled(index != INDEX_REL_LAST_SELECTED_PIVOT);
        mAlignBottomAct->setEnabled(index != INDEX_REL_LAST_SELECTED_PIVOT);
    });
}

void TransformToolBar::triggerAlign(const Qt::Alignment &align)
{
    if (!mCanvas) { return; }

    const auto alignPivot = static_cast<AlignPivot>(mTranformAlignPivot->currentIndex());
    const auto relativeTo = static_cast<AlignRelativeTo>(mTransformAlignRelativeTo->currentIndex());

    mCanvas->alignSelectedBoxes(align, alignPivot, relativeTo);
    mCanvas->finishedAction();
}

void TransformToolBar::setComboBoxItemState(QComboBox *box,
                                            int index,
                                            bool enabled)
{
    auto model = qobject_cast<QStandardItemModel*>(box->model());
    if (!model) { return; }

    if (index >= box->count() || index < 0) { return; }

    auto item = model->item(index);
    if (!item) { return; }

    item->setEnabled(enabled);
}
