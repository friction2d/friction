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

#include "xmlexporthelpers.h"

#include <QMatrix4x4>

#include "Properties/property.h"
#include "Paint/simplebrushwrapper.h"
#include "Paint/brushescontext.h"
#include "xevexporter.h"
#include "xevimporter.h"
#include "exceptions.h"
#include "appsupport.h"

SkBlendMode XmlExportHelpers::stringToBlendMode(const QString& compOpStr) {
    if(compOpStr.isEmpty()) return SkBlendMode::kSrcOver;
    if(compOpStr == "svg:src-over") {
        return SkBlendMode::kSrcOver;
    } else if(compOpStr == "svg:multiply") {
        return SkBlendMode::kMultiply;
    } else if(compOpStr == "svg:screen") {
        return SkBlendMode::kScreen;
    } else if(compOpStr == "svg:overlay") {
        return SkBlendMode::kOverlay;
    } else if(compOpStr == "svg:darken") {
        return SkBlendMode::kDarken;
    } else if(compOpStr == "svg:lighten") {
        return SkBlendMode::kLighten;
    } else if(compOpStr == "svg:color-dodge") {
        return SkBlendMode::kColorDodge;
    } else if(compOpStr == "svg:color-burn") {
        return SkBlendMode::kColorBurn;
    } else if(compOpStr == "svg:hard-light") {
        return SkBlendMode::kHardLight;
    } else if(compOpStr == "svg:soft-light") {
        return SkBlendMode::kSoftLight;
    } else if(compOpStr == "svg:difference") {
        return SkBlendMode::kDifference;
    } else if(compOpStr == "svg:color") {
        return SkBlendMode::kColor;
    } else if(compOpStr == "svg:luminosity") {
        return SkBlendMode::kLuminosity;
    } else if(compOpStr == "svg:hue") {
        return SkBlendMode::kHue;
    } else if(compOpStr == "svg:saturation") {
        return SkBlendMode::kSaturation;
    } else if(compOpStr == "svg:plus") {
        return SkBlendMode::kPlus;
    } else if(compOpStr == "svg:dst-in") {
        return SkBlendMode::kDstIn;
    } else if(compOpStr == "svg:dst-out") {
        return SkBlendMode::kDstOut;
    } else if(compOpStr == "svg:src-atop") {
        return SkBlendMode::kSrcATop;
    } else if(compOpStr == "svg:dst-atop") {
        return SkBlendMode::kDstATop;
    } else return SkBlendMode::kSrcOver;
}

QString XmlExportHelpers::blendModeToString(const SkBlendMode blendMode) {
    switch(blendMode) {
    case SkBlendMode::kSrcOver: return "svg:src-over";
    case SkBlendMode::kMultiply: return "svg:multiply";
    case SkBlendMode::kScreen: return "svg:screen";
    case SkBlendMode::kOverlay: return "svg:overlay";
    case SkBlendMode::kDarken: return "svg:darken";
    case SkBlendMode::kLighten: return "svg:lighten";
    case SkBlendMode::kColorDodge: return "svg:color-dodge";
    case SkBlendMode::kColorBurn: return "svg:color-burn";
    case SkBlendMode::kHardLight: return "svg:hard-light";
    case SkBlendMode::kSoftLight: return "svg:soft-light";
    case SkBlendMode::kDifference: return "svg:difference";
    case SkBlendMode::kColor: return "svg:color";
    case SkBlendMode::kLuminosity: return "svg:luminosity";
    case SkBlendMode::kHue: return "svg:hue";
    case SkBlendMode::kSaturation: return "svg:saturation";
    case SkBlendMode::kPlus: return "svg:plus";
    case SkBlendMode::kDstIn: return "svg:dst-in";
    case SkBlendMode::kDstOut: return "svg:dst-out";
    case SkBlendMode::kSrcATop: return "svg:src-atop";
    case SkBlendMode::kDstATop: return "svg:dst-atop";
    default: return "svg:src-over";
    }
}

qreal XmlExportHelpers::stringToDouble(const QStringRef& string) {
    bool ok;
    const qreal value = string.toDouble(&ok);
    if(!ok) RuntimeThrow("Invalid value " + string.toString());
    return value;
}

qreal XmlExportHelpers::stringToDouble(const QString& string) {
    return stringToDouble(&string);
}

int XmlExportHelpers::stringToInt(const QStringRef& string) {
    bool ok;
    const int value = string.toInt(&ok);
    if(!ok) RuntimeThrow("Invalid value " + string.toString());
    return value;
}

int XmlExportHelpers::stringToInt(const QString& string) {
    return stringToInt(&string);
}

QDomElement XevExportHelpers::brushToElement(
        SimpleBrushWrapper* const brush, QDomDocument& doc) {
    auto ele = doc.createElement("Brush");
    ele.setAttribute("collection", brush ? brush->getCollectionName() : "");
    ele.setAttribute("name", brush ? brush->getBrushName() : "");
    return ele;
}

