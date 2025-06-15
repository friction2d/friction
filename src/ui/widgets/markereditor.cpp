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

#include "markereditor.h"
#include "GUI/global.h"
#include "themesupport.h"

#include <QVBoxLayout>
#include <QPushButton>

using namespace Friction::Ui;

MarkerEditor::MarkerEditor(Scene *scene,
                           QWidget *parent)
    : QWidget{parent}
    , mScene(scene)
    , mTree(nullptr)
{
    const auto lay = new QVBoxLayout(this);

    setContentsMargins(0, 0, 0, 0);
    lay->setContentsMargins(0, 0, 0, 0);

    mTree = new QTreeWidget(this);
    lay->addWidget(mTree);
    setup();
    populate();

    if (mScene) {
        connect(mScene,&Scene::markersChanged,
                this, &MarkerEditor::populate);
    }
}

void MarkerEditor::setup()
{
    mTree->setPalette(
        ThemeSupport::getDefaultPalette(
            ThemeSupport::getThemeButtonBorderColor()));

    mTree->setHeaderLabels(QStringList()
                           << tr("Visibility")
                           << tr("Frame")
                           << tr("Comment"));

    mTree->setTabKeyNavigation(true);
    mTree->setAlternatingRowColors(true);
    mTree->setSortingEnabled(false);
    mTree->setHeaderHidden(false);
    mTree->setRootIsDecorated(false);

    mTree->headerItem()->setText(0, QString());
    mTree->headerItem()->setIcon(0, QIcon::fromTheme("visible"));
    mTree->headerItem()->setIcon(1, QIcon::fromTheme("frame"));
    mTree->headerItem()->setIcon(2, QIcon::fromTheme("markers-add"));

    mTree->resizeColumnToContents(0);

    connect(mTree, &QTreeWidget::itemChanged,
            this, [this](QTreeWidgetItem *item) {
        if (!mScene || item->text(1).isEmpty()) { return; }
        const int oframe = item->data(1, Qt::UserRole).toInt();
        const int frame = item->text(1).toInt();
        const QString title = item->text(2);
        const bool checked = item->checkState(0) == Qt::Checked;
        if (frame != oframe && duplicate(item, frame)) {
            mTree->blockSignals(true);
            item->setText(1, QString::number(oframe));
            mTree->blockSignals(false);
            return;
        }
        if (mScene->hasMarker(frame)) {
            mScene->editMarker(frame, title,
                               checked ? true : false);
        } else {
            if (frame != oframe) {
                mScene->removeMarker(oframe);
            }
            mScene->setMarker(title, frame);
        }
        mScene->updateMarkers();

        mTree->blockSignals(true);
        item->setData(1, Qt::UserRole, frame);
        mTree->blockSignals(false);
    });
}

void MarkerEditor::populate()
{
    if (!mScene) { return; }
    mTree->clear();
    const auto markers = mScene->getMarkers();
    mTree->blockSignals(true);
    for (const auto &marker : markers) {
        auto item = new QTreeWidgetItem(mTree);
        item->setText(2, marker.title);
        item->setText(1, QString::number(marker.frame));
        item->setData(1, Qt::UserRole, marker.frame);
        item->setCheckState(0, marker.enabled ? Qt::Checked : Qt::Unchecked);
        item->setFlags(item->flags() | Qt::ItemIsEditable);
        mTree->addTopLevelItem(item);
    }
    mTree->blockSignals(false);
}

bool MarkerEditor::duplicate(QTreeWidgetItem *item,
                             const int &frame)
{
    for (int i = 0; i < mTree->topLevelItemCount(); i++) {
        auto tItem = mTree->topLevelItem(i);
        if (!tItem) { continue; }
        if (tItem->text(1).toInt() == frame) {
            if (tItem != item) { return true; }
        }
    }
    return false;
}

void MarkerEditor::addMarker()
{
    if (!mScene) { return; }
    const int frame = mScene->getCurrentFrame();
    for (int i = 0; i < mTree->topLevelItemCount(); ++i) {
        auto existingItem = mTree->topLevelItem(i);
        if (existingItem && existingItem->data(1, Qt::UserRole).toInt() == frame) {
            return;
        }
    }
    mScene->setMarker(frame);
}

void MarkerEditor::removeMarker()
{
    auto item = mTree->selectedItems().count() > 0 ? mTree->selectedItems().at(0) : nullptr;
    if (!item) { return; }
    const int frame = item->text(1).toInt();
    if (mScene) { mScene->removeMarker(frame); }
    delete mTree->takeTopLevelItem(mTree->indexOfTopLevelItem(item));
}

void MarkerEditor::clearMarkers()
{
    if (!mScene) { return; }
    mScene->clearMarkers();
    mTree->clear();
}
