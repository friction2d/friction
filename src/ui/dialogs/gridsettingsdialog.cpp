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

#include "gridsettingsdialog.h"

#include "gridcontroller.h"
#include "GUI/coloranimatorbutton.h"
#include "GUI/global.h"
#include "Private/esettings.h"

#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QPushButton>
#include <QSizePolicy>
#include <QColor>
#include <QLabel>

using Friction::Core::GridSettings;

namespace {
constexpr double kMinSpacing = 1.0;
constexpr double kMaxSpacing = 10000.0;
constexpr double kOriginRange = 100000.0;
constexpr int kMaxSnapThreshold = 200;
constexpr int kMaxMajorEvery = 100;
}

GridSettingsDialog::GridSettingsDialog(QWidget* parent)
    : Friction::Ui::Dialog(parent)
    , mSizeX(nullptr)
    , mSizeY(nullptr)
    , mOriginX(nullptr)
    , mOriginY(nullptr)
    , mSnapThreshold(nullptr)
    , mMajorEveryX(nullptr)
    , mMajorEveryY(nullptr)
    , mSaveAsDefault(nullptr)
    , mApplyButton(nullptr)
    , mOkButton(nullptr)
    , mCancelButton(nullptr)
    , mColorButton(nullptr)
    , mMajorColorButton(nullptr)
    , mColorAnimator(enve::make_shared<ColorAnimator>())
    , mMajorColorAnimator(enve::make_shared<ColorAnimator>())
    , mSnapEnabled(true)
    , mStoredSnapToCanvas(false)
    , mStoredSnapToBoxes(false)
    , mStoredSnapToNodes(false)
    , mStoredSnapToPivots(false)
    , mStoredSnapAnchorPivot(true)
    , mStoredSnapAnchorBounds(true)
    , mStoredSnapAnchorNodes(false)
{
    setModal(false);
    const QColor defaultMinor = GridSettings::defaults().colorAnimator->getColor();
    const QColor defaultMajor = GridSettings::defaults().majorColorAnimator->getColor();
    if (auto* settings = eSettings::sInstance) {
        mColorAnimator->setColor(settings->fGridColor);
        mMajorColorAnimator->setColor(settings->fGridMajorColor);
    } else {
        mColorAnimator->setColor(defaultMinor);
        mMajorColorAnimator->setColor(defaultMajor);
    }
    setupUi();
}

void GridSettingsDialog::setupUi()
{
    setWindowTitle(tr("Grid Settings"));
    auto* layout = new QVBoxLayout(this);

    auto* form = new QGridLayout();
    form->setColumnStretch(1, 1);
    form->setColumnStretch(2, 1);
    form->setContentsMargins(0, 0, 0, 0);
    form->setHorizontalSpacing(12);
    form->setVerticalSpacing(8);

    auto addLabel = [this, form](int row, const QString& text, QWidget* buddy) {
        auto* label = new QLabel(text, this);
        if (buddy) {
            label->setBuddy(buddy);
        }
        form->addWidget(label, row, 0);
    };

    int formRow = 0;

    mOriginX = new QDoubleSpinBox(this);
    mOriginX->setDecimals(0);
    mOriginX->setRange(-kOriginRange, kOriginRange);
    mOriginX->setSingleStep(1.0);
    mOriginX->setToolTip(tr("Horizontal origin offset"));
    mOriginY = new QDoubleSpinBox(this);
    mOriginY->setDecimals(0);
    mOriginY->setRange(-kOriginRange, kOriginRange);
    mOriginY->setSingleStep(1.0);
    mOriginY->setToolTip(tr("Vertical origin offset"));
    addLabel(formRow, tr("Origin"), mOriginX);
    form->addWidget(mOriginX, formRow, 1);
    form->addWidget(mOriginY, formRow, 2);
    ++formRow;

    mSizeX = new QDoubleSpinBox(this);
    mSizeX->setDecimals(0);
    mSizeX->setRange(kMinSpacing, kMaxSpacing);
    mSizeX->setSingleStep(1.0);
    mSizeX->setToolTip(tr("Horizontal grid spacing"));
    mSizeY = new QDoubleSpinBox(this);
    mSizeY->setDecimals(0);
    mSizeY->setRange(kMinSpacing, kMaxSpacing);
    mSizeY->setSingleStep(1.0);
    mSizeY->setToolTip(tr("Vertical grid spacing"));
    addLabel(formRow, tr("Spacing"), mSizeX);
    form->addWidget(mSizeX, formRow, 1);
    form->addWidget(mSizeY, formRow, 2);
    ++formRow;

    mSnapThreshold = new QSpinBox(this);
    mSnapThreshold->setRange(0, kMaxSnapThreshold);
    mSnapThreshold->setSingleStep(1);
    addLabel(formRow, tr("Snap radius"), mSnapThreshold);
    form->addWidget(mSnapThreshold, formRow, 1, 1, 2);
    ++formRow;

    mMajorEveryX = new QSpinBox(this);
    mMajorEveryX->setRange(1, kMaxMajorEvery);
    mMajorEveryX->setSingleStep(1);
    mMajorEveryX->setToolTip(tr("Horizontal major grid line interval"));
    mMajorEveryY = new QSpinBox(this);
    mMajorEveryY->setRange(1, kMaxMajorEvery);
    mMajorEveryY->setSingleStep(1);
    mMajorEveryY->setToolTip(tr("Vertical major grid line interval"));
    addLabel(formRow, tr("Major line every"), mMajorEveryX);
    form->addWidget(mMajorEveryX, formRow, 1);
    form->addWidget(mMajorEveryY, formRow, 2);
    ++formRow;

    mMajorColorButton = new ColorAnimatorButton(mMajorColorAnimator.get(), this);
    mMajorColorButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    addLabel(formRow, tr("Major line color"), mMajorColorButton);
    form->addWidget(mMajorColorButton, formRow, 1, 1, 2);
    ++formRow;

    mColorButton = new ColorAnimatorButton(mColorAnimator.get(), this);
    mColorButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    addLabel(formRow, tr("Minor line color"), mColorButton);
    form->addWidget(mColorButton, formRow, 1, 1, 2);
    ++formRow;

    mApplyButton = new QPushButton(QIcon::fromTheme("dialog-apply"), tr("Apply"), this);
    mApplyButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    mApplyButton->setAutoDefault(false);
    mApplyButton->setDefault(false);
    form->addWidget(mApplyButton, formRow, 0, 1, 3);

    layout->addLayout(form);
    eSizesUI::widget.addSpacing(layout);

    mSaveAsDefault = new QCheckBox(tr("Save as default"), this);
    layout->addWidget(mSaveAsDefault);

    mOkButton = new QPushButton(QIcon::fromTheme("dialog-ok"), tr("Ok"), this);
    mCancelButton = new QPushButton(QIcon::fromTheme("dialog-cancel"), tr("Cancel"), this);

    auto* buttonLayout = new QHBoxLayout();
    layout->addLayout(buttonLayout);

    buttonLayout->addWidget(mOkButton);
    buttonLayout->addWidget(mCancelButton);

    connect(mOkButton, &QPushButton::released,
            this, &GridSettingsDialog::accept);
    connect(mCancelButton, &QPushButton::released,
            this, &GridSettingsDialog::reject);
    connect(mApplyButton, &QPushButton::released, this, [this]() {
        emit applyRequested(settings(), saveAsDefault());
    });
    connect(this, &QDialog::rejected, this, &QDialog::close);
}

