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

#include "shadereffectcreator.h"
#include "exceptions.h"
#include "shadereffect.h"
#include "shadervaluehandler.h"
#include "regexhelpers.h"
#include "uniformspecifiercreator.h"
#include "PropertyCreators/qrealanimatorcreator.h"
#include "PropertyCreators/qpointfanimatorcreator.h"
#include "PropertyCreators/qvector3danimatorcreator.h"

#include <QDomDocument>

QList<stdsptr<ShaderEffectCreator>> ShaderEffectCreator::sEffectCreators;

bool ShaderEffectCreator::compatible(const QList<ShaderPropertyType> &props) const {
    if(props.count() != fProperties.count()) return false;
    for(int i = 0; i < props.count(); i++) {
        const auto& iType = props.at(i);
        const auto prop = fProperties.at(i).get();
        switch(iType) {
        case ShaderPropertyType::floatProperty: {
            const bool iCompatible = enve_cast<QrealAnimatorCreator*>(prop);
            if(!iCompatible) return false;
        } break;
        case ShaderPropertyType::intProperty: {
            const bool iCompatible = enve_cast<IntAnimatorCreator*>(prop);
            if(!iCompatible) return false;
        } break;
        case ShaderPropertyType::vec2Property: {
            const bool iCompatible = enve_cast<QPointFAnimatorCreator*>(prop);
            if(!iCompatible) return false;
        } break;
        case ShaderPropertyType::vec3Property: {
            const bool iCompatible = enve_cast<QVector3DAnimatorCreator*>(prop);
            if(!iCompatible) return false;
        } break;
        case ShaderPropertyType::colorProperty: {
            const bool iCompatible = enve_cast<ColorAnimatorCreator*>(prop);
            if(!iCompatible) return false;
        } break;
        default: return false;
        };
    }
    return true;
}

void ShaderEffectCreator::reloadProgram(QGL33 * const gl, const QString &fragPath) {
    try {
        fProgram->reloadFragmentShader(gl, fragPath);
    } catch(...) {
        RuntimeThrow("Failed to load a new version of '" + fragPath + "'");
    }
}

qsptr<ShaderEffect> ShaderEffectCreator::create() const {
    auto shaderEffect = enve::make_shared<ShaderEffect>(
                fName, this, &*fProgram, fProperties);
    return shaderEffect;
}

ShaderPropertyType creatorPropertyType(ShaderPropertyCreator* const prop) {
    if(enve_cast<QrealAnimatorCreator*>(prop)) {
        return ShaderPropertyType::floatProperty;
    } else if(enve_cast<IntAnimatorCreator*>(prop)) {
        return ShaderPropertyType::intProperty;
    } else if(enve_cast<QPointFAnimatorCreator*>(prop)) {
        return ShaderPropertyType::vec2Property;
    } else if(enve_cast<ColorAnimatorCreator*>(prop)) {
        return ShaderPropertyType::colorProperty;
    } else RuntimeThrow("Unsupported type");
}

void ShaderEffectCreator::writeIdentifier(eWriteStream &dst) const {
    dst << fName;
    dst << fGrePath;
    const int nChildren = fProperties.count();
    dst << nChildren;
    for(const auto& anim : fProperties) {
        const ShaderPropertyType type = creatorPropertyType(anim.get());
        dst.write(&type, sizeof(ShaderPropertyType));
    }
}

void ShaderEffectCreator::writeIdentifierXEV(QDomElement& ele) const {
    ele.setAttribute("name", fName);
    ele.setAttribute("grePath", fGrePath);
    QString props;
    for(const auto& anim : fProperties) {
        const ShaderPropertyType type = creatorPropertyType(anim.get());
        if(!props.isEmpty()) props += ',';
        props += QString::number(int(type));
    }
    ele.setAttribute("properties", props);
}

ShaderEffectCreator::Identifier ShaderEffectCreator::sReadIdentifier(eReadStream &src) {
    QString name; src >> name;
    QString grePath; src >> grePath;
    QList<ShaderPropertyType> props;
    int nChildren; src >> nChildren;
    for(int i = 0; i < nChildren; i++) {
        ShaderPropertyType type;
        src.read(&type, sizeof(ShaderPropertyType));
        props << type;
    }
    return Identifier(grePath, name, props);
}

