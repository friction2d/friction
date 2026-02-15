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


#include "performancesettingswidget.h"
#include "Private/esettings.h"
#include "exceptions.h"
#include "hardwareinfo.h"
#include "GUI/global.h"

#include "Sound/audiohandler.h"
#include "appsupport.h"

#include <QTimer>
#include <QGroupBox>
#include <QScrollArea>

#define RASTER_HW_SUPPORT_ID Qt::UserRole + 1

PerformanceSettingsWidget::PerformanceSettingsWidget(QWidget *parent)
    : SettingsWidget(parent)
    , mAudioDevicesCombo(nullptr)
{
    const auto capGroup = new QGroupBox(tr("Limits"), this);
    capGroup->setObjectName("BlueBox");
    const auto capLayout = new QVBoxLayout(capGroup);
    addWidget(capGroup);

    QHBoxLayout* cpuCapSett = new QHBoxLayout;

    mCpuThreadsCapCheck = new QCheckBox(tr("CPU"), this);
    mCpuThreadsCapLabel = new QLabel(this);
    mCpuThreadsCapSlider = new QSlider(Qt::Horizontal);
    mCpuThreadsCapSlider->setRange(1, HardwareInfo::sCpuThreads());
    connect(mCpuThreadsCapCheck, &QCheckBox::toggled,
            mCpuThreadsCapSlider, &QWidget::setEnabled);
    connect(mCpuThreadsCapCheck, &QCheckBox::toggled,
            mCpuThreadsCapLabel, &QWidget::setEnabled);
    mCpuThreadsCapSlider->setEnabled(false);
    mCpuThreadsCapLabel->setEnabled(false);
    connect(mCpuThreadsCapSlider, &QSlider::valueChanged,
            this, [this](const int val) {
        const int nTot = HardwareInfo::sCpuThreads();
        mCpuThreadsCapLabel->setText(QString("%1 / %2").arg(val).arg(nTot));
    });

    cpuCapSett->addWidget(mCpuThreadsCapCheck);
    cpuCapSett->addWidget(mCpuThreadsCapSlider);
    cpuCapSett->addWidget(mCpuThreadsCapLabel);
    capLayout->addLayout(cpuCapSett);

    QHBoxLayout* ramCapSett = new QHBoxLayout;

    mRamMBCapCheck = new QCheckBox(tr("RAM"), this);
    mRamMBCapSpin = new QSpinBox(this);
    mRamMBCapSpin->setRange(250, intMB(HardwareInfo::sRamKB()).fValue);
    mRamMBCapSpin->setSuffix(" MB");
    mRamMBCapSpin->setEnabled(false);

    mRamMBCapSlider = new QSlider(Qt::Horizontal);
    mRamMBCapSlider->setRange(250, intMB(HardwareInfo::sRamKB()).fValue);
    mRamMBCapSlider->setEnabled(false);

    connect(mRamMBCapCheck, &QCheckBox::toggled,
            mRamMBCapSpin, &QWidget::setEnabled);
    connect(mRamMBCapCheck, &QCheckBox::toggled,
            mRamMBCapSlider, &QWidget::setEnabled);

    connect(mRamMBCapSpin, qOverload<int>(&QSpinBox::valueChanged),
            mRamMBCapSlider, &QSlider::setValue);
    connect(mRamMBCapSlider, &QSlider::valueChanged,
            mRamMBCapSpin, &QSpinBox::setValue);

    ramCapSett->addWidget(mRamMBCapCheck);
    ramCapSett->addWidget(mRamMBCapSlider);
    ramCapSett->addWidget(mRamMBCapSpin);
    capLayout->addLayout(ramCapSett);

    const auto gpuGroup = new QGroupBox(HardwareInfo::sGpuRendererString(),
                                        this);
    gpuGroup->setObjectName("BlueBox");
    const auto gpuGroupLayout = new QVBoxLayout(gpuGroup);

    mAccPreferenceLabel = new QLabel(tr("Acceleration preference"));
    const auto sliderWidget = new QWidget(this);
    const auto sliderLayout = new QHBoxLayout(sliderWidget);
    mAccPreferenceCpuLabel = new QLabel(tr("CPU"));
    mAccPreferenceSlider = new QSlider(Qt::Horizontal);
    mAccPreferenceSlider->setRange(0, 4);
    mAccPreferenceGpuLabel = new QLabel(tr("GPU"));
    sliderLayout->addWidget(mAccPreferenceCpuLabel);
    sliderLayout->addWidget(mAccPreferenceSlider);
    sliderLayout->addWidget(mAccPreferenceGpuLabel);
    mAccPreferenceDescLabel = new QLabel(this);
    mAccPreferenceDescLabel->setAlignment(Qt::AlignCenter);
    connect(mAccPreferenceSlider, &QSlider::valueChanged,
            this, &PerformanceSettingsWidget::updateAccPreferenceDesc);
    gpuGroupLayout->addWidget(mAccPreferenceLabel);
    gpuGroupLayout->addWidget(sliderWidget);
    gpuGroupLayout->addWidget(mAccPreferenceDescLabel);

    mPathGpuAccCheck = new QCheckBox(tr("Path GPU acceleration"), this);

    // MSAA
    const auto msaaLabel = new QLabel(tr("Multisample anti-aliasing"), this);
    mMsaa = new QComboBox(this);
    mMsaa->addItems({"0", "2", "4", "8", "16"});

    const auto msaaWidget = new QWidget(this);
    const auto msaaLayout = new QHBoxLayout(msaaWidget);
    msaaWidget->setContentsMargins(0, 0, 0, 0);

    msaaLayout->addWidget(msaaLabel);
    msaaLayout->addWidget(mMsaa);

    gpuGroupLayout->addWidget(msaaWidget);
    gpuGroupLayout->addWidget(mPathGpuAccCheck);
    addWidget(gpuGroup);

    const auto audioWidget = new QGroupBox(this);
    audioWidget->setObjectName("BlueBox");
    audioWidget->setTitle(tr("Audio Device"));

    const auto audioLayout = new QHBoxLayout(audioWidget);

    //QLabel *audioLabel = new QLabel(tr("Output"), this);
    mAudioDevicesCombo = new QComboBox(this);
    mAudioDevicesCombo->setSizePolicy(QSizePolicy::Preferred,
                                      QSizePolicy::Preferred);
    mAudioDevicesCombo->setFocusPolicy(Qt::NoFocus);
    mAudioDevicesCombo->setEnabled(false);

    //audioLayout->addWidget(audioLabel);
    audioLayout->addWidget(mAudioDevicesCombo);

    addWidget(audioWidget);

    setupRasterEffectWidgets();

    eSizesUI::widget.add(mCpuThreadsCapCheck, [this](const int size) {
        mCpuThreadsCapCheck->setFixedHeight(size);
        mRamMBCapCheck->setFixedHeight(size);
        mPathGpuAccCheck->setFixedHeight(size);
        mAudioDevicesCombo->setFixedHeight(eSizesUI::button);
    });

    QTimer::singleShot(250, this,
                       &PerformanceSettingsWidget::updateAudioDevices);
    connect(AudioHandler::sInstance, &AudioHandler::deviceChanged,
            this, [this]() { updateAudioDevices(); });
}

