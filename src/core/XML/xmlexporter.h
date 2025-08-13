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

#ifndef FRICTION_XML_EXPORTER_H
#define FRICTION_XML_EXPORTER_H

#include "smartPointers/stdselfref.h"

#include <QDomDocument>

namespace Friction
{
    namespace Core
    {
        class RuntimeIdToWriteId;
        class XmlZipFileSaver;

        class CORE_EXPORT XmlExporter : public StdSelfRef
        {
        public:
            XmlExporter(QDomDocument& doc,
                        const std::shared_ptr<XmlZipFileSaver>& xevFileSaver,
                        const RuntimeIdToWriteId& objListIdConv,
                        const QString& path,
                        const QString& assetsPath = "");

            const RuntimeIdToWriteId& objListIdConv() const;

            QDomDocument& doc() const { return mDoc; }

            stdsptr<XmlExporter> withAssetsPath(const QString& path) const;

            QDomElement createElement(const QString& tagName) const;
            QDomText createTextNode(const QString& data) const;

            using Processor = std::function<void(QIODevice* const dst)>;
            void processAsset(const QString& file,
                              const Processor& func,
                              const bool compress = true) const;

            QString absPathToRelPath(const QString& absPath) const;

        private:
            QDomDocument& mDoc;
            const stdsptr<XmlZipFileSaver> mFileSaver;
            const RuntimeIdToWriteId& mObjectListIdConv;
            const QString mPath;
            const QString mAssetsPath;
        };
    }
}

#endif // FRICTION_XML_EXPORTER_H