ShaderEffectCreator::Identifier ShaderEffectCreator::sReadIdentifierXEV(
        const QDomElement& ele) {
    const QString name = ele.attribute("name");
    const QString grePath = ele.attribute("grePath");
    const QString propsStr = ele.attribute("properties");
    const auto propsStrList = propsStr.splitRef(',');
    QList<ShaderPropertyType> props;
    for(const auto &propStr : propsStrList) {
        const int propInt = XmlExportHelpers::stringToInt(propStr);
        const ShaderPropertyType prop = static_cast<ShaderPropertyType>(propInt);
        props << prop;
    }

    return Identifier(grePath, name, props);
}

qreal stringToDouble(const QString &str) {
    if(str.isEmpty()) RuntimeThrow("Can not convert an empty string to double");
    bool ok;
    qreal val = str.toDouble(&ok);
    if(!ok) RuntimeThrow("Can not convert \"" + str + "\" to double");
    return val;
}

int stringToInt(const QString &str) {
    if(str.isEmpty()) RuntimeThrow("Can not convert an empty string to int");
    bool ok;
    int val = str.toInt(&ok);
    if(!ok) RuntimeThrow("Can not convert \"" + str + "\" to int");
    return val;
}

int stringToBool(const QString &str) {
    if(str.isEmpty()) RuntimeThrow("Can not convert an empty string to bool");
    const auto simpl = str.simplified();
    bool result;
    if(simpl == "true") result = true;
    else if(simpl == "false") result = false;
    else RuntimeThrow("Can not convert \"" + str + "\" to bool");
    return result;
}

qreal attrToDouble(const QDomElement &elem,
                   const QString& elemName,
                   const QString& attr,
                   const QString& def) {
    const QString valS = elem.attribute(attr, def);
    try {
        const qreal val = stringToDouble(valS);
        return val;
    } catch(...) {
        RuntimeThrow("Invalid '" + attr + "\" value \"" + valS + "' for '" +
                     elemName + "'.\nExpected number");
    }
}

int attrToInt(const QDomElement &elem,
              const QString& elemName,
              const QString& attr,
              const QString& def) {
    const QString valS = elem.attribute(attr, def);
    try {
        const int val = stringToInt(valS);
        return val;
    } catch(...) {
        RuntimeThrow("Invalid '" + attr + "' value \"" + valS + "\" for '" +
                     elemName + "'.\nExpected integer");
    }
}

bool attrToBool(const QDomElement &elem,
                const QString& elemName,
                const QString& attr,
                const QString& def) {
    const QString valS = elem.attribute(attr, def);
    try {
        const bool val = stringToBool(valS);
        return val;
    } catch(...) {
        RuntimeThrow("Invalid '" + attr + "' value \"" + valS + "\" for '" +
                     elemName + "'.\nExpected \"true\" or \"false\"");
    }
}

QVector3D attrToQVector3D(const QDomElement &elem,
                          const QString& elemName,
                          const QString& attr,
                          const QString& def,
                          const bool allowSingleValue) {
    const QString valS = elem.attribute(attr, def);
    const QRegExp rx("\\[" REGEX_THREE_FLOATS "\\]");
    if(rx.exactMatch(valS)) {
        rx.indexIn(valS);
        const QStringList xyz = rx.capturedTexts();
        return {xyz.at(1).toFloat(), xyz.at(2).toFloat(), xyz.at(3).toFloat()}; // FIXME: we're losing some precision here
    } else if(allowSingleValue) {
        bool ok;
        auto value = valS.toFloat(&ok);
        if(!ok) RuntimeThrow("Invalid '" + attr + "' value \"" + valS + "\" for '" +
                             elemName + "'.\nExpected \"[x, y]\" or \"x\".");
        return {value, value, value};
    } else RuntimeThrow("Invalid '" + attr + "' value \"" + valS + "\" for '" +
                         elemName + "'.\nExpected \"[x, y]\".");
}

QPointF attrToQPointF(const QDomElement &elem,
                      const QString& elemName,
                      const QString& attr,
                      const QString& def,
                      const bool allowSingleValue) {
    auto vector = attrToQVector3D(elem, elemName, attr, def, allowSingleValue);
    return QPointF(vector.x(), vector.y());
}

