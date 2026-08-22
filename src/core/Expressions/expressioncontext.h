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

#ifndef EXPRESSIONCONTEXT_H
#define EXPRESSIONCONTEXT_H

#include <QObject>
#include <QJSValue>

#include "propertybindingparser.h"

class QJSEngine;

class ExpressionContext : public QObject {
    Q_OBJECT
public:
    ExpressionContext(const Property* context,
                      const PropertyBindingMap& bindings,
                      QJSEngine* engine);

    Q_INVOKABLE QJSValue valueAtTime(qreal time);
    Q_INVOKABLE QJSValue bindingValueAtTime(const QString& bindingName,
                                            qreal time);

    bool usesTemporalSampling() const { return mUsesTemporalSampling; }
    qreal contextRelFrame(const qreal absFrame) const;

signals:
    void temporalSamplingUsed();

private:
    bool absFrameAtTime(qreal time, qreal& absFrame);
    QJSValue throwTypeError(const QString& message);
    QJSValue throwRangeError(const QString& message);
    QJSValue throwReferenceError(const QString& message);
    void markTemporalSamplingUsed();

    const qptr<const Property> mContext;
    const PropertyBindingMap mBindings;
    QJSEngine* const mEngine;
    bool mUsesTemporalSampling = false;
};

#endif // EXPRESSIONCONTEXT_H
