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

#include "scenesettingsdialog.h"
#include "canvas.h"
#include "GUI/coloranimatorbutton.h"
#include "appsupport.h"
#include "Private/document.h"
#include "Private/esettings.h"

SceneSettingsDialog::SceneSettingsDialog(Canvas * const canvas,
                                         QWidget * const parent)
    : SceneSettingsDialog(canvas->prp_getName(),
                          canvas->getCanvasWidth(),
                          canvas->getCanvasHeight(),
                          canvas->getFrameRange(),
                          canvas->getFps(),
                          canvas->getBgColorAnimator(),
                          parent,
                          false)
{
    mTargetCanvas = canvas;
}

SceneSettingsDialog::SceneSettingsDialog(const QString &defName,
                                         QWidget * const parent)
    : SceneSettingsDialog(defName,
                          AppSupport::getSettings("scene",
                                                  "DefaultWidth",
                                                  1920).toInt(),
                          AppSupport::getSettings("scene",
                                                  "DefaultHeight",
                                                  1080).toInt(),
                          {AppSupport::getSettings("scene",
                                                   "DefaultMin",
                                                   0).toInt(),
                           AppSupport::getSettings("scene",
                                                   "DefaultMax",
                                                   300).toInt()},
                          AppSupport::getSettings("scene",
                                                  "DefaultFps",
                                                  30.).toDouble(),
                          nullptr,
                          parent,
                          true)
{

}

