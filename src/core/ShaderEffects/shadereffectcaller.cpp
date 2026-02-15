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

#include "shadereffectcaller.h"
#include "shadereffectprogram.h"

ShaderEffectCaller::ShaderEffectCaller(std::unique_ptr<ShaderEffectJS>&& engine,
                                       const ShaderEffectProgram &program,
                                       const ShaderEffect *parentEffect,
                                       const qreal &relFrame,
                                       const qreal &resolution,
                                       const qreal &influence)
    : RasterEffectCaller(HardwareSupport::gpuOnly,
                         false,
                         QMargins())
    , mEngine(std::move(engine))
    , mProgramId(program.fId)
    , mProgram(program)
{
    Q_ASSERT(mEngine.get());
    calc(parentEffect,
         relFrame,
         resolution,
         influence);
}

ShaderEffectCaller::~ShaderEffectCaller()
{
    mProgram.fEngines.push_back(std::move(mEngine));
}

void ShaderEffectCaller::processGpu(QGL33 * const gl,
                                    GpuRenderTools &renderTools)
{
    renderTools.switchToOpenGL(gl);

    renderTools.requestTargetFbo().bind(gl);
    gl->glClear(GL_COLOR_BUFFER_BIT);

    setupProgram(gl);

    gl->glActiveTexture(GL_TEXTURE0);
    renderTools.getSrcTexture().bind(gl);

    gl->glBindVertexArray(renderTools.getSquareVAO());
    gl->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    renderTools.swapTextures();
}

void ShaderEffectCaller::calc(const ShaderEffect *pEff,
                              const qreal relFrame,
                              const qreal resolution,
                              const qreal influence)
{
    if (!pEff) { return; }
    mEngine->clearSetters();
    UniformSpecifiers& uniSpecs = mUniformSpecifiers;
    const int argsCount = mProgram.fPropUniLocs.count();
    for (int i = 0; i < argsCount; i++) {
        const GLint loc = mProgram.fPropUniLocs.at(i);
        const auto prop = pEff->ca_getChildAt(i);
        const auto& uniformC = mProgram.fPropUniCreators.at(i);
        uniformC->create(getJSEngine(),
                         loc,
                         prop,
                         relFrame,
                         resolution,
                         influence,
                         uniSpecs);
    }
    mEngine->updateValues();
    const int valsCount = mProgram.fValueHandlers.count();
    for (int i = 0; i < valsCount; i++) {
        const GLint loc = mProgram.fValueLocs.at(i);
        const auto& value = mProgram.fValueHandlers.at(i);
        uniSpecs << value->create(loc, getJSEngine(), i);
    }
}

QMargins ShaderEffectCaller::getMargin(const SkIRect &srcRect)
{
    mEngine->setSceneRect(srcRect);
    mEngine->evaluate();
    return mEngine->getMargins();
}

void ShaderEffectCaller::setupProgram(QGL33 * const gl)
{
    gl->glUseProgram(mProgramId);
    for (const auto& uni : qAsConst(mUniformSpecifiers)) { uni(gl); }
}
