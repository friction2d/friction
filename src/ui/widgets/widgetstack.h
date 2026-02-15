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

#ifndef WIDGETSTACK_H
#define WIDGETSTACK_H

#include "ui_global.h"

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QBoxLayout>
#include <QMainWindow>

#include "GUI/global.h"
#include "XML/xmlexporthelpers.h"

#define STACK_TMPL_DEFS \
    int (QWidget::*DimGetter)() const, void (*DimSetter)(QWidget*, int), \
    int (QWidget::*PosGetter)() const, void (*PosSetter)(int, QWidget*), \
    int (QWidget::*OtherDimGetter)() const, void (*OtherDimSetter)(QWidget*, int), \
    int (QMouseEvent::*MousePosGetter)() const, class TResizer

#define V_STACK_TMPL \
    &QWidget::height, &gResizeH, \
    &QWidget::y, &gMoveY, \
    &QWidget::width, &gResizeW, \
    &QMouseEvent::y, VStackResizer

#define H_STACK_TMPL \
    &QWidget::width, &gResizeW, \
    &QWidget::x, &gMoveX, \
    &QWidget::height, &gResizeH, \
    &QMouseEvent::x, HStackResizer

void gMoveX(const int x, QWidget * const widget);
void gMoveY(const int y, QWidget * const widget);

void gResizeW(QWidget * const widget, const int w);
void gResizeH(QWidget * const widget, const int h);

bool gReplaceWidget(QWidget * const from, QWidget * const to,
                    bool * const centralWid = nullptr);

template <STACK_TMPL_DEFS>
class UI_EXPORT StackResizerBase {
protected:
    StackResizerBase() {}

    void setThis(QWidget * const thisP) {
        mThis = thisP;
        DimSetter(mThis, 10);
        OtherDimSetter(mThis, 2*eSizesUI::widget*100);
        mThis->setWindowFlags(Qt::WindowStaysOnTopHint);
        mThis->show();
    }

    void displace(const int totDDim) {
        int dDim = totDDim;
        const int newAboveDim = (mPrevWidget->*DimGetter)() + dDim;
        const int newBelowDim = (mNextWidget->*DimGetter)() - dDim;
        if(newAboveDim < 2*eSizesUI::widget) {
            dDim = 2*eSizesUI::widget - (mPrevWidget->*DimGetter)();
        } else if(newBelowDim < 2*eSizesUI::widget) {
            dDim = (mNextWidget->*DimGetter)() - 2*eSizesUI::widget;
        }
        if(totDDim != dDim) {
            if(totDDim > 0) {
                if(mNextResizer)
                    mNextResizer->displace(totDDim - dDim);
            } else {
                if(mPrevResizer)
                    mPrevResizer->displace(totDDim - dDim);
            }
        }
        DimSetter(mPrevWidget, (mPrevWidget->*DimGetter)() + dDim);
        DimSetter(mNextWidget, (mNextWidget->*DimGetter)() - dDim);
        PosSetter((mNextWidget->*PosGetter)() + dDim, mNextWidget);
        PosSetter((mThis->*PosGetter)() + dDim, mThis);
    }

    void mouseMoveEventB(QMouseEvent * const event) {
        displace((event->*MousePosGetter)() - mPressDim);
    }

    void mousePressEventB(QMouseEvent * const event) {
        mPressed = true;
        mPressDim = (event->*MousePosGetter)();
        mThis->update();
    }

    void mouseReleaseEventB(QMouseEvent * const) {
        mPressed = false;
        mThis->update();
    }

    void enterEventB() {
        mHover = true;
        mThis->update();
    }

    void leaveEventB() {
        mHover = false;
        mThis->update();
    }
public:
    void setAboveWidget(QWidget * const aboveWidget) {
        mPrevWidget = aboveWidget;
    }

    void setBelowWidget(QWidget * const belowWidget) {
        mNextWidget = belowWidget;
    }

    void setAboveResizer(StackResizerBase * const aboveResizer) {
        mPrevResizer = aboveResizer;
    }

    void setBelowResizer(StackResizerBase * const belowResizer) {
        mNextResizer = belowResizer;
    }

    bool hover() const { return mHover; }
    bool pressed() const { return mPressed; }
private:
    QWidget *mThis = nullptr;

    bool mHover = false;
    bool mPressed = false;
    int mPressDim;
    StackResizerBase *mPrevResizer = nullptr;
    StackResizerBase *mNextResizer = nullptr;
    QWidget *mPrevWidget = nullptr;
    QWidget *mNextWidget = nullptr;
};

