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

#include "qrealanimator.h"

#include "qrealpoint.h"
#include "qrealkey.h"
#include "../Expressions/expression.h"
#include "../simpletask.h"
#include "typemenu.h"
#include "GUI/dialogsinterface.h"
#include "Segments/fitcurves.h"
#include "svgexporter.h"
#include "Properties/namedproperty.h"

QrealAnimator::QrealAnimator(const qreal iniVal,
                             const qreal minVal,
                             const qreal maxVal,
                             const qreal prefferdStep,
                             const QString &name) :
    GraphAnimator(name) {
    mCurrentBaseValue = iniVal;
    mClampMin = minVal;
    mClampMax = maxVal;
    mPrefferedValueStep = prefferdStep;
}

QrealAnimator::QrealAnimator(const QString &name) : GraphAnimator(name) {}

void QrealAnimator::prp_setupTreeViewMenu(PropertyMenu * const menu) {
    if(menu->hasActionsForType<QrealAnimator>()) return;
    menu->addedActionsForType<QrealAnimator>();

    const PropertyMenu::PlainSelectedOp<QrealAnimator> sOp =
    [](QrealAnimator * aTarget) {
        const auto& iface = DialogsInterface::instance();
        iface.showExpressionDialog(aTarget);
    };
    menu->addPlainAction(QIcon::fromTheme("preferences"), tr("Set Expression"), sOp);

    const PropertyMenu::PlainSelectedOp<QrealAnimator> aOp =
    [](QrealAnimator * aTarget) {
        const auto& iface = DialogsInterface::instance();
        iface.showApplyExpressionDialog(aTarget);
    };
    menu->addPlainAction(QIcon::fromTheme("dialog-ok"), tr("Apply Expression"), aOp)->setEnabled(hasExpression());

    const PropertyMenu::PlainSelectedOp<QrealAnimator> cOp =
    [](QrealAnimator * aTarget) {
        aTarget->clearExpressionAction();
    };
    menu->addPlainAction(QIcon::fromTheme("trash"), tr("Clear Expression"), cOp)->setEnabled(hasExpression());
    menu->addSeparator();
    Animator::prp_setupTreeViewMenu(menu);
}

void QrealAnimator::prp_writeProperty_impl(eWriteStream& dst) const {
    anim_writeKeys(dst);
    dst << mCurrentBaseValue;
    dst << !!mExpression;
    if(mExpression) {
        dst << mExpression->bindingsString();
        dst << mExpression->definitionsString();
        dst << mExpression->scriptString();
    }
}

stdsptr<Key> QrealAnimator::anim_createKey() {
    return enve::make_shared<QrealKey>(this);
}

void QrealAnimator::prp_readProperty_impl(eReadStream& src) {
    anim_readKeys(src);

    qreal val; src >> val;
    const auto evVersion = src.evFileVersion();
    if(evVersion > 8 && evVersion < 15) {
        QString expression; src >> expression;
    } else if(evVersion >= 15) {
        bool hasExpr; src >> hasExpr;
        if(hasExpr) {
            QString bindingsStr; src >> bindingsStr;
            QString definitionsStr; src >> definitionsStr;
            QString scriptStr; src >> scriptStr;
            SimpleTask::sScheduleContexted(this,
            [this, bindingsStr, definitionsStr, scriptStr]() {
                setExpression(Expression::sCreate(
                                  bindingsStr, definitionsStr, scriptStr, this,
                                  Expression::sQrealAnimatorTester));
            });
        }
    }
    if(!anim_hasKeys()) setCurrentBaseValue(val);
}

