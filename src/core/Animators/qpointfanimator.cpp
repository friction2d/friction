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

#include "qpointfanimator.h"

#include "qrealanimator.h"
#include "qrealkey.h"
#include "svgexporter.h"
#include "clipboardcontainer.h"

QPointFAnimator::QPointFAnimator(const QString &name) :
    StaticComplexAnimator(name) {
    mXAnimator = enve::make_shared<QrealAnimator>("x");
    mYAnimator = enve::make_shared<QrealAnimator>("y");
    ca_addChild(mXAnimator);
    ca_addChild(mYAnimator);
}

QPointFAnimator::QPointFAnimator(const QPointF &iniValue,
                                 const QPointF &minValue,
                                 const QPointF &maxValue,
                                 const QPointF &valueStep,
                                 const QString &name) :
    QPointFAnimator(name) {
    mXAnimator->setValueRange(minValue.x(), maxValue.x());
    mYAnimator->setValueRange(minValue.y(), maxValue.y());
    mXAnimator->setPrefferedValueStep(valueStep.x());
    mYAnimator->setPrefferedValueStep(valueStep.y());
    setBaseValue(iniValue);
}

QPointFAnimator::QPointFAnimator(const QPointF &iniValue,
                                 const QPointF &minValue,
                                 const QPointF &maxValue,
                                 const QPointF &valueStep,
                                 const QString& nameX,
                                 const QString& nameY,
                                 const QString &name) :
    QPointFAnimator(iniValue, minValue, maxValue, valueStep, name) {
    mXAnimator->prp_setName(nameX);
    mYAnimator->prp_setName(nameY);
}

QJSValue toArray(QJSEngine& e, const QPointF& value) {
    auto array = e.newArray(2);
    array.setProperty(0, value.x());
    array.setProperty(1, value.y());
    return array;
}

QJSValue QPointFAnimator::prp_getBaseJSValue(QJSEngine& e) const {
    return toArray(e, getBaseValue());
}

QJSValue QPointFAnimator::prp_getBaseJSValue(QJSEngine& e, const qreal relFrame) const {
    return toArray(e, getBaseValue(relFrame));
}

QJSValue QPointFAnimator::prp_getEffectiveJSValue(QJSEngine& e) const {
    return toArray(e, getEffectiveValue());
}

QJSValue QPointFAnimator::prp_getEffectiveJSValue(QJSEngine& e, const qreal relFrame) const {
    return toArray(e, getEffectiveValue(relFrame));
}

QPointF QPointFAnimator::getBaseValue() const {
    return QPointF(mXAnimator->getCurrentBaseValue(),
                   mYAnimator->getCurrentBaseValue());
}

QPointF QPointFAnimator::getEffectiveValue() const {
    return QPointF(mXAnimator->getEffectiveValue(),
                   mYAnimator->getEffectiveValue());
}

QPointF QPointFAnimator::getBaseValueAtAbsFrame(const qreal frame) const {
    return getBaseValue(prp_absFrameToRelFrameF(frame));
}

QPointF QPointFAnimator::getBaseValue(const qreal relFrame) const {
    return QPointF(mXAnimator->getBaseValue(relFrame),
                   mYAnimator->getBaseValue(relFrame));
}

QPointF QPointFAnimator::getEffectiveValueAtAbsFrame(const qreal frame) const {
    return getEffectiveValue(prp_absFrameToRelFrameF(frame));
}

QPointF QPointFAnimator::getEffectiveValue(const qreal relFrame) const {
    return QPointF(mXAnimator->getEffectiveValue(relFrame),
                   mYAnimator->getEffectiveValue(relFrame));
}

void QPointFAnimator::setPrefferedValueStep(const qreal valueStep) {
    mXAnimator->setPrefferedValueStep(valueStep);
    mYAnimator->setPrefferedValueStep(valueStep);
}

bool QPointFAnimator::getBeingTransformed() {
    return mXAnimator->getBeingTransformed() ||
            mYAnimator->getBeingTransformed();
}

qreal QPointFAnimator::getEffectiveXValue() {
    return mXAnimator->getEffectiveValue();
}

qreal QPointFAnimator::getEffectiveYValue() {
    return mYAnimator->getEffectiveValue();
}

qreal QPointFAnimator::getEffectiveXValue(const qreal relFrame) {
    return mXAnimator->getEffectiveValue(relFrame);
}

qreal QPointFAnimator::getEffectiveYValue(const qreal relFrame) {
    return mYAnimator->getEffectiveValue(relFrame);
}

void QPointFAnimator::setBaseValue(const QPointF &val) {
    mXAnimator->setCurrentBaseValue(val.x());
    mYAnimator->setCurrentBaseValue(val.y());
}

void QPointFAnimator::setValuesRange(const qreal minVal,
                                     const qreal maxVal) {
    mXAnimator->setValueRange(minVal, maxVal);
    mYAnimator->setValueRange(minVal, maxVal);
}

void QPointFAnimator::setBaseValueWithoutCallingUpdater(
        const QPointF &val) {
    mXAnimator->setCurrentBaseValueNoUpdate(val.x());
    mYAnimator->setCurrentBaseValueNoUpdate(val.y());
}

void QPointFAnimator::incBaseValuesWithoutCallingUpdater(
        const qreal x, const qreal y) {
    mXAnimator->incCurrentValueNoUpdate(x);
    mYAnimator->incCurrentValueNoUpdate(y);
}

