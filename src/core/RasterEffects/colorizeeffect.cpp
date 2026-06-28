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

#include "colorizeeffect.h"
#include "gpurendertools.h"
#include "openglrastereffectcaller.h"

#include "colorhelpers.h"
#include "Animators/qrealanimator.h"
#include "ReadWrite/evformat.h"
#include "Properties/newproperty.h"

#include "appsupport.h"

ColorizeEffect::ColorizeEffect() :
    RasterEffect("colorize",
                 AppSupport::getRasterEffectHardwareSupport("Colorize",
                                                            HardwareSupport::cpuOnly),
                 true,
                 RasterEffectType::COLORIZE)
{
    using InfluenceType = NewProperty<QrealAnimator, EvFormat::colorizeInfluence>;
    mInfluence = enve::make_shared<InfluenceType>(1, 0, 1, 0.01, "influence");
    ca_addChild(mInfluence);

    mHue = enve::make_shared<QrealAnimator>(180, -9999, 9999, 1, "hue");
    ca_addChild(mHue);

    mSaturation = enve::make_shared<QrealAnimator>(0.5, 0, 1, 0.01, "saturation");
    ca_addChild(mSaturation);

    mLightness = enve::make_shared<QrealAnimator>(0, -1, 1, 0.01, "lightness");
    ca_addChild(mLightness);
}

class ColorizeEffectCaller : public OpenGLRasterEffectCaller {
public:
    ColorizeEffectCaller(const HardwareSupport hwSupport,
                         const qreal influence,
                         const qreal hue,
                         const qreal saturation,
                         const qreal lightness) :
        OpenGLRasterEffectCaller(sInitialized, sProgramId,
                                 ":/shaders/colorizeeffect.frag",
                                 hwSupport),
        mInfluence(influence),
        mHue(hue),
        mSaturation(saturation),
        mLightness(lightness) {}

    void processCpu(CpuRenderTools& renderTools,
                    const CpuRenderData& data);
protected:
    void iniVars(QGL33 * const gl) const {
        sInfluenceU = gl->glGetUniformLocation(sProgramId, "influence");
        sHueU = gl->glGetUniformLocation(sProgramId, "hue");
        sSaturationU = gl->glGetUniformLocation(sProgramId, "saturation");
        sLightnessU = gl->glGetUniformLocation(sProgramId, "lightness");
    }

    void setVars(QGL33 * const gl) const {
        gl->glUseProgram(sProgramId);
        gl->glUniform1f(sInfluenceU, mInfluence);
        gl->glUniform1f(sHueU, mHue);
        gl->glUniform1f(sSaturationU, mSaturation);
        gl->glUniform1f(sLightnessU, mLightness);
    }
private:
    static bool sInitialized;
    static GLuint sProgramId;

    static GLint sInfluenceU;
    static GLint sHueU;
    static GLint sSaturationU;
    static GLint sLightnessU;

    const qreal mInfluence;
    const qreal mHue;
    const qreal mSaturation;
    const qreal mLightness;
};

bool ColorizeEffectCaller::sInitialized = false;
GLuint ColorizeEffectCaller::sProgramId = 0;

GLint ColorizeEffectCaller::sInfluenceU = -1;
GLint ColorizeEffectCaller::sHueU = -1;
GLint ColorizeEffectCaller::sSaturationU = -1;
GLint ColorizeEffectCaller::sLightnessU = -1;

stdsptr<RasterEffectCaller> ColorizeEffect::getEffectCaller(
        const qreal relFrame, const qreal resolution,
        const qreal influence, BoxRenderData * const data) const {
    Q_UNUSED(resolution)
    Q_UNUSED(data)

    const qreal infl = mInfluence->getEffectiveValue(relFrame) * influence;
    const qreal hue = mHue->getEffectiveValue(relFrame);
    const qreal saturation = mSaturation->getEffectiveValue(relFrame) * influence;
    const qreal lightness = mLightness->getEffectiveValue(relFrame);

    return enve::make_shared<ColorizeEffectCaller>(instanceHwSupport(), infl,
                                                   hue, saturation, lightness);
}

void ColorizeEffectCaller::processCpu(CpuRenderTools& renderTools,
                                      const CpuRenderData& data) {
    const int xMin = data.fTexTile.left();
    const int xMax = data.fTexTile.right();
    const int yMin = data.fTexTile.top();
    const int yMax = data.fTexTile.bottom();

    for(int yi = yMin; yi <= yMax; yi++) {
        auto dst = static_cast<uchar*>(renderTools.fDstBtmp.getAddr(0, yi - yMin));
        auto src = static_cast<uchar*>(renderTools.fSrcBtmp.getAddr(xMin, yi));
        for(int xi = xMin; xi <= xMax; xi++) {
            const uchar texR = *src++;
            const uchar texG = *src++;
            const uchar texB = *src++;
            const uchar texA = *src++;

            const qreal texRF = texR/255.;
            const qreal texGF = texG/255.;
            const qreal texBF = texB/255.;
            const qreal texAF = texA/255.;

            if(texA == 0) {
                for(int i = 0; i < 4; i++) *dst++ = 0;
                continue;
            }
            qreal h = texRF/texAF;
            qreal s = texGF/texAF;
            qreal l = texBF/texAF;
            qrgb_to_hsl(h, s, l);
            h = mHue / 360.;
            s = mSaturation;
            l = qBound(0., l + mLightness, 1.);
            qhsl_to_rgb(h, s, l);

            *dst++ = 255*(h*texAF*mInfluence + texRF*(1 - mInfluence));
            *dst++ = 255*(s*texAF*mInfluence + texGF*(1 - mInfluence));
            *dst++ = 255*(l*texAF*mInfluence + texBF*(1 - mInfluence));
            *dst++ = texA;
        }
    }
}
