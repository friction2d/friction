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

#include "generalsettingswidget.h"
#include "appsupport.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QScrollArea>

#include "Private/esettings.h"
#include "GUI/global.h"

#include "../mainwindow.h"
#include "widgets/twocolumnlayout.h"

GeneralSettingsWidget::GeneralSettingsWidget(QWidget *parent)
    : SettingsWidget(parent)
    , mAutoBackup(nullptr)
    , mAutoSave(nullptr)
    , mAutoSaveTimer(nullptr)
    , mDefaultInterfaceScaling(nullptr)
    , mInterfaceScaling(nullptr)
    , mImportFileDir(nullptr)
{
    const auto mGeneralWidget = new QWidget(this);
    mGeneralWidget->setContentsMargins(0, 0, 0, 0);
    const auto mGeneralLayout = new QVBoxLayout(mGeneralWidget);
    mGeneralLayout->setContentsMargins(0, 0, 0, 0);

    const auto mProjectWidget = new QGroupBox(this);
    mProjectWidget->setObjectName("BlueBox");
    mProjectWidget->setTitle(tr("Project I/O"));
    mProjectWidget->setContentsMargins(0, 0, 0, 0);
    const auto mProjectLayout = new QVBoxLayout(mProjectWidget);

    mAutoBackup = new QCheckBox(tr("Enable Backup on Save"), this);
    mAutoBackup->setCheckable(true);
    mAutoBackup->setToolTip(tr("Creates a backup file after each successful save.\n\n"
                               "Backup files are stored in a folder called PROJECT.friction_backup."));
    mProjectLayout->addWidget(mAutoBackup);

    mGeneralLayout->addWidget(mProjectWidget);

    const auto mAutoSaveWidget = new QWidget(this);
    mAutoSaveWidget->setContentsMargins(0, 0, 0, 0);
    const auto mAutoSaveLayout = new QHBoxLayout(mAutoSaveWidget);
    mAutoSaveLayout->setContentsMargins(0, 0, 0, 0);
    mAutoSaveLayout->setMargin(0);

    mAutoSave = new QCheckBox(tr("Enable Auto Save"), this);
    mAutoSave->setCheckable(true);
    mAutoSave->setToolTip(tr("Will auto save each X min if project is unsaved.\n\n"
                             "Enable Backup on Save for incremental saves (and as a failsafe)."));

    mAutoSaveTimer = new QSpinBox(this);
    mAutoSaveTimer->setRange(1, 60);
    mAutoSaveTimer->setSuffix(tr(" min"));

    mAutoSaveLayout->addWidget(mAutoSave);
    mAutoSaveLayout->addStretch();
    mAutoSaveLayout->addWidget(mAutoSaveTimer);

    mProjectLayout->addWidget(mAutoSaveWidget);

    const auto mScaleWidget = new QGroupBox(this);
    mScaleWidget->setObjectName("BlueBox");
    mScaleWidget->setTitle(tr("Interface Scaling"));
    mScaleWidget->setContentsMargins(0, 0, 0, 0);
    const auto mScaleLayout = new QVBoxLayout(mScaleWidget);

    const auto mScaleContainer = new QWidget(this);
    mScaleContainer->setContentsMargins(0, 0, 0, 0);
    const auto mScaleContainerLayout = new QHBoxLayout(mScaleContainer);
    mScaleLayout->addWidget(mScaleContainer);

    mInterfaceScaling = new QSlider(Qt::Horizontal, this);
    mInterfaceScaling->setRange(50, 150);
    mScaleContainerLayout->addWidget(mInterfaceScaling);

    const auto mScaleLabel = new QLabel(this);
    connect(mInterfaceScaling, &QSlider::valueChanged,
            mScaleLabel, [mScaleLabel](const int value) {
        mScaleLabel->setText(QString("%1 %").arg(value));
    });
    emit mInterfaceScaling->valueChanged(100);
    mScaleContainerLayout->addWidget(mScaleLabel);

    mDefaultInterfaceScaling = new QCheckBox(this);
    mDefaultInterfaceScaling->setText(tr("Auto"));
    mDefaultInterfaceScaling->setToolTip(tr("Use scaling reported by the system."));
    connect(mDefaultInterfaceScaling, &QCheckBox::stateChanged,
            this, [this]() {
        mInterfaceScaling->setEnabled(!mDefaultInterfaceScaling->isChecked());
    });

    mScaleContainerLayout->addWidget(mDefaultInterfaceScaling);

    // setting for HiDPI scale factor
    const auto passThroughInterfaceScaling = new QCheckBox(this);
    passThroughInterfaceScaling->setText(tr("HiDPI PassThrough"));
    passThroughInterfaceScaling->setToolTip(tr("If enabled the scaling factor for the UI will not be rounded, recommended for HiDPI displays.\n"
                                               "If you use a negative font/display system scaling and/or see artifacts on UI elements "
                                               "then disable this option.\nThe scaling factor will then round up for .75 and above."));
    passThroughInterfaceScaling->setChecked(AppSupport::getSettings("settings",
                                                                    "interfaceScalingPassThrough",
                                                                    true).toBool());
    connect(passThroughInterfaceScaling, &QCheckBox::stateChanged,
            this, [passThroughInterfaceScaling]() {
        // we just save on change, we don't care to integrate
        // this setting with the rest of the system
        // as qApp/eSettings etc is not available when this setting is needed
        AppSupport::setSettings("settings",
                                "interfaceScalingPassThrough",
                                passThroughInterfaceScaling->isChecked());
    });
    mScaleContainerLayout->addWidget(passThroughInterfaceScaling);

    const auto infoLabel = new QLabel(this);
    infoLabel->setText(tr("Changes here will require a restart of Friction."));
    mScaleLayout->addWidget(infoLabel);

    mGeneralLayout->addWidget(mScaleWidget);

    const auto mImportFileWidget = new QWidget(this);
    mImportFileWidget->setContentsMargins(0, 0, 0, 0);
    const auto mImportFileLayout = new QHBoxLayout(mImportFileWidget);
    mImportFileLayout->setContentsMargins(0, 0, 0, 0);
    mImportFileLayout->setMargin(0);

    const auto mImportFileLabel = new QLabel(tr("Default import directory"), this);
    mImportFileDir = new QComboBox(this);
    mImportFileDir->addItem(tr("Last used directory"), eSettings::ImportFileDirRecent);
    mImportFileDir->addItem(tr("Project directory"), eSettings::ImportFileDirProject);

    mImportFileLayout->addWidget(mImportFileLabel);
    mImportFileLayout->addWidget(mImportFileDir);

    mProjectLayout->addSpacing(10);
    mProjectLayout->addWidget(mImportFileWidget);

    mGeneralLayout->addStretch();
    addWidget(mGeneralWidget);

    eSizesUI::widget.add(mAutoBackup, [this](const int size) {
        mAutoBackup->setFixedHeight(size);
        mAutoSave->setFixedHeight(size);
        mDefaultInterfaceScaling->setFixedHeight(size);
    });
}

