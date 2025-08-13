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

#include "xmlexporter.h"
#include "XML/xmlzipfilesaver.h"
#include "smartPointers/ememory.h"

using namespace Friction::Core;

XmlExporter::XmlExporter(QDomDocument& doc,
                         const std::shared_ptr<Friction::Core::XmlZipFileSaver>& xevFileSaver,
                         const RuntimeIdToWriteId& objListIdConv,
                         const QString& path,
                         const QString& assetsPath)
    : mDoc(doc)
    , mFileSaver(xevFileSaver)
    , mObjectListIdConv(objListIdConv)
    , mPath(path)
    , mAssetsPath(assetsPath)
{

}

const RuntimeIdToWriteId &XmlExporter::objListIdConv() const
{
    return mObjectListIdConv;
}

stdsptr<XmlExporter> XmlExporter::withAssetsPath(const QString& path) const
{
    return enve::make_shared<XmlExporter>(mDoc,
                                          mFileSaver,
                                          mObjectListIdConv,
                                          mPath,
                                          mAssetsPath + path);
}

QDomElement XmlExporter::createElement(const QString& tagName) const
{
    return mDoc.createElement(tagName);
}

QDomText XmlExporter::createTextNode(const QString& data) const
{
    return mDoc.createTextNode(data);
}

void XmlExporter::processAsset(const QString& file,
                               const Processor& func,
                               const bool compress) const
{
    auto& fileSaver = mFileSaver->fileSaver();
    fileSaver.process(mPath + "assets/" + mAssetsPath + file,
                      func,
                      compress);
}

QString XmlExporter::absPathToRelPath(const QString& absPath) const
{
    return mFileSaver->absPathToRelPath(absPath);
}