void QrealAnimator::graph_getValueConstraints(
        GraphKey *key, const QrealPointType type,
        qreal &minMoveValue, qreal &maxMoveValue) const {
    Q_UNUSED(key)
    if(type == QrealPointType::c1Pt) {
        minMoveValue = -DBL_MAX;
        maxMoveValue = DBL_MAX;
//        auto nextKey = key->getNextKey<GraphKey>();
//        if(!nextKey) {
//            minMoveValue = mMinPossibleVal;
//            maxMoveValue = mMaxPossibleVal;
//            return;
//        }
//        qreal p0 = key->getValueForGraph();
//        qreal p2 = nextKey->getC0Value();
//        qreal p3 = nextKey->getValueForGraph();
//        int iMax = 2*(nextKey->getRelFrame() - key->getRelFrame());
//        minMoveValue = DBL_MIN;
//        maxMoveValue = DBL_MAX;
//        for(int i = 1; i < iMax; i++) {
//            qreal t = static_cast<qreal>(i)/iMax;
//            qreal maxVal = gSolveForP1(p0, p2, p3, t, mMaxPossibleVal);
//            maxMoveValue = qMin(maxMoveValue, maxVal);
//            qreal minVal = gSolveForP1(p0, p2, p3, t, mMinPossibleVal);
//            minMoveValue = qMax(minMoveValue, minVal);
//        }
    } else if(type == QrealPointType::c0Pt) {
        minMoveValue = -DBL_MAX;
        maxMoveValue = DBL_MAX;
//        auto prevKey = key->getPrevKey<GraphKey>();
//        if(!prevKey) {
//            minMoveValue = mMinPossibleVal;
//            maxMoveValue = mMaxPossibleVal;
//            return;
//        }
//        qreal p0 = prevKey->getValueForGraph();
//        qreal p1 = prevKey->getC1Value();
//        qreal p3 = key->getValueForGraph();
//        int iMax = 2*(key->getRelFrame() - prevKey->getRelFrame());
//        minMoveValue = DBL_MIN;
//        maxMoveValue = DBL_MAX;
//        for(int i = 1; i < iMax; i++) {
//            qreal t = static_cast<qreal>(i)/iMax;
//            qreal maxVal = gSolveForP2(p0, p1, p3, t, mMaxPossibleVal);
//            maxMoveValue = qMin(maxMoveValue, maxVal);
//            qreal minVal = gSolveForP2(p0, p1, p3, t, mMinPossibleVal);
//            minMoveValue = qMax(minMoveValue, minVal);
//        }
    } else { // keyPt
        minMoveValue = mClampMin;
        maxMoveValue = mClampMax;
    }
}

QrealSnapshot QrealAnimator::makeSnapshot(
        const qreal frameMultiplier,
        const qreal valueMultiplier) const {
    QrealSnapshot snapshot(mCurrentBaseValue, frameMultiplier, valueMultiplier);
    const auto& keys = anim_getKeys();
    for(const auto& key : keys) {
        const auto qaKey = static_cast<QrealKey*>(key);
        snapshot.appendKey(qaKey);
    }
    return snapshot;
}

QrealSnapshot QrealAnimator::makeSnapshot(
        const int minFrame, const int maxFrame,
        const qreal frameMultiplier,
        const qreal valueMultiplier) const {
    const int prevKeyId = anim_getPrevKeyId(minFrame);
    const int nextKeyId = anim_getNextKeyId(maxFrame);
    const auto& keys = anim_getKeys();
    const int minI = clamp(prevKeyId, 0, keys.count() - 1);
    const int maxI = clamp(nextKeyId, 0, keys.count() - 1);
    QrealSnapshot snapshot(mCurrentBaseValue, frameMultiplier, valueMultiplier);
    for(int i = minI; i <= maxI; i++) {
        const auto iKey = keys.atId<QrealKey>(i);
        snapshot.appendKey(iKey);
    }
    return snapshot;
}

void QrealAnimator::setValueRange(const qreal minVal, const qreal maxVal) {
    mClampMin = minVal;
    mClampMax = maxVal;
    setCurrentBaseValue(mCurrentBaseValue);
}

void QrealAnimator::setMinValue(const qreal minVal) {
    mClampMin = minVal;
    setCurrentBaseValue(mCurrentBaseValue);
}

void QrealAnimator::setMaxValue(const qreal maxVal) {
    mClampMax = maxVal;
    setCurrentBaseValue(mCurrentBaseValue);
}

void QrealAnimator::incAllValues(const qreal valInc) {
    const auto& keys = anim_getKeys();
    for(const auto &key : keys) {
        static_cast<QrealKey*>(key)->incValue(valInc);
    }
    incCurrentBaseValue(valInc);
}

qreal QrealAnimator::getMinPossibleValue() {
    return mClampMin;
}

qreal QrealAnimator::getMaxPossibleValue() {
    return mClampMax;
}

qreal QrealAnimator::getPrefferedValueStep() {
    return mPrefferedValueStep;
}

void QrealAnimator::setPrefferedValueStep(const qreal valueStep) {
    mPrefferedValueStep = valueStep;
}

void QrealAnimator::graphFixMinMaxValues() {
    mGraphMinMaxValuesFixed = true;
}

qreal QrealAnimator::getEffectiveValueAtAbsFrame(const qreal frame) const {
    return getEffectiveValue(prp_absFrameToRelFrameF(frame));
}

qreal QrealAnimator::getBaseValueAtAbsFrame(const qreal frame) const {
    return getBaseValue(prp_absFrameToRelFrameF(frame));
}

bool QrealAnimator::prp_dependsOn(const Property* const prop) const {
    if(Property::prp_dependsOn(prop)) return true;
    if(!mExpression) return false;
    return mExpression->dependsOn(prop);
}

bool QrealAnimator::hasValidExpression() const {
    return mExpression ? mExpression->isValid() : false;
}