QColor attrToQColor(const QDomElement &elem,
                    const QString& elemName,
                    const QString& attr,
                    const QString& def) {
    const QString valS = elem.attribute(attr, def);
    const QRegExp rxRGBA("\\[" REGEX_FOUR_FLOATS "\\]");
    if(rxRGBA.exactMatch(valS)) {
        rxRGBA.indexIn(valS);
        const QStringList rgba = rxRGBA.capturedTexts();
        return QColor::fromRgbF(rgba.at(1).toDouble(),
                                rgba.at(2).toDouble(),
                                rgba.at(3).toDouble(),
                                rgba.at(4).toDouble());
    } else {
        const QRegExp rxRGB("\\[" REGEX_THREE_FLOATS "\\]");
        if(rxRGB.exactMatch(valS)) {
            rxRGB.indexIn(valS);
            const QStringList rgb = rxRGB.capturedTexts();
            return QColor::fromRgbF(rgb.at(1).toDouble(),
                                    rgb.at(2).toDouble(),
                                    rgb.at(3).toDouble());
        } else {
            RuntimeThrow("Invalid '" + attr + "' value \"" + valS + "\" for '" +
                         elemName + "'.\nExpected \"[r, g, b, a]\".");
        }
    }
}

void parseFloatPropertyCreators(const QString& name,
                                const QString& nameUI,
                                const QDomElement &elem,
                                stdsptr<ShaderPropertyCreator>& propC,
                                stdsptr<UniformSpecifierCreator>& uniC) {
    const qreal minVal = attrToDouble(elem, name, "min", "0");
    const qreal maxVal = attrToDouble(elem, name, "max", "100");
    const qreal iniVal = attrToDouble(elem, name, "ini", "0");
    const qreal stepVal = qMax(0.1, attrToDouble(elem, name, "step", "1"));
    const bool glValue = attrToBool(elem, name, "glValue", "false");
    const bool resolutionScaled = attrToBool(elem, name, "resolutionScaled", "false");
    const bool influenceScaled = attrToBool(elem, name, "influenceScaled", "false");

    propC = enve::make_shared<QrealAnimatorCreator>(
                iniVal, minVal, maxVal, stepVal,
                glValue, name, nameUI);
    uniC = enve::make_shared<UniformSpecifierCreator>(
                ShaderPropertyType::floatProperty, glValue,
                resolutionScaled, influenceScaled);
}

void parseIntPropertyCreators(const QString& name,
                              const QString& nameUI,
                              const QDomElement &elem,
                              stdsptr<ShaderPropertyCreator>& propC,
                              stdsptr<UniformSpecifierCreator>& uniC) {
    const int minVal = attrToInt(elem, name, "min", "0");
    const int maxVal = attrToInt(elem, name, "max", "100");
    const int iniVal = attrToInt(elem, name, "ini", "0");
    const int stepVal = attrToInt(elem, name, "step", "1");
    const bool glValue = attrToBool(elem, name, "glValue", "false");
    const bool resolutionScaled = attrToBool(elem, name, "resolutionScaled", "false");
    const bool influenceScaled = attrToBool(elem, name, "influenceScaled", "false");

    propC = enve::make_shared<IntAnimatorCreator>(
                iniVal, minVal, maxVal, stepVal,
                glValue, name, nameUI);
    uniC = enve::make_shared<UniformSpecifierCreator>(
                ShaderPropertyType::intProperty, glValue,
                resolutionScaled, influenceScaled);
}

void parseVec2PropertyCreators(const QString& name,
                               const QString& nameUI,
                               const QDomElement &elem,
                               stdsptr<ShaderPropertyCreator>& propC,
                               stdsptr<UniformSpecifierCreator>& uniC) {
    const QString nameX = elem.attribute("xnameUI", "x");
    const QString nameY = elem.attribute("ynameUI", "y");
    const auto minVal = attrToQPointF(elem, name, "min", "0", true);
    const auto maxVal = attrToQPointF(elem, name, "max", "100", true);
    const auto iniVal = attrToQPointF(elem, name, "ini", "0", true);
    const auto stepVal = attrToQPointF(elem, name, "step", "1", true);
    const bool glValue = attrToBool(elem, name, "glValue", "false");
    const bool resolutionScaled = attrToBool(elem, name, "resolutionScaled", "false");
    const bool influenceScaled = attrToBool(elem, name, "influenceScaled", "false");

    propC = enve::make_shared<QPointFAnimatorCreator>(
                iniVal, minVal, maxVal, stepVal, glValue,
                nameX, nameY, name, nameUI);
    uniC = enve::make_shared<UniformSpecifierCreator>(
                ShaderPropertyType::vec2Property, glValue,
                resolutionScaled, influenceScaled);
}