SimpleBrushWrapper* XevExportHelpers::brushFromElement(const QDomElement& ele) {
    const QString coll = ele.attribute("collection");
    const QString name = ele.attribute("name");
    return BrushCollectionData::sGetBrush(coll, name);
}

QMatrix XmlExportHelpers::stringToMatrix(const QString& str) {
    const auto vals = str.split(' ', Qt::SkipEmptyParts);
    if(vals.count() != 6) RuntimeThrow("Invalid matrix value '" + str + "'");
    const qreal m11 = stringToDouble(vals[0]);
    const qreal m12 = stringToDouble(vals[1]);
    const qreal m21 = stringToDouble(vals[2]);
    const qreal m22 = stringToDouble(vals[3]);
    const qreal dx = stringToDouble(vals[4]);
    const qreal dy = stringToDouble(vals[5]);
    return QMatrix(m11, m12, m21, m22, dx, dy);
}

QString XmlExportHelpers::matrixToString(const QMatrix& m) {
    return QString("%1 %2 %3 %4 %5 %6").arg(m.m11()).arg(m.m12()).
                                        arg(m.m21()).arg(m.m22()).
                                        arg(m.dx()).arg(m.dy());
}

QMatrix4x4 XmlExportHelpers::stringToMatrix4x4(const QString& str) {
    const auto vals = str.split(' ', Qt::SkipEmptyParts);
    if(vals.count() != 7) RuntimeThrow("Invalid matrix value '" + str + "'");
    const qreal m11 = stringToDouble(vals[0]);
    const qreal m12 = stringToDouble(vals[1]);
    const qreal m13 = stringToDouble(vals[1]);
    const qreal m14 = stringToDouble(vals[1]);
    const qreal m21 = stringToDouble(vals[2]);
    const qreal m22 = stringToDouble(vals[3]);
    const qreal m23 = stringToDouble(vals[1]);
    const qreal m24 = stringToDouble(vals[1]);
    const qreal m31 = stringToDouble(vals[1]);
    const qreal m32 = stringToDouble(vals[1]);
    const qreal m33 = stringToDouble(vals[1]);
    const qreal m34 = stringToDouble(vals[1]);
    const qreal m41 = stringToDouble(vals[1]);
    const qreal m42 = stringToDouble(vals[1]);
    const qreal m43 = stringToDouble(vals[1]);
    const qreal m44 = stringToDouble(vals[1]);
    return QMatrix4x4(m11, m12, m13, m14, m21, m22, m23, m24, m31, m32, m33, m34, m41, m42, m43, m44);
}
// TODO
/*QString XmlExportHelpers::matrix4x4ToString(const QMatrix4x4& m) {
    return QString("%1 %2 %3 %4 %5 %6 %7 %8 %9 %10 %11 %12 %13 %14").arg(m.m11()).arg(m.m12())
                                                                    .arg(m.m13()).arg(m.m14())
                                                                    .arg(m.m21()).arg(m.m22())
                                                                    .arg(m.m23()).arg(m.m24())
                                                                    .arg(m.m31()).arg(m.m32())
                                                                    .arg(m.m33()).arg(m.m34())
                                                                    .arg(m.m41()).arg(m.42())
                                                                    .arg(m.m43()).arg(m.m44());
                                                                    }*/

void XevExportHelpers::setAbsAndRelFileSrc(const QString& absSrc,
                                           QDomElement& ele,
                                           const XevExporter& exp) {
    ele.setAttribute("relSrc", exp.absPathToRelPath(absSrc));
    ele.setAttribute("absSrc", absSrc);
}

QString XevExportHelpers::getAbsAndRelFileSrc(const QDomElement& ele,
                                              const XevImporter& imp) {
    const auto relSrc = ele.attribute("relSrc");
    const auto absRelSrc = imp.relPathToAbsPath(relSrc);
    if(QFileInfo(absRelSrc).exists()) {
        return absRelSrc;
    } else {
        const auto absSrc = ele.attribute("absSrc");
        if(absSrc.isEmpty()) return absRelSrc;
        else return absSrc;
    }
}

bool XevExportHelpers::writeProperty(
        QDomElement& ele, const XevExporter& exp,
        const QString& name, Property* const prop) {
    const auto childEle = prop->prp_writeNamedPropertyXEV(name, exp);
    if(childEle.isNull()) return false;
    ele.appendChild(childEle);
    return true;
}

bool XevExportHelpers::readProperty(
        const QDomElement& ele, const XevImporter& imp,
        const QString& name, Property* const prop) {
    const auto childEle = ele.firstChildElement(name);
    if(childEle.isNull()) return false;
    prop->prp_readPropertyXEV(childEle, imp);
    return true;
}
