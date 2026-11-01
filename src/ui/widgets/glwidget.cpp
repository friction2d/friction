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

#include "glwidget.h"

#include "colorhelpers.h"
#include "GUI/global.h"
#include "Animators/paintsettingsanimator.h"

GLWidget::GLWidget(QWidget *parent) : QOpenGLWidget(parent)
{
//    setFormat(QSurfaceFormat::defaultFormat());
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    setMinimumSize(eSizesUI::widget, eSizesUI::widget);
}

void GLWidget::initializeGL()
{
#ifdef USE_GLES
    initializeOpenGLFunctions();
#else
    if (!initializeOpenGLFunctions()) {
        RuntimeThrow(tr("Initializing OpenGL 3.3 failed."));
    }
#endif

    glClearColor(0, 0, 0, 1);

    // Set blending
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    iniPlainVShaderVAO(this, mPlainSquareVAO);
}