QString QrealAnimator::getExpressionBindingsString() const {
    if(!mExpression) return "";
    return mExpression->bindingsString();
}

QString QrealAnimator::getExpressionDefinitionsString() const {
    if(!mExpression) return "";
    return mExpression->definitionsString();
}

QString QrealAnimator::getExpressionScriptString() const {
    if(!mExpression) return "";
    return mExpression->scriptString();
}

void QrealAnimator::setExpressionAction(const qsptr<Expression> &expression)
{
    {
        prp_pushUndoRedoName(tr("Change Expression"));
        UndoRedo ur;
        const auto oldValue = mExpression.sptr();
        const auto newValue = expression;
        ur.fUndo = [this, oldValue]() { setExpression(oldValue); };
        ur.fRedo = [this, newValue]() { setExpression(newValue); };
        prp_addUndoRedo(ur);
    }
    setExpression(expression);
}

void QrealAnimator::setExpressionEasingAction(const qsptr<Expression> &expression)
{
    {
        prp_pushUndoRedoName(tr("Change Easing"));
        UndoRedo ur;
        ur.fUndo = [this]() { setExpression(nullptr); };
        ur.fRedo = [this, expression]() { setExpression(expression); };
        prp_addUndoRedo(ur);
    }
    setExpression(expression);
}

void QrealAnimator::applyExpressionSub(const FrameRange& relRange,
                                       const int sampleInc,
                                       const bool action,
                                       const qreal accuracy) {
    if(!relRange.isValid()) return;
    const int count = qCeil(relRange.span()/qreal(sampleInc));
    QVector<QPointF> pts;
    pts.reserve(count);

    const qreal frameMultiplier = 100;
    const qreal frameDivider = 1/frameMultiplier;

    qreal valSum = 0;
    for(int i = 0; i < count; i++) {
        const qreal relFrame = relRange.fMin + i*sampleInc;
        const qreal value = mExpression->evaluate(relFrame).toNumber();
        pts << QPointF{relFrame*frameMultiplier, value};
        valSum += qAbs(value);
    }

    anim_removeKeys(relRange, action);

    const qreal valAvg = valSum/count;

    QrealKey* prevKey = nullptr;
    const auto adder = [this, frameDivider, &prevKey, action](
                       const int n, const BezierCurve curve) {
        Q_UNUSED(n)
        const auto qptData = reinterpret_cast<QPointF*>(curve);
        const QPointF& p0 = qptData[0];
        const QPointF& c1 = qptData[1];
        const QPointF& c2 = qptData[2];
        const QPointF& p3 = qptData[3];

        const int frame0 = qRound(p0.x()*frameDivider);
        const qreal frame1 = c1.x()*frameDivider;
        const qreal frame2 = c2.x()*frameDivider;
        const int frame3 = qRound(p3.x()*frameDivider);

        stdsptr<QrealKey> key0Ref;
        QrealKey* key0 = nullptr;
        if(prevKey) {
            key0 = prevKey;
        } else {
            key0Ref = enve::make_shared<QrealKey>(p0.y(), frame0, this);
            if(action) anim_appendKeyAction(key0Ref);
            else anim_appendKey(key0Ref);
            key0 = key0Ref.get();
        }
        const auto key1 = enve::make_shared<QrealKey>(p3.y(), frame3, this);
        if(action) anim_appendKeyAction(key1);
        else anim_appendKey(key1);

        key0->setC1Enabled(true);
        key0->setC1Frame(frame1);
        key0->setC1Value(c1.y());
        key0->guessCtrlsMode();

        key1->setC0Enabled(true);
        key1->setCtrlsMode(CtrlsMode::corner);
        key1->setC0Frame(frame2);
        key1->setC0Value(c2.y());

        prevKey = key1.get();
    };

    FitCurves::FitCurve(pts, 0.01*(0.01 + valAvg/accuracy), adder, true, true);
}

void QrealAnimator::applyExpression(const FrameRange& relRange,
                                    const qreal accuracy,
                                    const bool action,
                                    const bool easing)
{
    qDebug() << "applyExpression" << prp_getName() << relRange.fMin << relRange.fMax << accuracy << action << easing;
    if (!hasValidExpression()) {}
    else if (!relRange.isValid()) {}
    else if (isZero4Dec(accuracy) || accuracy < 0) {}
    else {
        const int sampleInc = qMax(1, qRound(1/accuracy));

        prp_pushUndoRedoName(easing ? tr("Apply Easing") : tr("Apply Expression"));

        const bool isStatic = mExpression->isStatic();
        if (isStatic) {
            const qreal value = mExpression->evaluate(relRange.fMin).toNumber();
            setCurrentBaseValue(value);
        } else {
            const auto absRange = prp_relRangeToAbsRange(relRange);
            QList<FrameRange> ranges;
            for (int i = relRange.fMin; i < relRange.fMax; i++) {
                const int absFrame = absRange.fMin + i - relRange.fMin;
                const auto nextNonUnary = mExpression->nextNonUnaryIdenticalRelRange(absFrame);
                if (!nextNonUnary.inRange(i)) {
                    ranges << relRange*FrameRange{i, nextNonUnary.fMin};
                }
                i = nextNonUnary.fMax;
            }

            for (const auto& range : qAsConst(ranges)) {
                applyExpressionSub(range, sampleInc, action, accuracy);
            }
        }
    }

    if (easing) { setExpressionEasingAction(nullptr); }
    else if (action) { setExpressionAction(nullptr); }
    else { setExpression(nullptr); }
}

