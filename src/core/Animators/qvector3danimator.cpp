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

#include "qvector3danimator.h"

#include <QVector3D>

#include "qrealanimator.h"
#include "qrealkey.h"
#include "svgexporter.h"
#include "clipboardcontainer.h"

QVector3DAnimator::QVector3DAnimator(const QString &name) :
    StaticComplexAnimator(name) {
    mXAnimator = enve::make_shared<QrealAnimator>("x");
    mYAnimator = enve::make_shared<QrealAnimator>("y");
    mZAnimator = enve::make_shared<QrealAnimator>("z");
    ca_addChild(mXAnimator);
    ca_addChild(mYAnimator);
    ca_addChild(mZAnimator);
}

QVector3DAnimator::QVector3DAnimator(const QVector3D &iniValue,
                                 const QVector3D &minValue,
                                 const QVector3D &maxValue,
                                 const QVector3D &valueStep,
                                 const QString &name) :
    QVector3DAnimator(name) {
    mXAnimator->setValueRange(minValue.x(), maxValue.x());
    mYAnimator->setValueRange(minValue.y(), maxValue.y());
    mZAnimator->setValueRange(minValue.z(), maxValue.z());

    mXAnimator->setPrefferedValueStep(valueStep.x());
    mYAnimator->setPrefferedValueStep(valueStep.y());
    mZAnimator->setPrefferedValueStep(valueStep.z());

    setBaseValue(iniValue);
}

QVector3DAnimator::QVector3DAnimator(const QVector3D &iniValue,
                                 const QVector3D &minValue,
                                 const QVector3D &maxValue,
                                 const QVector3D &valueStep,
                                 const QString& nameX,
                                 const QString& nameY,
                                 const QString& nameZ,
                                 const QString &name) :
    QVector3DAnimator(iniValue, minValue, maxValue, valueStep, name) {
    mXAnimator->prp_setName(nameX);
    mYAnimator->prp_setName(nameY);
    mZAnimator->prp_setName(nameZ);
}

QJSValue toArray(QJSEngine& e, const QVector3D& value) {
    auto array = e.newArray(2);
    array.setProperty(0, value.x());
    array.setProperty(1, value.y());
    array.setProperty(2, value.z());
    return array;
}

QJSValue QVector3DAnimator::prp_getBaseJSValue(QJSEngine& e) const {
    return toArray(e, getBaseValue());
}

QJSValue QVector3DAnimator::prp_getBaseJSValue(QJSEngine& e, const qreal relFrame) const {
    return toArray(e, getBaseValue(relFrame));
}

QJSValue QVector3DAnimator::prp_getEffectiveJSValue(QJSEngine& e) const {
    return toArray(e, getEffectiveValue());
}

QJSValue QVector3DAnimator::prp_getEffectiveJSValue(QJSEngine& e, const qreal relFrame) const {
    return toArray(e, getEffectiveValue(relFrame));
}

QVector3D QVector3DAnimator::getBaseValue() const {
    return QVector3D(mXAnimator->getCurrentBaseValue(),
                     mYAnimator->getCurrentBaseValue(),
                     mZAnimator->getCurrentBaseValue());
}

QVector3D QVector3DAnimator::getEffectiveValue() const {
    return QVector3D(mXAnimator->getEffectiveValue(),
                     mYAnimator->getEffectiveValue(),
                     mZAnimator->getEffectiveValue());
}

QVector3D QVector3DAnimator::getBaseValueAtAbsFrame(const qreal frame) const {
    return getBaseValue(prp_absFrameToRelFrameF(frame));
}

QVector3D QVector3DAnimator::getBaseValue(const qreal relFrame) const {
    return QVector3D(mXAnimator->getBaseValue(relFrame),
                     mYAnimator->getBaseValue(relFrame),
                     mZAnimator->getBaseValue(relFrame));
}

QVector3D QVector3DAnimator::getEffectiveValueAtAbsFrame(const qreal frame) const {
    return getEffectiveValue(prp_absFrameToRelFrameF(frame));
}

QVector3D QVector3DAnimator::getEffectiveValue(const qreal relFrame) const {
    return QVector3D(mXAnimator->getEffectiveValue(relFrame),
                     mYAnimator->getEffectiveValue(relFrame),
                     mZAnimator->getEffectiveValue(relFrame));
}

