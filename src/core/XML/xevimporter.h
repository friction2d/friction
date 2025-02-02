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

#ifndef XML_IMPORTER_H
#define XML_IMPORTER_H

#include "core_global.h"

#include <functional>
#include <QList>
#include <QString>
#include <QIODevice>

class BoundingBox;

namespace Friction
{
    namespace Core
    {
        class RuntimeIdToWriteId;
        class ZipFileLoader;

        class CORE_EXPORT XmlReadBoxesHandler
        {
        public:
            ~XmlReadBoxesHandler();
            void addReadBox(const int readId,
                            BoundingBox * const box);
            BoundingBox *getBoxByReadId(const int readId) const;
            using XmlImporterDoneTask = std::function<void(const XmlReadBoxesHandler&)>;
            void addXevImporterDoneTask(const XmlImporterDoneTask& task);
        private:
            std::map<int, BoundingBox*> mReadBoxes;
            QList<XmlImporterDoneTask> mDoneTasks;
        };

        class CORE_EXPORT XmlImporter
        {
        public:
            XmlImporter(XmlReadBoxesHandler& xevReadBoxesHandler,
                        ZipFileLoader& fileLoader,
                        const RuntimeIdToWriteId& objListIdConv,
                        const QString& path,
                        const QString& assetsPath = "");

            XmlReadBoxesHandler& getXevReadBoxesHandler() const {
                return mXevReadBoxesHandler;
            }

            const RuntimeIdToWriteId& objListIdConv() const { return mObjectListIdConv; }
            XmlImporter withAssetsPath(const QString& path) const;
            using Processor = std::function<void(QIODevice* const dst)>;
            void processAsset(const QString& file, const Processor& func) const;
            QString relPathToAbsPath(const QString& relPath) const;

        private:
            XmlReadBoxesHandler& mXevReadBoxesHandler;
            ZipFileLoader& mFileLoader;
            const RuntimeIdToWriteId& mObjectListIdConv;
            const QString mPath;
            const QString mAssetsPath;
        };
    }
}

#endif // XML_IMPORTER_H
