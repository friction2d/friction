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

#ifndef XMLEXPORTHELPERS_H
#define XMLEXPORTHELPERS_H

#include "../exceptions.h"

#include "../skia/skiaincludes.h"

#include <QDomElement>
#include <QString>

class QMatrix4x4;
class SimpleBrushWrapper;
class XevExporter;
class XevImporter;
class Property;

namespace XmlExportHelpers {
    CORE_EXPORT
    SkBlendMode stringToBlendMode(const QString& compOpStr);
    CORE_EXPORT
    QString blendModeToString(const SkBlendMode blendMode);

    CORE_EXPORT
    qreal stringToDouble(const QStringRef& string);
    CORE_EXPORT
    qreal stringToDouble(const QString& string);
    CORE_EXPORT
    int stringToInt(const QStringRef& string);
    CORE_EXPORT
    int stringToInt(const QString& string);

    template <typename T, typename S>
    T stringToEnum(const S& string) {
        const int intVal = stringToInt(string);
        return static_cast<T>(intVal);
    }

    template <typename T, typename S>
    T stringToEnum(const S& string, const T min, const T max) {
        const auto result = stringToEnum<T>(string);
        if(result < min || result > max)
            RuntimeThrow("Value outside of enum value range");
        return result;
    }

    template <typename T, typename S>
    T stringToEnum(const S& string, const T max) {
        return stringToEnum(string, 0, max);
    }

    CORE_EXPORT
    QMatrix stringToMatrix(const QString& str);
    CORE_EXPORT
    QString matrixToString(const QMatrix& m);

    CORE_EXPORT
    QMatrix4x4 stringToMatrix4x4(const QString& str);
    /*CORE_EXPORT TODO
    QString matrix4x4ToString(const QMatrix4x4& matrix);*/
};

namespace XevExportHelpers {
    CORE_EXPORT
    QDomElement brushToElement(SimpleBrushWrapper* const brush,
                               QDomDocument& doc);
    CORE_EXPORT
    SimpleBrushWrapper* brushFromElement(const QDomElement& ele);

    CORE_EXPORT
    void setAbsAndRelFileSrc(const QString& absSrc,
                             QDomElement& ele,
                             const XevExporter& exp);
    CORE_EXPORT
    QString getAbsAndRelFileSrc(const QDomElement& ele,
                                const XevImporter& imp);
    CORE_EXPORT
    bool writeProperty(QDomElement& ele, const XevExporter& exp,
                       const QString& name, Property* const prop);
    CORE_EXPORT
    bool readProperty(const QDomElement& ele, const XevImporter& imp,
                      const QString& name, Property* const prop);
};

#endif // XMLEXPORTHELPERS_H
