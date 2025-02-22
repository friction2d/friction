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

#ifndef VALUEANIMATORS_H
#define VALUEANIMATORS_H

#include "graphanimator.h"
#include "qrealsnapshot.h"
#include "../conncontextptr.h"
#include "qpointfanimator.h"
#include "qvector3danimator.h"

class QrealKey;
class Expression;

class CORE_EXPORT QrealAnimator :  public GraphAnimator {
    Q_OBJECT
    e_OBJECT
    friend class QPointFAnimator;
    friend class QVector3DAnimator;
protected:
    QrealAnimator(const QString& name);
    QrealAnimator(const qreal iniVal,
                  const qreal minVal,
                  const qreal maxVal,
                  const qreal prefferdStep,
                  const QString& name);

    QDomElement prp_writePropertyXEV_impl(const XevExporter& exp) const;
    void prp_readPropertyXEV_impl(const QDomElement& ele, const XevImporter& imp);
public:
    QJSValue prp_getBaseJSValue(QJSEngine& e) const {
        Q_UNUSED(e)
        return getCurrentBaseValue();
    }

    QJSValue prp_getBaseJSValue(QJSEngine& e, const qreal relFrame) const {
        Q_UNUSED(e)
        return getBaseValue(relFrame);
    }

    QJSValue prp_getEffectiveJSValue(QJSEngine& e) const {
        Q_UNUSED(e)
        return getEffectiveValue();
    }

    QJSValue prp_getEffectiveJSValue(QJSEngine& e, const qreal relFrame) const {
        Q_UNUSED(e)
        return getEffectiveValue(relFrame);
    }

    void prp_setupTreeViewMenu(PropertyMenu * const menu);

    void prp_startTransform();
    void prp_finishTransform();
    void prp_cancelTransform();

    FrameRange prp_getIdenticalRelRange(const int relFrame) const;
    FrameRange prp_nextNonUnaryIdenticalRelRange(const int relFrame) const;

    void prp_afterChangedAbsRange(const FrameRange& range,
                                  const bool clip = true);
    void prp_afterFrameShiftChanged(const FrameRange &oldAbsRange,
                                    const FrameRange &newAbsRange);

    void anim_setAbsFrame(const int frame);
    void anim_removeAllKeys();
    void anim_addKeyAtRelFrame(const int relFrame);

    QPainterPath graph_getPathForSegment(
            const GraphKey * const prevKey,
            const GraphKey * const nextKey) const;

    qValueRange graph_getMinAndMaxValues() const;
    qValueRange graph_getMinAndMaxValuesBetweenFrames(
            const int startFrame, const int endFrame) const;

    qreal graph_clampGraphValue(const qreal value);

    void prp_writeProperty_impl(eWriteStream& dst) const;
    void prp_readProperty_impl(eReadStream& src);
    stdsptr<Key> anim_createKey();
protected:
    void graph_getValueConstraints(GraphKey *key, const QrealPointType type,
                                   qreal &minMoveValue, qreal &maxMoveValue) const;
public:
    QrealSnapshot makeSnapshot(const qreal frameMultiplier = 1,
                          const qreal valueMultiplier = 1) const;
    QrealSnapshot makeSnapshot(const int minFrame, const int maxFrame,
                          const qreal frameMultiplier = 1,
                          const qreal valueMultiplier = 1) const;

    void setPrefferedValueStep(const qreal valueStep);

    void setValueRange(const qreal minVal, const qreal maxVal);
    void setMinValue(const qreal minVal);
    void setMaxValue(const qreal maxVal);

    void setCurrentBaseValue(const qreal newValue);
    void setCurrentBaseValueNoUpdate(const qreal newValue);
    void incCurrentBaseValue(const qreal incBy);
    void incCurrentValueNoUpdate(const qreal incBy);
    void multCurrentBaseValue(const qreal mult);

    qreal getCurrentBaseValue() const;
    qreal getEffectiveValue() const;
    qreal getBaseValue(const qreal relFrame) const;
    qreal getBaseValueAtAbsFrame(const qreal frame) const;
    qreal getEffectiveValue(const qreal relFrame) const;
    qreal getEffectiveValueAtAbsFrame(const qreal frame) const;

    qreal getSavedBaseValue();
    void incAllValues(const qreal valInc);

    qreal clamped(const qreal value) const
    { return qBound(mClampMin, value, mClampMax); }

