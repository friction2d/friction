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

#ifndef EXPRESSION_H
#define EXPRESSION_H

#include <QObject>
#include <QJSEngine>

#include "propertybindingparser.h"

class CORE_EXPORT Expression : public QObject {
    Q_OBJECT
    Expression(const QString& definitionsStr,
               const QString& scriptStr,
               PropertyBindingMap&& bindings,
               std::unique_ptr<QJSEngine>&& engine,
               QJSValue&& eEvaluate);
public:
    static void sAddDefinitionsTo(const QString& definitionsStr,
                                  QJSEngine& e);
    using ResultTester = std::function<void(const QJSValue&)>;
    static void sAddScriptTo(const QString& scriptStr,
                             const PropertyBindingMap& bindings,
                             QJSEngine& e, QJSValue& eEvaluate,
                             const ResultTester& resultTester);
    static qsptr<Expression> sCreate(const QString& definitionsStr,
                                     const QString& scriptStr,
                                     PropertyBindingMap&& bindings,
                                     std::unique_ptr<QJSEngine>&& engine,
                                     QJSValue&& eEvaluate);
    static qsptr<Expression> sCreate(const QString& bindingsStr,
                                     const QString& definitionsStr,
                                     const QString& scriptStr,
                                     const Property* const context,
                                     const ResultTester& resultTester);

    static ResultTester sQrealAnimatorTester;
    static ResultTester sQStringAnimatorTester;

    bool setAbsFrame(const int absFrame);

    bool isStatic() const;
    bool isValid();
    bool dependsOn(const Property* const prop);

    QJSValue evaluate();
    QJSValue evaluate(const qreal relFrame);

    int nextDifferentRelFrame(const int absFrame) const
    { return identicalRelRange(absFrame).adjusted(0, 1).fMax; }
    int prevDifferentRelFrame(const int absFrame) const
    { return identicalRelRange(absFrame).adjusted(-1, 0).fMin; }
    FrameRange identicalRelRange(const int absFrame) const;
    FrameRange nextNonUnaryIdenticalRelRange(const int absFrame) const;

    QString bindingsString() const;
    const QString& definitionsString() const { return mDefinitionsStr; }
    const QString& scriptString() const { return mScriptStr; }
signals:
    void relRangeChanged(const FrameRange& range);
    void currentValueChanged();
private:
    const QString mDefinitionsStr;
    const QString mScriptStr;

    QJSValue mEEvaluate;
    const PropertyBindingMap mBindings;
    const std::unique_ptr<QJSEngine> mEngine;
};

#endif // EXPRESSION_H
