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

#ifndef GLHELPERS_H
#define GLHELPERS_H

#include <QPoint>
#include "skia/skiaincludes.h"

#include "exceptions.h"

#if USE_GLES
#include <QOpenGLExtraFunctions>
typedef QOpenGLExtraFunctions QGL33;
#else
#include <QOpenGLFunctions_3_3_Core>
typedef QOpenGLFunctions_3_3_Core QGL33;
#endif

#define BUFFER_OFFSET(i) ((void*)(i))

CORE_EXPORT
extern void checkGLErrors(QGL33 * const gl, const QString& msg = "");

//! @brief Creates a program, compiles, and attaches associated shaders.
CORE_EXPORT
extern void gIniProgram(QGL33 * const gl,
                        GLuint& program,
                        const QString &vShaderPath,
                        const QString &fShaderPath);

CORE_EXPORT
extern void iniTexturedVShaderVBO(QGL33 * const gl);
CORE_EXPORT
extern void iniTexturedVShaderVAO(QGL33 * const gl, GLuint& VAO);
CORE_EXPORT
extern void iniPlainVShaderVBO(QGL33 * const gl);
CORE_EXPORT
extern void iniPlainVShaderVAO(QGL33 * const gl, GLuint& VAO);

CORE_EXPORT
extern QString GL_PLAIN_VERT;
CORE_EXPORT
extern GLuint GL_PLAIN_SQUARE_VBO;

CORE_EXPORT
extern QString GL_TEXTURED_VERT;
CORE_EXPORT
extern GLuint GL_TEXTURED_SQUARE_VBO;

struct CORE_EXPORT CpuRenderData {
    //! @brief Tile rect in texture coordinates
    SkIRect fTexTile;

    //! @brief Pixel {0, 0} position in scene coordinates
    QPoint fPos;

    //! @brief Texture size
    uint fWidth;
    uint fHeight;
};

#endif // GLHELPERS_H
