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

#ifndef SHADEREFFECTPROGRAM_H
#define SHADEREFFECTPROGRAM_H
#include "uniformspecifiercreator.h"
#include "shadervaluehandler.h"
#include "shadereffectjs.h"
#include <mutex>

typedef QList<stdsptr<UniformSpecifierCreator>> UniformSpecifierCreators;
struct CORE_EXPORT ShaderEffectProgram {
    ShaderEffectProgram(const QList<stdsptr<ShaderPropertyCreator>>& propCs);
    ShaderEffectProgram(const ShaderEffectProgram& s) = delete;
    ShaderEffectProgram& operator=(const ShaderEffectProgram& s) = delete;

    GLuint fId = 0;
    GLuint fFragShader;
    GLint fTexLocation;
    QList<GLint> fPropUniLocs;
    UniformSpecifierCreators fPropUniCreators;
    QList<stdsptr<ShaderValueHandler>> fValueHandlers;
    QList<GLint> fValueLocs;
    std::shared_ptr<ShaderEffectJS::Blueprint> fJSBlueprint;

    mutable std::mutex fEnginesMutex;
    mutable std::vector<std::unique_ptr<ShaderEffectJS>> fEngines;

    const QList<stdsptr<ShaderPropertyCreator>> fProperties;

    void reloadFragmentShader(QGL33 * const gl, const QString &fragPath);

    static std::unique_ptr<ShaderEffectProgram> sCreateProgram(
            QGL33 * const gl, const QString &fragPath,
            const QList<stdsptr<ShaderPropertyCreator>>& propCs,
            const std::shared_ptr<ShaderEffectJS::Blueprint>& jsBlueprint,
            const UniformSpecifierCreators& uniCs,
            const QList<stdsptr<ShaderValueHandler>>& values);
};

#endif // SHADEREFFECTPROGRAM_H