void QPointFAnimator::incBaseValues(const qreal x, const qreal y) {
    mXAnimator->incCurrentBaseValue(x);
    mYAnimator->incCurrentBaseValue(y);
}

void QPointFAnimator::incAllBaseValues(const qreal x, const qreal y) {
    mXAnimator->incAllValues(x);
    mYAnimator->incAllValues(y);
}

void QPointFAnimator::incSavedValueToCurrentValue(const qreal incXBy,
                                                  const qreal incYBy) {
    mXAnimator->incSavedValueToCurrentValue(incXBy);
    mYAnimator->incSavedValueToCurrentValue(incYBy);
}

QrealAnimator *QPointFAnimator::getXAnimator() {
    return mXAnimator.data();
}

QrealAnimator *QPointFAnimator::getYAnimator() {
    return mYAnimator.data();
}

void QPointFAnimator::multSavedValueToCurrentValue(const qreal sx,
                                                   const qreal sy) {
    mXAnimator->multSavedValueToCurrentValue(sx);
    mYAnimator->multSavedValueToCurrentValue(sy);
}

void QPointFAnimator::multCurrentValues(const qreal sx, const qreal sy) {
    mXAnimator->multCurrentBaseValue(sx);
    mYAnimator->multCurrentBaseValue(sy);
}

QPointF QPointFAnimator::getSavedValue() {
    return QPointF(mXAnimator->getSavedBaseValue(),
                   mYAnimator->getSavedBaseValue());
}

qreal QPointFAnimator::getSavedXValue() {
    return mXAnimator->getSavedBaseValue();
}

qreal QPointFAnimator::getSavedYValue() {
    return mYAnimator->getSavedBaseValue();
}

void QPointFAnimator::applyTransform(const QMatrix &transform) {
    mXAnimator->anim_coordinateKeysWith(mYAnimator.get());
    const auto& xKeys = mXAnimator->anim_getKeys();
    const auto& yKeys = mYAnimator->anim_getKeys();
    Q_ASSERT(xKeys.count() == yKeys.count());
    const int nKeys = xKeys.count();
    if(nKeys == 0) {
        prp_startTransform();
        setBaseValue(transform.map(getEffectiveValue()));
        prp_finishTransform();
    } else {
        for(int i = 0; i < nKeys; i++) {
            const auto xKey = xKeys.atId<QrealKey>(i);
            const auto yKey = yKeys.atId<QrealKey>(i);
            const QPointF oldValue(xKey->getValue(), yKey->getValue());
            const QPointF newValue = transform.map(oldValue);
            xKey->startValueTransform();
            yKey->startValueTransform();
            xKey->setValue(newValue.x());
            yKey->setValue(newValue.y());
            xKey->finishValueTransform();
            yKey->finishValueTransform();
        }
    }
}

void QPointFAnimator::saveQPointFSVG(SvgExporter& exp,
                                     QDomElement& parent,
                                     const FrameRange& visRange,
                                     const QString& name,
                                     const bool transform,
                                     const QString& type) const {
    Animator::saveSVG(exp, parent, visRange, name,
                      [this](const qreal relFrame) {
        const auto value = getEffectiveValue(relFrame);
        return QString::number(value.x()) + " " +
               QString::number(value.y());
    }, transform, type);
}

void QPointFAnimator::saveQPointFSVGX(SvgExporter& exp,
                                      QDomElement& parent,
                                      const FrameRange& visRange,
                                      const QString& name,
                                      const qreal y,
                                      const qreal multiplier,
                                      const bool transform,
                                      const QString& type,
                                      const QString &beginEvent,
                                      const QString &endEvent,
                                      const bool motion,
                                      const bool motionRotate,
                                      const QString &motionPath) const
{
    const QString templ = "%1 " + QString::number(y);
    mXAnimator->saveQrealSVG(exp,
                             parent,
                             visRange,
                             name,
                             multiplier,
                             transform,
                             type,
                             templ,
                             beginEvent,
                             endEvent,
                             motion,
                             motionRotate,
                             motionPath);
}

void QPointFAnimator::saveQPointFSVGY(SvgExporter& exp,
                                      QDomElement& parent,
                                      const FrameRange& visRange,
                                      const QString& name,
                                      const qreal x,
                                      const qreal multiplier,
                                      const bool transform,
                                      const QString& type,
                                      const QString &beginEvent,
                                      const QString &endEvent,
                                      const bool motion,
                                      const bool motionRotate,
                                      const QString &motionPath) const
{
    const QString templ = QString::number(x) + " %1";
    mYAnimator->saveQrealSVG(exp,
                             parent,
                             visRange,
                             name,
                             multiplier,
                             transform,
                             type,
                             templ,
                             beginEvent,
                             endEvent,
                             motion,
                             motionRotate,
                             motionPath);
}

QDomElement QPointFAnimator::prp_writePropertyXEV_impl(const XevExporter& exp) const {
    auto result = exp.createElement("Vec2");

    auto x = mXAnimator->prp_writeNamedPropertyXEV("X", exp);
    result.appendChild(x);

    auto y = mYAnimator->prp_writeNamedPropertyXEV("Y", exp);
    result.appendChild(y);

    return result;
}
