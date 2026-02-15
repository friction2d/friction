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

#include "shadereffectprogram.h"

ShaderEffectProgram::ShaderEffectProgram(
        const QList<stdsptr<ShaderPropertyCreator> >& propCs) :
    fProperties(propCs) {}

void ShaderEffectProgram::reloadFragmentShader(
        QGL33 * const gl, const QString &fragPath) {
    GLuint newProgram;
    if(!QFile(fragPath).exists())
        RuntimeThrow("Failed to open '" + fragPath + "'");

    try {
        gIniProgram(gl, newProgram, GL_TEXTURED_VERT, fragPath);
    } catch(...) {
        RuntimeThrow("Could not initialize a program for ShaderEffectProgram");
    }

    QList<GLint> propUniLocs;
    for(const auto& propC : fProperties) {
        if(propC->fGLValue) {
            const GLint loc = gl->glGetUniformLocation(newProgram, propC->fName.toLatin1());
            if(loc < 0) {
                gl->glDeleteProgram(newProgram);
                RuntimeThrow("'" + propC->fName +
                             "' does not correspond to an active uniform variable.");
            }
            propUniLocs.append(loc);
        } else propUniLocs.append(-1);
    }
    QList<GLint> valueLocs;
    for(const auto& value : qAsConst(fValueHandlers)) {
        const GLint loc = gl->glGetUniformLocation(newProgram, value->fName.toLatin1());
        if(loc < 0) {
            gl->glDeleteProgram(newProgram);
            RuntimeThrow("'" + value->fName +
                         "' does not correspond to an active uniform variable.");
        }
        valueLocs.append(loc);
    }

    GLint texLocation = gl->glGetUniformLocation(newProgram, "tex");
    if(texLocation < 0) {
        gl->glDeleteProgram(newProgram);
        RuntimeThrow("Invalid location received for 'texture'.");
    }
    gl->glUniform1i(texLocation, 0);

    if(fId != 0) gl->glDeleteProgram(fId);

    fId = newProgram;
    fPropUniLocs = propUniLocs;
    fValueLocs = valueLocs;
    fTexLocation = texLocation;
}

std::unique_ptr<ShaderEffectProgram>
ShaderEffectProgram::sCreateProgram(
        QGL33 * const gl, const QString &fragPath,
        const QList<stdsptr<ShaderPropertyCreator>>& propCs,
        const std::shared_ptr<ShaderEffectJS::Blueprint>& jsBlueprint,
        const UniformSpecifierCreators& uniCs,
        const QList<stdsptr<ShaderValueHandler>>& values) {
    std::unique_ptr<ShaderEffectProgram> program =
            std::make_unique<ShaderEffectProgram>(propCs);
    program->fJSBlueprint = jsBlueprint;
    program->fValueHandlers = values;
    program->fPropUniCreators = uniCs;

    program->reloadFragmentShader(gl, fragPath);

    return program;
}

