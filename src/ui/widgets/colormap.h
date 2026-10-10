/*
#
# Friction - https://friction.graphics
#
# Copyright (c) Friction contributors
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3.
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

#ifndef FRICTION_COLOR_MAP_H
#define FRICTION_COLOR_MAP_H

#include <QOpenGLWidget>

#include "ui_global.h"
#include "colorwidget.h"
#include "Animators/coloranimator.h"
#include "conncontextptr.h"

namespace Friction
{
    namespace Ui
    {
        class UI_EXPORT ColorMap : public ColorWidget
        {
            Q_OBJECT
        public:
            ColorMap(QWidget *parent_t = nullptr);

            void setTarget(ColorAnimator * const target);
            void setColor(qreal h, qreal s, qreal v);

        signals:
            void editingStarted();
            void colorChanged(qreal h, qreal s, qreal v);
            void editingFinished();

        protected:
            void initializeGL() override;
            void paintGL() override;
            void resizeGL(int w, int h) override;

            void mousePressEvent(QMouseEvent *e) override;
            void mouseReleaseEvent(QMouseEvent *e) override;
            void mouseMoveEvent(QMouseEvent *e) override;

        private:
            void applyToTarget(bool isStart, bool isFinish);
            void mapInteraction(int x, int y);

            bool mIsDragging = false;
            GLuint mProgram = 0;
            GLuint mVAO = 0;

            ConnContextQPtr<ColorAnimator> mTarget;
        };
    }
}

#endif // FRICTION_COLOR_MAP_H