void QVector3DAnimator::setPrefferedValueStep(const qreal valueStep) {
    mXAnimator->setPrefferedValueStep(valueStep);
    mYAnimator->setPrefferedValueStep(valueStep);
    mZAnimator->setPrefferedValueStep(valueStep);
}

bool QVector3DAnimator::getBeingTransformed() {
    return mXAnimator->getBeingTransformed() ||
            mYAnimator->getBeingTransformed() ||
            mZAnimator->getBeingTransformed();
}

qreal QVector3DAnimator::getEffectiveXValue() {
    return mXAnimator->getEffectiveValue();
}

qreal QVector3DAnimator::getEffectiveYValue() {
    return mYAnimator->getEffectiveValue();
}

qreal QVector3DAnimator::getEffectiveZValue() {
    return mZAnimator->getEffectiveValue();
}

qreal QVector3DAnimator::getEffectiveXValue(const qreal relFrame) {
    return mXAnimator->getEffectiveValue(relFrame);
}

qreal QVector3DAnimator::getEffectiveYValue(const qreal relFrame) {
    return mYAnimator->getEffectiveValue(relFrame);
}

qreal QVector3DAnimator::getEffectiveZValue(const qreal relFrame) {
    return mZAnimator->getEffectiveValue(relFrame);
}

void QVector3DAnimator::setBaseValue(const QVector3D &val) {
    mXAnimator->setCurrentBaseValue(val.x());
    mYAnimator->setCurrentBaseValue(val.y());
    mZAnimator->setCurrentBaseValue(val.z());
}

void QVector3DAnimator::setValuesRange(const qreal minVal,
                                     const qreal maxVal) {
    mXAnimator->setValueRange(minVal, maxVal);
    mYAnimator->setValueRange(minVal, maxVal);
    mZAnimator->setValueRange(minVal, maxVal);
}

void QVector3DAnimator::setBaseValueWithoutCallingUpdater(
        const QVector3D &val) {
    mXAnimator->setCurrentBaseValueNoUpdate(val.x());
    mYAnimator->setCurrentBaseValueNoUpdate(val.y());
    mZAnimator->setCurrentBaseValueNoUpdate(val.z());
}

void QVector3DAnimator::incBaseValuesWithoutCallingUpdater(
        const qreal x, const qreal y) {
    mXAnimator->incCurrentValueNoUpdate(x);
    mYAnimator->incCurrentValueNoUpdate(y);
}

void QVector3DAnimator::incBaseValuesWithoutCallingUpdater(
        const qreal x, const qreal y, const qreal z) {
    mXAnimator->incCurrentValueNoUpdate(x);
    mYAnimator->incCurrentValueNoUpdate(y);
    mZAnimator->incCurrentValueNoUpdate(z);
}

void QVector3DAnimator::incBaseValues(const qreal x, const qreal y) {
    mXAnimator->incCurrentBaseValue(x);
    mYAnimator->incCurrentBaseValue(y);
}

void QVector3DAnimator::incBaseValues(const qreal x, const qreal y, const qreal z) {
    mXAnimator->incCurrentBaseValue(x);
    mYAnimator->incCurrentBaseValue(y);
    mZAnimator->incCurrentBaseValue(z);
}

void QVector3DAnimator::incAllBaseValues(const qreal x, const qreal y) {
    mXAnimator->incAllValues(x);
    mYAnimator->incAllValues(y);
}

void QVector3DAnimator::incAllBaseValues(const qreal x, const qreal y, const qreal z) {
    mXAnimator->incAllValues(x);
    mYAnimator->incAllValues(y);
    mZAnimator->incAllValues(z);
}

void QVector3DAnimator::incSavedValueToCurrentValue(const qreal incXBy,
                                                  const qreal incYBy) {
    mXAnimator->incSavedValueToCurrentValue(incXBy);
    mYAnimator->incSavedValueToCurrentValue(incYBy);
}

void QVector3DAnimator::incSavedValueToCurrentValue(const qreal incXBy,
                                                  const qreal incYBy,
                                                  const qreal incZBy) {
    mXAnimator->incSavedValueToCurrentValue(incXBy);
    mYAnimator->incSavedValueToCurrentValue(incYBy);
    mZAnimator->incSavedValueToCurrentValue(incZBy);
}

QrealAnimator *QVector3DAnimator::getXAnimator() {
    return mXAnimator.data();
}

