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

#include "noisefadeeffect.h"
#include "gpurendertools.h"
#include "openglrastereffectcaller.h"

#include "Animators/qrealanimator.h"

#include "appsupport.h"

NoiseFadeEffect::NoiseFadeEffect() :
    RasterEffect("noise fade",
                 AppSupport::getRasterEffectHardwareSupport("NoiseFade",
                                                            HardwareSupport::cpuOnly),
                 false,
                 RasterEffectType::NOISE_FADE)
{
    mSeed = enve::make_shared<QrealAnimator>(0, -999.99, 999.99, 0.01, "seed");
    ca_addChild(mSeed);

    mSize = enve::make_shared<QrealAnimator>(1, -9.99, 9.99, 0.01, "size");
    ca_addChild(mSize);

    mSharpness = enve::make_shared<QrealAnimator>(0, 0, 1, 0.01, "sharpness");
    ca_addChild(mSharpness);

    mTime = enve::make_shared<QrealAnimator>(0, 0, 1, 0.01, "time");
    ca_addChild(mTime);
}

class NoiseFadeEffectCaller : public OpenGLRasterEffectCaller {
public:
    NoiseFadeEffectCaller(const HardwareSupport hwSupport,
                          const qreal seed,
                          const qreal size,
                          const qreal sharpness,
                          const qreal time) :
        OpenGLRasterEffectCaller(sInitialized, sProgramId,
                                 ":/shaders/noisefadeeffect.frag",
                                 hwSupport),
        mSeed(seed),
        mSize(size),
        mSharpness(sharpness),
        mTime(time) {}

    void processCpu(CpuRenderTools& renderTools,
                    const CpuRenderData& data);
protected:
    void iniVars(QGL33 * const gl) const {
        sSeedU = gl->glGetUniformLocation(sProgramId, "seed");
        sSizeU = gl->glGetUniformLocation(sProgramId, "size");
        sSharpnessU = gl->glGetUniformLocation(sProgramId, "sharpness");
        sTimeU = gl->glGetUniformLocation(sProgramId, "time");
    }

    void setVars(QGL33 * const gl) const {
        gl->glUniform1f(sSeedU, mSeed);
        gl->glUniform1f(sSizeU, mSize);
        gl->glUniform1f(sSharpnessU, mSharpness);
        gl->glUniform1f(sTimeU, mTime);
    }
private:
    qreal r(const QPointF& p) const;
    qreal n(const QPointF& p) const;
    qreal noise(const QPointF& p) const;

    static bool sInitialized;
    static GLuint sProgramId;

    static GLint sSeedU;
    static GLint sSizeU;
    static GLint sSharpnessU;
    static GLint sTimeU;

    const qreal mSeed;
    const qreal mSize;
    const qreal mSharpness;
    const qreal mTime;
};

bool NoiseFadeEffectCaller::sInitialized = false;
GLuint NoiseFadeEffectCaller::sProgramId = 0;

GLint NoiseFadeEffectCaller::sSeedU = -1;
GLint NoiseFadeEffectCaller::sSizeU = -1;
GLint NoiseFadeEffectCaller::sSharpnessU = -1;
GLint NoiseFadeEffectCaller::sTimeU = -1;

stdsptr<RasterEffectCaller> NoiseFadeEffect::getEffectCaller(
        const qreal relFrame, const qreal resolution,
        const qreal influence, BoxRenderData * const data) const {
    Q_UNUSED(resolution)
    Q_UNUSED(data)

    const qreal seed = mSeed->getEffectiveValue(relFrame);
    const qreal size = mSize->getEffectiveValue(relFrame);
    const qreal sharpness = mSharpness->getEffectiveValue(relFrame);
    const qreal time = mTime->getEffectiveValue(relFrame) * influence;

    return enve::make_shared<NoiseFadeEffectCaller>(instanceHwSupport(),
                                                    seed, size, sharpness, time);
}

qreal GLSL_smoothstep(const qreal edge0,
                      const qreal edge1,
                      const qreal x) {
    const qreal t = clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
    return t * t * (3.0 - 2.0 * t);
}

QPointF GLSL_smoothstep(const QPointF& edge0,
                        const QPointF& edge1,
                        const QPointF& x) {
    const qreal rx = GLSL_smoothstep(edge0.x(), edge1.x(), x.x());
    const qreal ry = GLSL_smoothstep(edge0.y(), edge1.y(), x.y());
    return {rx, ry};
}

QPointF GLSL_floor(const QPointF& p) {
    return {floor(p.x()), floor(p.y())};
}

qreal GLSL_mix(const qreal x, const qreal y, const qreal a) {
    return x*(1 - a) + y*a;
}

qreal GLSL_fract(const qreal x) {
    return x - floor(x);
}

QPointF GLSL_fract(const QPointF& p) {
    return p - GLSL_floor(p);
}

qreal NoiseFadeEffectCaller::r(const QPointF& p) const {
    return GLSL_fract(cos((p.x() + 0.00001*mSeed)*42.98 +
                          (p.y() + 0.00001*mSeed)*43.23) * 1127.53);
}

qreal NoiseFadeEffectCaller::n(const QPointF& p) const {
    const QPointF fn = GLSL_floor(p);
    const QPointF sn = GLSL_smoothstep(QPointF{0. ,0.},
                                       QPointF{1., 1.},
                                       GLSL_fract(p));

    const qreal h1 = GLSL_mix(r(fn),
                              r(fn + QPointF{1., 0.}), sn.x());
    const qreal h2 = GLSL_mix(r(fn + QPointF{0., 1.}),
                              r(fn + QPointF{1., 1.}), sn.x());
    return GLSL_mix(h1 ,h2, sn.y());
}

qreal NoiseFadeEffectCaller::noise(const QPointF& p) const {
    const qreal s = mSize*0.001;
    return 0.58 * n(p/(32.*s)) +
           0.2 * n(p/(16.*s)) +
           0.1 * n(p/(8.*s)) +
           0.05 * n(p/(4.*s)) +
           0.02 * n(p/(2.*s)) +
           0.0125 * n(p/s);
}

void NoiseFadeEffectCaller::processCpu(CpuRenderTools& renderTools,
                                       const CpuRenderData& data) {
    const qreal imgWidth = renderTools.fSrcBtmp.width();
    const qreal imgHeight = renderTools.fSrcBtmp.height();

    const int xMin = data.fTexTile.left();
    const int xMax = data.fTexTile.right();
    const int yMin = data.fTexTile.top();
    const int yMax = data.fTexTile.bottom();

    const qreal t = abs(sin(0.5*PI*mTime));
    const qreal b = 0.25*(0.75 - 0.749*mSharpness);

    for(int yi = yMin; yi <= yMax; yi++) {
        auto dst = static_cast<uchar*>(renderTools.fDstBtmp.getAddr(0, yi - yMin));
        auto src = static_cast<uchar*>(renderTools.fSrcBtmp.getAddr(xMin, yi));
        for(int xi = xMin; xi <= xMax; xi++) {
            const qreal x = xi/imgWidth;
            const qreal y = yi/imgHeight;

            const qreal c = GLSL_smoothstep(t + b, t - b, noise(QPointF{x, y} * .4));

            for(int i = 0; i < 4; i++) {
                *dst++ = *src++ * (1 - c);
            }
        }
    }
}
