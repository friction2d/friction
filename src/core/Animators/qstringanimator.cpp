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

#include "qstringanimator.h"

#include "undoredo.h"
#include "simplemath.h"
#include "svgexporthelpers.h"
#include "appsupport.h"
#include "simpletask.h"
#include "ReadWrite/evformat.h"
#include "typemenu.h"
#include "GUI/dialogsinterface.h"
#include "Expressions/expression.h"
#include "canvas.h"

QStringAnimator::QStringAnimator(const QString &name) :
    SteppedAnimator<QString>(name) {}

void QStringAnimator::prp_writeProperty_impl(eWriteStream& dst) const {
    SteppedAnimator<QString>::prp_writeProperty_impl(dst);
    dst << !!mExpression;
    if(mExpression) {
        dst << mExpression->bindingsString();
        dst << mExpression->definitionsString();
        dst << mExpression->scriptString();
    }
}

void QStringAnimator::prp_readProperty_impl(eReadStream& src) {
    SteppedAnimator<QString>::prp_readProperty_impl(src);

    if(src.evFileVersion() >= EvFormat::textExpression) {
        bool hasExpr = false;
        src >> hasExpr;
        if(hasExpr) {
            QString bindingsStr; src >> bindingsStr;
            QString definitionsStr; src >> definitionsStr;
            QString scriptStr; src >> scriptStr;
            SimpleTask::sScheduleContexted(this,
            [this, bindingsStr, definitionsStr, scriptStr]() {
                setExpression(Expression::sCreate(
                                  bindingsStr, definitionsStr, scriptStr, this,
                                  Expression::sQStringAnimatorTester));
            });
        }
    }
}

QDomElement createTextElement(SvgExporter& exp, const QString& text) {
    auto textEle = exp.createElement("text");

    const QStringList lines = text.split(QRegExp("\n|\r\n|\r"));
    for(int i = 0; i < lines.count(); i++) {
        const auto& line = lines.at(i);
        auto tspan = exp.createElement("tspan");
        if(i != 0) tspan.setAttribute("dy", "1.2em");
        tspan.setAttribute("x", 0);
        const auto textNode = exp.createTextNode(line);
        tspan.appendChild(textNode);
        textEle.appendChild(tspan);
    }

    return textEle;
}

void QStringAnimator::saveSVG(SvgExporter& exp, QDomElement& parent,
                              const PropSetter& propSetter) const {
    const auto relRange = prp_absRangeToRelRange(exp.fAbsRange);
    const auto idRange = prp_getIdenticalRelRange(relRange.fMin);
    const int span = exp.fAbsRange.span();
    if(idRange.inRange(relRange) || span == 1) {
        auto ele = createTextElement(exp, getValueAtRelFrame(relRange.fMin));
        propSetter(ele);
        parent.appendChild(ele);
    } else {
        int i = relRange.fMin;
        while(true) {
            const auto iRange = exp.fAbsRange*prp_getIdenticalAbsRange(i);

            auto ele = createTextElement(exp, getValueAtRelFrame(i));
            propSetter(ele);
            SvgExportHelpers::assignVisibility(exp, ele, iRange);
            parent.appendChild(ele);

            if(iRange.fMax >= relRange.fMax) break;
            i = prp_nextDifferentRelFrame(i);
        }
    }
}

QJSValue QStringAnimator::prp_getBaseJSValue(QJSEngine& e) const {
    Q_UNUSED(e)
    return getBaseValueAtRelFrame(anim_getCurrentRelFrame());
}

QJSValue QStringAnimator::prp_getBaseJSValue(QJSEngine& e,
                                             const qreal relFrame) const {
    Q_UNUSED(e)
    return getBaseValueAtRelFrame(relFrame);
}

QJSValue QStringAnimator::prp_getEffectiveJSValue(QJSEngine& e) const {
    Q_UNUSED(e)
    return getEffectiveValue(anim_getCurrentRelFrame());
}

QJSValue QStringAnimator::prp_getEffectiveJSValue(QJSEngine& e,
                                                  const qreal relFrame) const {
    Q_UNUSED(e)
    return getEffectiveValue(relFrame);
}

