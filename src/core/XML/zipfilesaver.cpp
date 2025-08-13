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

#include "zipfilesaver.h"
#include "exceptions.h"

using namespace Friction::Core;

ZipFileSaver::ZipFileSaver() {}

void ZipFileSaver::setZipPath(const QString &path)
{
    mZip.setZipName(path);
    if (!mZip.open(QuaZip::mdCreate)) {
        RuntimeThrow(QObject::tr("Could not create %1").arg(path));
    }
    mFile.setZip(&mZip);
}

void ZipFileSaver::setIoDevice(QIODevice * const src)
{
    mZip.setIoDevice(src);
    if (!mZip.open(QuaZip::mdCreate)) {
        RuntimeThrow(QObject::tr("Could not open QIODevice"));
    }
    mFile.setZip(&mZip);
}

void ZipFileSaver::process(const QString &file,
                           const Processor &func,
                           const bool compress)
{
    if (!mFile.open(QIODevice::WriteOnly,
                    QuaZipNewInfo(file),
                    NULL, compress ? Z_DEFLATED : 0)) {
        RuntimeThrow(QObject::tr("Could not open %1 in %2").arg(file,
                                                                mZip.getZipName()));
    }
    try { func(&mFile); } catch(...) {
        mFile.close();
        RuntimeThrow(QObject::tr("Could not write %1 to %2").arg(file,
                                                                 mZip.getZipName()));
    }
    mFile.close();
}

void ZipFileSaver::processText(const QString& file,
                               const TextProcessor& func,
                               const bool compress)
{
    process(file, [&func](QIODevice* const dst) {
        QTextStream stream(dst);
        func(stream);
    }, compress);
}