void GridSettingsDialog::setSettings(const GridSettings& settings)
{
    mSnapEnabled = settings.enabled;
    mSizeX->setValue(settings.sizeX);
    mSizeY->setValue(settings.sizeY);
    mOriginX->setValue(settings.originX);
    mOriginY->setValue(settings.originY);
    mSnapThreshold->setValue(settings.snapThresholdPx);
    mStoredSnapToCanvas = settings.snapToCanvas;
    mStoredSnapToBoxes = settings.snapToBoxes;
    mStoredSnapToNodes = settings.snapToNodes;
    mStoredSnapToPivots = settings.snapToPivots;
    mStoredSnapAnchorPivot = settings.snapAnchorPivot;
    mStoredSnapAnchorBounds = settings.snapAnchorBounds;
    mStoredSnapAnchorNodes = settings.snapAnchorNodes;
    mMajorEveryX->setValue(settings.majorEveryX);
    mMajorEveryY->setValue(settings.majorEveryY);
    mStoredShow = settings.show;
    mStoredDrawOnTop = settings.drawOnTop;
    if (mSaveAsDefault) {
        mSaveAsDefault->setChecked(false);
    }

    const auto ensureAnimator = [](qsptr<ColorAnimator>& animator,
                                   ColorAnimatorButton* button)
    {
        if (!animator) {
            animator = enve::make_shared<ColorAnimator>();
            if (button) {
                button->setColorTarget(animator.get());
            }
        }
    };

    ensureAnimator(mColorAnimator, mColorButton);
    ensureAnimator(mMajorColorAnimator, mMajorColorButton);

    const QColor appliedColor = settings.colorAnimator
        ? settings.colorAnimator->getColor()
        : GridSettings::defaults().colorAnimator->getColor();
    mColorAnimator->setColor(appliedColor);

    const QColor appliedMajorColor = settings.majorColorAnimator
        ? settings.majorColorAnimator->getColor()
        : GridSettings::defaults().majorColorAnimator->getColor();
    mMajorColorAnimator->setColor(appliedMajorColor);
}

GridSettings GridSettingsDialog::settings() const
{
    GridSettings result;
    result.enabled = mSnapEnabled;
    result.sizeX = mSizeX->value();
    result.sizeY = mSizeY->value();
    result.originX = mOriginX->value();
    result.originY = mOriginY->value();
    result.snapThresholdPx = mSnapThreshold->value();
    result.snapToCanvas = mStoredSnapToCanvas;
    result.snapToBoxes = mStoredSnapToBoxes;
    result.snapToNodes = mStoredSnapToNodes;
    result.snapToPivots = mStoredSnapToPivots;
    result.snapAnchorPivot = mStoredSnapAnchorPivot;
    result.snapAnchorBounds = mStoredSnapAnchorBounds;
    result.snapAnchorNodes = mStoredSnapAnchorNodes;
    result.majorEveryX = mMajorEveryX->value();
    result.majorEveryY = mMajorEveryY->value();
    result.show = mStoredShow;
    result.drawOnTop = mStoredDrawOnTop;

    const QColor finalColor = mColorAnimator
        ? mColorAnimator->getColor()
        : GridSettings::defaults().colorAnimator->getColor();
    result.colorAnimator = enve::make_shared<ColorAnimator>();
    result.colorAnimator->setColor(finalColor);

    const QColor finalMajorColor = mMajorColorAnimator
        ? mMajorColorAnimator->getColor()
        : GridSettings::defaults().majorColorAnimator->getColor();
    result.majorColorAnimator = enve::make_shared<ColorAnimator>();
    result.majorColorAnimator->setColor(finalMajorColor);
    return result;
}

bool GridSettingsDialog::saveAsDefault() const
{
    return mSaveAsDefault && mSaveAsDefault->isChecked();
}
