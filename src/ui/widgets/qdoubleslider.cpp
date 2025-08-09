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

#include "qdoubleslider.h"
#include <QPainter>
#include <QKeyEvent>
#include <QJSEngine>
#include <QApplication>

#include "Private/document.h"
#include "pointhelpers.h"
#include "GUI/global.h"
#include "themesupport.h"

SliderEdit::SliderEdit(QWidget* const parent) :
    QLineEdit(parent) {
    setAttribute(Qt::WA_TranslucentBackground);
    eSizesUI::widget.add(this, [this](const int size) {
        setFixedHeight(size);
    });

    connect(this, &QLineEdit::editingFinished,
            this, &SliderEdit::lineEditingFinished);
}

void SliderEdit::mousePressEvent(QMouseEvent* e) {
    if(!rect().contains(e->pos())) clearFocus();
    QLineEdit::mousePressEvent(e);
}

void SliderEdit::keyPressEvent(QKeyEvent* e) {
    const int key = e->key();
    switch(key) {
    case Qt::Key_Escape:
        mCanceled = true;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        clearFocus();
        break;
    case Qt::Key_Tab:
        clearFocus();
        emit tabPressed();
        break;
    default: QLineEdit::keyPressEvent(e);
    }
}

void SliderEdit::hideEvent(QHideEvent* e) {
    releaseMouse();
    unsetCursor();
    emit hoverChanged();
    QLineEdit::hideEvent(e);
}

void SliderEdit::showEvent(QShowEvent* e) {
    grabMouse();
    setCursor(Qt::IBeamCursor);
    QLineEdit::showEvent(e);
}

void SliderEdit::leaveEvent(QEvent *e)
{
    emit hoverChanged();
    QLineEdit::leaveEvent(e);
}

void SliderEdit::lineEditingFinished() {
    deselect();
    clearFocus();
    hide();
    if(mCanceled) {
        mCanceled = false;
        return;
    }
    QJSEngine engine;
    const auto result = engine.evaluate(text());
    if(!result.isNumber()) return;
    const qreal newValue = result.toNumber();
    emit valueSet(newValue);
}

QDoubleSlider::QDoubleSlider(const qreal minVal, const qreal maxVal,
                             const qreal prefferedStep,
                             QWidget * const parent,
                             bool autoAdjust) : QWidget(parent) {
    mAutoAdjustWidth = autoAdjust;
    mMinValue = minVal;
    mMaxValue = maxVal;
    mPrefferedValueStep = prefferedStep;
    setDisplayedValue(minVal);
    updateValueString();

    mLineEdit = new SliderEdit(this);
    mLineEdit->hide();

    connect(mLineEdit, &SliderEdit::hoverChanged,
            this, [this] {
        mHovered = false;
        unsetCursor();
        update();
    });
    connect(mLineEdit, &SliderEdit::tabPressed,
            this, [this]() { emit tabPressed(); });
    connect(mLineEdit, &SliderEdit::valueSet,
            this, [this](const qreal value) {
        const qreal clampedValue = clamped(value);
        startTransform(clampedValue);
        setValue(clampedValue);
        finishTransform(clampedValue);
        Document::sInstance->actionFinished();
    });

    setContentsMargins(0, 0, 0, 0);
    /*eSizesUI::widget.add(this, [this](const int size) {
        setFixedHeight(size);
        fitWidthToContent();
    });*/
}

QDoubleSlider::QDoubleSlider(const QString &name,
                             const qreal minVal,
                             const qreal maxVal,
                             const qreal prefferedStep,
                             QWidget * const parent) :
    QDoubleSlider(minVal, maxVal, prefferedStep, parent) {
    setName(name);
}

QDoubleSlider::QDoubleSlider(QWidget *parent) :
    QDoubleSlider(0, 100, 1, parent) {}

QDoubleSlider::~QDoubleSlider() {
    removeEventFilter(this);
    mLineEdit->removeEventFilter(this);
}

