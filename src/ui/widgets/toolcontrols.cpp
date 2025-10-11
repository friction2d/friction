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

#include "toolcontrols.h"
#include "Animators/qpointfanimator.h"
#include "Animators/transformanimator.h"
#include "Private/document.h"
#include "Boxes/circle.h"
#include "Boxes/rectangle.h"

using namespace Friction::Ui;

ToolControls::ToolControls(QWidget *parent)
    : ToolBar("ToolControls", parent, true)
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
{
    setToolButtonStyle(Qt::ToolButtonIconOnly);
    setContextMenuPolicy(Qt::NoContextMenu);
    setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    setWindowTitle(tr("Tool Controls"));

    setupWidgets();
}

void ToolControls::setCurrentCanvas(Canvas * const target)
{
    mCanvas.assign(target);
    if (target) {
        mCanvas << connect(mCanvas, &Canvas::currentBoxChanged,
                           this, &ToolControls::setCurrentBox);
        mCanvas << connect(mCanvas, &Canvas::canvasModeSet,
                           this, &ToolControls::setCanvasMode);
    }
    setCurrentBox(target ? target->getCurrentBox() : nullptr);
}

void ToolControls::setCurrentBox(BoundingBox * const target)
{
    setTransform(target);
}

void ToolControls::setCanvasMode(const CanvasMode &mode)
{
    mCanvasMode = mode;

    const bool hasPivot = mTransformPX->hasTarget() && mTransformPY->hasTarget();
    const bool hasOpacity = mTransformOX->hasTarget();

    const bool isBoxMode = mode == CanvasMode::boxTransform;
    const bool isBoxOrPointMode = isBoxMode || mode == CanvasMode::pointTransform;

    const bool hasRadius = mTransformRX->hasTarget() && mTransformRY->hasTarget();
    const bool hasRectangle = mTransformBX->hasTarget() && mTransformBY->hasTarget();

    const bool showRectangle = isBoxOrPointMode || mode == CanvasMode::rectCreate;

    bool showRadius = showRectangle || mode == CanvasMode::circleCreate;

    // we don't want to show radius in 'point' mode for rectangle
    if (showRadius && hasRectangle && mode == CanvasMode::pointTransform) {
        showRadius = false;
    }

    mTransformPivot->setVisible(hasPivot && isBoxMode);
    mTransformOpacity->setVisible(hasOpacity && isBoxMode);
    mTransformRadius->setVisible(hasRadius && showRadius);
    mTransformBottomRight->setVisible(hasRectangle && showRectangle);
    mTransformInteract->setVisible(isBoxMode);
}