void PerformanceSettingsWidget::applySettings()
{
    mSett.fCpuThreadsCap = mCpuThreadsCapCheck->isChecked() ?
                mCpuThreadsCapSlider->value() : 0;
    mSett.fRamMBCap = intMB(mRamMBCapCheck->isChecked() ?
                mRamMBCapSpin->value() : 0);
    mSett.fAccPreference = static_cast<AccPreference>(
                mAccPreferenceSlider->value());
    mSett.fPathGpuAcc = mPathGpuAccCheck->isChecked();
    mSett.fInternalMultisampleCount = mMsaa->currentText().toInt();

    saveRasterEffectsSupport();

    const auto audioHandler = AudioHandler::sInstance;
    if (mAudioDevicesCombo->currentText() != audioHandler->getDeviceName()) {
        audioHandler->initializeAudio(mAudioDevicesCombo->currentText(), true);
    }
}

void PerformanceSettingsWidget::updateSettings(bool restore)
{
    const bool capCpu = mSett.fCpuThreadsCap > 0;
    mCpuThreadsCapCheck->setChecked(capCpu);
    const int nThreads = capCpu ? mSett.fCpuThreadsCap :
                                  HardwareInfo::sCpuThreads();
    mCpuThreadsCapSlider->setValue(nThreads);

    const bool capRam = mSett.fRamMBCap.fValue > 250;
    mRamMBCapCheck->setChecked(capRam);
    const int nRamMB = capRam ? mSett.fRamMBCap.fValue :
                                intMB(HardwareInfo::sRamKB()).fValue;
    mRamMBCapSpin->setValue(nRamMB);

    mAccPreferenceSlider->setValue(static_cast<int>(mSett.fAccPreference));
    updateAccPreferenceDesc();
    mPathGpuAccCheck->setChecked(mSett.fPathGpuAcc);
    mMsaa->setCurrentText(QString::number(mSett.fInternalMultisampleCount));

    if (restore) {
        AudioHandler::sInstance->initializeAudio(QString(), true);
        restoreDefaultRasterEffectsSupport();
    }
}