QrealAnimator *QVector3DAnimator::getYAnimator() {
    return mYAnimator.data();
}

QrealAnimator *QVector3DAnimator::getZAnimator() {
    return mZAnimator.data();
}

void QVector3DAnimator::multSavedValueToCurrentValue(const qreal sx,
                                                   const qreal sy) { // TODO
    mXAnimator->multSavedValueToCurrentValue(sx);
    mYAnimator->multSavedValueToCurrentValue(sy);
}

void QVector3DAnimator::multSavedValueToCurrentValue(const qreal sx,
                                                   const qreal sy,
                                                   const qreal sz) {
    mXAnimator->multSavedValueToCurrentValue(sx);
    mYAnimator->multSavedValueToCurrentValue(sy);
    mZAnimator->multSavedValueToCurrentValue(sz);
}

void QVector3DAnimator::multCurrentValues(const qreal sx, const qreal sy, const qreal sz) {
    mXAnimator->multCurrentBaseValue(sx);
    mYAnimator->multCurrentBaseValue(sy);
    mZAnimator->multCurrentBaseValue(sz);
}

QVector3D QVector3DAnimator::getSavedValue() {
    return QVector3D(mXAnimator->getSavedBaseValue(),
                     mYAnimator->getSavedBaseValue(),
                     mZAnimator->getSavedBaseValue());
}

qreal QVector3DAnimator::getSavedXValue() {
    return mXAnimator->getSavedBaseValue();
}

qreal QVector3DAnimator::getSavedYValue() {
    return mYAnimator->getSavedBaseValue();
}

qreal QVector3DAnimator::getSavedZValue() {
    return mZAnimator->getSavedBaseValue();
}

void QVector3DAnimator::applyTransform(const QMatrix4x4 &transform) {
    mXAnimator->anim_coordinateKeysWith(mYAnimator.get());
    const auto& xKeys = mXAnimator->anim_getKeys();
    const auto& yKeys = mYAnimator->anim_getKeys();
    const auto& zKeys = mZAnimator->anim_getKeys();
    Q_ASSERT(xKeys.count() == yKeys.count() == zKeys.count());
    const int nKeys = xKeys.count();
    if(nKeys == 0) {
        prp_startTransform();
        setBaseValue(transform.map(getEffectiveValue()));
        prp_finishTransform();
    } else {
        for(int i = 0; i < nKeys; i++) {
            const auto xKey = xKeys.atId<QrealKey>(i);
            const auto yKey = yKeys.atId<QrealKey>(i);
            const auto zKey = zKeys.atId<QrealKey>(i);
            const QVector3D oldValue(xKey->getValue(), yKey->getValue(), zKey->getValue());
            const QVector3D newValue = transform.map(oldValue);

            xKey->startValueTransform();
            yKey->startValueTransform();
            zKey->startValueTransform();

            xKey->setValue(newValue.x());
            yKey->setValue(newValue.y());
            zKey->setValue(newValue.z());

            xKey->finishValueTransform();
            yKey->finishValueTransform();
            zKey->finishValueTransform();
        }
    }
}

void QVector3DAnimator::saveQVector3DSVG(SvgExporter& exp,
                                     QDomElement& parent,
                                     const FrameRange& visRange,
                                     const QString& name,
                                     const bool transform,
                                     const QString& type) const { // TODO: add suport for z prop
    Animator::saveSVG(exp, parent, visRange, name,
                      [this](const int relFrame) {
        const auto value = getEffectiveValue(relFrame);
        return QString::number(value.x()) + " " +
               QString::number(value.y());
    }, transform, type);
}

void QVector3DAnimator::saveQVector3DSVGX(SvgExporter& exp,
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

void QVector3DAnimator::saveQVector3DSVGY(SvgExporter& exp,
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

// Writes the properties to the Timeline widget
QDomElement QVector3DAnimator::prp_writePropertyXEV_impl(const XevExporter& exp) const {
    auto result = exp.createElement("Vec2");

    auto x = mXAnimator->prp_writeNamedPropertyXEV("X", exp);
    result.appendChild(x);

    auto y = mYAnimator->prp_writeNamedPropertyXEV("Y", exp);
    result.appendChild(y);

    auto z = mZAnimator->prp_writeNamedPropertyXEV("Z", exp);
    result.appendChild(z);

    return result;
}