void QrealAnimator::setExpression(const qsptr<Expression>& expression) {
    auto& conn = mExpression.assign(expression);
    if(expression) {
        const int absFrame = anim_getCurrentAbsFrame();
        expression->setAbsFrame(absFrame);
        conn << connect(expression.get(), &Expression::currentValueChanged,
                        this, [this]() {
            updateCurrentEffectiveValue();
        });
        conn << connect(expression.get(), &Expression::relRangeChanged,
                        this, [this](const FrameRange& range) {
            prp_afterChangedRelRange(range);
        });
    }
    updateCurrentEffectiveValue();
    prp_afterWholeInfluenceRangeChanged();
    emit expressionChanged();
}

qreal QrealAnimator::calculateBaseValueAtRelFrame(const qreal frame) const {
    if(!anim_hasKeys()) return mCurrentBaseValue;
    const auto pn = anim_getPrevAndNextKeyIdF(frame);
    const int prevId = pn.first;
    const int nextId = pn.second;

    const bool adjKeys = nextId - prevId == 1;
    const auto keyAtRelFrame = adjKeys ? nullptr :
                               anim_getKeyAtIndex<QrealKey>(prevId + 1);
    if(keyAtRelFrame) return keyAtRelFrame->getValue();
    const auto prevKey = anim_getKeyAtIndex<QrealKey>(prevId);
    const auto nextKey = anim_getKeyAtIndex<QrealKey>(nextId);

    if(prevKey && nextKey) {
        const qCubicSegment1D seg{qreal(prevKey->getRelFrame()),
                                  prevKey->getC1Frame(),
                                  nextKey->getC0Frame(),
                                  qreal(nextKey->getRelFrame())};
        const qreal t = gTFromX(seg, frame);
        const qreal p0y = prevKey->getValue();
        const qreal p1y = prevKey->getC1Value();
        const qreal p2y = nextKey->getC0Value();
        const qreal p3y = nextKey->getValue();
        return clamp(gCubicValueAtT({p0y, p1y, p2y, p3y}, t),
                     mClampMin, mClampMax);
    } else if(prevKey) {
        return prevKey->getValue();
    } else if(nextKey) {
        return nextKey->getValue();
    }
    return mCurrentBaseValue;
}

qreal QrealAnimator::getBaseValue(const qreal relFrame) const {
    if(isZero4Dec(relFrame - anim_getCurrentRelFrame()))
        return mCurrentBaseValue;
    return calculateBaseValueAtRelFrame(relFrame);
}

qreal QrealAnimator::getEffectiveValue(const qreal relFrame) const {
    if(isZero4Dec(relFrame - anim_getCurrentRelFrame()))
        return getEffectiveValue();
    if(mExpression) {
        const auto ret = mExpression->evaluate(relFrame);
        if(ret.isNumber()) return clamped(ret.toNumber());
    }
    return getBaseValue(relFrame);
}

qreal QrealAnimator::getCurrentBaseValue() const {
    return mCurrentBaseValue;
}

qreal QrealAnimator::getEffectiveValue() const {
    if(mExpression) return mCurrentEffectiveValue;
    else return mCurrentBaseValue;
}

bool QrealAnimator::assignCurrentBaseValue(const qreal newValue) {
    if(isZero4Dec(newValue - mCurrentBaseValue)) return false;
    mCurrentBaseValue = newValue;
    emit baseValueChanged(mCurrentBaseValue);
    if(mExpression) updateCurrentEffectiveValue();
    else emit effectiveValueChanged(mCurrentBaseValue);
    return true;
}

void QrealAnimator::setCurrentBaseValue(const qreal newValue) {
    if(assignCurrentBaseValue(clamped(newValue))) {
        const auto currKey = anim_getKeyOnCurrentFrame<QrealKey>();
        if(currKey) currKey->setValue(mCurrentBaseValue);
        else prp_afterWholeInfluenceRangeChanged();
    }
}

bool QrealAnimator::updateCurrentBaseValue() {
    const qreal newValue = calculateBaseValueAtRelFrame(anim_getCurrentRelFrame());
    return assignCurrentBaseValue(newValue);
}

