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

#include "qrealanimatorvalueslider.h"
#include "Animators/qpointfanimator.h"
#include "Animators/eboxorsound.h"
#include "themesupport.h"
#include "canvas.h"
#include "Private/document.h"

#include <QMenu>
#include <QMouseEvent>

QrealAnimatorValueSlider::QrealAnimatorValueSlider(qreal minVal,
                                                   qreal maxVal,
                                                   qreal prefferedStep,
                                                   QWidget *parent)
    : QDoubleSlider(minVal,
                    maxVal,
                    prefferedStep,
                    parent)
{

}

QrealAnimatorValueSlider::QrealAnimatorValueSlider(qreal minVal,
                                                   qreal maxVal,
                                                   qreal prefferedStep,
                                                   QWidget *parent,
                                                   bool autoAdjust)
    : QDoubleSlider(minVal,
                    maxVal,
                    prefferedStep,
                    parent,
                    autoAdjust)
{

}

QrealAnimatorValueSlider::QrealAnimatorValueSlider(qreal minVal,
                                                   qreal maxVal,
                                                   qreal prefferedStep,
                                                   QrealAnimator *animator,
                                                   QWidget *parent)
    : QDoubleSlider(minVal,
                    maxVal,
                    prefferedStep,
                    parent)
{
    setTarget(animator);
    connect(this, &QDoubleSlider::tabPressed,
            this, &QrealAnimatorValueSlider::handleTabPressed);
}

QrealAnimatorValueSlider::QrealAnimatorValueSlider(QrealAnimator *animator,
                                                   QWidget *parent)
    : QDoubleSlider(parent)
{
    setTarget(animator);
    connect(this, &QDoubleSlider::tabPressed,
            this, &QrealAnimatorValueSlider::handleTabPressed);
}

QrealAnimatorValueSlider::QrealAnimatorValueSlider(QWidget *parent)
    : QrealAnimatorValueSlider(nullptr,
                               parent)
{

}

QrealAnimatorValueSlider::QrealAnimatorValueSlider(QString name,
                                                   qreal minVal,
                                                   qreal maxVal,
                                                   qreal prefferedStep,
                                                   QWidget *parent)
    : QDoubleSlider(name,
                    minVal,
                    maxVal,
                    prefferedStep,
                    parent)
{

}

QrealAnimator* QrealAnimatorValueSlider::getTransformTargetSibling()
{
    if (mTransformTarget) {
        const auto parent = mTransformTarget->getParent();
        if (const auto qPA = enve_cast<QPointFAnimator*>(parent)) {
            const bool thisX = qPA->getXAnimator() == mTransformTarget;
            return thisX ? qPA->getYAnimator() : qPA->getXAnimator();
        }
    }
    return nullptr;
}

QrealAnimator *QrealAnimatorValueSlider::getTargetSibling()
{
    if (mTarget) {
        const auto parent = mTarget->getParent();
        if (const auto qPA = enve_cast<QPointFAnimator*>(parent)) {
            const bool thisX = qPA->getXAnimator() == mTarget;
            return thisX ? qPA->getYAnimator() : qPA->getXAnimator();
        }
    }
    return nullptr;
}

void QrealAnimatorValueSlider::mouseMoveEvent(QMouseEvent *e)
{
    if ((e->buttons() & Qt::LeftButton) && getLockedAncestor()) {
        qCDebug(lcLocked) << "slider mouseMoveEvent blocked for locked" << (mTarget ? mTarget->prp_getName() : "null");
        return;
    }
    const bool uniform = e->modifiers() & Qt::ShiftModifier;
    QDoubleSlider::mouseMoveEvent(e);
    if (uniform) {
        const auto other = getTransformTargetSibling();
        if (other) {
            other->setCurrentBaseValue(mTarget->getCurrentBaseValue());
        }
    }
}

void QrealAnimatorValueSlider::keyPressEvent(QKeyEvent *e)
{
    mUniform = e->modifiers() & Qt::ShiftModifier;
    QDoubleSlider::keyPressEvent(e);
}