SceneSettingsDialog::SceneSettingsDialog(const QString &name,
                                         const int width,
                                         const int height,
                                         const FrameRange& range,
                                         const qreal fps,
                                         ColorAnimator * const bg,
                                         QWidget * const parent,
                                         bool isNew)
    : Friction::Ui::Dialog(parent)
{
    const auto presetsFpsSettings = AppSupport::getFpsPresetStatus();
    const auto presetsResolutionSettings = AppSupport::getResolutionPresetStatus();

    mEnableFpsPresets = presetsFpsSettings.first;
    mEnableFpsPresetsAuto = presetsFpsSettings.second;
    mEnableResolutionPresets = presetsResolutionSettings.first;
    mEnableResolutionPresetsAuto = presetsResolutionSettings.second;

    setWindowTitle(tr("Scene Properties"));
    mMainLayout = new QVBoxLayout(this);
    mMainLayout->setSizeConstraint(QLayout::SetFixedSize);

    setLayout(mMainLayout);

    mNameLayout = new QHBoxLayout();
    mNameEditLabel = new QLabel(tr("Name"), this);
    mNameEdit = new QLineEdit(name, this);
    connect(mNameEdit, &QLineEdit::textChanged,
            this, &SceneSettingsDialog::validate);
    mNameLayout->addWidget(mNameEditLabel);
    mNameLayout->addWidget(mNameEdit);
    mMainLayout->addLayout(mNameLayout);

    mWidthLabel = new QLabel(tr("Width"), this);
    mWidthSpinBox = new QSpinBox(this);
    mWidthSpinBox->setRange(1, INT_MAX);
    mWidthSpinBox->setValue(width);

    mHeightLabel = new QLabel(tr("Height"), this);
    mHeightSpinBox = new QSpinBox(this);
    mHeightSpinBox->setRange(1, INT_MAX);
    mHeightSpinBox->setValue(height);

    mResToolButton = new QToolButton(this);
    mResToolButton->setArrowType(Qt::NoArrow);
    mResToolButton->setPopupMode(QToolButton::InstantPopup);
    mResToolButton->setObjectName("FlatButton");
    mResToolButton->setIcon(QIcon::fromTheme("dots"));
    mResToolButton->setVisible(mEnableResolutionPresets);
    mResToolButton->setEnabled(mEnableResolutionPresets);
    mResToolButton->setIconSize(QSize(eSizesUI::widget, eSizesUI::widget));
    mResToolButton->setFixedSize(QSize(eSizesUI::widget, eSizesUI::widget));

    mSizeLayout = new QHBoxLayout();
    mSizeLayout->addWidget(mWidthLabel);
    mSizeLayout->addWidget(mWidthSpinBox);
    mSizeLayout->addWidget(mHeightLabel);
    mSizeLayout->addWidget(mHeightSpinBox);
    mSizeLayout->addWidget(mResToolButton);
    mMainLayout->addLayout(mSizeLayout);

    mFrameRangeLabel = new QLabel(tr("Duration"), this);
    mMinFrameSpin = new QSpinBox(this);
    mMinFrameSpin->setRange(0, INT_MAX);
    mMinFrameSpin->setValue(range.fMin);

    mMaxFrameSpin = new QSpinBox(this);
    mMaxFrameSpin->setRange(0, INT_MAX);
    mMaxFrameSpin->setValue(range.fMax);

    mTypeTime = new QComboBox(this);
    mTypeTime->addItem(tr("Frames"), "Frames");
    mTypeTime->addItem(tr("Seconds"), "Seconds");

    mFrameRangeLayout = new QHBoxLayout();
    mFrameRangeLayout->addWidget(mFrameRangeLabel);
    mFrameRangeLayout->addWidget(mMinFrameSpin);
    mFrameRangeLayout->addWidget(mMaxFrameSpin);
    mFrameRangeLayout->addWidget(mTypeTime);

    mMainLayout->addLayout(mFrameRangeLayout);

    mFpsToolButton = new QToolButton(this);
    mFpsToolButton->setArrowType(Qt::NoArrow);
    mFpsToolButton->setPopupMode(QToolButton::InstantPopup);
    mFpsToolButton->setObjectName("FlatButton");
    mFpsToolButton->setIcon(QIcon::fromTheme("dots"));
    mFpsToolButton->setVisible(mEnableFpsPresets);
    mFpsToolButton->setEnabled(mEnableFpsPresets);
    mFpsToolButton->setIconSize(QSize(eSizesUI::widget, eSizesUI::widget));
    mFpsToolButton->setFixedSize(QSize(eSizesUI::widget, eSizesUI::widget));

    mFPSLabel = new QLabel(tr("Fps"), this);
    mFPSSpinBox = new QDoubleSpinBox(this);
    mFPSSpinBox->setLocale(QLocale(QLocale::English,
                                   QLocale::UnitedStates));
    mFPSSpinBox->setRange(1, INT_MAX);
    mFPSSpinBox->setDecimals(3);
    mFPSSpinBox->setValue(fps);

    mFPSLayout = new QHBoxLayout();
    mFPSLayout->addWidget(mFPSLabel);
    mFPSLayout->addWidget(mFPSSpinBox);
    mFPSLayout->addWidget(mFpsToolButton);
    mMainLayout->addLayout(mFPSLayout);

    mBgColorLabel = new QLabel(tr("Background"), this);
    mBgColorButton = new ColorAnimatorButton(bg, this);
    if (!bg) {
        if (isNew) {
            mBgColorButton->setColor(AppSupport::getSettings("scene",
                                                             "DefaultColor",
                                                             QColor(Qt::black)).value<QColor>());
        } else { mBgColorButton->setColor(Qt::black); }
    }

    mBgColorLayout = new QHBoxLayout();
    mBgColorLayout->addWidget(mBgColorLabel);
    mBgColorLayout->addWidget(mBgColorButton);
    mMainLayout->addLayout(mBgColorLayout);

    mErrorLabel = new QLabel(this);
    mErrorLabel->setObjectName("errorLabel");
    mMainLayout->addWidget(mErrorLabel);

    mOkButton = new QPushButton(QIcon::fromTheme("dialog-ok"),
                                tr("Ok"), this);
    mCancelButton = new QPushButton(QIcon::fromTheme("dialog-cancel"),
                                    tr("Cancel"), this);
    mButtonsLayout = new QHBoxLayout();

    if (isNew) {
        mSaveAsDefault = new QCheckBox(this);
        mSaveAsDefault->setText(tr("Set as default"));
        mSaveAsDefault->setToolTip(tr("Use selected properties as default for new scenes"));
        mSaveAsDefault->setChecked(AppSupport::getSettings("scene",
                                                           "SaveDefault",
                                                           false).toBool());
        mMainLayout->addWidget(mSaveAsDefault);
    }

    mMainLayout->addLayout(mButtonsLayout);

    mButtonsLayout->addWidget(mOkButton);
    mButtonsLayout->addWidget(mCancelButton);

    connect(mOkButton, &QPushButton::released,
            this, &SceneSettingsDialog::accept);
    connect(mCancelButton, &QPushButton::released,
            this, &SceneSettingsDialog::reject);
    connect(this, &QDialog::rejected, this, &QDialog::close);
    connect(mTypeTime, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SceneSettingsDialog::updateDuration);

    validate();

    populateResPresets();
    populateFpsPresets();
}

bool SceneSettingsDialog::validate()
{
    QString nameError;
    const bool validName = Property::prp_sValidateName(mNameEdit->text(),
                                                       &nameError);
    if (!nameError.isEmpty()) { nameError += "\n"; }
    mErrorLabel->setText(nameError);
    const bool valid = validName;
    mOkButton->setEnabled(valid);
    return valid;
}

int SceneSettingsDialog::getCanvasWidth() const
{
    return mWidthSpinBox->value();
}

int SceneSettingsDialog::getCanvasHeight() const
{
    return mHeightSpinBox->value();
}

QString SceneSettingsDialog::getCanvasName() const
{
    return mNameEdit->text();
}

