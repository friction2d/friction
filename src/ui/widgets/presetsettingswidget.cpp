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

#include "presetsettingswidget.h"
#include "appsupport.h"

#include <QGroupBox>
#include <QScrollArea>
#include <QHeaderView>
#include <QList>
#include <QPair>
#include <QPushButton>

#include "GUI/global.h"

PresetSettingsWidget::PresetSettingsWidget(QWidget *parent)
    : SettingsWidget(parent)
    , mTreeResolutions(nullptr)
    , mTreeFps(nullptr)
    , mCheckResolutions(nullptr)
    , mCheckResolutionsAuto(nullptr)
    , mCheckFps(nullptr)
    , mCheckFpsAuto(nullptr)
{
    setupResolutionPresetWidget();
    setupFpsPresetWidget();

    eSizesUI::widget.add(this, [this](const int size) {
        mCheckResolutions->setFixedHeight(size);
        mCheckResolutionsAuto->setFixedHeight(size);
        mCheckFps->setFixedHeight(size);
        mCheckFpsAuto->setFixedHeight(size);
    });
}

void PresetSettingsWidget::applySettings()
{
    saveResolutionsPresets();
    saveFpsPresets();
}

void PresetSettingsWidget::updateSettings(bool restore)
{
    Q_UNUSED(restore)
    populateResolutionsPresets();
    populateFpsPresets();
}

void PresetSettingsWidget::setupResolutionPresetWidget()
{
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

    container->setTitle(tr("Scene Resolutions"));

    container->setContentsMargins(0, 0, 0, 0);

    containerInnerLayout->setContentsMargins(0, 0, 0, 0);

    containerLayout->addWidget(area);

    mTreeResolutions = new QTreeWidget(this);
    mTreeResolutions->setHeaderLabels(QStringList() << QString("") << tr("Width") << tr("Height"));
    mTreeResolutions->setAlternatingRowColors(true);
    mTreeResolutions->setSortingEnabled(false);
    mTreeResolutions->setHeaderHidden(true);
    mTreeResolutions->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    mTreeResolutions->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    const auto mPushButtonAddResolution = new QPushButton(QIcon::fromTheme("plus"),
                                                          QString(),
                                                          this);
    mPushButtonAddResolution->setObjectName("FlatButton");
    mPushButtonAddResolution->setFocusPolicy(Qt::NoFocus);

    connect(mPushButtonAddResolution, &QPushButton::pressed,
            this, [this]() {
        QTreeWidgetItem *item = new QTreeWidgetItem(mTreeResolutions);
        item->setCheckState(0, Qt::Checked);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        item->setText(1, QString::number(0));
        item->setText(2, QString::number(0));
        mTreeResolutions->addTopLevelItem(item);
    });
    mTreeResolutions->addScrollBarWidget(mPushButtonAddResolution, Qt::AlignBottom);

    mCheckResolutions = new QCheckBox(this);
    mCheckResolutionsAuto = new QCheckBox(this);

    mCheckResolutions->setText(tr("Enable presets"));
    mCheckResolutionsAuto->setText(tr("Auto add presets"));

    containerInnerLayout->addWidget(mCheckResolutions);
    containerInnerLayout->addWidget(mCheckResolutionsAuto);
    containerInnerLayout->addWidget(mTreeResolutions);

    addWidget(container);
}

void PresetSettingsWidget::populateResolutionsPresets()
{
    mCheckResolutions->setChecked(AppSupport::getSettings("presets",
                                                          "EnableResolutions",
                                                          true).toBool());
    mCheckResolutionsAuto->setChecked(AppSupport::getSettings("presets",
                                                              "EnableResolutionsAuto",
                                                              true).toBool());

    mTreeResolutions->clear();
    for (auto &resolution: AppSupport::getResolutionPresets()) {
        QTreeWidgetItem *item = new QTreeWidgetItem(mTreeResolutions);
        item->setCheckState(0, Qt::Checked);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        item->setText(1, QString::number(resolution.first));
        item->setText(2, QString::number(resolution.second));
        mTreeResolutions->addTopLevelItem(item);
    }
}

