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

#include "pluginssettingswidget.h"
#include "appsupport.h"
#include "effectsloader.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDir>
#include <QFile>
#include <QTreeWidgetItem>
#include <QHeaderView>

#include "GUI/global.h"
#include "Private/document.h"

PluginsSettingsWidget::PluginsSettingsWidget(QWidget *parent)
    : SettingsWidget(parent)
    , mShaderPath(nullptr)
    , mShaderTree(nullptr)
    , mCorePath(nullptr)
    , mCoreTree(nullptr)
{
#ifndef USE_GLES
    mShadersList = EffectsLoader::sInstance->getLoadedShaderEffects();

    mShadersDisabled = AppSupport::getSettings("settings",
                                               "DisabledShaders").toStringList();

    const auto mShaderWidget = new QWidget(this);
    mShaderWidget->setContentsMargins(0, 0, 0, 0);
    const auto mShaderLayout = new QHBoxLayout(mShaderWidget);
    mShaderLayout->setContentsMargins(0, 0, 0, 0);

    const auto mShaderLabel = new QLabel(tr("Shaders Path"), this);
    mShaderLabel->setToolTip(tr("This location will be scanned for shader plugins during startup."));
    mShaderPath = new QLineEdit(this);
    mShaderPath->setText(AppSupport::getAppShaderEffectsPath());
    const auto mShaderPathButton = new QPushButton(QIcon::fromTheme("file_folder"),
                                                   QString(),
                                                   this);
    mShaderPathButton->setFocusPolicy(Qt::NoFocus);

    eSizesUI::widget.add(mShaderPathButton, [mShaderPathButton](const int size) {
        Q_UNUSED(size)
        mShaderPathButton->setFixedSize(eSizesUI::button, eSizesUI::button);
    });

    mShaderLayout->addWidget(mShaderLabel);
    mShaderLayout->addWidget(mShaderPath);
    mShaderLayout->addWidget(mShaderPathButton);

    addWidget(mShaderWidget);

    connect(mShaderPathButton, &QPushButton::pressed,
            this, [this]() {
        QString path = AppSupport::getExistingDirectory(this,
                                                        tr("Select directory"),
                                                        QDir::homePath());
        if (QFile::exists(path)) { mShaderPath->setText(path); }
    });

    mShaderTree = new QTreeWidget(this);
    mShaderTree->setHeaderLabels(QStringList() << tr("Shader") << tr("Group"));
    mShaderTree->setAlternatingRowColors(true);
    mShaderTree->setSortingEnabled(false);
    mShaderTree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

    addWidget(mShaderTree);
    populateShaderTree();
#endif

    mCoreDisabled = AppSupport::getSettings("settings",
                                            "DisabledCorePlugins").toStringList();

    const auto mCoreWidget = new QWidget(this);
    mCoreWidget->setContentsMargins(0, 0, 0, 0);
    const auto mCoreLayout = new QHBoxLayout(mCoreWidget);
    mCoreLayout->setContentsMargins(0, 0, 0, 0);

    const auto mCoreLabel = new QLabel(tr("Core Plugins Path"), this);
    mCoreLabel->setToolTip(tr("This location will be scanned for core plugins during startup."));
    mCorePath = new QLineEdit(this);
    mCorePath->setText(AppSupport::getAppUserCorePluginsPath());
    const auto mCorePathButton = new QPushButton(QIcon::fromTheme("file_folder"),
                                                 QString(),
                                                 this);
    mCorePathButton->setFocusPolicy(Qt::NoFocus);

    eSizesUI::widget.add(mCorePathButton, [mCorePathButton](const int size) {
        Q_UNUSED(size)
        mCorePathButton->setFixedSize(eSizesUI::button, eSizesUI::button);
    });

    mCoreLayout->addWidget(mCoreLabel);
    mCoreLayout->addWidget(mCorePath);
    mCoreLayout->addWidget(mCorePathButton);

    addWidget(mCoreWidget);

    connect(mCorePathButton, &QPushButton::pressed,
            this, [this]() {
        QString path = AppSupport::getExistingDirectory(this,
                                                        tr("Select directory"),
                                                        QDir::homePath());
        if (QFile::exists(path)) { mCorePath->setText(path); }
    });

    mCoreTree = new QTreeWidget(this);
    mCoreTree->setHeaderLabels(QStringList() << tr("Plugin") << tr("Description"));
    mCoreTree->setAlternatingRowColors(true);
    mCoreTree->setSortingEnabled(false);
    mCoreTree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

    addWidget(mCoreTree);
    populateCoreTree();
}

