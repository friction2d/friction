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

#ifndef SHADEREFFECT_H
#define SHADEREFFECT_H
#include "RasterEffects/rastereffect.h"
#include "shadereffectcreator.h"
#include "Tasks/updatable.h"

template <typename T> using stduptr = std::unique_ptr<T>;

class CORE_EXPORT ShaderEffect : public RasterEffect {
    e_OBJECT
    ShaderEffect(const QString &name,
                 const ShaderEffectCreator * const creator,
                 const ShaderEffectProgram * const program,
                 const QList<stdsptr<ShaderPropertyCreator>>& props);
public:
    void writeIdentifier(eWriteStream &dst) const;
    void writeIdentifierXML(QDomElement& ele) const;

    stdsptr<RasterEffectCaller> getEffectCaller(
            const qreal relFrame, const qreal resolution,
            const qreal influence, BoxRenderData* const data) const;

    void updateIfUsesProgram(const ShaderEffectProgram * const program) {
        if(program == mProgram)
            prp_afterWholeInfluenceRangeChanged();
    }

    void giveBackJSEngine(stduptr<ShaderEffectJS>&& engineUPtr);
private:
    void takeJSEngine(stduptr<ShaderEffectJS>& engineUPtr) const;

    const ShaderEffectProgram * const mProgram;
    const ShaderEffectCreator * const mCreator;
};

#endif // SHADEREFFECT_H