void PresetSettingsWidget::saveResolutionsPresets()
{
    AppSupport::setSettings("presets",
                            "EnableResolutions",
                            mCheckResolutions->isChecked());
    AppSupport::setSettings("presets",
                            "EnableResolutionsAuto",
                            mCheckResolutionsAuto->isChecked());

    QList<QPair<int, int>> resolutions;
    for (int i = 0; i < mTreeResolutions->topLevelItemCount(); ++i) {
        const auto item = mTreeResolutions->topLevelItem(i);
        if (item->checkState(0) != Qt::Checked) { continue; }
        QPair<int, int> resolution;
        resolution.first = item->text(1).toInt();
        resolution.second = item->text(2).toInt();
        resolutions << resolution;
    }
    if (resolutions != AppSupport::getResolutionPresets()) {
        AppSupport::saveResolutionPresets(resolutions);
    }
}

void PresetSettingsWidget::setupFpsPresetWidget()
{
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

    container->setTitle(tr("Scene Frame Rates"));

    container->setContentsMargins(0, 0, 0, 0);

    containerInnerLayout->setContentsMargins(0, 0, 0, 0);

    containerLayout->addWidget(area);

    mTreeFps = new QTreeWidget(this);
    mTreeFps->setHeaderLabels(QStringList() << tr("FPS"));
    mTreeFps->setAlternatingRowColors(true);
    mTreeFps->setSortingEnabled(false);
    mTreeFps->setHeaderHidden(true);
    mTreeFps->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    const auto mPushButtonAddFps = new QPushButton(QIcon::fromTheme("plus"),
                                                   QString(),
                                                   this);
    mPushButtonAddFps->setObjectName("FlatButton");
    mPushButtonAddFps->setFocusPolicy(Qt::NoFocus);

    connect(mPushButtonAddFps, &QPushButton::pressed,
            this, [this]() {
        QTreeWidgetItem *item = new QTreeWidgetItem(mTreeFps);
        item->setCheckState(0, Qt::Checked);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        item->setText(0, QString::number(0));
        mTreeFps->addTopLevelItem(item);
    });
    mTreeFps->addScrollBarWidget(mPushButtonAddFps, Qt::AlignBottom);

    mCheckFps = new QCheckBox(this);
    mCheckFpsAuto = new QCheckBox(this);

    mCheckFps->setText(tr("Enable presets"));
    mCheckFpsAuto->setText(tr("Auto add presets"));

    containerInnerLayout->addWidget(mCheckFps);
    containerInnerLayout->addWidget(mCheckFpsAuto);
    containerInnerLayout->addWidget(mTreeFps);

    addWidget(container);
}

void PresetSettingsWidget::populateFpsPresets()
{
    mCheckFps->setChecked(AppSupport::getSettings("presets",
                                                  "EnableFPS",
                                                  true).toBool());
    mCheckFpsAuto->setChecked(AppSupport::getSettings("presets",
                                                      "EnableFPSAuto",
                                                      true).toBool());

    mTreeFps->clear();
    for (auto &fps: AppSupport::getFpsPresets()) {
        QTreeWidgetItem *item = new QTreeWidgetItem(mTreeFps);
        item->setCheckState(0, Qt::Checked);
        item->setText(0, fps);
        mTreeFps->addTopLevelItem(item);
    }
}

void PresetSettingsWidget::saveFpsPresets()
{
    AppSupport::setSettings("presets",
                            "EnableFPS",
                            mCheckFps->isChecked());
    AppSupport::setSettings("presets",
                            "EnableFPSAuto",
                            mCheckFpsAuto->isChecked());

    QStringList fps;
    for (int i = 0; i < mTreeFps->topLevelItemCount(); ++i) {
        const auto item = mTreeFps->topLevelItem(i);
        if (item->checkState(0) != Qt::Checked ||
            item->text(0).toDouble() <= 1.) { continue; }
        fps << item->text(0);
    }
    if (fps != AppSupport::getFpsPresets()) {
        AppSupport::saveFpsPresets(fps);
    }
}