void QStringAnimator::prp_setupTreeViewMenu(PropertyMenu * const menu) {
    if(menu->hasActionsForType<QStringAnimator>()) { return; }
    menu->addedActionsForType<QStringAnimator>();

    const PropertyMenu::PlainSelectedOp<QStringAnimator> sOp =
    [](QStringAnimator * aTarget) {
        const auto& iface = DialogsInterface::instance();
        iface.showExpressionDialog(aTarget);
    };
    menu->addPlainAction(QIcon::fromTheme("preferences"),
                         tr("Set Expression"), sOp);

    const PropertyMenu::PlainSelectedOp<QStringAnimator> aOp =
    [](QStringAnimator * aTarget) {
        const auto scene = aTarget->getFirstAncestor<Canvas>();
        if(!scene) return;
        const auto relRange = aTarget->prp_absRangeToRelRange(
                    scene->getFrameRange());
        aTarget->applyExpression(relRange, true);
    };
    menu->addPlainAction(QIcon::fromTheme("dialog-ok"),
                         tr("Apply Expression"), aOp)->setEnabled(hasExpression());

    const PropertyMenu::PlainSelectedOp<QStringAnimator> cOp =
    [](QStringAnimator * aTarget) {
        aTarget->clearExpressionAction();
    };
    menu->addPlainAction(QIcon::fromTheme("trash"),
                         tr("Clear Expression"), cOp)->setEnabled(hasExpression());

    menu->addSeparator();
    Animator::prp_setupTreeViewMenu(menu);
}

FrameRange QStringAnimator::prp_getIdenticalRelRange(const int relFrame) const {
    const auto base = SteppedAnimator<QString>::prp_getIdenticalRelRange(relFrame);
    if(mExpression) {
        const int absFrame = prp_relFrameToAbsFrame(relFrame);
        return base * mExpression->identicalRelRange(absFrame);
    }
    return base;
}

