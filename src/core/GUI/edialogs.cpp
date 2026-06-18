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

#include "edialogs.h"

#include "Private/esettings.h"

#include <QFileDialog>
#include <QFileIconProvider>
#include <QFileSystemModel>
#include <QSortFilterProxyModel>
#include <QMessageBox>

#include "themesupport.h"

class evIconProvider : public QFileIconProvider {
public:
    evIconProvider() {
        mEvIcon = QIcon::fromTheme(ThemeSupport::getAppIconName(true));
    }

    QIcon icon(const QFileInfo & info) const {
        const QString name = info.fileName();
        if(name.endsWith(".friction") ||
           name.endsWith(".ev")) return mEvIcon;
        return QFileIconProvider::icon(info);
    }
private:
    QIcon mEvIcon;
};

QString eDialogs::openFile(const QString &title,
                           const QString &path,
                           const QString &filter) {

    QFileDialog dialog(nullptr, title, path);
    // dialog.setOption(QFileDialog::DontUseNativeDialog);
    dialog.setNameFilter(filter);
    auto iconProvider = new evIconProvider;
    dialog.setIconProvider(iconProvider);
    if(dialog.exec()) {
        const QStringList paths = dialog.selectedFiles();
        const QString openPath(paths.isEmpty() ? "" : paths.first());
        return openPath;
    }
    return "";
}

QStringList eDialogs::openFiles(const QString &title,
                                const QString &path,
                                const QString &filter) {
    QFileDialog dialog(nullptr, title, path);
    // dialog.setOption(QFileDialog::DontUseNativeDialog);
    dialog.setFileMode(QFileDialog::ExistingFiles);
    dialog.setNameFilter(filter);
    auto iconProvider = new evIconProvider;
    dialog.setIconProvider(iconProvider);
    if(dialog.exec()) return dialog.selectedFiles();
    return QStringList();
}

QString eDialogs::openDir(const QString &title, const QString &path) {
    QFileDialog dialog(nullptr, title, path);
    // dialog.setOption(QFileDialog::DontUseNativeDialog);
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setOption(QFileDialog::ShowDirsOnly);
    auto iconProvider = new evIconProvider;
    dialog.setIconProvider(iconProvider);
    if(dialog.exec()) {
        const QStringList paths = dialog.selectedFiles();
        const QString openPath(paths.isEmpty() ? "" : paths.first());
        return openPath;
    }
    return "";
}

QString eDialogs::saveFile(const QString &title,
                           const QString &path,
                           const QString &filter,
                           const QString &defSuffix)
{
    QString currentPath = path;

    while (true) {
        QFileDialog dialog(nullptr, title, currentPath);
        dialog.setAcceptMode(QFileDialog::AcceptSave);
        if (!AppSupport::isFlatpak() &&
            !defSuffix.isEmpty()) { dialog.setDefaultSuffix(defSuffix); }
        dialog.setFileMode(QFileDialog::AnyFile);
        dialog.setNameFilter(filter);

        evIconProvider iconProvider;
        dialog.setIconProvider(&iconProvider);

        if (!dialog.exec()) { return QString(); }

        const QStringList paths = dialog.selectedFiles();
        if (paths.isEmpty()) { return QString(); }

        QString openPath = paths.first();
        if (QUrl(openPath).isLocalFile()) {
            openPath = QUrl(openPath).toLocalFile();
        }

        QFileInfo fileInfo(openPath);
        if (AppSupport::isFlatpak() &&
            !defSuffix.isEmpty() &&
            fileInfo.suffix().isEmpty()) {
            QMessageBox::warning(nullptr,
                                 QObject::tr("Missing file extension"),
                                 QObject::tr("Please add missing file extension (%1)").arg(defSuffix));
            currentPath = openPath + "." + defSuffix;
            continue;
        }
        return openPath;
    }

    return QString();
}
