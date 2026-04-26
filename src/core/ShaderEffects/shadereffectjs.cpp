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

#include "shadereffectjs.h"

#include <QPointF>
#include <QColor>
#include <QRegularExpression>
#include <QtMath>

#include "exceptions.h"
#include "threadsafeqjsengine.h"

#define MARGIN_VAR_NAME "_eMargin"
#define MARGIN_GETTER_NAME "_eGet" MARGIN_VAR_NAME

QString glValueGetterName(const QString& glValueName)
{
    return "_eGet_" + glValueName;
}


void throwIfIsError(const QJSValue& value, const QString& name)
{
    if (value.isError()) {
        RuntimeThrow("Uncaught exception in " + name + " at line "
                     + value.property("lineNumber").toString() +
                     ":\n" + value.toString());
    }
}

ShaderEffectJS::ShaderEffectJS(const Blueprint& blueprint)
    : fMargin(blueprint.fMargin)
{
    const auto eClass = mEngine.evaluate(blueprint.fClassDef);
    throwIfIsError(eClass, "eClass");

    const auto eObj = mEngine.evaluate("var _eObj; _eObj = new _eClass()");
    throwIfIsError(eObj, "eObj");

    m_eSetSceneRect = mEngine.evaluate("_eObj._eSetSceneRect");
    throwIfIsError(m_eSetSceneRect, "m_eSetSceneRect");

    m_eSet = mEngine.evaluate("_eObj._eSet");
    throwIfIsError(m_eSet, "m_eSet");

    m_eEvaluate = mEngine.evaluate("_eObj._eEvaluate");
    throwIfIsError(m_eEvaluate, "m_eEvaluate");

    for (const auto& glVal : blueprint.fGlValues) {
        const auto getterName = glValueGetterName(glVal);
        auto getter = mEngine.evaluate("_eObj." + getterName);
        mGlValueGetters.append(getter);
    }

    if (fMargin) {
        mMarginGetter = mEngine.evaluate("_eObj." MARGIN_GETTER_NAME);
    }
}

void ShaderEffectJS::setValues(const QJSValueList& args)
{
    ThreadSafeQJSEngine::call(&mEngine, [&]{ m_eSet.call(args); });
}

void ShaderEffectJS::updateValues()
{
    ThreadSafeQJSEngine::call(&mEngine, [&]{ m_eSet.call(mSetters); });
}

void ShaderEffectJS::clearSetters()
{
    ThreadSafeQJSEngine::call(&mEngine, [&]{ mSetters.clear(); });
}

void ShaderEffectJS::addSetter(const QPointF &val)
{
    ThreadSafeQJSEngine::call(&mEngine, [&] { mSetters << toValue(val); });
}

void ShaderEffectJS::addSetter(const QColor &val)
{
    ThreadSafeQJSEngine::call(&mEngine, [&] { mSetters << toValue(val); });
}

void ShaderEffectJS::addSetter(const qreal &val)
{
    ThreadSafeQJSEngine::call(&mEngine, [&] { mSetters << val; });
}

void ShaderEffectJS::evaluate()
{
    ThreadSafeQJSEngine::call(&mEngine, [&]{ m_eEvaluate.call(); });
}

int ShaderEffectJS::glValueCount() const
{
    return mGlValueGetters.count();
}

QJSValue ShaderEffectJS::getGlValue(const int index)
{
    return ThreadSafeQJSEngine::call(&mEngine, [&]{ return mGlValueGetters[index].call(); });
}

QJSValue& ShaderEffectJS::getGlValueGetter(const int index)
{
    return mGlValueGetters[index];
}

double ShaderEffectJS::getGlValueDouble(const int index)
{
    const QJSValue val = getGlValue(index);
    if (val.isNumber()) {
        return val.toNumber();
    } else { RuntimeThrow("Invalid value. Expected double."); }
}

ShaderEffectJS::DV2 ShaderEffectJS::getGlValueDouble2(const int index)
{
    DV2 vec;
    const QJSValue val = getGlValue(index);
    if (val.isArray()) {
        const int len = val.property("length").toInt();
        if (len != 2) { RuntimeThrow("Invalid value. Expected vec2."); }
        vec.v0 = val.property(0).toNumber();
        vec.v1 = val.property(1).toNumber();
        return vec;
    } else { RuntimeThrow("Invalid value. Expected vec2."); }
}

ShaderEffectJS::DV3 ShaderEffectJS::getGlValueDouble3(const int index)
{
    DV3 vec;
    const QJSValue val = getGlValue(index);
    if (val.isArray()) {
        const int len = val.property("length").toInt();
        if (len != 3) { RuntimeThrow("Invalid value. Expected vec3."); }
        vec.v0 = val.property(0).toNumber();
        vec.v1 = val.property(1).toNumber();
        vec.v2 = val.property(2).toNumber();
        return vec;
    } else { RuntimeThrow("Invalid value. Expected vec3."); }
}

ShaderEffectJS::DV4 ShaderEffectJS::getGlValueDouble4(const int index)
{
    DV4 vec;
    const QJSValue val = getGlValue(index);
    if (val.isArray()) {
        const int len = val.property("length").toInt();
        if (len != 4) { RuntimeThrow("Invalid value. Expected vec4."); }
        vec.v0 = val.property(0).toNumber();
        vec.v1 = val.property(1).toNumber();
        vec.v2 = val.property(2).toNumber();
        vec.v3 = val.property(3).toNumber();
        return vec;
    } else { RuntimeThrow("Invalid value. Expected vec4."); }
}