void PluginsSettingsWidget::applySettings()
{
#ifndef USE_GLES
    AppSupport::setSettings("settings",
                            "CustomShaderPath",
                            mShaderPath->text());
    QStringList disabledShaders;
    for (int i = 0; i < mShaderTree->topLevelItemCount(); ++i ) {
        const auto item = mShaderTree->topLevelItem(i);
        if (item->checkState(0) == Qt::Unchecked) {
            disabledShaders << item->data(0, Qt::UserRole).toString();
        }
    }
    AppSupport::setSettings("settings",
                            "DisabledShaders",
                            disabledShaders);
#endif

    AppSupport::setSettings("settings",
                            "CustomCorePluginsPath",
                            mCorePath->text());
    QStringList disabledCore;
    for (int i = 0; i < mCoreTree->topLevelItemCount(); ++i ) {
        const auto item = mCoreTree->topLevelItem(i);
        if (item->checkState(0) == Qt::Unchecked) {
            disabledCore << item->data(0, Qt::UserRole).toString();
        }
    }
    AppSupport::setSettings("settings",
                            "DisabledCorePlugins",
                            disabledCore);
}

void PluginsSettingsWidget::updateSettings(bool restore)
{
#ifndef USE_GLES
    mShaderPath->setText(AppSupport::getAppShaderEffectsPath(restore));
    mShadersDisabled = AppSupport::getSettings("settings",
                                               "DisabledShaders").toStringList();
    if (restore) {
        mShadersDisabled.clear();
        populateShaderTree();
    }
#endif

    mCorePath->setText(AppSupport::getAppUserCorePluginsPath(restore));
    mCoreDisabled = AppSupport::getSettings("settings",
                                            "DisabledCorePlugins").toStringList();
    if (restore) {
        mCoreDisabled.clear();
        populateCoreTree();
    }
}

void PluginsSettingsWidget::populateShaderTree()
{
#ifndef USE_GLES
    mShaderTree->setSortingEnabled(false);
    mShaderTree->clear();
    for (auto &shader: mShadersList) {
        QPair<QString, QString> shaderID = AppSupport::getShaderID(shader);
        QTreeWidgetItem *item = new QTreeWidgetItem(mShaderTree);
        item->setCheckState(0, mShadersDisabled.contains(shader) ? Qt::Unchecked : Qt::Checked);
        item->setToolTip(0, shader);
        item->setData(0, Qt::UserRole, shader);
        item->setText(0, shaderID.first);
        item->setText(1, shaderID.second);
        mShaderTree->addTopLevelItem(item);
    }
    mShaderTree->setSortingEnabled(true);
    mShaderTree->sortByColumn(0, Qt::AscendingOrder);
#endif
}

void PluginsSettingsWidget::populateCoreTree()
{
    mCoreTree->setSortingEnabled(false);
    mCoreTree->clear();
    const auto plugins = Document::sInstance->getCorePlugins();
    for (const auto& plugin : plugins) {
        QString pluginId = plugin.meta.value("id").toString().trimmed();
        QString pluginName = plugin.meta.value("name").toString().trimmed();
        QString pluginDesc = plugin.meta.value("description").toString().trimmed();
        if (pluginId.isEmpty() ||
            pluginName.isEmpty() ||
            pluginDesc.isEmpty()) { continue; }

        QTreeWidgetItem *item = new QTreeWidgetItem(mCoreTree);
        item->setCheckState(0, mCoreDisabled.contains(pluginId) ? Qt::Unchecked : Qt::Checked);
        item->setToolTip(0, QString("%1 (%2)\n\n%3").arg(pluginName,
                                                         pluginId,
                                                         pluginDesc));
        item->setData(0, Qt::UserRole, pluginId);
        item->setText(0, pluginName);
        item->setText(1, pluginDesc);
        mCoreTree->addTopLevelItem(item);
    }
    mCoreTree->setSortingEnabled(true);
    mCoreTree->sortByColumn(0, Qt::AscendingOrder);
}