void QDoubleSlider::setValueSliderVisible(const bool valueSliderVisible) {
    mShowValueSlider = valueSliderVisible;
    update();
}

void QDoubleSlider::setNameVisible(const bool nameVisible) {
    mShowName = nameVisible;
    if (mAutoAdjustWidth) { fitWidthToContent(); }
}

void QDoubleSlider::setName(const QString &name) {
    mName = name;
    update();
}

void QDoubleSlider::setNumberDecimals(const int decimals) {
    mDecimals = decimals;
    if (mAutoAdjustWidth) { fitWidthToContent(); }
    updateValueString();
}

void QDoubleSlider::setLineEditFocus()
{
    updateLineEditFromValue();
    mCanceled = false;
    mLineEdit->show();
    mLineEdit->setFocus();
    mLineEdit->selectAll();
}

void QDoubleSlider::updateLineEditFromValue() {
    mLineEdit->setText(getEditText());
}

bool QDoubleSlider::cancelMove() {
    if(!textEditing() && mMouseMoved && !mCanceled) {
        cancelTransform();
        return true;
    }
    return false;
}

void QDoubleSlider::setValueRange(const qreal min, const qreal max) {
    mMinValue = min;
    mMaxValue = max;
    setDisplayedValue(clamped(mValue));
    if (mAutoAdjustWidth) { fitWidthToContent(); }
}

void QDoubleSlider::paint(QPainter * const p, const bool enabled) {
    paint(p, enabled ? ThemeSupport::getThemeButtonBaseColor() : ThemeSupport::getThemeAlternateColor(),
          enabled ? ThemeSupport::getThemeButtonBorderColor(150) : ThemeSupport::getThemeBaseColor(),
          enabled ? ThemeSupport::getThemeButtonBorderColor() : Qt::darkGray,
          enabled ? Qt::white : Qt::darkGray);
}

QString QDoubleSlider::valueToText(const qreal value) const {
    return QString::number(value, 'f', mDecimals);
}

qreal QDoubleSlider::getDValueForMouseMove(const int mouseX) const {
    return (mouseX - mLastX)*0.1*mPrefferedValueStep;
}

bool QDoubleSlider::textEditing() const {
    return mLineEdit->isVisible();
}

void QDoubleSlider::paint(QPainter *p) {
    paint(p, isEnabled());
}

