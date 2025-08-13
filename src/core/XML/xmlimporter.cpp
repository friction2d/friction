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

#include "xmlimporter.h"
#include "zipfileloader.h"

using namespace Friction::Core;

XmlReadBoxesHandler::~XmlReadBoxesHandler()
{
    for (const auto& task : mDoneTasks) { task(*this); }
}

void XmlReadBoxesHandler::addReadBox(const int readId,
                                     BoundingBox* const box)
{
    mReadBoxes[readId] = box;
}

BoundingBox *XmlReadBoxesHandler::getBoxByReadId(const int readId) const
{
    const auto it = mReadBoxes.find(readId);
    if (it == mReadBoxes.end()) { return nullptr; }
    else { return it->second; }
}

void XmlReadBoxesHandler::addXevImporterDoneTask(const XmlImporterDoneTask& task)
{
    mDoneTasks << task;
}

XmlImporter::XmlImporter(XmlReadBoxesHandler& xevReadBoxesHandler,
                         ZipFileLoader& fileLoader,
                         const RuntimeIdToWriteId& objListIdConv,
                         const QString& path,
                         const QString& assetsPath)
    : mXevReadBoxesHandler(xevReadBoxesHandler)
    , mFileLoader(fileLoader)
    , mObjectListIdConv(objListIdConv)
    , mPath(path)
    , mAssetsPath(assetsPath)
{

}

XmlReadBoxesHandler &XmlImporter::getXevReadBoxesHandler() const
{
    return mXevReadBoxesHandler;
}

const RuntimeIdToWriteId &XmlImporter::objListIdConv() const
{
    return mObjectListIdConv;
}

XmlImporter XmlImporter::withAssetsPath(const QString& path) const
{
    return XmlImporter(mXevReadBoxesHandler,
                       mFileLoader,
                       mObjectListIdConv,
                       mPath,
                       mAssetsPath + path);
}

void XmlImporter::processAsset(const QString& file,
                               const Processor& func) const
{
    mFileLoader.process(mPath + "assets/" + mAssetsPath + file, func);
}

QString XmlImporter::relPathToAbsPath(const QString& relPath) const
{
    return mFileLoader.relPathToAbsPath(relPath);
}