class UI_EXPORT VStackResizer : public QWidget, public StackResizerBase<V_STACK_TMPL> {
    Q_OBJECT
public:
    VStackResizer(QWidget * const parent) : QWidget(parent) {
        setThis(this);
        setCursor(Qt::SplitVCursor);
    }
protected:
    void paintEvent(QPaintEvent *) {
        QPainter p(this);
        if(pressed()) {
            p.fillRect(rect().adjusted(0, 3, 0, -4), Qt::black);
        } else if(hover()) {
            p.fillRect(rect().adjusted(0, 4, 0, -4), Qt::black);
        } else {
            p.fillRect(rect().adjusted(0, 5, 0, -4), Qt::black);
        }
        p.end();
    }

    void mouseMoveEvent(QMouseEvent * const event) {
        mouseMoveEventB(event);
    }

    void mousePressEvent(QMouseEvent * const event) {
        mousePressEventB(event);
    }

    void mouseReleaseEvent(QMouseEvent * const event) {
        mouseReleaseEventB(event);
        emit finishedChanging();
    }

    void enterEvent(QEvent *) {
        enterEventB();
    }

    void leaveEvent(QEvent *) {
        leaveEventB();
    }
signals:
    void finishedChanging();
};

class UI_EXPORT HStackResizer : public QWidget, public StackResizerBase<H_STACK_TMPL> {
    Q_OBJECT
public:
    HStackResizer(QWidget * const parent) : QWidget(parent) {
        setThis(this);
        setCursor(Qt::SplitHCursor);
    }
protected:
    void paintEvent(QPaintEvent *) {
        QPainter p(this);
        if(pressed()) {
            p.fillRect(rect().adjusted(3, 0, -4, 0), Qt::black);
        } else if(hover()) {
            p.fillRect(rect().adjusted(4, 0, -4, 0), Qt::black);
        } else {
            p.fillRect(rect().adjusted(5, 0, -4, 0), Qt::black);
        }
        p.end();
    }

    void mouseMoveEvent(QMouseEvent * const event) {
        mouseMoveEventB(event);
    }

    void mousePressEvent(QMouseEvent * const event) {
        mousePressEventB(event);
    }

    void mouseReleaseEvent(QMouseEvent * const event) {
        mouseReleaseEventB(event);
        emit finishedChanging();
    }

    void enterEvent(QEvent *) {
        enterEventB();
    }

    void leaveEvent(QEvent *) {
        leaveEventB();
    }
signals:
    void finishedChanging();
};

template <STACK_TMPL_DEFS>
class UI_EXPORT WidgetStackBase {
public:
    virtual ~WidgetStackBase() {
        for(int i = 0; i < mWidgets.count();) {
            delete takeWidget(i);
        }
    }
protected:
    WidgetStackBase() {}

    void setThis(QWidget * const thisP) {
        Q_ASSERT(!mThis && thisP);
        mThis = thisP;
    }

    void updateSizesAndPositions() {
        if(mWidgets.isEmpty()) return;

        int accumulated = 0;
        const int wCount = mWidgets.count() - 1;
        const int thisDim = (mThis->*DimGetter)();
        const int thisOtherDim = (mThis->*OtherDimGetter)();;
        for(int i = 0; i < wCount; i++) {
            const auto widget = mWidgets.at(i);
            PosSetter(accumulated, widget);
            const int iNewDim = qMax(minDim(), qRound(mDimPercent.at(i)*thisDim));
            DimSetter(widget, iNewDim);
            OtherDimSetter(widget, thisOtherDim);
            accumulated += iNewDim;
            if(i < mResizers.count()) {
                PosSetter(accumulated - 5, mResizers.at(i));
            }
        }
        const auto lastWidget = mWidgets.last();
        PosSetter(accumulated, lastWidget);
        const int lastDim = qMax(minDim(), thisDim - accumulated);
        DimSetter(lastWidget, lastDim);
        OtherDimSetter(lastWidget, thisOtherDim);
    }

    void updateResizers() {
        TResizer *lastRes = nullptr;
        for(int i = 0; i < mWidgets.count() - 1; i++) {
            const auto currWid = mWidgets.at(i);
            const auto nextWid = mWidgets.at(i + 1);

            TResizer *res;
            if(i < mResizers.count()) {
                res = mResizers.at(i);
            } else {
                res = new TResizer(mThis);
                mResizers << res;
                QObject::connect(res, &TResizer::finishedChanging,
                                 mThis, [this]() { updatePercent(); });
            }
            res->raise();
            res->setAboveWidget(currWid);
            res->setBelowWidget(nextWid);
            PosSetter((nextWid->*PosGetter)(), res);
            res->setAboveResizer(lastRes);
            if(lastRes) lastRes->setBelowResizer(res);
            lastRes = res;
        }
        if(lastRes) lastRes->setBelowResizer(nullptr);
        for(int i = qMax(0, mWidgets.count() - 1); i < mResizers.count(); i++) {
            delete mResizers.takeAt(i);
        }
    }