QJSValue ShaderEffectJS::getMarginValue()
{
    if (!fMargin) { return 0.; }
    return ThreadSafeQJSEngine::call(&mEngine, [&]{ return mMarginGetter.call(); });
}

const QMargins ShaderEffectJS::getMargins()
{
    const auto jsVal = getMarginValue();
    if (jsVal.isNumber()) {
        return QMargins() + qCeil(jsVal.toNumber());
    } else if (jsVal.isArray()) {
        const int len = jsVal.property("length").toInt();
        if (len == 2) {
            const int valX = qCeil(jsVal.property(0).toNumber());
            const int valY = qCeil(jsVal.property(1).toNumber());
            return QMargins(valX, valY, valX, valY);
        } else if (len == 4) {
            const int valLeft = qCeil(jsVal.property(0).toNumber());
            const int valTop = qCeil(jsVal.property(1).toNumber());
            const int valRight = qCeil(jsVal.property(2).toNumber());
            const int valBottom = qCeil(jsVal.property(3).toNumber());
            return QMargins(valLeft, valTop, valRight, valBottom);
        } else {
            RuntimeThrow("Invalid Margin script");
        }
    } else { RuntimeThrow("Invalid Margin script result type"); }
    return QMargins();
}

void ShaderEffectJS::setSceneRect(const SkIRect& rect)
{
    const int x = rect.x();
    const int y = rect.y();
    const int w = rect.width();
    const int h = rect.height();

    ThreadSafeQJSEngine::call(&mEngine, [&] {
        QJSValueList args;
        args << x << y << w << h;
        m_eSetSceneRect.call(args);
    });
}

QJSValue ShaderEffectJS::toValue(const QPointF& val)
{
    QJSValue arr = mEngine.newArray(2);
    arr.setProperty(0, val.x());
    arr.setProperty(1, val.y());

    return arr;
}

QJSValue ShaderEffectJS::toValue(const QColor& val)
{
    QJSValue arr = mEngine.newArray(4);
    arr.setProperty(0, val.redF());
    arr.setProperty(1, val.greenF());
    arr.setProperty(2, val.blueF());
    arr.setProperty(3, val.alphaF());

    return arr;
}

QStringList extractExternFromScript(QString& calc)
{
    QStringList result;
    QRegularExpression rx("\\bextern\\s+([a-zA-Z_][a-zA-Z_0-9]*)");
    auto matchIterator = rx.globalMatch(calc);
    while (matchIterator.hasNext()) {
        const auto match = matchIterator.next();
        result << match.captured(1);
    }
    calc.remove(QRegExp("\\bextern\\s+"));
    return result;
}

// function _eClass {
//     -Definitions
//     -Extern Variables
//     -Property Variables
//     -_eSet Property Values Function
//     -glValue Variables
//     -_eMargin Variable
//     -_eEvaluate Function {
//          -Calculate Script
//          -set glValue Variables
//          -set _eMargin Variable
//          -return _eCalculate result
//      }
//     -glValue Getter Functions
//     -_eMargin Getter Functions
// }
std::shared_ptr<ShaderEffectJS::Blueprint>
ShaderEffectJS::Blueprint::sCreate(const QString& defs, QString calc,
                                   const QStringList& properties,
                                   const QList<GlValueBlueprint>& glValueBPs,
                                   const QString& marginScript)
{
    const QStringList externVars = extractExternFromScript(calc);
    QString externDefs;
    for (const auto& ext : externVars) {
        externDefs += "var " + ext + ";\n";
    }
    const QString _eSetArgs = properties.isEmpty() ? "" : ("_" + properties.join(",_"));

    QString propDefs;
    QString _eSet = "\nthis._eSet = function(" + _eSetArgs + ") {\n";
        for (const auto& prop : properties) {
            propDefs += "var " + prop + ";\n";
            _eSet += "" + prop + "=_" + prop + ";\n";
        }
    _eSet += "};\n";

    QStringList glValues;
    QString getters;
    QString glValueDefs;
    const QString _eEval_Args = properties.join(",");
    QString _eEvaluate = "\nthis._eEvaluate = function() {\n" + calc;
        for (const auto& glVal : glValueBPs) {
            glValues << glVal.fName;
            glValueDefs += "var " + glVal.fName + ";\n";
            _eEvaluate += glVal.fName + "=" + glVal.fScript + ";\n";
            const QString getterName = glValueGetterName(glVal.fName);
            getters += "\nthis." + getterName + " = function() {\n"
                           "return " + glVal.fName + ";\n"
                       "};\n";
        }
        const bool hasMargin = !marginScript.isEmpty();
        if (hasMargin) {
            glValueDefs += "var " MARGIN_VAR_NAME ";\n";
            _eEvaluate += MARGIN_VAR_NAME "=" + marginScript + ";\n";
            getters += "\nthis." MARGIN_GETTER_NAME " = function() {\n"
                           "return " MARGIN_VAR_NAME ";\n"
                       "};\n";
        }
    _eEvaluate += "};\n";

    const QString _eSetSceneRect =
            "\nthis._eSetSceneRect = function(_x, _y, _w, _h) {\n"
                "_eRect = [_x, _y, _w, _h];\n"
            "};\n";

    const QString classContent = defs + externDefs + propDefs + "\nvar _eRect;\n" +
                                 _eSet + glValueDefs + _eEvaluate + _eSetSceneRect + getters;
    const QString classDef = "function _eClass() {\n" + classContent + "\n}";

    return std::shared_ptr<Blueprint>(new Blueprint{classDef, glValues, hasMargin});
}