void PerformanceSettingsWidget::updateAudioDevices()
{
    const auto mAudioHandler = AudioHandler::sInstance;
    mAudioDevicesCombo->blockSignals(true);
    mAudioDevicesCombo->clear();
    mAudioDevicesCombo->addItems(mAudioHandler->listDevices());
    if (mAudioDevicesCombo->count() > 0) {
        mAudioDevicesCombo->setCurrentText(mAudioHandler->getDeviceName());
        mAudioDevicesCombo->setEnabled(true);
    } else { mAudioDevicesCombo->setEnabled(false); }
    mAudioDevicesCombo->blockSignals(false);
}

void PerformanceSettingsWidget::setupRasterEffectWidgets()
{
    QStringList effects = {"Blur",
                           "BrightnessContrast",
                           "Colorize",
                           "MotionBlur",
                           "NoiseFade",
                           "Shadow",
                           "Wipe"};
    HardwareSupport defaultSupport = HardwareSupport::gpuPreffered;

    addSeparator();

    const auto area = new QScrollArea(this);
    const auto container = new QGroupBox(this);
    container->setObjectName("BlueBox");
    const auto containerLayout = new QVBoxLayout(container);
    const auto containerInner = new QWidget(this);
    const auto containerInnerLayout = new QVBoxLayout(containerInner);

    area->setWidget(containerInner);
    area->setWidgetResizable(true);
    area->setContentsMargins(0, 0, 0, 0);
    area->setFrameShape(QFrame::NoFrame);

    container->setTitle(tr("Raster Effects"));

    container->setContentsMargins(0, 0, 0, 0);

    containerInnerLayout->setMargin(5);

    containerLayout->addWidget(area);

    for (const auto &effect : effects) {
        const auto box = new QComboBox(this);
        box->addItem(tr("CPU-only"), static_cast<int>(HardwareSupport::cpuOnly));
        box->addItem(tr("CPU preferred"), static_cast<int>(HardwareSupport::cpuPreffered));
        box->addItem(tr("GPU-only"), static_cast<int>(HardwareSupport::gpuOnly));
        box->addItem(tr("GPU preferred"), static_cast<int>(HardwareSupport::gpuPreffered));
        box->setItemData(0, effect, RASTER_HW_SUPPORT_ID);
        box->setCurrentText(AppSupport::getRasterEffectHardwareSupportString(effect, defaultSupport));
        mRasterEffectsHardwareSupport << box;

        const auto label = new QLabel(effect, this);
        const auto wid = new QWidget(this);
        const auto lay = new QHBoxLayout(wid);

        wid->setContentsMargins(0, 0, 0, 0);
        lay->setMargin(0);
        lay->addWidget(label);
        lay->addWidget(box);

        containerInnerLayout->addWidget(wid);
    }

    containerLayout->addStretch();

    addWidget(container);
}

void PerformanceSettingsWidget::saveRasterEffectsSupport()
{
    for (const auto &box : qAsConst(mRasterEffectsHardwareSupport)) {
        AppSupport::setSettings("RasterEffects",
                                QString("%1HardwareSupport")
                                    .arg(box->itemData(0, RASTER_HW_SUPPORT_ID).toString()),
                                box->currentData());
    }
}

void PerformanceSettingsWidget::restoreDefaultRasterEffectsSupport()
{
    for (const auto &box : qAsConst(mRasterEffectsHardwareSupport)) {
        box->setCurrentText(tr("GPU preferred"));
    }
}

void PerformanceSettingsWidget::updateAccPreferenceDesc()
{
    const int value = mAccPreferenceSlider->value();
    QString toolTip;
    if (value == 0) {
        mAccPreferenceDescLabel->setText(tr("Strong CPU preference"));
        toolTip = tr("Use the GPU only for tasks not supported by the CPU");
    } else if (value == 1) {
        mAccPreferenceDescLabel->setText(tr("Soft CPU preference"));
        toolTip = tr("Use the GPU only for tasks marked as preferred for the GPU");
    } else if (value == 2) {
        mAccPreferenceDescLabel->setText(tr(" Hardware agnostic (recommended) "));
        toolTip = tr("Adhere to the default hardware preference");
    } else if (value == 3) {
        mAccPreferenceDescLabel->setText(tr("Soft GPU preference"));
        toolTip = tr("Use the CPU only for tasks marked as preferred for the CPU");
    } else if (value == 4) {
        mAccPreferenceDescLabel->setText(tr("Strong GPU preference"));
        toolTip = tr("Use the CPU only for tasks not supported by the GPU");
    }
    mAccPreferenceDescLabel->setToolTip(gSingleLineTooltip(toolTip));
    mAccPreferenceSlider->setToolTip(gSingleLineTooltip(toolTip));
}
