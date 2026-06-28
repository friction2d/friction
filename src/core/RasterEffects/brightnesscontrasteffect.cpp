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

#include "brightnesscontrasteffect.h"
#include "gpurendertools.h"
#include "openglrastereffectcaller.h"

#include "colorhelpers.h"
#include "Animators/qrealanimator.h"

#include "appsupport.h"

BrightnessContrastEffect::BrightnessContrastEffect() :
    RasterEffect("brightness-contrast",
                 AppSupport::getRasterEffectHardwareSupport("BrightnessContrast",
                                                            HardwareSupport::cpuOnly),
                 true,
                 RasterEffectType::BRIGHTNESS_CONTRAST)
{
    mBrightness = enve::make_shared<QrealAnimator>(0, -1, 1, 0.01, "brightness");
    ca_addChild(mBrightness);

    mContrast = enve::make_shared<QrealAnimator>(0, -1, 1, 0.01, "contrast");
    ca_addChild(mContrast);
}

class BrightnessContrastEffectCaller : public OpenGLRasterEffectCaller {
public:
    BrightnessContrastEffectCaller(const HardwareSupport hwSupport,
                         const qreal brightness,
                         const qreal contrast) :
        OpenGLRasterEffectCaller(sInitialized, sProgramId,
                                 ":/shaders/brightnesscontrasteffect.frag",
                                 hwSupport),
        mBrightness(brightness),
        mContrast(contrast) {}

    void processCpu(CpuRenderTools& renderTools,
                    const CpuRenderData& data);
protected:
    void iniVars(QGL33 * const gl) const {
        sBrightnessU = gl->glGetUniformLocation(sProgramId, "brightness");
        sContrastU = gl->glGetUniformLocation(sProgramId, "contrast");
    }

    void setVars(QGL33 * const gl) const {
        gl->glUseProgram(sProgramId);
        gl->glUniform1f(sBrightnessU, mBrightness);
        gl->glUniform1f(sContrastU, mContrast);
    }
private:
    static bool sInitialized;
    static GLuint sProgramId;

    static GLint sBrightnessU;
    static GLint sContrastU;

    const qreal mBrightness;
    const qreal mContrast;
};

bool BrightnessContrastEffectCaller::sInitialized = false;
GLuint BrightnessContrastEffectCaller::sProgramId = 0;

GLint BrightnessContrastEffectCaller::sBrightnessU = -1;
GLint BrightnessContrastEffectCaller::sContrastU = -1;

stdsptr<RasterEffectCaller> BrightnessContrastEffect::getEffectCaller(
        const qreal relFrame, const qreal resolution,
        const qreal influence, BoxRenderData * const data) const {
    Q_UNUSED(resolution)
    Q_UNUSED(data)

    const qreal brightness = mBrightness->getEffectiveValue(relFrame);
    const qreal contrast = mContrast->getEffectiveValue(relFrame) * influence;

    return enve::make_shared<BrightnessContrastEffectCaller>(
                instanceHwSupport(), brightness, contrast);
}

void BrightnessContrastEffectCaller::processCpu(CpuRenderTools& renderTools,
                                                const CpuRenderData& data) {
    const int xMin = data.fTexTile.left();
    const int xMax = data.fTexTile.right();
    const int yMin = data.fTexTile.top();
    const int yMax = data.fTexTile.bottom();

    for(int yi = yMin; yi <= yMax; yi++) {
        auto dst = static_cast<uchar*>(renderTools.fDstBtmp.getAddr(0, yi - yMin));
        auto src = static_cast<uchar*>(renderTools.fSrcBtmp.getAddr(xMin, yi));
        for(int xi = xMin; xi <= xMax; xi++) {
            const uchar r = *src++;
            const uchar g = *src++;
            const uchar b = *src++;
            const uchar a = *src++;

            *dst++ = (r - 0.5*a)*(mContrast + 1.) + a*(0.5 + mBrightness);
            *dst++ = (g - 0.5*a)*(mContrast + 1.) + a*(0.5 + mBrightness);
            *dst++ = (b - 0.5*a)*(mContrast + 1.) + a*(0.5 + mBrightness);
            *dst++ = a;
        }
    }
}
