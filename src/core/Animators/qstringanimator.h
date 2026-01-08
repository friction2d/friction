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

#ifndef QSTRINGANIMATOR_H
#define QSTRINGANIMATOR_H
#include "Animators/steppedanimator.h"
#include "../conncontextptr.h"

typedef KeyT<QString> QStringKey;

class Expression;

class CORE_EXPORT QStringAnimator : public SteppedAnimator<QString> {
    e_OBJECT
protected:
    QStringAnimator(const QString& name);

    void prp_writeProperty_impl(eWriteStream& dst) const override;
    void prp_readProperty_impl(eReadStream& src) override;

    void prp_readPropertyXEV_impl(const QDomElement& ele, const XevImporter& imp) override;
    QDomElement prp_writePropertyXEV_impl(const XevExporter& exp) const override;
public:
    using PropSetter = std::function<void(QDomElement&)>;
    void saveSVG(SvgExporter& exp, QDomElement& parent,
                 const PropSetter& propSetter) const;

    QJSValue prp_getBaseJSValue(QJSEngine& e) const override;
    QJSValue prp_getBaseJSValue(QJSEngine& e, const qreal relFrame) const override;
    QJSValue prp_getEffectiveJSValue(QJSEngine& e) const override;
    QJSValue prp_getEffectiveJSValue(QJSEngine& e, const qreal relFrame) const override;

    void prp_setupTreeViewMenu(PropertyMenu * const menu) override;

    FrameRange prp_getIdenticalRelRange(const int relFrame) const override;
    FrameRange prp_nextNonUnaryIdenticalRelRange(const int relFrame) const override;
    void prp_afterFrameShiftChanged(const FrameRange& oldAbsRange,
                                    const FrameRange& newAbsRange) override;
    void anim_setAbsFrame(const int frame) override;

    bool prp_dependsOn(const Property* const prop) const override;
    bool hasValidExpression() const;
    bool hasExpression() const { return mExpression; }
    void clearExpressionAction() { setExpressionAction(nullptr); }

    QString getExpressionBindingsString() const;
    QString getExpressionDefinitionsString() const;
    QString getExpressionScriptString() const;

    void setExpression(const qsptr<Expression>& expression);
    void setExpressionAction(const qsptr<Expression>& expression);
    void applyExpression(const FrameRange& relRange, const bool action);

    QString getValueAtRelFrame(const qreal frame) const;
private:
    QString getBaseValueAtRelFrame(const qreal frame) const;
    QString getEffectiveValue(const qreal relFrame) const;
    bool updateExpressionRelFrame();
    bool updateCurrentEffectiveValue();

    QString mCurrentEffectiveValue;
    ConnContextQSPtr<Expression> mExpression;
};

#endif // QSTRINGANIMATOR_H