FrameRange QStringAnimator::prp_nextNonUnaryIdenticalRelRange(
        const int relFrame) const {
    if(hasExpression()) {
        const int absFrame = prp_relFrameToAbsFrame(relFrame);
        for(int i = relFrame, j = absFrame; i < FrameRange::EMAX; i++, j++) {
            FrameRange range{FrameRange::EMIN, FrameRange::EMAX};
            int lowestMax = INT_MAX;

            {
                const auto childRange =
                        SteppedAnimator<QString>::prp_nextNonUnaryIdenticalRelRange(i);
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
    }
    return SteppedAnimator<QString>::prp_nextNonUnaryIdenticalRelRange(relFrame);
}

void QStringAnimator::prp_afterFrameShiftChanged(
        const FrameRange& oldAbsRange,
        const FrameRange& newAbsRange) {
    SteppedAnimator<QString>::prp_afterFrameShiftChanged(oldAbsRange, newAbsRange);
    updateExpressionRelFrame();
}

void QStringAnimator::anim_setAbsFrame(const int frame) {
    SteppedAnimator<QString>::anim_setAbsFrame(frame);
    const bool exprFrameChanged = updateExpressionRelFrame();
    const bool exprValueChanged = updateCurrentEffectiveValue();
    if(exprFrameChanged || exprValueChanged) {
        prp_afterChangedCurrent(UpdateReason::frameChange);
    }
}

bool QStringAnimator::prp_dependsOn(const Property* const prop) const {
    if(!mExpression) return false;
    return mExpression->dependsOn(prop);
}

bool QStringAnimator::hasValidExpression() const {
    return mExpression ? mExpression->isValid() : false;
}

QString QStringAnimator::getExpressionBindingsString() const {
    if(!mExpression) return "";
    return mExpression->bindingsString();
}

QString QStringAnimator::getExpressionDefinitionsString() const {
    if(!mExpression) return "";
    return mExpression->definitionsString();
}

QString QStringAnimator::getExpressionScriptString() const {
    if(!mExpression) return "";
    return mExpression->scriptString();
}

void QStringAnimator::setExpressionAction(const qsptr<Expression>& expression) {
    if(expression || mExpression) {
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

void QStringAnimator::setExpression(const qsptr<Expression>& expression) {
    auto& conn = mExpression.assign(expression);
    if(expression) {
        const int absFrame = anim_getCurrentAbsFrame();
        expression->setAbsFrame(absFrame);
        conn << connect(expression.get(), &Expression::currentValueChanged,
                        this, [this]() {
            if(updateCurrentEffectiveValue()) {
                prp_afterChangedCurrent(UpdateReason::frameChange);
            }
        });
        conn << connect(expression.get(), &Expression::relRangeChanged,
                        this, [this](const FrameRange& range) {
            prp_afterChangedRelRange(range);
        });
    }
    updateCurrentEffectiveValue();
    prp_afterWholeInfluenceRangeChanged();
}

void QStringAnimator::applyExpression(const FrameRange& relRange,
                                      const bool action) {
    if(!hasValidExpression()) return;
    if(!relRange.isValid()) return;

    prp_pushUndoRedoName(tr("Apply Expression"));

    QString currentValue;
    bool first = true;
    for(int relFrame = relRange.fMin; relFrame <= relRange.fMax; relFrame++) {
        const int absFrame = prp_relFrameToAbsFrame(relFrame);
        mExpression->setAbsFrame(absFrame);
        const QString value = getEffectiveValue(relFrame);
        if(first || value != currentValue) {
            currentValue = value;
            if(auto key = anim_getKeyAtRelFrame<QStringKey>(relFrame)) {
                key->setValue(value);
            } else {
                const auto newKey = enve::make_shared<QStringKey>(value, relFrame, this);
                if(action) anim_appendKeyAction(newKey);
                else anim_appendKey(newKey);
            }
            first = false;
        }
    }

    if(action) { setExpressionAction(nullptr); }
    else { setExpression(nullptr); }
}

QString QStringAnimator::getValueAtRelFrame(const qreal frame) const {
    return getEffectiveValue(frame);
}

QString QStringAnimator::getBaseValueAtRelFrame(const qreal frame) const {
    return SteppedAnimator<QString>::getValueAtRelFrame(frame);
}

QString QStringAnimator::getEffectiveValue(const qreal relFrame) const {
    if(mExpression) {
        const auto ret = mExpression->evaluate(relFrame);
        if(ret.isNull() || ret.isUndefined()) return QString();
        return ret.toString();
    }
    return getBaseValueAtRelFrame(relFrame);
}

bool QStringAnimator::updateExpressionRelFrame() {
    if(!mExpression) return false;
    const int absFrame = anim_getCurrentAbsFrame();
    return mExpression->setAbsFrame(absFrame);
}

bool QStringAnimator::updateCurrentEffectiveValue() {
    if(!mExpression) return false;
    const auto ret = mExpression->evaluate();
    if(ret.isNull() || ret.isUndefined()) return false;
    const QString newValue = ret.toString();
    if(newValue == mCurrentEffectiveValue) return false;
    mCurrentEffectiveValue = newValue;
    return true;
}

void QStringAnimator::prp_readPropertyXEV_impl(
        const QDomElement& ele, const XevImporter& imp) {
    if(ele.hasAttribute("frames")) {
        const auto framesStr = ele.attribute("frames");
        const auto frameStrs = framesStr.splitRef(' ', Qt::SkipEmptyParts);

        for(const QStringRef& frame : frameStrs) {
            const int iFrame = XmlExportHelpers::stringToInt(frame);
            imp.processAsset(frame + ".txt", [&](QIODevice* const src) {
                QString value;
                QTextStream stream(src);
                value = stream.readAll();
                const auto key = enve::make_shared<QStringKey>(value, iFrame, this);
                anim_appendKey(key);
            });
        }
    } else {
        imp.processAsset("value.txt", [&](QIODevice* const src) {
            QString value;
            QTextStream stream(src);
            value = stream.readAll();
            setCurrentValue(value);
        });
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
                              bindings, definitions, script, this,
                              Expression::sQStringAnimatorTester));
        });
    }
}

void saveTextXEV(const QString& path, const XevExporter& exp,
                 const QString& txt) {
    exp.processAsset(path, [&](QIODevice* const dst) {
        QTextStream stream(dst);
        stream << txt;
    }, false);
}

QDomElement QStringAnimator::prp_writePropertyXEV_impl(const XevExporter& exp) const {
    auto result = exp.createElement("Text");
    if(anim_hasKeys()) {
        QString frames;
        for(const auto& key : anim_getKeys()) {
            const auto txtKey = static_cast<QStringKey*>(key);
            const auto frameStr = QString::number(key->getRelFrame());
            if(!frames.isEmpty()) frames += ' ';
            frames += frameStr;
            saveTextXEV(frameStr + ".txt", exp, txtKey->getValue());
        }
        result.setAttribute("frames", frames);
    } else {
        saveTextXEV("value.txt", exp, getCurrentValue());
    }

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
