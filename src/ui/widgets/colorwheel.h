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

#ifndef FRICTION_COLOR_WHEEL_H
#define FRICTION_COLOR_WHEEL_H

#include <QOpenGLWidget>

#include "ui_global.h"
#include "colorwidget.h"
#include "colorsetting.h"
#include "Animators/coloranimator.h"
#include "conncontextptr.h"

namespace Friction
{
    namespace Ui
    {
        class UI_EXPORT ColorWheel : public ColorWidget
        {
            Q_OBJECT
        public:
            enum Focus {H, SV, NONE};

            ColorWheel(QWidget *parent_t = nullptr);

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
            void mouseReleaseEvent(QMouseEvent *) override;
            void mouseMoveEvent(QMouseEvent *e) override;
            //void wheelEvent(QWheelEvent *e) override;

        private:
            void drawTriangle();
            void drawWheel();
            bool isInTriangle(const QPoint &pos_t);
            bool isInWheel(const QPoint &pos_t);
            void wheelInteraction(const int x_t, const int y_t);
            void triangleInteraction(int x_t, int y_t);

            void applyToTarget(bool isStart,
                               bool isFinish);

            GLuint mWheelDim = 128;
            GLuint mWheelThickness = 20;

            Focus mFocus = NONE;
            float mOuterCircleR = 0;
            float mInnerCircleR = 0;

            GLuint mWheelProgram = 0;
            GLuint mTriangleProgram = 0;
            GLuint mVAO = 0;

            bool mIsDragging = false;

            ConnContextQPtr<ColorAnimator> mTarget;
        };
    }
}

#endif // FRICTION_COLOR_WHEEL_H
