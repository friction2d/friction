// SPDX-License-Identifier: GPL-3.0-only

#include "mainwindow.h"
#include "appsupport.h"

#include <QDir>
#include <QPluginLoader>
#include <QDebug>
#include <QMenu>
#include <QHash>

void MainWindow::setupMenuPlugins()
{
    if (!mPluginsMenu) { return; }

    QHash<QString, QMenu*> groupMenus;
    const auto loadedPlugins = mDocument.getCorePlugins();

    for (auto it = loadedPlugins.begin(); it != loadedPlugins.end(); ++it) {
        const CorePluginData &pluginData = it.value();
        FrictionCorePluginInterface *plugin = pluginData.instance;
        QJsonObject meta = pluginData.meta;

        if (!plugin) { continue; }

        const auto menuActs = plugin->createMenuActions(mPluginsMenu);
        for (const auto &action : menuActs) {
            if (!action) { continue; }
            if (action->icon().isNull()) { action->setIcon(QIcon::fromTheme("plugin")); }

            QString groupName = meta.value("group").toString();
            QMenu *targetMenu = mPluginsMenu;

            if (!groupName.trimmed().isEmpty()) {
                if (groupMenus.contains(groupName)) {
                    targetMenu = groupMenus.value(groupName);
                } else {
                    QMenu *newGroupMenu = new QMenu(groupName, mPluginsMenu);
                    newGroupMenu->setIcon(QIcon::fromTheme("group"));
                    mPluginsMenu->addMenu(newGroupMenu);
                    groupMenus.insert(groupName, newGroupMenu);
                    targetMenu = newGroupMenu;
                }
            }

            targetMenu->addAction(action);
            QObject::connect(action, &QAction::triggered,
                             this, [this, plugin, action]() {
                plugin->triggerAction(mDocument, mDocument.fActiveScene, action);
            });
        }

        const auto toolActs = plugin->createToolbarActions(mToolbar);
        for (const auto &action : toolActs) {
            if (!action) { continue; }
            if (action->icon().isNull()) { action->setIcon(QIcon::fromTheme("plugin")); }
            mToolbar->addAction(action);
            QObject::connect(action, &QAction::triggered,
                             this, [this, plugin, action]() {
                plugin->triggerAction(mDocument, mDocument.fActiveScene, action);
            });
        }
    }
}