FrameRange SceneSettingsDialog::getFrameRange() const
{
    FrameRange range;
    const QString typetime = mTypeTime->currentData().toString();
    if (typetime == "Frames") {
        range = {mMinFrameSpin->value(), mMaxFrameSpin->value()};
    } else {
        const int maxFrame = mMaxFrameSpin->value();
        const int minFrame = mMinFrameSpin->value();
        const qreal fpsFrame = mFPSSpinBox->value();
        range = {qRound(minFrame*fpsFrame),
                 qRound(maxFrame*fpsFrame)};
    }
    range.fixOrder();
    return range;
}

qreal SceneSettingsDialog::getFps() const
{
    return mFPSSpinBox->value();
}

void SceneSettingsDialog::populateFpsPresets()
{
    if (!mEnableFpsPresets) { return; }
    QStringList presets = AppSupport::getFpsPresets();

    QMap<double, QString> m;
    for (auto s : presets) { m[s.toDouble()] = s; }
    presets = QStringList(m.values());

    for (const auto &preset : presets) {
        const auto act = new QAction(preset, this);
        connect (act, &QAction::triggered, [this, preset]() {
            mFPSSpinBox->setValue(preset.toDouble());
        });
        mFpsToolButton->addAction(act);
    }
}

void SceneSettingsDialog::populateResPresets()
{
    if (!mEnableResolutionPresets) { return; }
    const auto presets = AppSupport::getResolutionPresets();
    for (const auto &preset : presets) {
        const auto act = new QAction(QString("%1 x %2")
                                     .arg(preset.first)
                                     .arg(preset.second),
                                     this);
        connect (act, &QAction::triggered, [this, preset]() {
            mWidthSpinBox->setValue(preset.first);
            mHeightSpinBox->setValue(preset.second);
        });
        mResToolButton->addAction(act);
    }
}

void SceneSettingsDialog::applySettingsToCanvas(Canvas * const canvas) const
{
    if (!canvas) { return; }
    canvas->prp_setNameAction(getCanvasName());
    canvas->setCanvasSize(getCanvasWidth(), getCanvasHeight());
    canvas->setFps(getFps());
    canvas->setFrameRange(getFrameRange());

    if (mSaveAsDefault) {
        const bool saveDef = mSaveAsDefault->isChecked();
        AppSupport::setSettings("scene", "SaveDefault", saveDef);
        if (saveDef) {
            AppSupport::setSettings("scene", "DefaultWidth", getCanvasWidth());
            AppSupport::setSettings("scene", "DefaultHeight", getCanvasHeight());
            AppSupport::setSettings("scene", "DefaultMin", getFrameRange().fMin);
            AppSupport::setSettings("scene", "DefaultMax", getFrameRange().fMax);
            AppSupport::setSettings("scene", "DefaultFps", getFps());
            AppSupport::setSettings("scene", "DefaultColor", mBgColorButton->color());
        }
    }
    if (mEnableFpsPresets &&
        mEnableFpsPresetsAuto) { AppSupport::saveFpsPreset(getFps()); }
    if (mEnableResolutionPresets &&
        mEnableResolutionPresetsAuto) { AppSupport::saveResolutionPreset(getCanvasWidth(),
                                                                         getCanvasHeight()); }
    if (canvas != mTargetCanvas) {
        canvas->getBgColorAnimator()->setColor(mBgColorButton->color());

        // Adjust default fill/stroke color to background color
        auto settings = eSettings::sInstance;
        settings->fLastUsedFillColor = AppSupport::adjustColorVisibility(eSettings::instance().fLastUsedFillColor,
                                                                         mBgColorButton->color());
        settings->fLastUsedStrokeColor = AppSupport::adjustColorVisibility(eSettings::instance().fLastUsedStrokeColor,
                                                                           mBgColorButton->color());
    }
}

void SceneSettingsDialog::sNewSceneDialog(Document& document,
                                          QWidget * const parent)
{
    const QString defName = tr("Scene %1").arg(document.fScenes.count());

    const auto dialog = new SceneSettingsDialog(defName, parent);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    const auto docPtr = &document;
    connect(dialog, &QDialog::accepted, dialog, [dialog, docPtr]() {
        const auto newCanvas = docPtr->createNewScene();
        const auto block = newCanvas->blockUndoRedo();
        dialog->applySettingsToCanvas(newCanvas);
        newCanvas->anim_setAbsFrame(newCanvas->getFrameRange().fMin);
        dialog->close();
        docPtr->actionFinished();
    });

    dialog->show();
}

void SceneSettingsDialog::updateDuration(int index)
{
    const qreal fps = mFPSSpinBox->value();
    switch (index) {
        case 0: // Convert seconds to frames
            mMinFrameSpin->setValue(qRound(mMinFrameSpin->value() * fps));
            mMaxFrameSpin->setValue(qRound(mMaxFrameSpin->value() * fps));
            break;
        case 1: // Convert frames to seconds
            mMinFrameSpin->setValue(qRound(mMinFrameSpin->value() / fps));
            mMaxFrameSpin->setValue(qRound(mMaxFrameSpin->value() / fps));
            break;
        default:;
    }
}