void QrealAnimatorValueSlider::keyReleaseEvent(QKeyEvent *e)
{
    mUniform = e->modifiers() & Qt::ShiftModifier;
    QDoubleSlider::keyReleaseEvent(e);
}

bool QrealAnimatorValueSlider::eventFilter(QObject *obj,
                                           QEvent *event)
{
    return QDoubleSlider::eventFilter(obj, event);
}

void QrealAnimatorValueSlider::handleTabPressed()
{
    const auto other = getTargetSibling();
    if (other) { emit other->requestWidgetFocus(); }
}

eBoxOrSound* QrealAnimatorValueSlider::getLockedAncestor() const
{
    if (!mTarget) return nullptr;
    return mTarget->getFirstAncestor<eBoxOrSound>([](Property* p) {
        const auto e = enve_cast<eBoxOrSound*>(p);
        return e && e->isLocked();
    });
}

void QrealAnimatorValueSlider::mousePressEvent(QMouseEvent* e)
{
    if (e->button() == Qt::LeftButton) {
        const auto locked = getLockedAncestor();
        qCDebug(lcLocked) << "slider mousePressEvent" << (mTarget ? mTarget->prp_getName() : "null") << "locked=" << (locked ? locked->prp_getName() : "none");
        if (locked) { emit locked->lockedModificationAttempted(); }
    }
    QDoubleSlider::mousePressEvent(e);
}

void QrealAnimatorValueSlider::mouseReleaseEvent(QMouseEvent* e)
{
    const auto locked = (e->button() == Qt::LeftButton) ? getLockedAncestor() : nullptr;
    qCDebug(lcLocked) << "slider mouseReleaseEvent" << (mTarget ? mTarget->prp_getName() : "null") << "mouseMoved=" << mouseMoved() << "locked=" << (locked ? locked->prp_getName() : "none");
    if (e->button() == Qt::LeftButton && !mouseMoved() && locked) {
        Actions::sInstance->finishSmoothChange();
        return;
    }
    QDoubleSlider::mouseReleaseEvent(e);
}

void QrealAnimatorValueSlider::startTransform(const qreal value)
{
    const auto locked = getLockedAncestor();
    qCDebug(lcLocked) << "slider startTransform" << (mTarget ? mTarget->prp_getName() : "null") << "locked=" << (locked ? locked->prp_getName() : "none") << "mTransformTarget=" << (mTransformTarget ? "set" : "null");
    if (locked) {
        qCDebug(lcLocked) << "slider startTransform: canceling drag due to locked ancestor";
        QDoubleSlider::cancelTransform();
        if (mTarget) { setDisplayedValue(mTarget->getEffectiveValue()); }
        return;
    }
    if (mTarget) {
        mTransformTarget = mTarget;
        mTransformTarget->prp_startTransform();
        const auto other = getTransformTargetSibling();
        if (other) {
            other->prp_startTransform();
        }
    }
    QDoubleSlider::startTransform(value);
}

QString QrealAnimatorValueSlider::getEditText() const
{
    if (mTarget && mTarget->hasExpression()) {
        return valueToText(mBaseValue);
    }
    return QDoubleSlider::getEditText();
}

void QrealAnimatorValueSlider::setValue(const qreal value)
{
    if (mTransformTarget) {
        qCDebug(lcLocked) << "slider setValue: applying to" << mTransformTarget->prp_getName() << "value=" << value;
        mTransformTarget->setCurrentBaseValue(value);
        emit valueEdited(this->value());
    } else  { QDoubleSlider::setValue(value); }
}

void QrealAnimatorValueSlider::finishTransform(const qreal value)
{
    if (mTransformTarget) {
        mTransformTarget->prp_finishTransform();
        const auto other = getTransformTargetSibling();
        if (other) {
            if (mUniform) {
                other->prp_startTransform();
                other->setCurrentBaseValue(mTarget->getCurrentBaseValue());
                mUniform = false;
            }
            other->prp_finishTransform();
        }
        mTransformTarget = nullptr;
    }
    QDoubleSlider::finishTransform(value);
}