bool QrealAnimator::updateCurrentEffectiveValue() {
    if(!mExpression) return false;
    const auto ret = mExpression->evaluate();
    if(!ret.isNumber()) return false;
    const qreal newValue = clamped(ret.toNumber());
    if(isZero4Dec(newValue - mCurrentEffectiveValue)) return false;
    mCurrentEffectiveValue = newValue;
    emit effectiveValueChanged(newValue);
    return true;
}

void QrealAnimator::saveValueToKey(const int frame, const qreal value) {
    const auto keyAtFrame = anim_getKeyAtAbsFrame<QrealKey>(frame);
    if(keyAtFrame) {
        keyAtFrame->setValue(value);
    } else {
        const auto newKey = enve::make_shared<QrealKey>(value, frame, this);
        anim_appendKey(newKey);
    }
}

bool QrealAnimator::updateExpressionRelFrame() {
    if(!mExpression) return false;
    const int absFrame = anim_getCurrentAbsFrame();
    return mExpression->setAbsFrame(absFrame);
}

void QrealAnimator::prp_afterFrameShiftChanged(const FrameRange &oldAbsRange,
                                               const FrameRange &newAbsRange) {
    GraphAnimator::prp_afterFrameShiftChanged(oldAbsRange, newAbsRange);
    updateExpressionRelFrame();
}

void QrealAnimator::anim_setAbsFrame(const int frame) {
    GraphAnimator::anim_setAbsFrame(frame);
    const bool baseValChanged = updateCurrentBaseValue();
    const bool exprValChanged = updateExpressionRelFrame();
    const bool changed = baseValChanged || exprValChanged;
    if(changed) prp_afterChangedCurrent(UpdateReason::frameChange);
}

void QrealAnimator::anim_addKeyAtRelFrame(const int relFrame) {
    if(anim_getKeyAtRelFrame(relFrame)) return;
    const qreal value = getBaseValue(relFrame);
    const auto newKey = enve::make_shared<QrealKey>(value, relFrame, this);
    graph_adjustCtrlsForKeyAdd(newKey.get());
    anim_appendKeyAction(newKey);
}

void QrealAnimator::anim_removeAllKeys() {
    if(!anim_hasKeys()) return;
    startBaseValueTransform();

    const qreal currentValue = mCurrentBaseValue;
    GraphAnimator::anim_removeAllKeys();

    setCurrentBaseValue(currentValue);
    finishBaseValueTransform();
}

QPainterPath QrealAnimator::graph_getPathForSegment(
        const GraphKey * const prevKey,
        const GraphKey * const nextKey) const {
    QPainterPath path;
    if(prevKey) {
        path.moveTo(prevKey->getRelFrame(), prevKey->getValueForGraph());
        if(nextKey) {
            path.cubicTo(QPointF(prevKey->getC1Frame(),
                                 prevKey->getC1Value()),
                         QPointF(nextKey->getC0Frame(),
                                 nextKey->getC0Value()),
                         QPointF(nextKey->getRelFrame(),
                                 nextKey->getValueForGraph()));
        } else {
            path.lineTo(50000, prevKey->getValueForGraph());
        }
    } else {
        if(nextKey) {
            const qreal nextVal = nextKey->getValueForGraph();
            path.moveTo(-50000, nextVal);
            path.lineTo(nextKey->getRelFrame(), nextVal);
        } else {
            path.moveTo(-50000, mCurrentBaseValue);
            path.lineTo(50000, mCurrentBaseValue);
        }
    }
    return path;
}

qValueRange QrealAnimator::graph_getMinAndMaxValues() const {
    if(mGraphMinMaxValuesFixed) {
        return {mClampMin, mClampMax};
    }
    if(!anim_hasKeys()) {
        return {mCurrentBaseValue - mPrefferedValueStep,
                mCurrentBaseValue + mPrefferedValueStep};
    }
    qreal minVal = TEN_MIL;
    qreal maxVal = -TEN_MIL;
    const auto& keys = anim_getKeys();
    for(const auto &key : keys) {
        const auto qaKey = static_cast<QrealKey*>(key);
        const qreal keyVal = qaKey->getValue();
        const qreal startVal = qaKey->getC0Value();
        const qreal endVal = qaKey->getC1Value();
        const qreal maxKeyVal = qMax(qMax(startVal, endVal), keyVal);
        const qreal minKeyVal = qMin(qMin(startVal, endVal), keyVal);
        if(maxKeyVal > maxVal) maxVal = maxKeyVal;
        if(minKeyVal < minVal) minVal = minKeyVal;
    }

    return {minVal - mPrefferedValueStep, maxVal + mPrefferedValueStep};
}