void parseColorPropertyCreators(const QString& name,
                                const QString& nameUI,
                                const QDomElement &elem,
                                stdsptr<ShaderPropertyCreator>& propC,
                                stdsptr<UniformSpecifierCreator>& uniC) {
    const QColor iniVal = attrToQColor(elem, name, "ini", "[0, 0, 0]");
    const bool glValue = attrToBool(elem, name, "glValue", "false");

    propC = enve::make_shared<ColorAnimatorCreator>(
                iniVal, glValue, name, nameUI);
    uniC = enve::make_shared<UniformSpecifierCreator>(
                ShaderPropertyType::colorProperty, glValue,
                false, false);
}

void parsePropertyCreators(const QString& type,
                           const QString& name,
                           const QString& nameUI,
                           const QDomElement &elem,
                           stdsptr<ShaderPropertyCreator>& propC,
                           stdsptr<UniformSpecifierCreator>& uniC) {
    if(type == "float") {
        parseFloatPropertyCreators(name, nameUI, elem, propC, uniC);
    } else if(type == "int") {
        parseIntPropertyCreators(name, nameUI, elem, propC, uniC);
    } else if(type == "vec2") {
        parseVec2PropertyCreators(name, nameUI, elem, propC, uniC);
    } else if(type == "color") {
        parseColorPropertyCreators(name, nameUI, elem, propC, uniC);
    } else {
        RuntimeThrow("Invalid Property type '" + type + "' for " + name + ".");
    }
}

void readPropertyCreators(const QDomElement &elem,
                         stdsptr<ShaderPropertyCreator>& propC,
                         stdsptr<UniformSpecifierCreator>& uniC) {
    const QString name = elem.attribute("name");
    if(name.isEmpty()) RuntimeThrow("Property name not defined.");
    const QRegExp rx("[A-Za-z_][A-Za-z0-9_]*");
    if(!rx.exactMatch(name)) RuntimeThrow("Invalid Property name '" + name + "'.");

    const QString type = elem.attribute("type");
    if(type.isEmpty()) RuntimeThrow("Property type not defined for " + name + ".");

    const QString nameUI = elem.attribute("nameUI", name);

    try {
        parsePropertyCreators(type, name, nameUI, elem, propC, uniC);
    } catch(...) {
        RuntimeThrow("Error while parsing Property '" + name +
                     "' of type " + type + ".");
    }
}

QString getScript(const QString& name, const QDomElement& ele) {
    const QString value = ele.attribute("value");
    const QString text = ele.text();
    if(value.isEmpty() && text.isEmpty()) {
        RuntimeThrow("Value '" + name + "' value not defined.");
    }
    if(!value.isEmpty() && !text.isEmpty()) {
        RuntimeThrow("Two '" + name + "' values defined.");
    }
    return value.isEmpty() ? text : value;
}

void getGlValues(const QDomElement& glValuesNode,
                 QList<stdsptr<ShaderValueHandler>>& glValues) {
    const QDomNodeList valueNodes = glValuesNode.elementsByTagName("glValue");
    for(int i = 0; i < valueNodes.count(); i++) {
        const QDomNode& valNode = valueNodes.at(i);
        if(!valNode.isElement()) {
            RuntimeThrow("glValue node " + QString::number(i) +
                         " is not an Element.");
        }
        const QDomElement valEle = valNode.toElement();
        try {
            const QString name = valEle.attribute("name");
            if(name.isEmpty()) RuntimeThrow("glValue name not defined.");
            const auto typeStr = valEle.attribute("type");
            if(typeStr.isEmpty()) RuntimeThrow("glValue '" + name + "' type not defined.");
            GLValueType type{GLValueType::Float};
            if(typeStr == "float") type = GLValueType::Float;
            else if(typeStr == "vec2") type = GLValueType::Vec2;
            else if(typeStr == "vec3") type = GLValueType::Vec3;
            else if(typeStr == "vec4") type = GLValueType::Vec4;
            else if(typeStr == "int") type = GLValueType::Int;
            else if(typeStr == "ivec2") type = GLValueType::iVec2;
            else if(typeStr == "ivec3") type = GLValueType::iVec3;
            else if(typeStr == "ivec4") type = GLValueType::iVec4;

            const QString script = getScript(name, valEle);
            glValues << enve::make_shared<ShaderValueHandler>(name, type, script);
        } catch(...) {
            RuntimeThrow("Value " + QString::number(i) + " is invalid.");
        }
    }
}

