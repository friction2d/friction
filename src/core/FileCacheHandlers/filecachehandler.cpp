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

#include "filecachehandler.h"
#include "Boxes/boundingbox.h"
#include "filedatacachehandler.h"

#include <QMessageBox>

FileCacheHandler::FileCacheHandler() {}

void FileCacheHandler::reloadAction() {
    updateFileMissing();
    reload();
    emit reloaded();
}

bool FileCacheHandler::deleteAction() {
    if(mReferenceCount) {
        const int buttonId = QMessageBox::question(
                    nullptr, "Delete", QString("Are you sure you want to delete "
                    "%1 object(s) referencing \"%2\"?").arg(mReferenceCount).arg(mPath),
                    "Cancel", "Delete");
        if(buttonId == 0) return false;
    }
    const auto selfRef = ref<FileCacheHandler>();
    emit deleteApproved(selfRef);
    return true;
}

void FileCacheHandler::setPath(const QString &path) {
    if(mPath == path) return;
    mPath = path;
    updateFileMissing();
    reload();
    emit pathChanged(path);
}

void FileCacheHandler::setMissing(const bool missing) {
    mFileMissing = missing;
}

void FileCacheHandler::updateFileMissing() {
    mFileMissing = !QFileInfo::exists(mPath);
}
