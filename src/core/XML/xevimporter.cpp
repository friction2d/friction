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

#include "xevimporter.h"

#include "../zipfileloader.h"

using namespace Friction::Core;

XMLReadBoxesHandler::~XMLReadBoxesHandler()
{
    for (const auto& task : mDoneTasks) { task(*this); }
}

void XMLReadBoxesHandler::addReadBox(const int readId,
                                     BoundingBox* const box)
{
    mReadBoxes[readId] = box;
}

BoundingBox *XMLReadBoxesHandler::getBoxByReadId(const int readId) const
{
    const auto it = mReadBoxes.find(readId);
    if (it == mReadBoxes.end()) { return nullptr; }
    else { return it->second; }
}

void XMLReadBoxesHandler::addXevImporterDoneTask(const XevImporterDoneTask& task)
{
    mDoneTasks << task;
}

XevImporter::XevImporter(XMLReadBoxesHandler& xevReadBoxesHandler,
                         ZipFileLoader& fileLoader,
                         const RuntimeIdToWriteId& objListIdConv,
                         const QString& path,
                         const QString& assetsPath) :
    mXevReadBoxesHandler(xevReadBoxesHandler),
    mFileLoader(fileLoader),
    mObjectListIdConv(objListIdConv),
    mPath(path), mAssetsPath(assetsPath) {}

XevImporter XevImporter::withAssetsPath(const QString& path) const {
    return XevImporter(mXevReadBoxesHandler,
                       mFileLoader, mObjectListIdConv,
                       mPath, mAssetsPath + path);
}

void XevImporter::processAsset(const QString& file, const Processor& func) const {
    mFileLoader.process(mPath + "assets/" + mAssetsPath + file, func);
}

QString XevImporter::relPathToAbsPath(const QString& relPath) const {
    return mFileLoader.relPathToAbsPath(relPath);
}