    void updatePercent() {
        int totDim = 0;
        for(const auto wid : qAsConst(mWidgets)) {
            const int widDim = (wid->*DimGetter)();
            totDim += qMax(minDim(), widDim);
        }

        mDimPercent.clear();
        for(int i = 0; i < mWidgets.count(); i++) {
            const int widDim = qMax(minDim(), (mWidgets.at(i)->*DimGetter)());
            mDimPercent << qreal(widDim)/totDim;
        }
    }

    int minDim() const {
        return mThis->isVisible() ? 2*eSizesUI::widget : 1;
    }
public:
    qreal percentAt(const int id) const {
        return mDimPercent.at(id);
    }

    int childId(QWidget * const child) const {
        return mWidgets.indexOf(child);
    }

    void appendWidget(QWidget * const widget, const qreal frac = 0.5) {
        insertWidget(mWidgets.count(), widget, frac);
    }

    void insertWidget(const int id, QWidget * const widget,
                      const qreal frac = 0.5) {
        mWidgets.insert(id >= 0 ? id : mWidgets.count(), widget);
        widget->setParent(mThis);
        if(id > 0 && mWidgets.count() > 1) {
            const auto prevWid = mWidgets.at(id - 1);
            const int prevDim = (prevWid->*DimGetter)();
            const int widDim = qRound(prevDim*frac);
            DimSetter(widget, widDim);
            DimSetter(prevWid, prevDim - widDim);
        } else {
            DimSetter(widget, minDim());
        }
        updateAll();
        widget->show();

        QObject::connect(widget, &QObject::destroyed, mThis,
                         [this, widget]() { removeWidgetBeforeDestroyed(widget); });
    }

    QWidget* replaceWidget(QWidget * const oldWid, QWidget * const newWid) {
        const int id = mWidgets.indexOf(oldWid);
        if(id == -1) return nullptr;
        mWidgets.replace(id, newWid);
        newWid->setParent(mThis);
        oldWid->setParent(nullptr);
        DimSetter(newWid, (oldWid->*DimGetter)());
        updateAll();
        newWid->show();
        QObject::connect(newWid, &QObject::destroyed, mThis,
                         [this, newWid]() { removeWidgetBeforeDestroyed(newWid); });
        QObject::disconnect(oldWid, nullptr, mThis, nullptr);
        return oldWid;
    }

    QWidget* takeWidget(const int id) {
        return takeWidget(mWidgets.at(id));
    }

    QWidget* takeWidget(QWidget * const widget) {
        if(mWidgets.removeOne(widget)) {
            QObject::disconnect(widget, nullptr, mThis, nullptr);
            updateAll();
            widget->setParent(nullptr);
            return widget;
        }
        return nullptr;
    }

    QWidget* asWidget() { return mThis; }

    int count() const { return mWidgets.count(); }
    bool isEmpty() const { return mWidgets.isEmpty(); }
private:
    void updateAll() {
        updatePercent();
        updateResizers();
        updateSizesAndPositions();
    }

    bool removeWidgetBeforeDestroyed(QObject * const widObj) {
        mWidgets.removeOne(static_cast<QWidget*>(widObj));
        updateAll();
        return false;
    }

    QWidget * mThis = nullptr;
    QList<QWidget*> mWidgets;
    QList<qreal> mDimPercent;
    QList<TResizer*> mResizers;
};

struct VSplitStackItem;
struct HSplitStackItem;

class UI_EXPORT VWidgetStack : public QWidget, public WidgetStackBase<V_STACK_TMPL> {
public:
    VWidgetStack(QWidget * const parent = nullptr);
protected:
    void resizeEvent(QResizeEvent *) {
        updateSizesAndPositions();
    }
};

class UI_EXPORT HWidgetStack : public QWidget, public WidgetStackBase<H_STACK_TMPL> {
public:
    HWidgetStack(QWidget * const parent = nullptr);
protected:
    void resizeEvent(QResizeEvent *) {
        updateSizesAndPositions();
    }
};

#endif // WIDGETSTACK_H
