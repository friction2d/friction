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

#ifndef XEVIMPORTER_H
#define XEVIMPORTER_H

#include "xmlexporthelpers.h"

#include "runtimewriteid.h"
#include "zipfileloader.h"

class BoundingBox;

namespace Friction
{
    namespace Core
    {
        class CORE_EXPORT XMLReadBoxesHandler
        {
        public:
            ~XMLReadBoxesHandler();
            void addReadBox(const int readId,
                            BoundingBox * const box);
            BoundingBox *getBoxByReadId(const int readId) const;
            using XevImporterDoneTask = std::function<void(const XMLReadBoxesHandler&)>;
            void addXevImporterDoneTask(const XevImporterDoneTask& task);
        private:
            std::map<int, BoundingBox*> mReadBoxes;
            QList<XevImporterDoneTask> mDoneTasks;
        };
    }
}

class XevImporter {
public:
    XevImporter(Friction::Core::XMLReadBoxesHandler& xevReadBoxesHandler,
                Friction::Core::ZipFileLoader& fileLoader,
                const RuntimeIdToWriteId& objListIdConv,
                const QString& path,
                const QString& assetsPath = "");

    Friction::Core::XMLReadBoxesHandler& getXevReadBoxesHandler() const {
        return mXevReadBoxesHandler;
    }

    const RuntimeIdToWriteId& objListIdConv() const { return mObjectListIdConv; }

    XevImporter withAssetsPath(const QString& path) const;

    using Processor = std::function<void(QIODevice* const dst)>;
    void processAsset(const QString& file, const Processor& func) const;

    QString relPathToAbsPath(const QString& relPath) const;
private:
    Friction::Core::XMLReadBoxesHandler& mXevReadBoxesHandler;
    Friction::Core::ZipFileLoader& mFileLoader;
    const RuntimeIdToWriteId& mObjectListIdConv;
    const QString mPath;
    const QString mAssetsPath;
};

#endif // XEVIMPORTER_H