qValueRange QrealAnimator::graph_getMinAndMaxValuesBetweenFrames(
        const int startFrame, const int endFrame) const {
    if(!anim_hasKeys()) return {mCurrentBaseValue, mCurrentBaseValue};
    bool first = true;
    qreal minVal = TEN_MIL;
    qreal maxVal = -TEN_MIL;
    const auto& keys = anim_getKeys();
    for(const auto &key : keys) {
        const auto qaKey = static_cast<QrealKey*>(key);
        const int keyFrame = key->getAbsFrame();
        if(keyFrame > endFrame || keyFrame < startFrame) continue;
        if(first) first = false;
        const qreal keyVal = qaKey->getValue();
        const qreal startVal = qaKey->getC0Value();
        const qreal endVal = qaKey->getC1Value();
        const qreal maxKeyVal = qMax(qMax(startVal, endVal), keyVal);
        const qreal minKeyVal = qMin(qMin(startVal, endVal), keyVal);
        if(maxKeyVal > maxVal) maxVal = maxKeyVal;
        if(minKeyVal < minVal) minVal = minKeyVal;
    }
    if(first) {
        const int midFrame = (startFrame + endFrame)/2;
        const qreal value = getBaseValueAtAbsFrame(midFrame);
        return {value, value};
    }
    return {minVal, maxVal};
}

qreal QrealAnimator::graph_clampGraphValue(const qreal value) {
    return value;
}

void QrealAnimator::incSavedValueToCurrentValue(const qreal incBy) {
    setCurrentBaseValue(mSavedCurrentValue + incBy);
}

void QrealAnimator::multSavedValueToCurrentValue(const qreal multBy) {
    setCurrentBaseValue(mSavedCurrentValue * multBy);
}

void QrealAnimator::setCurrentBaseValueNoUpdate(const qreal newValue) {
    assignCurrentBaseValue(clamped(newValue));
}

void QrealAnimator::incCurrentValueNoUpdate(const qreal incBy) {
    setCurrentBaseValueNoUpdate(mCurrentBaseValue + incBy);
}

void QrealAnimator::incCurrentBaseValue(const qreal incBy) {
    setCurrentBaseValue(mCurrentBaseValue + incBy);
}

void QrealAnimator::prp_startTransform() {
    if(mTransformed) return;
    if(anim_isRecording() && !anim_getKeyOnCurrentFrame()) {
        anim_saveCurrentValueAsKey();
    }
    if(const auto key = anim_getKeyOnCurrentFrame<QrealKey>()) {
        key->startValueTransform();
    }
    startBaseValueTransform();
}

#include "canvas.h"
void QrealAnimator::prp_finishTransform() {
    if(const auto key = anim_getKeyOnCurrentFrame<QrealKey>()) {
        if(!mTransformed) return;
        mTransformed = false;
        key->finishValueTransform();
    } else {
        finishBaseValueTransform();
    }
}

void QrealAnimator::startBaseValueTransform() {
    if(mTransformed) return;
    mSavedCurrentValue = mCurrentBaseValue;
    mTransformed = true;
}

void QrealAnimator::finishBaseValueTransform() {
    if(!mTransformed) return;
    mTransformed = false;
    {
        UndoRedo ur;
        const qreal oldValue = mSavedCurrentValue;
        const qreal newValue = mCurrentBaseValue;
        ur.fUndo = [this, oldValue]() {
            setCurrentBaseValue(oldValue);
        };
        ur.fRedo = [this, newValue]() {
            setCurrentBaseValue(newValue);
        };
        prp_addUndoRedo(ur);
    }
}

void QrealAnimator::prp_cancelTransform() {
    if(!mTransformed) return;
    mTransformed = false;

    if(const auto key = anim_getKeyOnCurrentFrame<QrealKey>()) {
        key->cancelValueTransform();
    } else {
        setCurrentBaseValue(mSavedCurrentValue);
    }
}

FrameRange QrealAnimator::prp_getIdenticalRelRange(const int relFrame) const {
    const auto base = Animator::prp_getIdenticalRelRange(relFrame);
    if(mExpression) {
        const int absFrame = prp_relFrameToAbsFrame(relFrame);
        return base * mExpression->identicalRelRange(absFrame);
    }
    else return base;
}

