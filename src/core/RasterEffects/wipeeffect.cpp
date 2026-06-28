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

#include "wipeeffect.h"
#include "gpurendertools.h"
#include "openglrastereffectcaller.h"

#include "Animators/qrealanimator.h"

#include "appsupport.h"

WipeEffect::WipeEffect() :
    RasterEffect("wipe",
                 AppSupport::getRasterEffectHardwareSupport("Wipe",
                                                            HardwareSupport::cpuOnly),
                 true,
                 RasterEffectType::WIPE)
{
    mSharpness = enve::make_shared<QrealAnimator>(0, 0, 1, 0.01, "sharpness");
    ca_addChild(mSharpness);

    mDirection = enve::make_shared<QrealAnimator>(0, -9999, 9999, 1, "direction");
    ca_addChild(mDirection);

    mTime = enve::make_shared<QrealAnimator>(0, 0, 1, 0.01, "time");
    ca_addChild(mTime);
}

class WipeEffectCaller : public OpenGLRasterEffectCaller {
public:
    WipeEffectCaller(const HardwareSupport hwSupport,
                     const qreal sharpness,
                     const qreal direction,
                     const qreal time) :
        OpenGLRasterEffectCaller(sInitialized, sProgramId,
                                 ":/shaders/wipeeffect.frag",
                                 hwSupport),
        mSharpness(sharpness),
        mDirection(direction),
        mTime(time) {}

    void processCpu(CpuRenderTools& renderTools,
                    const CpuRenderData& data);
protected:
    void iniVars(QGL33 * const gl) const {
        sSharpnessU = gl->glGetUniformLocation(sProgramId, "sharpness");
        sDirectionU = gl->glGetUniformLocation(sProgramId, "direction");
        sTimeU = gl->glGetUniformLocation(sProgramId, "time");

        sX0 = gl->glGetUniformLocation(sProgramId, "x0");
        sX1 = gl->glGetUniformLocation(sProgramId, "x1");
    }

    void setVars(QGL33 * const gl) const {
        gl->glUniform1f(sSharpnessU, mSharpness);
        gl->glUniform1f(sDirectionU, mDirection * PI / 180.);
        gl->glUniform1f(sTimeU, mTime);

        const qreal width = 2 - mSharpness;
        const qreal margin = 0.5*(width - 1);
        const qreal x0 = width * mTime - margin;
        const qreal x1 = x0 + 1 - mSharpness;

        gl->glUniform1f(sX0, x0);
        gl->glUniform1f(sX1, x1);
    }
private:
    static bool sInitialized;
    static GLuint sProgramId;

    static GLint sSharpnessU;
    static GLint sDirectionU;
    static GLint sTimeU;

    static GLint sX0;
    static GLint sX1;

    const qreal mSharpness;
    const qreal mDirection;
    const qreal mTime;
};

bool WipeEffectCaller::sInitialized = false;
GLuint WipeEffectCaller::sProgramId = 0;

GLint WipeEffectCaller::sSharpnessU = -1;
GLint WipeEffectCaller::sDirectionU = -1;
GLint WipeEffectCaller::sTimeU = -1;

GLint WipeEffectCaller::sX0 = -1;
GLint WipeEffectCaller::sX1 = -1;

stdsptr<RasterEffectCaller> WipeEffect::getEffectCaller(
        const qreal relFrame, const qreal resolution,
        const qreal influence, BoxRenderData * const data) const {
    Q_UNUSED(resolution)
    Q_UNUSED(data)

    const qreal sharpness = mSharpness->getEffectiveValue(relFrame);
    const qreal direction = mDirection->getEffectiveValue(relFrame);
    const qreal time = mTime->getEffectiveValue(relFrame) * influence;

    return enve::make_shared<WipeEffectCaller>(instanceHwSupport(),
                                               sharpness, direction, time);
}

qreal GLSL_mod(const qreal x, const qreal y) {
    return x - y * floor(x/y);
}

void WipeEffectCaller::processCpu(CpuRenderTools& renderTools,
                                  const CpuRenderData& data) {
    const qreal width = 2 - mSharpness;
    const qreal margin = 0.5*(width - 1);
    const qreal x0 = width * mTime - margin;
    const qreal x1 = x0 + 1 - mSharpness;

    qreal direction = mDirection * PI / 180.;

    const bool i = GLSL_mod(direction, PI) > 0.5*PI;
    if(i) direction = PI - direction;
    const bool ii = GLSL_mod(direction, 2 * PI) > PI;

    const qreal imgWidth = renderTools.fSrcBtmp.width();
    const qreal imgHeight = renderTools.fSrcBtmp.height();

    const int xMin = data.fTexTile.left();
    const int xMax = data.fTexTile.right();
    const int yMin = data.fTexTile.top();
    const int yMax = data.fTexTile.bottom();

    const qreal c = 0.25*PI - direction;

    for(int yi = yMin; yi <= yMax; yi++) {
        auto dst = static_cast<uchar*>(renderTools.fDstBtmp.getAddr(0, yi - yMin));
        auto src = static_cast<uchar*>(renderTools.fSrcBtmp.getAddr(xMin, yi));
        for(int xi = xMin; xi <= xMax; xi++) {
            qreal x = xi/imgWidth;
            const qreal y = yi/imgHeight;

            if(i) x = 1 - x;

            const qreal a = sqrt(x*x + y*y);
            const qreal b = direction - asin(y / a);

            qreal f = a * cos(b) / (cos(c) * sqrt(2));

            if(ii) f = 1 - f;

            f += 0.33333 * sqrt(2) * (1 - mSharpness);

            float alpha;
            if(f < x0) {
                alpha = 0;
            } else if(f > x1) {
                alpha = 1;
            } else {
                alpha = 1 - 0.5*(cos(PI*(f - x0)/(1 - mSharpness)) + 1);
            }

            for(int i = 0; i < 4; i++) {
                *dst++ = *src++ * alpha;
            }
        }
    }
}
