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

#ifndef FRICTION_XML_ZIP_FILE_SAVER_H
#define FRICTION_XML_ZIP_FILE_SAVER_H

#include "core_global.h"
#include "zipfilesaver.h"

#include <QDir>

namespace Friction
{
    namespace Core
    {
        class CORE_EXPORT XmlZipFileSaver
        {
        public:
            XmlZipFileSaver();
            ZipFileSaver& fileSaver();
            void setPath(const QString& path);
            QString absPathToRelPath(const QString& absPath) const;

        private:
            QDir mDir;
            QFile mFile;
            ZipFileSaver mFileSaver;
        };
    }
}

#endif // FRICTION_XML_ZIP_FILE_SAVER_H