FrameRange QrealAnimator::prp_nextNonUnaryIdenticalRelRange(const int relFrame) const {
    if(hasExpression()) {
        const int absFrame = prp_relFrameToAbsFrame(relFrame);
        for(int i = relFrame, j = absFrame; i < FrameRange::EMAX; i++, j++) {
            FrameRange range{FrameRange::EMIN, FrameRange::EMAX};
            int lowestMax = INT_MAX;

            {
                const auto childRange = Animator::prp_nextNonUnaryIdenticalRelRange(i);
                lowestMax = qMin(lowestMax, childRange.fMax);
                range *= childRange;
            }
            {
                const auto childRange = mExpression->nextNonUnaryIdenticalRelRange(j);
                lowestMax = qMin(lowestMax, childRange.fMax);
                range *= childRange;
            }

            if(!range.isUnary()) return range;
            const int di = lowestMax - i;
            i += di;
            j += di;
        }

        return FrameRange::EMINMAX;
    } return Animator::prp_nextNonUnaryIdenticalRelRange(relFrame);
}

void QrealAnimator::prp_afterChangedAbsRange(const FrameRange &range,
                                             const bool clip) {
    if(range.inRange(anim_getCurrentAbsFrame()))
        updateCurrentBaseValue();
    GraphAnimator::prp_afterChangedAbsRange(range, clip);
}

void QrealAnimator::multCurrentBaseValue(const qreal mult) {
    setCurrentBaseValue(mCurrentBaseValue*mult);
}

qreal QrealAnimator::getSavedBaseValue() {
    return mSavedCurrentValue;
}

void QrealAnimator::saveQrealSVG(SvgExporter& exp,
                                 QDomElement& parent,
                                 const FrameRange& visRange,
                                 const QString& attrName,
                                 const qreal multiplier,
                                 const bool transform,
                                 const QString& type,
                                 const QString& templ,
                                 const QString &beginEvent,
                                 const QString &endEvent,
                                 const bool motion,
                                 const bool motionRotate,
                                 const QString &motionPath)
{
    const auto mangler = [multiplier](const qreal value) {
        return value*multiplier;
    };
    saveQrealSVG(exp,
                 parent,
                 visRange,
                 attrName,
                 mangler,
                 transform,
                 type,
                 templ,
                 beginEvent,
                 endEvent,
                 motion,
                 motionRotate,
                 motionPath);
}

void QrealAnimator::saveQrealSVG(SvgExporter& exp,
                                 QDomElement& parent,
                                 const FrameRange& visRange,
                                 const QString& attrName,
                                 const Mangler& mangler,
                                 const bool transform,
                                 const QString& type,
                                 const QString& templ,
                                 const QString &beginEvent,
                                 const QString &endEvent,
                                 const bool motion,
                                 const bool motionRotate,
                                 const QString &motionPath)
{
    if (hasValidExpression()) {
        const auto copy = enve::make_shared<QrealAnimator>("");
        const auto relRange = prp_absRangeToRelRange(exp.fAbsRange);
        copy->prp_setInheritedFrameShift(prp_getTotalFrameShift(), nullptr);
        copy->setExpression(mExpression.sptr());
        copy->applyExpression(relRange, 10, false);
        copy->saveQrealSVG(exp,
                           parent,
                           visRange,
                           attrName,
                           mangler,
                           transform,
                           type,
                           templ,
                           beginEvent,
                           endEvent,
                           motion,
                           motionRotate,
                           motionPath);
        setExpression(mExpression.sptr());
    } else {
        graph_saveSVG(exp, parent, visRange, attrName,
                      [this, mangler, &templ](const int relFrame) {
            const qreal val = mangler(getEffectiveValue(relFrame));
            return templ.arg(val);
        }, transform, type, beginEvent, endEvent, motion, motionRotate, motionPath);
    }
}

QDomElement QrealAnimator::prp_writePropertyXEV_impl(const XevExporter& exp) const {
    auto result = exp.createElement("Float");

    if(anim_hasKeys()) {
        QString ctrlModes;
        QString values;
        QString frames;
        const QString blueprint = QStringLiteral("%1 %2 %3");
        const auto& keys = anim_getKeys();
        for(const auto &key : keys) {
            const auto qaKey = static_cast<QrealKey*>(key);
            const qreal vc0 = qaKey->getC0Value();
            const qreal v = qaKey->getValue();
            const qreal vc1 = qaKey->getC1Value();

            const qreal fc0 = qaKey->getC0Frame();
            const int f = qaKey->getRelFrame();
            const qreal fc1 = qaKey->getC1Frame();

            const QString vc0Str = qaKey->getC0Enabled() ?
                                       QString::number(vc0) : "*";
            const QString vc1Str = qaKey->getC1Enabled() ?
                                       QString::number(vc1) : "*";

            const QString fc0Str = qaKey->getC0Enabled() ?
                                       QString::number(fc0) : "*";
            const QString fc1Str = qaKey->getC1Enabled() ?
                                       QString::number(fc1) : "*";

            if(!values.isEmpty()) values += ';';
            values += blueprint.arg(vc0Str).arg(v).arg(vc1Str);
            if(!frames.isEmpty()) frames += ';';
            frames += blueprint.arg(fc0Str).arg(f).arg(fc1Str);
            if(!ctrlModes.isEmpty()) ctrlModes += ';';
            const auto ctrlMode = qaKey->getCtrlsMode();
            ctrlModes += QString::number(static_cast<int>(ctrlMode));
        }
        result.setAttribute("values", values);
        result.setAttribute("frames", frames);
        result.setAttribute("ctrlModes", ctrlModes);
    } else result.setAttribute("value", mCurrentBaseValue);

    if(hasExpression()) {
        auto expression = exp.createElement("Expression");

        const auto definitions = mExpression->definitionsString();
        const auto defsNode = exp.createTextNode(definitions);
        auto defsEle = exp.createElement("Definitions");
        defsEle.appendChild(defsNode);
        expression.appendChild(defsEle);

        const auto bindings = mExpression->bindingsString();
        const auto bindNode = exp.createTextNode(bindings);
        auto bindEle = exp.createElement("Bindings");
        bindEle.appendChild(bindNode);
        expression.appendChild(bindEle);

        const auto script = mExpression->scriptString();
        const auto scriptNode = exp.createTextNode(script);
        auto scriptEle = exp.createElement("Script");
        scriptEle.appendChild(scriptNode);
        expression.appendChild(scriptEle);

        result.appendChild(expression);
    }

    return result;
}

