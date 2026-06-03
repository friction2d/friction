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
*/

#include "lottie/lottiepatheffects.h"

#include "Animators/qrealanimator.h"
#include "Boxes/pathbox.h"
#include "lottie/lottieanimatedproperty.h"
#include "PathEffects/patheffect.h"
#include "PathEffects/patheffectcollection.h"
#include "Properties/boolproperty.h"

#include <QJsonObject>
#include <QString>

namespace {

QJsonObject staticProperty(const QJsonValue& value)
{
    return LottieAnimatedProperty::staticProperty(value);
}

QJsonObject animatedScalarProperty(const QList<qreal>& values,
                                   const FrameRange& frameRange)
{
    return LottieAnimatedProperty::scalar(values, frameRange);
}

QrealAnimator* qrealChild(PathEffect* const effect,
                          const QString& name)
{
    if (!effect) { return nullptr; }
    for (int i = 0; i < effect->ca_getNumberOfChildren(); i++) {
        const auto child = effect->ca_getChildAt<Property>(i);
        if (child && child->prp_getName() == name) {
            return enve_cast<QrealAnimator*>(child);
        }
    }
    return nullptr;
}

BoolProperty* boolChild(PathEffect* const effect,
                        const QString& name)
{
    if (!effect) { return nullptr; }
    for (int i = 0; i < effect->ca_getNumberOfChildren(); i++) {
        const auto child = effect->ca_getChildAt<Property>(i);
        if (child && child->prp_getName() == name) {
            return enve_cast<BoolProperty*>(child);
        }
    }
    return nullptr;
}

QJsonObject trimPathObject(PathEffect* const effect,
                           const FrameRange& frameRange)
{
    const auto pathWise = boolChild(effect, QStringLiteral("path-wise"));
    const auto minLength = qrealChild(effect, QStringLiteral("min length"));
    const auto maxLength = qrealChild(effect, QStringLiteral("max length"));
    const auto offset = qrealChild(effect, QStringLiteral("offset"));

    QList<qreal> starts;
    QList<qreal> ends;
    QList<qreal> offsets;
    for (int frame = frameRange.fMin; frame <= frameRange.fMax; frame++) {
        starts << (minLength ? minLength->getEffectiveValue(frame) : 0);
        ends << (maxLength ? maxLength->getEffectiveValue(frame) : 100);
        offsets << (offset ? offset->getEffectiveValue(frame)*3.6 : 0);
    }

    QJsonObject object;
    object.insert(QStringLiteral("ty"), QStringLiteral("tm"));
    object.insert(QStringLiteral("s"), animatedScalarProperty(starts, frameRange));
    object.insert(QStringLiteral("e"), animatedScalarProperty(ends, frameRange));
    object.insert(QStringLiteral("o"), animatedScalarProperty(offsets, frameRange));
    object.insert(QStringLiteral("m"), pathWise && pathWise->getValue() ? 2 : 1);
    object.insert(QStringLiteral("nm"), QStringLiteral("Sub-Path"));
    object.insert(QStringLiteral("hd"), false);
    return object;
}

QJsonObject strokeDashEntry(const QString& name,
                            const QString& displayName,
                            const QJsonObject& value)
{
    return QJsonObject{
        {QStringLiteral("n"), name},
        {QStringLiteral("nm"), displayName},
        {QStringLiteral("v"), value}
    };
}

QJsonArray dashValues(PathEffect* const effect,
                      const FrameRange& frameRange)
{
    QList<qreal> values;
    const auto size = qrealChild(effect, QStringLiteral("size"));
    for (int frame = frameRange.fMin; frame <= frameRange.fMax; frame++) {
        values << (size ? size->getEffectiveValue(frame) : 0);
    }

    const auto property = animatedScalarProperty(values, frameRange);
    return QJsonArray{
        strokeDashEntry(QStringLiteral("d"), QStringLiteral("dash"), property),
        strokeDashEntry(QStringLiteral("g"), QStringLiteral("gap"), property),
        strokeDashEntry(QStringLiteral("o"), QStringLiteral("offset"), staticProperty(0))
    };
}

}

void LottiePathEffects::appendBasePathEffects(const PathBox* const box,
                                              const FrameRange& frameRange,
                                              QJsonArray& shapes)
{
    if (!box) { return; }

    const auto effects = const_cast<PathBox*>(box)->getPathEffectsAnimators();
    if (!effects || !effects->hasEffects()) { return; }

    for (int i = 0; i < effects->ca_getNumberOfChildren(); i++) {
        const auto effect = effects->getChild(i);
        if (!effect || !effect->isVisible()) { continue; }
        if (effect->getEffectType() != PathEffectType::SUB) { continue; }
        shapes.append(trimPathObject(effect, frameRange));
    }
}

void LottiePathEffects::appendStrokeDash(const PathBox* const box,
                                         const FrameRange& frameRange,
                                         QJsonObject& stroke)
{
    if (!box) { return; }

    const auto effects = const_cast<PathBox*>(box)->getPathEffectsAnimators();
    if (!effects || !effects->hasEffects()) { return; }

    for (int i = 0; i < effects->ca_getNumberOfChildren(); i++) {
        const auto effect = effects->getChild(i);
        if (!effect || !effect->isVisible()) { continue; }
        if (effect->getEffectType() != PathEffectType::DASH) { continue; }
        stroke.insert(QStringLiteral("d"), dashValues(effect, frameRange));
        return;
    }
}
