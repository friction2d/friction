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

#include "uniformspecifiercreator.h"

#include "shadereffectjs.h"

void qrealAnimatorCreate(ShaderEffectJS &engine,
                         const bool glValue,
                         const GLint loc,
                         Property * const property,
                         const qreal relFrame,
                         const qreal resolution,
                         const qreal influence,
                         UniformSpecifiers& uniSpec)
{
    const auto anim = static_cast<QrealAnimator*>(property);
    const qreal val = anim->getEffectiveValue(relFrame)*resolution*influence;
    const QString propName = anim->prp_getName();
    const QString valScript = propName + " = " + QString::number(val);
    engine.addSetter(val);

    if (!glValue) { return; }
    Q_ASSERT(loc >= 0);
    uniSpec << [loc, val, valScript](QGL33 * const gl) {
        gl->glUniform1f(loc, static_cast<GLfloat>(val));
    };
}

void intAnimatorCreate(ShaderEffectJS &engine,
                       const bool glValue,
                       const GLint loc,
                       Property * const property,
                       const qreal relFrame,
                       const qreal resolution,
                       const qreal influence,
                       UniformSpecifiers& uniSpec)
{
    const auto anim = static_cast<IntAnimator*>(property);
    const int val = qRound(anim->getEffectiveIntValue(relFrame)*resolution*influence);
    const QString valScript = anim->prp_getName() + " = " + QString::number(val);
    engine.addSetter(val);

    if (!glValue) { return; }
    Q_ASSERT(loc >= 0);
    uniSpec << [loc, val, valScript](QGL33 * const gl) {
        gl->glUniform1i(loc, val);
    };
}

QString vec2ValScript(const QString& name,
                      const QPointF& value)
{
    return name + " = [" + QString::number(value.x()) + "," +
                           QString::number(value.y()) + "]";
}

QString vec3ValScript(const QString& name,
                      const QVector3D& value)
{
    return name + " = [" + QString::number(value.x()) + "," +
                           QString::number(value.y()) + "," +
                           QString::number(value.z()) + "]";
}

void QPointFAnimatorCreate(ShaderEffectJS &engine,
                             const bool glValue,
                             const GLint loc,
                             Property * const property,
                             const qreal relFrame,
                             const qreal resolution,
                             const qreal influence,
                             UniformSpecifiers& uniSpec)
{
    const auto anim = static_cast<QPointFAnimator*>(property);
    const QPointF val = anim->getEffectiveValue(relFrame)*resolution*influence;
    const QString valScript = vec2ValScript(anim->prp_getName(), val);
    engine.addSetter(val);

    if (!glValue) { return; }
    Q_ASSERT(loc >= 0);
    uniSpec << [loc, val, valScript](QGL33 * const gl) {
        gl->glUniform2f(loc, val.x(), val.y());
    };
}

void QVector3DAnimatorCreate(ShaderEffectJS &engine,
                             const bool glValue,
                             const GLint loc,
                             Property * const property,
                             const qreal relFrame,
                             const qreal resolution,
                             const qreal influence,
                             UniformSpecifiers& uniSpec)
{
    const auto anim = static_cast<QVector3DAnimator*>(property);
    const auto val = anim->getEffectiveValue(relFrame)*resolution*influence;
    const QString valScript = vec3ValScript(anim->prp_getName(), val);
    //engine.addSetter(val);

    if (!glValue) { return; }
    Q_ASSERT(loc >= 0);
    uniSpec << [loc, val, valScript](QGL33 * const gl) {
        gl->glUniform2f(loc, val.x(), val.y());
    };
}

QString colorValScript(const QString& name,
                       const QColor& value)
{
    return name + " = [" + QString::number(value.redF()) + "," +
                           QString::number(value.greenF()) + "," +
                           QString::number(value.blueF()) + "," +
                           QString::number(value.alphaF()) + "]";
}

void colorAnimatorCreate(ShaderEffectJS &engine,
                         const bool glValue,
                         const GLint loc,
                         Property * const property,
                         const qreal relFrame,
                         UniformSpecifiers& uniSpec)
{
    const auto anim = static_cast<ColorAnimator*>(property);
    const QColor val = anim->getColor(relFrame);
    const QString valScript = colorValScript(anim->prp_getName(), val);
    engine.addSetter(val);

    if (!glValue) { return; }
    Q_ASSERT(loc >= 0);
    uniSpec << [loc, val, valScript](QGL33 * const gl) {
        gl->glUniform4f(loc, val.redF(), val.greenF(), val.blueF(),
                        val.alphaF());
    };
}

void UniformSpecifierCreator::create(ShaderEffectJS &engine,
                                     const GLint loc,
                                     Property * const property,
                                     const qreal relFrame,
                                     const qreal resolution,
                                     const qreal influence,
                                     UniformSpecifiers& uniSpec) const
{
    switch(mType) {
    case ShaderPropertyType::floatProperty:
        return qrealAnimatorCreate(engine,
                                   fGLValue,
                                   loc,
                                   property,
                                   relFrame,
                                   mResolutionScaled ? resolution : 1,
                                   mInfluenceScaled ? influence : 1,
                                   uniSpec);
    case ShaderPropertyType::intProperty:
        return intAnimatorCreate(engine,
                                 fGLValue,
                                 loc,
                                 property,
                                 relFrame,
                                 mResolutionScaled ? resolution : 1,
                                 mInfluenceScaled ? influence : 1,
                                 uniSpec);
    case ShaderPropertyType::vec2Property:
        return QPointFAnimatorCreate(engine,
                                     fGLValue,
                                     loc,
                                     property,
                                     relFrame,
                                     mResolutionScaled ? resolution : 1,
                                     mInfluenceScaled ? influence : 1,
                                     uniSpec);
    case ShaderPropertyType::vec3Property:
        return QVector3DAnimatorCreate(engine,
                                       fGLValue,
                                       loc,
                                       property,
                                       relFrame,
                                       mResolutionScaled ? resolution : 1,
                                       mInfluenceScaled ? influence : 1,
                                       uniSpec);
    case ShaderPropertyType::colorProperty:
        return colorAnimatorCreate(engine,
                                   fGLValue,
                                   loc,
                                   property,
                                   relFrame,
                                   uniSpec);
    default: RuntimeThrow("Unsupported type");
    }
}
