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

#include "timebinding.h"

#include <QtGlobal>

TimeBinding::TimeBinding(const Property* const context) :
    PropertyBindingBase(context) {}

qsptr<TimeBinding> TimeBinding::sCreate(const Property* const context) {
    return qsptr<TimeBinding>(new TimeBinding(context));
}

QJSValue TimeBinding::getJSValue(QJSEngine& e) {
    Q_UNUSED(e)
    return valueForRelFrame(relFrame());
}

QJSValue TimeBinding::getJSValue(QJSEngine& e, const qreal relFrame) {
    Q_UNUSED(e)
    return valueForRelFrame(relFrame);
}

FrameRange TimeBinding::identicalRelRange(const int absFrame) {
    if(mContext) {
        const int relFrame = mContext->prp_absFrameToRelFrame(absFrame);
        return {relFrame, relFrame};
    }
    return FrameRange::EMINMAX;
}

FrameRange TimeBinding::nextNonUnaryIdenticalRelRange(const int absFrame) {
    Q_UNUSED(absFrame)
    if(mContext) return {FrameRange::EMAX/2, FrameRange::EMAX};
    return FrameRange::EMINMAX;
}

QJSValue TimeBinding::valueForRelFrame(const qreal relFrame) const {
    if(!mContext) return QJSValue::NullValue;
    const qreal fps = mContext->prp_getSceneFPS();
    if(!qIsFinite(fps) || fps <= 0) return QJSValue::NullValue;
    const qreal absFrame = mContext->prp_relFrameToAbsFrameF(relFrame);
    return absFrame/fps;
}
