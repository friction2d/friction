/*
#
# Friction - https://friction.graphics
#
# Copyright (c) Ole-André Rodlie and contributors
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3.
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

#include "xmlzipfilesaver.h"
#include "exceptions.h"

using namespace Friction::Core;

XmlZipFileSaver::XmlZipFileSaver() {}

ZipFileSaver &XmlZipFileSaver::fileSaver()
{
    return mFileSaver;
}

void XmlZipFileSaver::setPath(const QString& path)
{
    mDir.setPath(QFileInfo(path).path());
    mFile.setFileName(path);
    if (mFile.exists()) { mFile.remove(); }
    if (!mFile.open(QIODevice::WriteOnly)) {
        RuntimeThrow("Could not open file for writing '" + path + "'.");
    }
    mFileSaver.setIoDevice(&mFile);
}

QString XmlZipFileSaver::absPathToRelPath(const QString& absPath) const
{
    return mDir.relativeFilePath(absPath);
}
