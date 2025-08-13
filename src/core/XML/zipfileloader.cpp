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

#include "zipfileloader.h"
#include "exceptions.h"

using namespace Friction::Core;

ZipFileLoader::ZipFileLoader() {}

void ZipFileLoader::setZipPath(const QString &path)
{
    mDir.setPath(QFileInfo(path).path());
    mZip.setZipName(path);
    if (!mZip.open(QuaZip::mdUnzip)) {
        RuntimeThrow(QObject::tr("Could not open %1").arg(path));
    }
    mFile.setZip(&mZip);
}

void ZipFileLoader::process(const QString &file,
                            const Processor &func)
{
    if (!mZip.setCurrentFile(file)) {
        RuntimeThrow(QObject::tr("No %1 found in %2").arg(file,
                                                          mZip.getZipName()));
    }
    if (!mFile.open(QIODevice::ReadOnly)) {
        RuntimeThrow(QObject::tr("Could not open %1 from %2").arg(file,
                                                                  mZip.getZipName()));
    }
    try { func(&mFile); } catch(...) {
        mFile.close();
        RuntimeThrow(QObject::tr("Could not parse %1 from %2").arg(file,
                                                                   mZip.getZipName()));
    }
    mFile.close();
}

void ZipFileLoader::processText(const QString& file,
                                const TextProcessor& func)
{
    process(file, [&](QIODevice* const src) {
        QTextStream stream(src);
        func(stream);
    });
}

QString ZipFileLoader::relPathToAbsPath(const QString& relPath) const
{
    const QString absPath = mDir.absoluteFilePath(relPath);
    const QFileInfo fi(absPath);
    return fi.absoluteFilePath();
}
