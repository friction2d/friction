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

#include "propertybindingparser.h"

#include "exceptions.h"
#include "framebinding.h"
#include "valuebinding.h"
#include "scenebinding.h"
#include "appsupport.h"

void skipSpaces(const QString& exp, int& position) {
    while(position < exp.count() && exp.at(position) == ' ') {
        position++;
    }
}

bool parseBindingAssignment(const QString& exp, int& pos) {
    if(pos < exp.count()) {
        const auto& character = exp.at(pos);
        if(character == '=') {
            pos++;
            return true;
        }
    }
    return false;
}

bool parseBindingName(const QString& exp, int& pos, QString& name) {
    int newPos = pos;
    name.clear();
    while(newPos < exp.count()) {
        const auto& character = exp.at(newPos);
        if(!character.isLetter() && !character.isDigit()
           && character != '_') break;
        newPos++;
        name.append(character);
    }
    if(name.isEmpty()) return false;
    pos = newPos;
    return true;
}

bool checkComment(const QString& exp, int& pos) {
    if(pos + 1 < exp.count()) {
        int newPos = pos;
        if(exp.at(newPos++) == '/' && exp.at(newPos++) == '/') {
            pos = newPos;
            return true;
        }
    }
    return false;
}

QString parse(const QString& exp, int& pos, const int n) {
    QString result;
    for(int i = 0; i < n && pos < exp.count(); i++) {
        result.append(exp.at(pos++));
    }
    return result;
}

bool parse(const QString& exp, int& pos, const QString& test) {
    int newPos = pos;
    const auto value = parse(exp, newPos, test.count());
    if(value == test) {
        pos = newPos;
        return true;
    } else return false;
}

int remaining(const QString& exp, int& pos) {
    return exp.count() - pos;
}

bool parseValue(const QString& exp, int& pos) {
    return parse(exp, pos, "$value");
}

bool parseFrame(const QString& exp, int& pos) {
    return parse(exp, pos, "$frame");
}

bool parseSceneFPS(const QString& exp, int& pos) {
    return parse(exp, pos, "$scene.fps");
}

bool parseSceneWidth(const QString& exp, int& pos) {
    return parse(exp, pos, "$scene.width");
}

bool parseSceneHeight(const QString& exp, int& pos) {
    return parse(exp, pos, "$scene.height");
}

bool parseSceneRangeMax(const QString& exp, int& pos) {
    return parse(exp, pos, "$scene.rangeMax");
}

bool parseSceneRangeMin(const QString& exp, int& pos) {
    return parse(exp, pos, "$scene.rangeMin");
}

void parseBinding(const QString& exp, int& pos, QString& binding) {
    while(pos < exp.count()) {
        const auto& c = exp.at(pos++);
        if(!c.isLetterOrNumber() && c != ' ' && c != '.' && c != '_') break;
        binding.append(c);
    }
}

qsptr<PropertyBindingBase> PropertyBindingParser::parseBinding(
        QString& name,
        const QString& exp,
        const PropertyBinding::Validator& validator,
        const Property* const context) {
    qsptr<PropertyBindingBase> result;
    int pos = 0;
    skipSpaces(exp, pos);
    if(checkComment(exp, pos)) return nullptr;
    if(!parseBindingName(exp, pos, name))
        PrettyRuntimeThrow("Invalid binding definition:\n'" + exp + "'");
    skipSpaces(exp, pos);
    if(!parseBindingAssignment(exp, pos))
        PrettyRuntimeThrow("Invalid binding definition:\n'" + exp + "'");
    skipSpaces(exp, pos);
    if(parseFrame(exp, pos)) {
        result = FrameBinding::sCreate(context);
    } else if(parseSceneFPS(exp, pos)) {
        result = SceneBinding::sCreate(context,
                                       SceneBinding::SceneBindingFps);
    } else if(parseSceneWidth(exp, pos)) {
        result = SceneBinding::sCreate(context,
                                       SceneBinding::SceneBindingWidth);
    } else if(parseSceneHeight(exp, pos)) {
        result = SceneBinding::sCreate(context,
                                       SceneBinding::SceneBindingHeight);
    } else if(parseSceneRangeMin(exp, pos)) {
        result = SceneBinding::sCreate(context,
                                       SceneBinding::SceneBindingRangeMin);
    } else if(parseSceneRangeMax(exp, pos)) {
        result = SceneBinding::sCreate(context,
                                       SceneBinding::SceneBindingRangeMax);
    } else if(parseValue(exp, pos)) {
        result = ValueBinding::sCreate(context);
    } else {
        QString binding;
        parseBinding(exp, pos, binding);
        result = PropertyBinding::sCreate(binding.trimmed(),
                                          validator, context);
        if(!result) PrettyRuntimeThrow("Binding could not be resolved:\n'" +
                                       binding + "'");
    }
    if(!result) PrettyRuntimeThrow("Binding could not be resolved:\n'" +
                                   exp + "'");
    return result;
}

PropertyBindingMap PropertyBindingParser::parseBindings(
        QString exp,
        const PropertyBinding::Validator& validator,
        const Property* const context) {
    const auto lines = exp.split(QRegExp("\n|\r\n|\r|;"),
                                 Qt::SkipEmptyParts);
    PropertyBindingMap result;
    for(const auto& line : lines) {
        QString name;
        const auto binding = parseBinding(name, line, validator, context);
        if(binding) {
            if(result.find(name) != result.end())
                PrettyRuntimeThrow(name + " is already defined");
            result[name] = binding;
        }
    }
    return result;
}