void getProperties(const QDomElement& propertiesNode,
                   QList<stdsptr<ShaderPropertyCreator>>& propCs,
                   UniformSpecifierCreators& uniCs) {
    const auto propertyNodes = propertiesNode.elementsByTagName("Property");
    for(int i = 0; i < propertyNodes.count(); i++) {
        const QDomNode& animNode = propertyNodes.at(i);
        if(!animNode.isElement()) {
            RuntimeThrow("Property node " + QString::number(i) +
                         " is not an Element.");
        }
        QDomElement animEle = animNode.toElement();
        try {
            stdsptr<ShaderPropertyCreator> propC;
            stdsptr<UniformSpecifierCreator> uniC;
            readPropertyCreators(animEle, propC, uniC);
            propCs << propC;
            uniCs << uniC;
        } catch(...) {
            RuntimeThrow("Property " + QString::number(i) + " is invalid.");
        }
    }
}

bool extractOptionalFirstOnlyElement(const QDomElement& from,
                                     const QString& tagName,
                                     QDomElement& ele) {
    QList<stdsptr<ShaderValueHandler>> glValues;
    const auto glValuesNodes = from.elementsByTagName(tagName);
    if(glValuesNodes.count() == 1) {
        const QDomNode& node = glValuesNodes.at(0);
        if(!node.isElement()) RuntimeThrow(tagName + " node is not an Element.");
        ele = node.toElement();
        return true;
    } else if(glValuesNodes.count() > 1)
        RuntimeThrow("Multiple '" + tagName + "' definitions");
    return false;
}

stdsptr<ShaderEffectJS::Blueprint> getJSBlueprint(
        const QDomElement& root,
        const QList<stdsptr<ShaderPropertyCreator>>& propCs,
        const QList<stdsptr<ShaderValueHandler>>& glValues) {
    QDomElement marEle;
    const bool mar = extractOptionalFirstOnlyElement(root, "Margin", marEle);
    const QString marginScript = mar ? getScript("Margin", marEle) : "";

    QDomElement scriptEle;
    const bool script = extractOptionalFirstOnlyElement(root, "Script", scriptEle);
    QString defsStr;
    QString calcStr;
    if(script) {
        QDomElement defsEle;
        const bool defs = extractOptionalFirstOnlyElement(
                              scriptEle, "Definitions", defsEle);
        if(defs) defsStr = defsEle.text();

        QDomElement calcEle;
        const bool calc = extractOptionalFirstOnlyElement(
                              scriptEle, "Calculate", calcEle);
        if(calc) calcStr = calcEle.text();
    }
    QStringList props;
    for(const auto& prop : propCs) props << prop->fName;

    QList<ShaderEffectJS::GlValueBlueprint> glValueBPs;
    for(const auto& glVal : glValues) {
        glValueBPs.append({glVal->fName, glVal->fScript});
    }

    return ShaderEffectJS::Blueprint::sCreate(defsStr, calcStr, props,
                                              glValueBPs, marginScript);
}

