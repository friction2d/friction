/*
#
# Friction - https://friction.graphics
#
# Copyright (c) Friction contributors
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

GeneralSettingsWidget::GeneralSettingsWidget(QWidget *parent)
    : SettingsWidget(parent)
    , mAutoBackup(nullptr)
    , mAutoSave(nullptr)
    , mAutoSaveTimer(nullptr)
    , mDefaultInterfaceScaling(nullptr)
    , mInterfaceScaling(nullptr)
    , mImportFileDir(nullptr)
    , mToolBarActionNew(nullptr)
    , mToolBarActionOpen(nullptr)
    , mToolBarActionSave(nullptr)
    , mToolBarActionScene(nullptr)
    , mToolBarActionRender(nullptr)
    , mToolBarActionPreview(nullptr)
    , mToolBarActionExport(nullptr)
{
    const auto mGeneralWidget = new QWidget(this);
    mGeneralWidget->setContentsMargins(0, 0, 0, 0);
    const auto mGeneralLayout = new QVBoxLayout(mGeneralWidget);
    mGeneralLayout->setContentsMargins(0, 0, 0, 0);

    const auto mAutoBackupWidget = new QGroupBox(this);
    mAutoBackupWidget->setTitle(tr("Backup"));
    mAutoBackupWidget->setContentsMargins(0, 0, 0, 0);
    const auto mAutoBackupLayout = new QHBoxLayout(mAutoBackupWidget);

    const auto mAutoBackupLabel = new QLabel(tr("Enable Backup on Save"), this);
    mAutoBackupLabel->setToolTip(tr("Creates a backup file after each successful save.\n\n"
                                    "Backup files are stored in a folder called PROJECT.friction_backup."));

    mAutoBackup = new QCheckBox(this);
    mAutoBackup->setCheckable(true);

    mAutoBackupLayout->addWidget(mAutoBackupLabel);
    mAutoBackupLayout->addStretch();
    mAutoBackupLayout->addWidget(mAutoBackup);

    mGeneralLayout->addWidget(mAutoBackupWidget);

    const auto mAutoSaveWidget = new QGroupBox(this);
    mAutoSaveWidget->setTitle(tr("Auto Save"));
    mAutoSaveWidget->setContentsMargins(0, 0, 0, 0);
    const auto mAutoSaveLayout = new QHBoxLayout(mAutoSaveWidget);

    const auto mAutoSaveLabel = new QLabel(tr("Enable Auto Save"), this);
    mAutoSaveLabel->setToolTip(tr("Will auto save each X min if project is unsaved.\n\n"
                                  "Enable Backup on Save for incremental saves (and as a failsafe)."));

    mAutoSave = new QCheckBox(this);
    mAutoSave->setCheckable(true);

    mAutoSaveTimer = new QSpinBox(this);
    mAutoSaveTimer->setRange(1, 60);
    mAutoSaveTimer->setSuffix(tr(" min"));

    mAutoSaveLayout->addWidget(mAutoSaveLabel);
    mAutoSaveLayout->addStretch();
    mAutoSaveLayout->addWidget(mAutoSave);
    mAutoSaveLayout->addWidget(mAutoSaveTimer);

    mGeneralLayout->addWidget(mAutoSaveWidget);

    const auto mScaleWidget = new QGroupBox(this);
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

    mScaleContainerLayout->addWidget(mDefaultInterfaceScaling);

    const auto infoLabel = new QLabel(this);
    infoLabel->setText(tr("Changes here will require a restart of Friction."));
    mScaleLayout->addWidget(infoLabel);

    mGeneralLayout->addWidget(mScaleWidget);

    const auto mImportWidget = new QGroupBox(this);
    mImportWidget->setTitle(tr("Import"));
    mImportWidget->setContentsMargins(0, 0, 0, 0);
    const auto mImportLayout = new QVBoxLayout(mImportWidget);

    const auto mImportFileWidget = new QWidget(this);
    mImportFileWidget->setContentsMargins(0, 0, 0, 0);
    const auto mImportFileLayout = new QHBoxLayout(mImportFileWidget);

    const auto mImportFileLabel = new QLabel(tr("Default import directory"), this);
    mImportFileDir = new QComboBox(this);
    mImportFileDir->addItem(tr("Last used directory"), eSettings::ImportFileDirRecent);
    mImportFileDir->addItem(tr("Project directory"), eSettings::ImportFileDirProject);

    mImportFileLayout->addWidget(mImportFileLabel);
    mImportFileLayout->addWidget(mImportFileDir);
    mImportLayout->addWidget(mImportFileWidget);
    mGeneralLayout->addWidget(mImportWidget);

    setupToolBarWidgets(mGeneralLayout);

    mGeneralLayout->addStretch();
    addWidget(mGeneralWidget);

    eSizesUI::widget.add(mAutoBackup, [this](const int size) {
        mAutoBackup->setFixedSize(QSize(size, size));
        mAutoBackup->setStyleSheet(QString("QCheckBox::indicator { width: %1px; height: %1px;}").arg(size/1.5));
        mAutoSave->setFixedSize(QSize(size, size));
        mAutoSave->setStyleSheet(QString("QCheckBox::indicator { width: %1px; height: %1px;}").arg(size/1.5));
        mDefaultInterfaceScaling->setFixedHeight(size);
        mDefaultInterfaceScaling->setStyleSheet(QString("QCheckBox::indicator { width: %1px; height: %1px;}").arg(size/1.5));
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

    mSett.fToolBarActionNew = mToolBarActionNew->isChecked();
    mSett.fToolBarActionOpen = mToolBarActionOpen->isChecked();
    mSett.fToolBarActionSave = mToolBarActionSave->isChecked();
    mSett.fToolBarActionScene = mToolBarActionScene->isChecked();
    mSett.fToolBarActionRender = mToolBarActionRender->isChecked();
    mSett.fToolBarActionPreview = mToolBarActionPreview->isChecked();
    mSett.fToolBarActionExport = mToolBarActionExport->isChecked();

    eSizesUI::font.updateSize();
    eSizesUI::widget.updateSize();
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
    mInterfaceScaling->setValue(mDefaultInterfaceScaling->isChecked() ? 100 : 100 * mSett.fInterfaceScaling);

    mToolBarActionNew->setChecked(mSett.fToolBarActionNew);
    mToolBarActionOpen->setChecked(mSett.fToolBarActionOpen);
    mToolBarActionSave->setChecked(mSett.fToolBarActionSave);
    mToolBarActionScene->setChecked(mSett.fToolBarActionScene);
    mToolBarActionRender->setChecked(mSett.fToolBarActionRender);
    mToolBarActionPreview->setChecked(mSett.fToolBarActionPreview);
    mToolBarActionExport->setChecked(mSett.fToolBarActionExport);

    for (int i = 0; i < mImportFileDir->count(); i++) {
        if (mImportFileDir->itemData(i).toInt() == mSett.fImportFileDirOpt) {
            mImportFileDir->setCurrentIndex(i);
            return;
        }
    }
}

void GeneralSettingsWidget::setupToolBarWidgets(QVBoxLayout *lay)
{
    if (!lay) { return; }

    const auto area = new QScrollArea(this);
    const auto container = new QGroupBox(this);
    const auto containerLayout = new QVBoxLayout(container);
    const auto containerInner = new QWidget(this);
    const auto containerInnerLayout = new QVBoxLayout(containerInner);

    area->setWidget(containerInner);
    area->setWidgetResizable(true);
    area->setContentsMargins(0, 0, 0, 0);

    container->setTitle(tr("Toolbar Actions"));

    container->setContentsMargins(0, 0, 0, 0);

    containerInnerLayout->setMargin(5);
    containerLayout->setMargin(0);

    containerLayout->addWidget(area);

    mToolBarActionNew = new QCheckBox(tr("New"), this);
    containerInnerLayout->addWidget(mToolBarActionNew);

    mToolBarActionOpen = new QCheckBox(tr("Open"), this);
    containerInnerLayout->addWidget(mToolBarActionOpen);

    mToolBarActionSave = new QCheckBox(tr("Save"), this);
    containerInnerLayout->addWidget(mToolBarActionSave);

    mToolBarActionScene = new QCheckBox(tr("Scene"), this);
    containerInnerLayout->addWidget(mToolBarActionScene);

    mToolBarActionRender = new QCheckBox(tr("Render"), this);
    containerInnerLayout->addWidget(mToolBarActionRender);

    mToolBarActionPreview = new QCheckBox(tr("Preview"), this);
    containerInnerLayout->addWidget(mToolBarActionPreview);

    mToolBarActionExport = new QCheckBox(tr("Export"), this);
    containerInnerLayout->addWidget(mToolBarActionExport);

    lay->addWidget(container);
}
