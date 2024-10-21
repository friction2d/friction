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

#ifndef SHADERVALUEHANDLER_H
#define SHADERVALUEHANDLER_H
#include <QJSEngine>

#include "ShaderEffects/shadereffectjs.h"
#include "glhelpers.h"
#include "smartPointers/ememory.h"

typedef std::function<void(QGL33 * const)> UniformSpecifier;

enum class GLValueType {
    Float, Vec2, Vec3, Vec4,
    Int, iVec2, iVec3, iVec4,
    Boolean,
    none
};

class CORE_EXPORT ShaderValueHandler : public StdSelfRef
{
public:
    ShaderValueHandler(const QString& name, const GLValueType type,
                       const QString& script);

    UniformSpecifier create(const GLint loc,
                            ShaderEffectJS &engine,
                            int index) const;

    const QString fName;
    const QString fScript;

private:
    const GLValueType mType;
};

#endif // SHADERVALUEHANDLER_H
