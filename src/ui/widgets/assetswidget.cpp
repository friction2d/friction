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

#include "assetswidget.h"

#include <QVBoxLayout>
#include <QFileInfo>
#include <QHeaderView>
#include <QDrag>
#include <QMenu>
#include <QDebug>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcAssets, "friction.ui.assets", QtWarningMsg)

#include "appsupport.h"
#include "Private/document.h"
#include "FileCacheHandlers/videocachehandler.h"
#include "FileCacheHandlers/imagecachehandler.h"
#include "FileCacheHandlers/imagesequencecachehandler.h"
#include "filesourcescache.h"
#include "themesupport.h"

#include "GUI/global.h"

AssetsTreeWidget::AssetsTreeWidget(QWidget *parent)
    : QTreeWidget(parent)
{
    setAcceptDrops(true);
    setItemsExpandable(false);
    setSortingEnabled(false);
    setDragEnabled(true);
    setWordWrap(true);
    setFrameShape(QFrame::NoFrame);
    setHeaderLabels(QStringList() << tr("Assets"));
    setHeaderHidden(true);
    header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    header()->setStretchLastSection(true);
    setContextMenuPolicy(Qt::CustomContextMenu);
    eSizesUI::widget.add(this, [this](const int size) {
        setIconSize(QSize(size, size));
    });
}

void AssetsTreeWidget::addAssets(const QList<QUrl> &urls)
{
    for (const QUrl &url : urls) {
        if (url.isLocalFile()) {
            const QString urlStr = url.toLocalFile();
            const QFileInfo fInfo(urlStr);
            const QString ext = fInfo.suffix();
            const auto filesHandler = FilesHandler::sInstance;
            if (fInfo.isDir()) {
                filesHandler->getFileHandler<ImageSequenceFileHandler>(urlStr);
            } else if (isSoundExt(ext)) {
                filesHandler->getFileHandler<SoundFileHandler>(urlStr);
            } else if (isImageExt(ext) || isLayersExt(ext)) {
                filesHandler->getFileHandler<ImageFileHandler>(urlStr);
            } else if (isVideoExt(ext)) {
                filesHandler->getFileHandler<VideoFileHandler>(urlStr);
            }
        }
    }
    Document::sInstance->actionFinished();
}

void AssetsTreeWidget::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        const QList<QUrl> urlList = event->mimeData()->urls();
        for (const QUrl &url : urlList) {
            if (url.isLocalFile()) {
                const QString urlStr = url.toLocalFile();
                const QFileInfo fInfo(urlStr);
                const QString ext = fInfo.suffix();
                const auto filesHandler = FilesHandler::sInstance;
                if (fInfo.isDir()) {
                    filesHandler->getFileHandler<ImageSequenceFileHandler>(urlStr);
                } else if (isSoundExt(ext)) {
                    filesHandler->getFileHandler<SoundFileHandler>(urlStr);
                } else if (isImageExt(ext) || isLayersExt(ext)) {
                    filesHandler->getFileHandler<ImageFileHandler>(urlStr);
                } else if (isVideoExt(ext)) {
                    filesHandler->getFileHandler<VideoFileHandler>(urlStr);
                }
            }
        }
        event->acceptProposedAction();
    }
    Document::sInstance->actionFinished();
}

void AssetsTreeWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) { event->acceptProposedAction(); }
}

void AssetsTreeWidget::dragMoveEvent(QDragMoveEvent *event)
{
    event->acceptProposedAction();
}

AssetsWidgetItem::AssetsWidgetItem(QTreeWidget *parent,
                                   FileCacheHandler *cache)
    : QTreeWidgetItem(parent)
    , mCache(cache)
{
    if (mCache) {
        setCache(mCache->path(), mCache->fileMissing());
        QObject::connect(mCache, &FileCacheHandler::pathChanged,
                         mCache, [this](const QString &newPath) {
            setCache(newPath, mCache->fileMissing());
        });
    }
}

FileCacheHandler *AssetsWidgetItem::getCache()
{
    return mCache;
}

void AssetsWidgetItem::setCache(const QString &path,
                                bool missing)
{
    QFileInfo info(path);
    setData(0, Qt::UserRole, path);
    setText(0, QString("%1.%2").arg(info.baseName(),
                                    info.completeSuffix()));
    setIcon(0, missing ? QIcon::fromTheme("error") : QIcon::fromTheme(AppSupport::getFileIcon(path)));
    setToolTip(0, missing ? QObject::tr("Media offline") : path);
}

AssetsWidget::AssetsWidget(QWidget *parent)
    : QWidget(parent)
    , mTree(nullptr)
{
    setAcceptDrops(true);
    setPalette(ThemeSupport::getDarkPalette());
    setAutoFillBackground(true);
    setContentsMargins(0, 10, 0, 0);
    QVBoxLayout *mLayout = new QVBoxLayout(this);
    mLayout->setMargin(0);

    mTree = new AssetsTreeWidget(this);
    mLayout->addWidget(mTree);

    connect(mTree, &QTreeWidget::customContextMenuRequested,
            this, [this](const QPoint &pos) {
        AssetsWidgetItem *item = dynamic_cast<AssetsWidgetItem*>(mTree->itemAt(pos));
        if (!item) { return; }

        QMenu menu(this);
        menu.addAction(QIcon::fromTheme("loop"), tr("Reload"));
        menu.addAction(QIcon::fromTheme("file_blank"), tr("Replace"));
        menu.addAction(QIcon::fromTheme("trash"), tr("Remove"));

        const auto selectedAct = menu.exec(mTree->mapToGlobal(pos));
        if (selectedAct && item->getCache()) {
            if (selectedAct->text() == tr("Reload")) {
                item->getCache()->reloadAction();
            } else if (selectedAct->text() == tr("Replace")) {
                item->getCache()->replace();
            } else if (selectedAct->text() == tr("Remove")) {
                item->getCache()->deleteAction();
            }
            Document::sInstance->actionFinished();
        }
    });

    connect(FilesHandler::sInstance, &FilesHandler::addedCacheHandler,
            this, &AssetsWidget::addCacheHandler);
    connect(FilesHandler::sInstance, &FilesHandler::removedCacheHandler,
            this, &AssetsWidget::removeCacheHandler);
}

void AssetsWidget::addCacheHandler(FileCacheHandler *handler)
{
    if (!handler) { return; }
    qCDebug(lcAssets) << "addCacheHandler" << handler->path();
    mCacheList << handler;
    AssetsWidgetItem *item = new AssetsWidgetItem(mTree, handler);
    mTree->addTopLevelItem(item);
}

void AssetsWidget::removeCacheHandler(FileCacheHandler *handler)
{
    if (!handler) { return; }
    for (int i = 0; i < mTree->topLevelItemCount(); i++) {
        AssetsWidgetItem *item = dynamic_cast<AssetsWidgetItem*>(mTree->topLevelItem(i));
        if (!item) { continue; }
        if (item->getCache() == handler) {
            delete mTree->takeTopLevelItem(i);
            break;
        }
    }
    for (int i = 0; i < mCacheList.count(); i++) {
        const auto& abs = mCacheList.at(i);
        if (abs == handler) {
            mCacheList.removeAt(i);
            break;
        }
    }
}

void AssetsWidget::addAssets(const QList<QUrl> &urls)
{
    mTree->addAssets(urls);
}
