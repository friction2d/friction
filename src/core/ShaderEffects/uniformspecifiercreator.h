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

#ifndef UNIFORMSPECIFIERCREATOR_H
#define UNIFORMSPECIFIERCREATOR_H

#include <QJSValueList>

#include "PropertyCreators/qrealanimatorcreator.h"
#include "PropertyCreators/intanimatorcreator.h"
#include "PropertyCreators/qpointfanimatorcreator.h"
#include "PropertyCreators/qvector3danimatorcreator.h"
#include "PropertyCreators/coloranimatorcreator.h"
#include "glhelpers.h"

class ShaderEffectJS;

enum class ShaderPropertyType {
    floatProperty,
    intProperty,
    vec2Property,
    vec3Property,
    colorProperty,
    invalid
};

typedef std::function<void(QGL33 * const)> UniformSpecifier;
typedef QList<UniformSpecifier> UniformSpecifiers;
struct CORE_EXPORT UniformSpecifierCreator : public StdSelfRef
{
    UniformSpecifierCreator(const ShaderPropertyType type,
                            const bool glValue,
                            const bool resolutionScaled,
                            const bool influenceScaled) :
        mType(type), fGLValue(glValue),
        mResolutionScaled(resolutionScaled),
        mInfluenceScaled(influenceScaled) {}

    void create(ShaderEffectJS &engine,
                const GLint loc,
                Property * const property,
                const qreal relFrame,
                const qreal resolution,
                const qreal influence,
                UniformSpecifiers& uniSpec) const;

    const ShaderPropertyType mType;
    const bool fGLValue;
    const bool mResolutionScaled;
    const bool mInfluenceScaled;
};

#endif // UNIFORMSPECIFIERCREATOR_H
