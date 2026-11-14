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
*/

#include "expressioncontext.h"

#include <QJSEngine>
#include <QtGlobal>

ExpressionContext::ExpressionContext(
        const Property* const context,
        const PropertyBindingMap& bindings,
        QJSEngine* const engine) :
    mContext(context),
    mBindings(bindings),
    mEngine(engine) {}

QJSValue ExpressionContext::valueAtTime(const qreal time) {
    markTemporalSamplingUsed();
    if(!mContext) {
        return throwReferenceError(
                    "valueAtTime() has no expression property context");
    }

    qreal absFrame;
    if(!absFrameAtTime(time, absFrame)) return QJSValue();
    const qreal relFrame = mContext->prp_absFrameToRelFrameF(absFrame);
    return mContext->prp_getBaseJSValue(*mEngine, relFrame);
}

QJSValue ExpressionContext::bindingValueAtTime(
        const QString& bindingName, const qreal time) {
    markTemporalSamplingUsed();
    const auto binding = mBindings.find(bindingName);
    if(binding == mBindings.end()) {
        return throwReferenceError(
                    "valueAtTime(): unknown binding '" + bindingName + "'");
    }

    qreal absFrame;
    if(!absFrameAtTime(time, absFrame)) return QJSValue();
    return binding->second->getJSValueAtAbsFrame(*mEngine, absFrame);
}

qreal ExpressionContext::contextRelFrame(const qreal absFrame) const {
    if(!mContext) return absFrame;
    return mContext->prp_absFrameToRelFrameF(absFrame);
}

bool ExpressionContext::absFrameAtTime(const qreal time, qreal& absFrame) {
    if(!qIsFinite(time)) {
        throwTypeError("valueAtTime(): time must be a finite number");
        return false;
    }
    if(!mContext) {
        throwReferenceError(
                    "valueAtTime() has no expression property context");
        return false;
    }

    const qreal fps = mContext->prp_getSceneFPS();
    if(!qIsFinite(fps) || fps <= 0) {
        throwTypeError("valueAtTime(): scene FPS must be greater than zero");
        return false;
    }

    absFrame = time*fps;
    if(!qIsFinite(absFrame) ||
       absFrame <= FrameRange::EMIN || absFrame >= FrameRange::EMAX) {
        throwRangeError("valueAtTime(): sampled frame is outside the valid range");
        return false;
    }
    return true;
}

QJSValue ExpressionContext::throwTypeError(const QString& message) {
    mEngine->throwError(QJSValue::TypeError, message);
    return QJSValue();
}

QJSValue ExpressionContext::throwRangeError(const QString& message) {
    mEngine->throwError(QJSValue::RangeError, message);
    return QJSValue();
}

QJSValue ExpressionContext::throwReferenceError(const QString& message) {
    mEngine->throwError(QJSValue::ReferenceError, message);
    return QJSValue();
}

void ExpressionContext::markTemporalSamplingUsed() {
    if(mUsesTemporalSampling) return;
    mUsesTemporalSampling = true;
    emit temporalSamplingUsed();
}