void QrealAnimator::prp_readPropertyXEV_impl(
        const QDomElement& ele, const XevImporter& imp) {
    Q_UNUSED(imp)
    const auto values = ele.attribute("values");
    if(!values.isEmpty()) {
        const auto allFrames = ele.attribute("frames");
        const auto ctrlModes = ele.attribute("ctrlModes");
        const auto keysValues = values.splitRef(';');
        const auto keysFrames = allFrames.splitRef(';');
        const auto keysCtrlModes = ctrlModes.splitRef(';');
        if(keysValues.count() != keysFrames.count())
            RuntimeThrow("The values count does not match the frames count");
        if(keysCtrlModes.count() != keysFrames.count())
            RuntimeThrow("The ctrlModes count does not match the frames count");
        const int iMax = keysValues.count();
        for(int i = 0; i < iMax; i++) {
            const auto values = keysValues[i].split(' ');
            if(values.count() != 3) {
                RuntimeThrow("Invalid values count " + values[i].toString());
            }
            const auto frames = keysFrames[i].split(' ');
            if(frames.count() != 3) {
                RuntimeThrow("Invalid frames count " + frames[i].toString());
            }
            const auto ctrlModeStr = keysCtrlModes[i];
            const auto ctrlMode = XmlExportHelpers::stringToEnum<CtrlsMode>(
                                        ctrlModeStr, CtrlsMode::smooth,
                                        CtrlsMode::corner);

            const qreal value = XmlExportHelpers::stringToDouble(values[1]);
            const int relFrame = XmlExportHelpers::stringToInt(frames[1]);

            const auto f0Str = frames[0];
            const auto f2Str = frames[2];

            const auto v0Str = values[0];
            const auto v2Str = values[2];

            const auto key = enve::make_shared<QrealKey>(value, relFrame, this);

            const bool c0Enabled = f0Str != '*' && v0Str != '*';
            const bool c1Enabled = f2Str != '*' && v2Str != '*';

            key->setC0Enabled(c0Enabled);
            key->setC1Enabled(c1Enabled);

            key->setC0Frame(c0Enabled ? XmlExportHelpers::stringToDouble(f0Str) : relFrame);
            key->setC1Frame(c1Enabled ? XmlExportHelpers::stringToDouble(f2Str) : relFrame);
            key->setC0Value(c0Enabled ? XmlExportHelpers::stringToDouble(v0Str) : value);
            key->setC1Value(c1Enabled ? XmlExportHelpers::stringToDouble(v2Str) : value);

            key->setCtrlsMode(ctrlMode);

            anim_appendKey(key);
        }
    } else {
        const auto value = ele.attribute("value");
        if(value.isEmpty()) RuntimeThrow("No values/frames and no value provided");
        setCurrentBaseValue(XmlExportHelpers::stringToDouble(value));
    }

    const auto expression = ele.firstChildElement("Expression");
    if(!expression.isNull()) {
        const auto defsEle =  expression.firstChildElement("Definitions");
        const QString definitions = defsEle.text();

         const auto bindEle =  expression.firstChildElement("Bindings");
         const QString bindings = bindEle.text();

         const auto scriptEle =  expression.firstChildElement("Script");
         const QString script = scriptEle.text();

         SimpleTask::sScheduleContexted(this,
         [this, bindings, definitions, script]() {
             setExpression(Expression::sCreate(
                               definitions, bindings, script, this,
                               Expression::sQrealAnimatorTester));
         });
    }
}