void ToolControls::setTransform(BoundingBox * const target)
{
    const bool multiple = target ? mCanvas->getSelectedBoxesCount() > 1 : false;
    // TODO: add support for multiple boxes

    if (!target || multiple) {
        resetWidgets();
        if (multiple &&
            mCanvasMode == CanvasMode::boxTransform) { mTransformInteract->setVisible(true); }
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

void ToolControls::resetWidgets()
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

    mTransformRadius->setVisible(false);
    mTransformBottomRight->setVisible(false);
    mTransformPivot->setVisible(false);
    mTransformOpacity->setVisible(false);
    mTransformInteract->setVisible(false);
}

void ToolControls::setupWidgets()
{
    mTransformMove = new QActionGroup(this);
    mTransformRotate = new QActionGroup(this);
    mTransformScale = new QActionGroup(this);
    mTransformRadius = new QActionGroup(this);
    mTransformBottomRight = new QActionGroup(this);
    mTransformPivot = new QActionGroup(this);
    mTransformOpacity = new QActionGroup(this);
    mTransformInteract = new QActionGroup(this);

    setupTransform();
}

void ToolControls::setupTransform()
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
    mTransformOX = new QrealAnimatorValueSlider(nullptr, this);

    mTransformMove->addAction(addSpacer(true, true));
    mTransformMove->addAction(addAction(QIcon::fromTheme("transform_translate"),
                                        tr("Move")));
    mTransformMove->addAction(addWidget(mTransformX));
    mTransformMove->addAction(addSeparator());
    mTransformMove->addAction(addWidget(mTransformY));

    mTransformRotate->addAction(addSpacer(true, true));
    mTransformRotate->addAction(addAction(QIcon::fromTheme("transform_rotate"),
                                          tr("Rotate")));
    mTransformRotate->addAction(addWidget(mTransformR));

    mTransformScale->addAction(addSpacer(true, true));
    mTransformScale->addAction(addAction(QIcon::fromTheme("transform_scale"),
                                         tr("Scale")));
    mTransformScale->addAction(addWidget(mTransformSX));
    mTransformScale->addAction(addSeparator());
    mTransformScale->addAction(addWidget(mTransformSY));

    mTransformPivot->addAction(addSpacer(true, true));
    mTransformPivot->addAction(addAction(QIcon::fromTheme("transform_pivot"),
                                         tr("Pivot")));
    mTransformPivot->addAction(addWidget(mTransformPX));
    mTransformPivot->addAction(addSeparator());
    mTransformPivot->addAction(addWidget(mTransformPY));

    mTransformOpacity->addAction(addSpacer(true, true));
    mTransformOpacity->addAction(addAction(QIcon::fromTheme("transform_opacity"),
                                           tr("Opacity")));
    mTransformOpacity->addAction(addWidget(mTransformOX));

    mTransformBottomRight->addAction(addSpacer(true, true));
    mTransformBottomRight->addAction(addAction(QIcon::fromTheme("rectCreate"),
                                              tr("Rectangle")));
    mTransformBottomRight->addAction(addWidget(mTransformBX));
    mTransformBottomRight->addAction(addSeparator());
    mTransformBottomRight->addAction(addWidget(mTransformBY));

    mTransformRadius->addAction(addSpacer(true, true));
    mTransformRadius->addAction(addAction(QIcon::fromTheme("transform_radius"),
                                          tr("Radius")));
    mTransformRadius->addAction(addWidget(mTransformRX));
    mTransformRadius->addAction(addSeparator());
    mTransformRadius->addAction(addWidget(mTransformRY));

    mTransformInteract->setExclusionPolicy(QActionGroup::ExclusionPolicy::None);
    mTransformInteract->addAction(addSpacer(true, true));
    mTransformInteract->addAction(addAction(QIcon::fromTheme("gizmos"),
                                            tr("Transform Interacts")));
    setupTransformInteract(Core::Gizmos::Interact::Position);
    setupTransformInteract(Core::Gizmos::Interact::Rotate);
    setupTransformInteract(Core::Gizmos::Interact::Scale);
    setupTransformInteract(Core::Gizmos::Interact::Shear);

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

void ToolControls::setupTransformInteract(const Core::Gizmos::Interact &ti)
{
    mTransformInteract->addAction(addSeparator());

    const auto mDocument = Document::sInstance;

    const bool visible = mDocument->getGizmoVisibility(ti);
    QIcon iconOn;
    QIcon iconOff;
    QString textOn;
    QString textOff;

    switch(ti) {
    case Core::Gizmos::Interact::Position:
        iconOn = QIcon::fromTheme("gizmo_translate_on");
        iconOff = QIcon::fromTheme("gizmo_translate_off");
        textOn = tr("Hide Position Interact");
        textOff = tr("Show Position Interact");
        break;
    case Core::Gizmos::Interact::Rotate:
        iconOn = QIcon::fromTheme("gizmo_rotate_on");
        iconOff = QIcon::fromTheme("gizmo_rotate_off");
        textOn = tr("Hide Rotate Interact");
        textOff = tr("Show Rotate Interact");
        break;
    case Core::Gizmos::Interact::Scale:
        iconOn = QIcon::fromTheme("gizmo_scale_on");
        iconOff = QIcon::fromTheme("gizmo_scale_off");
        textOn = tr("Hide Scale Interact");
        textOff = tr("Show Scale Interact");
        break;
    case Core::Gizmos::Interact::Shear:
        iconOn = QIcon::fromTheme("gizmo_shear_on");
        iconOff = QIcon::fromTheme("gizmo_shear_off");
        textOn = tr("Hide Shear Interact");
        textOff = tr("Show Shear Interact");
        break;
    default: return;
    }

    const auto interact = mTransformInteract->addAction(addAction(visible ? iconOn : iconOff, visible ? textOn : textOff));

    interact->setCheckable(true);
    interact->setChecked(visible);

    ThemeSupport::setToolbarButtonStyle("ToolBoxGizmo", this, interact);

    connect(interact, &QAction::triggered,
            this, [mDocument, ti]() {
        mDocument->setGizmoVisibility(ti, !mDocument->getGizmoVisibility(ti));
    });

    connect(mDocument, &Document::gizmoVisibilityChanged,
            this, [interact,
                   iconOn,
                   iconOff,
                   textOn,
                   textOff,
                   ti](Core::Gizmos::Interact i,
                       bool visible) {
        if (ti != i) { return; }
        interact->blockSignals(true);
        interact->setChecked(visible);
        interact->blockSignals(false);
        interact->setText(visible ? textOn : textOff);
        interact->setIcon(visible ? iconOn : iconOff);
    });
}