void QrealAnimatorValueSlider::cancelTransform()
{
    if (mTransformTarget) {
        mTransformTarget->prp_cancelTransform();
        const auto other = getTransformTargetSibling();
        if (other) {
            other->prp_cancelTransform();
        }
        mTransformTarget = nullptr;
    }

    mUniform = false;
    QDoubleSlider::cancelTransform();
}

qreal QrealAnimatorValueSlider::startSlideValue() const
{
    if (mTarget && mTarget->hasExpression()) { return mBaseValue; }
    else { return QDoubleSlider::startSlideValue(); }
}

#ifdef Q_OS_MAC
void QrealAnimatorValueSlider::wheelEvent(QWheelEvent *e)
{
    QDoubleSlider::wheelEvent(e);

    const bool alt = e->modifiers() & Qt::AltModifier;
    const bool ctrl = e->modifiers() & Qt::ControlModifier;
    const bool uniform = e->modifiers() & Qt::ShiftModifier;

    if (!mTransformTarget || !uniform) { return; }
    const auto other = getTransformTargetSibling();
    if (!other) { return; }

    if (e->phase() == Qt::NoScrollPhase && (alt || ctrl)) {
        other->prp_startTransform();
        other->setCurrentBaseValue(mTarget->getCurrentBaseValue());
        other->prp_finishTransform();
        return;
    }
    if (e->phase() == Qt::ScrollBegin) {
        other->prp_startTransform();
        return;
    } else if (e->phase() == Qt::ScrollEnd) {
        other->prp_finishTransform();
        return;
    }
    if (e->angleDelta().x() == 0 ||
        (e->phase() != Qt::ScrollUpdate &&
         e->phase() != Qt::ScrollMomentum)) { return; }
    other->setCurrentBaseValue(mTarget->getCurrentBaseValue());
}
#endif

void QrealAnimatorValueSlider::paint(QPainter *p)
{
    if (!mTarget) {
        QDoubleSlider::paint(p);
    } else {
        bool rec = false;
        bool key = false;
        const auto aTarget = static_cast<Animator*>(*mTarget);
        rec = aTarget->anim_isRecording();
        key = aTarget->anim_getKeyOnCurrentFrame();
        if (rec) {
            const bool disabled = isTargetDisabled() || !isEnabled();
            QDoubleSlider::paint(p,
                                 disabled ? ThemeSupport::getThemeButtonBaseColor(200) : ThemeSupport::getThemeHighlightAlternativeColor(),
                                 key ? (disabled ? ThemeSupport::getThemeAlternateColor() : ThemeSupport::getThemeHighlightSelectedColor()) : (disabled ? ThemeSupport::getThemeAlternateColor() : ThemeSupport::getThemeHighlightColor()),
                                 key ? (disabled ? Qt::gray : ThemeSupport::getThemeHighlightSelectedColor()) : (disabled ? Qt::darkGray : ThemeSupport::getThemeButtonBorderColor()),
                                 disabled ? Qt::darkGray : Qt::black);
        } else {
            QDoubleSlider::paint(p, !isTargetDisabled() && isEnabled());
        }
        if (!textEditing() && mTarget->hasExpression()) {
            if (mTarget->hasValidExpression()) {
                p->setBrush(ThemeSupport::getThemeHighlightColor());
            } else {
                p->setBrush(QColor(255, 125, 0));
            }
            p->setPen(Qt::NoPen);
            p->setRenderHint(QPainter::Antialiasing);
            p->drawEllipse({7, height()/2}, 3, 3);
        }
    }
}