    qreal getMinPossibleValue();
    qreal getMaxPossibleValue();

    qreal getPrefferedValueStep();

    void graphFixMinMaxValues();

    void saveValueToKey(const int frame, const qreal value);
    void removeThisFromGraphAnimator();

    void incSavedValueToCurrentValue(const qreal incBy);
    void multSavedValueToCurrentValue(const qreal multBy);

    int getNumberDecimals() { return mDecimals; }
    void setNumberDecimals(const int decimals) { mDecimals = decimals; }

    bool getBeingTransformed() { return mTransformed; }

    static auto sCreate0to1Animator(const QString& name) {
        auto anim = enve::make_shared<QrealAnimator>(0, 0, 1, 0.01, name);
        anim->graphFixMinMaxValues();
        return anim;
    }

    bool prp_dependsOn(const Property* const prop) const;
    bool hasValidExpression() const;
    bool hasExpression() const { return mExpression; }
    void clearExpressionAction() { setExpressionAction(nullptr); }

    QString getExpressionBindingsString() const;
    QString getExpressionDefinitionsString() const;
    QString getExpressionScriptString() const;

    void setExpression(const qsptr<Expression>& expression);
    void setExpressionAction(const qsptr<Expression>& expression);
    void setExpressionEasingAction(const qsptr<Expression>& expression);
    void applyExpression(const FrameRange& relRange,
                         const qreal accuracy,
                         const bool action,
                         const bool easing = false);

    void saveQrealSVG(SvgExporter& exp,
                      QDomElement& parent,
                      const FrameRange& visRange,
                      const QString& attrName,
                      const qreal multiplier = 1.,
                      const bool transform = false,
                      const QString& type = "",
                      const QString& templ = "%1",
                      const QString& beginEvent = "",
                      const QString& endEvent = "",
                      const bool motion = false,
                      const bool motionRotate = false,
                      const QString & motionPath = QString());
    using Mangler = std::function<qreal(qreal)>;
    void saveQrealSVG(SvgExporter& exp,
                      QDomElement& parent,
                      const FrameRange& visRange,
                      const QString& attrName,
                      const Mangler& mangler,
                      const bool transform = false,
                      const QString& type = "",
                      const QString& templ = "%1",
                      const QString& beginEvent = "",
                      const QString& endEvent = "",
                      const bool motion = false,
                      const bool motionRotate = false,
                      const QString & motionPath = QString());
private:
    qreal calculateBaseValueAtRelFrame(const qreal frame) const;
    void startBaseValueTransform();
    void finishBaseValueTransform();
    bool updateExpressionRelFrame();
    bool updateCurrentBaseValue();
    bool updateCurrentEffectiveValue();
    bool assignCurrentBaseValue(const qreal newValue);

    void applyExpressionSub(const FrameRange& relRange,
                            const int sampleInc,
                            const bool action,
                            const qreal accuracy);

    bool mGraphMinMaxValuesFixed = false;
    bool mTransformed = false;

    int mDecimals = 3;

    qreal mClampMax = TEN_MIL;
    qreal mClampMin = -TEN_MIL;

    qreal mCurrentEffectiveValue = 0;
    qreal mCurrentBaseValue = 0;
    qreal mSavedCurrentValue = 0;

    ConnContextQSPtr<Expression> mExpression;

    qreal mPrefferedValueStep = 1;
signals:
    void expressionChanged();
    void effectiveValueChanged(qreal);
    void baseValueChanged(qreal);
};

class CORE_EXPORT QrealAction {
    enum Type { START, SET, FINISH, CANCEL };
    QrealAction(const qreal value, const Type type) :
        mValue(value), mType(type) {}
public:
    void apply(QrealAnimator* const target) const {
        if(mType == START) target->prp_startTransform();
        else if(mType == SET) target->setCurrentBaseValue(mValue);
        else if(mType == FINISH) target->prp_finishTransform();
        else if(mType == CANCEL) target->prp_cancelTransform();
    }

    static QrealAction sMakeStart()
    { return QrealAction{0., START}; }
    static QrealAction sMakeSet(const qreal value)
    { return QrealAction{value, SET}; }
    static QrealAction sMakeFinish()
    { return QrealAction{0., FINISH}; }
    static QrealAction sMakeCancel()
    { return QrealAction{0., CANCEL}; }
private:
    qreal mValue;
    Type mType;
};

#endif // VALUEANIMATORS_H
