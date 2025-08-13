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

#ifndef FRICTION_XML_EXPORT_HELPERS_H
#define FRICTION_XML_EXPORT_HELPERS_H

#include "exceptions.h"

#include "skia/skiaincludes.h"

#include <QDomElement>
#include <QString>

class SimpleBrushWrapper;
class Property;

namespace Friction
{
    namespace Core
    {
        class XmlImporter;
        class XmlExporter;

        class CORE_EXPORT XmlExportHelpers
        {
        public:
            static SkBlendMode stringToBlendMode(const QString& compOpStr);
            static QString blendModeToString(const SkBlendMode blendMode);
            static qreal stringToDouble(const QStringRef& string);
            static qreal stringToDouble(const QString& string);
            static int stringToInt(const QStringRef& string);
            static int stringToInt(const QString& string);

            template <typename T, typename S>
            static T stringToEnum(const S& string)
            {
                const int intVal = stringToInt(string);
                return static_cast<T>(intVal);
            }

            template <typename T, typename S>
            static T stringToEnum(const S& string,
                                  const T min,
                                  const T max)
            {
                const auto result = stringToEnum<T>(string);
                if (result < min || result > max) {
                    RuntimeThrow("Value outside of enum value range");
                }
                return result;
            }

            template <typename T, typename S>
            static T stringToEnum(const S& string,
                                  const T max)
            {
                return stringToEnum(string, 0, max);
            }

            static QMatrix stringToMatrix(const QString& str);
            static QString matrixToString(const QMatrix& m);

            static QDomElement brushToElement(SimpleBrushWrapper* const brush,
                                              QDomDocument& doc);

            static SimpleBrushWrapper* brushFromElement(const QDomElement& ele);

            static void setAbsAndRelFileSrc(const QString& absSrc,
                                            QDomElement& ele,
                                            const XmlExporter& exp);

            static QString getAbsAndRelFileSrc(const QDomElement& ele,
                                               const XmlImporter& imp);

            static bool writeProperty(QDomElement& ele,
                                      const XmlExporter& exp,
                                      const QString& name,
                                      Property* const prop);

            static bool readProperty(const QDomElement& ele,
                                     const XmlImporter& imp,
                                     const QString& name,
                                     Property* const prop);
        };
    }
}

#endif // FRICTION_XML_EXPORT_HELPERS_H
