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

#include "shadervaluehandler.h"

ShaderValueHandler::ShaderValueHandler(const QString &name,
                                       const GLValueType type,
                                       const QString& script):
    fName(name), fScript(script), mType(type) {}

UniformSpecifier ShaderValueHandler::create(const GLint loc,
                                            ShaderEffectJS &engine,
                                            int index) const
{
    Q_ASSERT(loc >= 0);
    switch(mType) {
    case GLValueType::Boolean:
        return [loc, &engine, index](QGL33 * const gl) {
            const auto val = engine.getGlValueDouble(index);
            gl->glUniform1i(loc, static_cast<GLint>(qRound(val)));
        };
    case GLValueType::Float:
        return [loc, &engine, index](QGL33 * const gl) {
            const auto val = engine.getGlValueDouble(index);
            gl->glUniform1f(loc, static_cast<GLfloat>(val));
        };
    case GLValueType::Vec2:
        return [loc, &engine, index](QGL33 * const gl) {
            const auto val = engine.getGlValueDouble2(index);
            gl->glUniform2f(loc,
                            static_cast<GLfloat>(val.v0),
                            static_cast<GLfloat>(val.v1));
        };
    case GLValueType::Vec3:
        return [loc, &engine, index](QGL33 * const gl) {
            const auto val = engine.getGlValueDouble3(index);
            gl->glUniform3f(loc,
                            static_cast<GLfloat>(val.v0),
                            static_cast<GLfloat>(val.v1),
                            static_cast<GLfloat>(val.v2));
        };
    case GLValueType::Vec4:
        return [loc, &engine, index](QGL33 * const gl) {
            const auto val = engine.getGlValueDouble4(index);
            gl->glUniform4f(loc,
                            static_cast<GLfloat>(val.v0),
                            static_cast<GLfloat>(val.v1),
                            static_cast<GLfloat>(val.v2),
                            static_cast<GLfloat>(val.v3));
        };
    case GLValueType::Int:
        return [loc, &engine, index](QGL33 * const gl) {
            const auto val = engine.getGlValueDouble(index);
            gl->glUniform1i(loc, static_cast<GLint>(qRound(val)));
        };
    case GLValueType::iVec2:
        return [loc, &engine, index](QGL33 * const gl) {
            const auto val = engine.getGlValueDouble2(index);
            gl->glUniform2i(loc,
                            static_cast<GLint>(qRound(val.v0)),
                            static_cast<GLint>(qRound(val.v1)));
        };
    case GLValueType::iVec3:
        return [loc, &engine, index](QGL33 * const gl) {
            const auto val = engine.getGlValueDouble3(index);
            gl->glUniform3i(loc,
                            static_cast<GLint>(qRound(val.v0)),
                            static_cast<GLint>(qRound(val.v1)),
                            static_cast<GLint>(qRound(val.v2)));
        };
    case GLValueType::iVec4:
        return [loc, &engine, index](QGL33 * const gl) {
            const auto val = engine.getGlValueDouble4(index);
            gl->glUniform4i(loc,
                            static_cast<GLint>(qRound(val.v0)),
                            static_cast<GLint>(qRound(val.v1)),
                            static_cast<GLint>(qRound(val.v2)),
                            static_cast<GLint>(qRound(val.v3)));
        };
    default: RuntimeThrow("Unsupported type for " + fName);
    }
}