void QDoubleSlider::paint(QPainter *p,
                          const QColor &allFill,
                          const QColor &sliderFill,
                          const QColor &stroke,
                          const QColor &text)
{
    p->save();

    p->setRenderHint(QPainter::Antialiasing);
    QRectF boundingRect = rect().adjusted(1, 1, -1, -1);
    p->setPen(Qt::NoPen);

    if (mHovered && isEnabled()) { p->setBrush(ThemeSupport::getThemeBaseDarkerColor()); }
    else { p->setBrush(allFill); }

    if (mLeftNeighbour) {
        p->setClipRect(width()/2, 0, width()/2, height());
    } else if (mRightNeighbour) {
        p->setClipRect(0, 0, width()/2, height());
    }

    const int xR = 2;
    p->drawRoundedRect(boundingRect, xR, xR);
    if (mLeftNeighbour || mRightNeighbour) {
        if (mLeftNeighbour) {
            p->setClipRect(0, 0, width()/2, height());
        } else if (mRightNeighbour) {
            p->setClipRect(width()/2, 0, width()/2, height());
        }
        p->drawRect(boundingRect);
        p->setClipping(false);
    }

    if (!textEditing()) {
        if (mShowValueSlider) {
            p->setPen(Qt::NoPen);
            const qreal valFrac = (mValue - mMinValue)/(mMaxValue - mMinValue);
            const qreal valWidth = clamp(valFrac*width(), 0, width() - 3);

            if (mHovered && isEnabled()) { p->setBrush(ThemeSupport::getThemeHighlightDarkerColor()); }
            else { p->setBrush(sliderFill); }

            const qreal heightRemoval = qMax(0., eSizesUI::widget/2 - valWidth)*0.5;
            p->drawRoundedRect(QRectF(1, 1, valWidth, height() - 2).
                               adjusted(0, heightRemoval,
                                        0, -heightRemoval), xR, xR);
        }

        p->setPen(mHovered && isEnabled() ? Qt::white : text);

        if (mShowName) {
            p->drawText(rect(), Qt::AlignCenter,
                        mName + ": " + mValueString);
        } else {
            p->drawText(rect(), Qt::AlignCenter, mValueString);
        }
    }
    p->setPen(QPen(stroke, 1));
    p->setBrush(Qt::NoBrush);

    if (mLeftNeighbour) {
        p->setClipRect(width()/2, 0, width()/2, height());
    } else if (mRightNeighbour) {
        p->setClipRect(0, 0, width()/2, height());
    }

    p->drawRoundedRect(boundingRect, xR, xR);

    if (mLeftNeighbour || mRightNeighbour) {
        if (mLeftNeighbour) {
            boundingRect.adjust(-1, 0, 0, 0);
            p->setClipRect(0, 0, width()/2, height());
        } else if (mRightNeighbour) {
            boundingRect.adjust(0, 0, 1, 0);
            p->setClipRect(width()/2, 0, width()/2, height());
        }
        p->drawRect(boundingRect);
        p->setClipping(false);
    }

    p->restore();
}

void QDoubleSlider::startTransform(const qreal value) {
    emit editingStarted(value);
}

void QDoubleSlider::setValue(const qreal value) {
    setDisplayedValue(value);
    emit valueEdited(mValue);
}

void QDoubleSlider::finishTransform(const qreal value) {
    releaseMouse();
    releaseKeyboard();
    if(QApplication::overrideCursor())
        QApplication::restoreOverrideCursor();
    Actions::sInstance->finishSmoothChange();
    emit editingFinished(value);
}

void QDoubleSlider::cancelTransform() {
    releaseMouse();
    releaseKeyboard();
    if(QApplication::overrideCursor())
        QApplication::restoreOverrideCursor();
    Actions::sInstance->finishSmoothChange();
    Document::sInstance->actionFinished();
    mCanceled = true;
    emit editingCanceled();
}

void QDoubleSlider::updateValueString() {
    mValueString = valueToText(mValue);
    update();
}

void QDoubleSlider::setDisplayedValue(const qreal value) {
    const qreal clampedValue = clamped(value);
    if(isZero4Dec(mValue - clampedValue)) return;
    mValue = clampedValue;
    updateValueString();
}

void QDoubleSlider::fitWidthToContent() {
    const QFontMetrics fm(QApplication::font());
    QString textMax;
    QString textMin;
    if(mShowName) {
        textMax = mName + ": " + valueToText(mMaxValue);
        textMin = mName + ": " + valueToText(mMinValue);
    } else {
        textMax = valueToText(mMaxValue);
        textMin = valueToText(mMinValue);
    }
    const int textWidth = qMax(fm.horizontalAdvance(textMax),
                               fm.horizontalAdvance(textMin));
    const int maxWidth = (mShowName ? 6 : 3)*eSizesUI::widget;
    const int padding = eSizesUI::widget/2;
    const int newWidth = qMin(maxWidth, textWidth + padding);
    setFixedWidth(newWidth);
    mLineEdit->setFixedWidth(newWidth);
}

void QDoubleSlider::paintEvent(QPaintEvent *) {
    QPainter p(this);
    paint(&p);
    p.end();
}

void QDoubleSlider::mouseDoubleClickEvent(QMouseEvent *event) {
    Q_UNUSED(event)
}

