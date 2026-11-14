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

#ifndef TIMEBINDING_H
#define TIMEBINDING_H

#include "propertybindingbase.h"

class CORE_EXPORT TimeBinding : public PropertyBindingBase {
    TimeBinding(const Property* context);
public:
    static qsptr<TimeBinding> sCreate(const Property* context);

    QJSValue getJSValue(QJSEngine& e);
    QJSValue getJSValue(QJSEngine& e, qreal relFrame);

    FrameRange identicalRelRange(int absFrame);
    FrameRange nextNonUnaryIdenticalRelRange(int absFrame);
    QString path() const { return "$time"; }

private:
    QJSValue valueForRelFrame(qreal relFrame) const;
};

#endif // TIMEBINDING_H