stdsptr<ShaderEffectCreator> ShaderEffectCreator::sLoadFromFile(
        QGL33 * const gl, const QString &grePath) {
    QFile greFile(grePath);
    if(!greFile.exists())
        RuntimeThrow("ShaderEffect source file does not exist.");
    const QFileInfo info(greFile);
    const QString fragPath = info.path() + "/" + info.completeBaseName() + ".frag";
    const QFile fragFile(fragPath);
    if(!fragFile.exists())
        RuntimeThrow("ShaderEffect shader file (" +
                     fragPath + ") does not exist.");
    if(!greFile.open(QIODevice::ReadOnly))
        RuntimeThrow("ShaderEffect source file could not be opened.");
    QDomDocument document;
    QString errMsg;
    if(!document.setContent(&greFile, &errMsg))
        RuntimeThrow("Error while parsing ShaderEffect source:\n" + errMsg);
    greFile.close();

    QDomElement root = document.firstChildElement();
    if(root.tagName() != "ShaderEffect")
        RuntimeThrow("Unrecognized root " + root.tagName() +
                     " in ShaderEffect source.");
    const QString effectName = root.attribute("name");
    const QString menuPath = root.attribute("menuPath");

    QList<stdsptr<ShaderPropertyCreator>> propCs;
    UniformSpecifierCreators uniCs;
    QDomElement propsNode;
    const bool props = extractOptionalFirstOnlyElement(root, "Properties", propsNode);
    if(props) getProperties(propsNode, propCs, uniCs);

    QList<stdsptr<ShaderValueHandler>> glValues;
    QDomElement glValuesNode;
    const bool glVals = extractOptionalFirstOnlyElement(root, "glValues", glValuesNode);
    if(glVals) getGlValues(glValuesNode, glValues);

    const auto blueprint = getJSBlueprint(root, propCs, glValues);

    std::unique_ptr<ShaderEffectProgram> program;
    try {
        program = ShaderEffectProgram::sCreateProgram(
                    gl, fragPath, propCs, blueprint, uniCs, glValues);
    } catch(...) {
        RuntimeThrow("Could not create a program for ShaderEffect '" +
                     effectName + "'");
    }
    const auto shaderEffectCreator = enve::make_shared<ShaderEffectCreator>(
                grePath, effectName, propCs, menuPath, std::move(program));
    sEffectCreators << shaderEffectCreator;

    return shaderEffectCreator;
}

stdsptr<ShaderEffectCreator>
    ShaderEffectCreator::sWithGrePath(const QString &grePath) {
    for(const auto& effectC : sEffectCreators) {
        if(effectC->fGrePath == grePath) return effectC;
    }
    return nullptr;
}

stdsptr<ShaderEffectCreator>
    ShaderEffectCreator::sWithGrePathAndCompatible(
        const QString &grePath, const QList<ShaderPropertyType> &props) {
    const auto pathBased = sWithGrePath(grePath);
    if(!pathBased) return nullptr;
    if(pathBased->compatible(props))
        return pathBased;
    return nullptr;
}

QList<stdsptr<ShaderEffectCreator>>
    ShaderEffectCreator::sWithName(const QString &name) {
    QList<stdsptr<ShaderEffectCreator>> named;
    for(const auto& effectC : sEffectCreators) {
        if(effectC->fName == name) named << effectC;
    }
    return named;
}

QList<stdsptr<ShaderEffectCreator>>
    ShaderEffectCreator::sWithNameAndCompatible(
        const QString &name, const QList<ShaderPropertyType> &props) {
    QList<stdsptr<ShaderEffectCreator>> comp;
    for(const auto& effectC : sEffectCreators) {
        if(effectC->fName != name) continue;
        if(effectC->compatible(props)) comp << effectC;
    }
    return comp;
}

QList<stdsptr<ShaderEffectCreator>>
    ShaderEffectCreator::sWithCompatibleProps(
        const QList<ShaderPropertyType> &props) {
    QList<stdsptr<ShaderEffectCreator>> comp;
    for(const auto& effectC : sEffectCreators) {
        if(effectC->compatible(props)) comp << effectC;
    }
    return comp;
}

QList<stdsptr<ShaderEffectCreator>>
    ShaderEffectCreator::sGetBestCompatibleEffects(const Identifier &id) {
    const auto pathBased = sWithGrePathAndCompatible(id.fGrePath, id.fTypes);
    if(pathBased) return QList<stdsptr<ShaderEffectCreator>>() << pathBased;
    const auto nameBased = sWithNameAndCompatible(id.fName, id.fTypes);
    const auto compBased = sWithCompatibleProps(id.fTypes);
    auto result = nameBased;
    for(const auto& comp : compBased) {
        if(result.contains(comp)) continue;
        result.append(comp);
    }
    return result;
}