void QrealAnimatorValueSlider::targetHasExpressionChanged()
{
    QObject::disconnect(mExprConn);
    if (mTarget) {
        const bool hasExpression = mTarget->hasExpression();
        if (hasExpression) {
            mExprConn = connect(mTarget, &QrealAnimator::baseValueChanged,
                                this, [this](const qreal value) {
                mBaseValue = value;
                setName(valueToText(mBaseValue));
            });
        }
        mBaseValue = mTarget->getCurrentBaseValue();
        setName(valueToText(mBaseValue));
        setNameVisible(hasExpression);
    } else { setNameVisible(false); }
}

void QrealAnimatorValueSlider::setTarget(QrealAnimator * const animator)
{
    if (animator == mTarget) { return; }
    auto& conn = mTarget.assign(animator);
    targetHasExpressionChanged();
    if (animator) {
        conn << connect(animator, &QrealAnimator::effectiveValueChanged,
                        this, &QrealAnimatorValueSlider::setDisplayedValue);
        conn << connect(animator, &QrealAnimator::anim_changedKeyOnCurrentFrame,
                        this, qOverload<>(&QrealAnimatorValueSlider::update));
        conn << connect(animator, &QrealAnimator::expressionChanged,
                        this, &QrealAnimatorValueSlider::targetHasExpressionChanged);
        conn << connect(animator, &QrealAnimator::requestWidgetFocus,
                        this, [this]() {
            const auto locked = getLockedAncestor();
            if (locked) {
                emit locked->lockedModificationAttempted();
                return;
            }
            QDoubleSlider::setLineEditFocus();
        });

        setNumberDecimals(animator->getNumberDecimals());
        setValueRange(animator->getMinPossibleValue(),
                      animator->getMaxPossibleValue());
        setPrefferedValueStep(animator->getPrefferedValueStep());
        setDisplayedValue(animator->getEffectiveValue());
    }
}

bool QrealAnimatorValueSlider::hasTarget()
{
    return mTarget;
}

bool QrealAnimatorValueSlider::isTargetDisabled()
{
    if (hasTarget()) { return mTarget->SWT_isDisabled(); }
    return true;
}

void QrealAnimatorValueSlider::openContextMenu(const QPoint &globalPos)
{
    if (!mTarget) { return; }
    const auto aTarget = *mTarget;
    QMenu menu(this);

    const bool keyOnFrame = aTarget->anim_getKeyOnCurrentFrame();
    const auto deleteKey = menu.addAction(tr("Delete Key"),
                                          aTarget,
                                          &Animator::anim_deleteCurrentKeyAction);
    deleteKey->setEnabled(keyOnFrame);

    const auto addKey = menu.addAction(tr("Add Key"),
                                       aTarget,
                                       &Animator::anim_saveCurrentValueAsKey);
    addKey->setEnabled(!keyOnFrame);

    menu.addSeparator();

    const auto setExpression = menu.addAction(tr("Set Expression"));
    connect(setExpression, &QAction::triggered, this, [aTarget]() {
        const auto scene = aTarget->getParentScene();
        if (scene) { scene->openExpressionDialog(aTarget); }
    });

    const auto applyExpression = menu.addAction(tr("Apply Expression"));
    connect(applyExpression, &QAction::triggered, this, [aTarget]() {
        const auto scene = aTarget->getParentScene();
        if (scene) { scene->openApplyExpressionDialog(aTarget); }
    });
    applyExpression->setEnabled(aTarget->hasExpression());


    const auto clearExpression = menu.addAction(tr("Clear Expression"),
                                                aTarget,
                                                &QrealAnimator::clearExpressionAction);
    clearExpression->setEnabled(aTarget->hasExpression());

    menu.addSeparator();

    QAction * const recAct = menu.addAction(tr("Recording"));
    recAct->setCheckable(true);
    recAct->setChecked(aTarget->anim_isRecording());
    connect(recAct, &QAction::triggered,
            aTarget, &Animator::anim_setRecording);

    QAction * const selectedAction = menu.exec(globalPos);
    if (!selectedAction) { return; }
    else { Document::sInstance->actionFinished(); }
}