void GeneralSettingsWidget::applySettings()
{
    AppSupport::setSettings("files",
                            "BackupOnSave",
                            mAutoBackup->isChecked());
    AppSupport::setSettings("files",
                            "AutoSave",
                            mAutoSave->isChecked());
    AppSupport::setSettings("files",
                            "AutoSaveTimeout",
                            (mAutoSaveTimer->value() * 60) * 1000);
    MainWindow::sGetInstance()->updateAutoSaveBackupState();

    mSett.fDefaultInterfaceScaling = mDefaultInterfaceScaling->isChecked();
    mSett.fInterfaceScaling = mInterfaceScaling->value() * 0.01;
    mSett.fImportFileDirOpt = mImportFileDir->currentData().toInt();

    //eSizesUI::font.updateSize();
    //eSizesUI::widget.updateSize();
}

void GeneralSettingsWidget::updateSettings(bool restore)
{
    mAutoBackup->setChecked(restore ? false : AppSupport::getSettings("files",
                                                                      "BackupOnSave",
                                                                      false).toBool());
    mAutoSave->setChecked(restore ? false : AppSupport::getSettings("files",
                                                                    "AutoSave",
                                                                    false).toBool());
    int ms = restore ? 300000 : AppSupport::getSettings("files",
                                                        "AutoSaveTimeout",
                                                        300000).toInt();
    if (ms < 60000) { ms = 60000; }
    mAutoSaveTimer->setValue((ms / 1000) / 60);

    mDefaultInterfaceScaling->setChecked(mSett.fDefaultInterfaceScaling);
    mInterfaceScaling->setEnabled(!mDefaultInterfaceScaling->isChecked());
    mInterfaceScaling->setValue(mDefaultInterfaceScaling->isChecked() ? 100 : 100 * mSett.fInterfaceScaling);

    for (int i = 0; i < mImportFileDir->count(); i++) {
        if (mImportFileDir->itemData(i).toInt() == mSett.fImportFileDirOpt) {
            mImportFileDir->setCurrentIndex(i);
            return;
        }
    }
}