void QDoubleSlider::mousePressEvent(QMouseEvent *event) {
    if(event->button() == Qt::RightButton) {
        if(cancelMove()) {}
        else openContextMenu(event->globalPos());
    } else if(event->button() == Qt::LeftButton) {
        Actions::sInstance->startSmoothChange();
        mMouseMoved = false;
        mMovesCount = 0;
        mCanceled = false;
        mGlobalPressPos = event->globalPos();
        mLastX = event->globalX();
        mLastValue = startSlideValue();
    }
}

void QDoubleSlider::mouseReleaseEvent(QMouseEvent* event) {
    if(mCanceled) return;
    if(event->button() != Qt::LeftButton) return;
    if(mMouseMoved) {
        mMouseMoved = false;
        finishTransform(mLastValue);
        Document::sInstance->actionFinished();
    } else {
        updateLineEditFromValue();
        mCanceled = false;
        mLineEdit->show();
        mLineEdit->setFocus();
        mLineEdit->selectAll();
    }
}

void QDoubleSlider::setPrefferedValueStep(const qreal step) {
    mPrefferedValueStep = step;
}

void QDoubleSlider::setIsRightSlider(const bool value) {
    mLeftNeighbour = value;
    update();
}

void QDoubleSlider::setIsLeftSlider(const bool value) {
    mRightNeighbour = value;
    update();
}

void QDoubleSlider::mouseMoveEvent(QMouseEvent *event) {
    if(mCanceled) return;
    if(!(event->buttons() & Qt::LeftButton)) return;
    if(mMovesCount++ < 2) return;
    if(!mMouseMoved) {
        grabMouse();
        grabKeyboard();
        QApplication::setOverrideCursor(Qt::BlankCursor);
        startTransform(mLastValue);
        mMouseMoved = true;
    }

    const qreal dValue = getDValueForMouseMove(event->globalX());
    mLastValue = clamped(mLastValue + dValue);
    setValue(mLastValue);

    cursor().setPos(mGlobalPressPos);
    Document::sInstance->updateScenes();
}

#ifdef Q_OS_MAC
void QDoubleSlider::wheelEvent(QWheelEvent *event)
{
    const bool alt = event->modifiers() & Qt::AltModifier;
    const bool ctrl = event->modifiers() & Qt::ControlModifier;

    if (event->phase() == Qt::NoScrollPhase && (alt || ctrl)) {
        event->accept();
        mLastValue = mValue;
        startTransform(mLastValue);

        const bool up = event->angleDelta().y() > 0;
        const qreal step = 0.1 * (up ? (ctrl ? 50 : 10) : (ctrl ? -50 : -10)) * mPrefferedValueStep;

        mLastValue = clamped(mLastValue + step);
        setValue(mLastValue);
        Document::sInstance->updateScenes();

        finishTransform(mLastValue);
        Document::sInstance->actionFinished();
        return;
    }

    if (event->phase() == Qt::ScrollBegin) {
        event->accept();
        mLastValue = mValue;
        startTransform(mLastValue);
        return;
    } else if (event->phase() == Qt::ScrollEnd) {
        event->accept();
        finishTransform(mLastValue);
        Document::sInstance->actionFinished();
        return;
    }
    if (event->angleDelta().x() == 0 ||
        (event->phase() != Qt::ScrollUpdate &&
         event->phase() != Qt::ScrollMomentum)) { return; }

    event->accept();
    const qreal step = 0.1 * (event->inverted() ?
                                  event->angleDelta().x() :
                                  -event->angleDelta().x()) * mPrefferedValueStep;
    mLastValue = clamped(mLastValue + step);
    setValue(mLastValue);
    Document::sInstance->updateScenes();
}
#endif

void QDoubleSlider::enterEvent(QEvent *)
{
    mHovered = true;
    setCursor(Qt::SizeHorCursor);
    update();
}

void QDoubleSlider::leaveEvent(QEvent *)
{
    mHovered = false;
    unsetCursor();
    update();
}
