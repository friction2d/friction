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

#include "keysview.h"
#include "Animators/qrealkey.h"
#include "clipboardcontainer.h"
#include <QPainter>
#include <QMenu>
#include "mainwindow.h"
#include "GUI/BoxesList/boxscrollwidget.h"
#include "GUI/BoxesList/boxscroller.h"
#include "GUI/BoxesList/boxsinglewidget.h"
#include "Timeline/durationrectangle.h"
#include "GUI/global.h"
#include "pointhelpers.h"
#include "canvaswindow.h"
#include <QApplication>
#include "clipboardcontainer.h"
#include "Animators/qrealpoint.h"
#include "timelinehighlightwidget.h"
#include "GUI/dialogsinterface.h"
#include "themesupport.h"

KeysView::KeysView(BoxScrollWidget *boxesListVisible,
                   QWidget *parent) :
    QWidget(parent), mBoxesListWidget(boxesListVisible) {
    mBoxesListWidget->setSiblingKeysView(this);

    setMouseTracking(true);
    setAcceptDrops(true);
    mScrollTimer = new QTimer(this);

    eSizesUI::widget.add(this, [this](int) {
        updatePixelsPerFrame();
        update();
    });
}

TimelineHighlightWidget *KeysView::requestHighlighter() {
    if(!mHighlighter) {
        mHighlighter = new TimelineHighlightWidget(true, this, false);
        mHighlighter->resize(size());
    }
    return mHighlighter;
}

void KeysView::dropEvent(QDropEvent *event) {
    const int frame = qRound(xToFrame(event->posF().x()));
    Actions::sInstance->handleDropEvent(event, QPointF(0, 0), frame);
}

void KeysView::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls() ||
        event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist"))
    {
        event->acceptProposedAction();
        KFT_setFocus();
    }
}

void KeysView::dragMoveEvent(QDragMoveEvent *event) {
    if(event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void KeysView::setCurrentScene(Canvas * const scene)
{
    if (mCurrentScene) {
        disconnect(mCurrentScene.data(), &Canvas::objectSelectionChanged,
                   this, &KeysView::graphUpdateVisible);
        disconnect(mCurrentScene.data(), &Canvas::requestEasingAction,
                   this, &KeysView::graphEasingAction);
    }
    mCurrentScene = scene;
    if (mCurrentScene) {
        connect(mCurrentScene.data(), &Canvas::objectSelectionChanged,
                this, &KeysView::graphUpdateVisible);
        connect(mCurrentScene.data(), &Canvas::requestEasingAction,
                this, &KeysView::graphEasingAction);
    }
    graphUpdateVisible();
}

void KeysView::setGraphViewed(const bool bT) {
    if(mHighlighter) mHighlighter->setHidden(bT);
    mGraphViewed = bT;
    if(bT) {
        graphResetValueScaleAndMinShown();
        mValueInput.setForce1D(false);
    } else {
        mValueInput.setForce1D(true);
    }
    update();
}

void KeysView::middlePress(const QPointF &pressPos) {
    mSavedMinViewedFrame = mMinViewedFrame;
    mSavedMaxViewedFrame = mMaxViewedFrame;
    mMiddlePressPos = pressPos;
}

void KeysView::setViewedVerticalRange(const int top,
                                      const int bottom) {
    mViewedTop = top;
    mViewedBottom = bottom;
    update();
}

void KeysView::middleMove(const QPointF &movePos) {
    const qreal diffFrame = (movePos.x() - mMiddlePressPos.x())/
                                    mPixelsPerFrame;
    const int roundX = qRound(diffFrame);
    setFramesRange({mSavedMinViewedFrame - roundX,
                    mSavedMaxViewedFrame - roundX});

    emit changedViewedFrames({mMinViewedFrame, mMaxViewedFrame});
}

void KeysView::deleteSelectedKeys() {
    if(mHoveredKey && mHoveredKey->isSelected()) {
        clearHoveredKey();
    }
    for(const auto& anim : qAsConst(mSelectedKeysAnimators))
        anim->anim_deleteSelectedKeys();
}


void KeysView::getKeysInRect(QRectF selectionRect,
                             const qreal pixelsPerFrame,
                             QList<Key*>& listKeys) {
    const auto mainAbs = mBoxesListWidget->getMainAbstration();
    if(!mainAbs) return;
    const auto rules = mBoxesListWidget->getRulesCollection();
    QList<SWT_Abstraction*> abstractions;
//    selectionRect.adjust(-0.5, -(BOX_HEIGHT/* + KEY_RECT_SIZE*/)*0.5,
//                         0.5, (BOX_HEIGHT/* + KEY_RECT_SIZE*/)*0.5);
    selectionRect.adjust(0.5, 0, 0.5, 0);
    const int minX = qRound(selectionRect.top() - eSizesUI::widget*0.5);
    const int minY = qRound(selectionRect.bottom() - eSizesUI::widget*0.5);
    int currY = 0;
    const SetAbsFunc setter = [&abstractions](SWT_Abstraction * abs, int) {
        abstractions.append(abs);
    };
    mainAbs->setAbstractions(
            minX, minY, currY, 0, eSizesUI::widget,
            setter, rules, true, false);

    for(const auto& abs : abstractions) {
        const auto target = abs->getTarget();
        if(const auto asAnim = enve_cast<Animator*>(target)) {
            asAnim->anim_getKeysInRect(selectionRect, pixelsPerFrame,
                                       listKeys, KEY_RECT_SIZE);
        }
    }
}

void KeysView::selectKeysInSelectionRect() {
    if(mGraphViewed) {
        QList<GraphKey*> keysList;
        for(const auto& anim : qAsConst(mGraphAnimators)) {
            anim->gAddKeysInRectToList(mSelectionRect, keysList);
        }
        for(const auto& key : qAsConst(keysList)) {
            addKeyToSelection(key);
        }
    } else {
        QList<Key*> listKeys;
        getKeysInRect(mSelectionRect.translated(-0.5, 0),
                      mPixelsPerFrame, listKeys);
        for(const auto& key : qAsConst(listKeys)) {
            addKeyToSelection(key);
        }
    }
}

bool KeysView::hasFrameIn(const int frame)
{
    if (!mCurrentScene) { return false; }
    const auto frameIn = mCurrentScene->getFrameIn();
    if (frameIn.enabled && frame == frameIn.frame) { return true; }
    return false;
}

bool KeysView::hasFrameOut(const int frame)
{
    if (!mCurrentScene) { return false; }
    const auto frameOut = mCurrentScene->getFrameOut();
    if (frameOut.enabled && frame == frameOut.frame) { return true; }
    return false;
}

bool KeysView::hasFrameMarker(const int frame)
{
    if (!mCurrentScene) { return false; }
    return mCurrentScene->hasMarkerEnabled(frame);
}

void KeysView::resizeEvent(QResizeEvent *e) {
    if(mHighlighter) mHighlighter->resize(e->size());
    updatePixelsPerFrame();
    if(mGraphViewed) graphResizeEvent(e);
}

void KeysView::wheelEvent(QWheelEvent *e)
{
#ifdef Q_OS_MAC
    const bool alt = e->modifiers() & Qt::AltModifier;
    const bool ctrl = e->modifiers() & Qt::ControlModifier;
    const bool shift = e->modifiers() & Qt::ShiftModifier;
    const bool noMod = !alt && !ctrl && !shift;
    const auto x = qAbs(e->angleDelta().x());
    const auto y = qAbs(e->angleDelta().y());
    //qDebug() << e->phase() << x << y;
    if (e->phase() != Qt::NoScrollPhase && noMod) {
        if (e->phase() == Qt::ScrollBegin) { mPanEvent = x > y; }
        else if (e->phase() == Qt::ScrollEnd) { mPanEvent = false; }
        if (mPanEvent) { emit panEventSignal(e); }
    }
#endif

    const QPoint pos = e->position().toPoint();
    const QPoint posU = pos + QPoint(-eSizesUI::widget/2, 0);
    const qreal currentHoverFrame = static_cast<qreal>(posU.x()) / mPixelsPerFrame + mMinViewedFrame;
    // qDebug() << "currentHoverFrame" << currentHoverFrame;

    if (mGraphViewed) {
        if (!mPanEvent) { graphWheelEvent(e, currentHoverFrame); }
    } else {
        if (!mPanEvent) { emit wheelEventSignal(e, currentHoverFrame); }
        if (mSelecting) {
            const QPointF posU = mapFromGlobal(QCursor::pos()) +
                           QPointF(-eSizesUI::widget/2, 0.);
            mSelectionRect.setBottom(posU.y() + mViewedTop);
        }
        //mBoxesList->handleWheelEvent(e);
    }
}

void KeysView::cancelTransform() {
    if(!mFirstMove) {
        if(mGraphViewed) {
            for(const auto& anim : qAsConst(mGraphAnimators)) {
                anim->graph_cancelSelectedKeysTransform();
            }
        } else {
            for(const auto& anim : qAsConst(mSelectedKeysAnimators)) {
                anim->anim_cancelSelectedKeysTransform();
            }
        }
    }

    releaseMouseAndDontTrack();
}

void KeysView::finishTransform() {
    if(!mFirstMove) {
        if(mGraphViewed) {
            if(mGPressedPoint && mGPressedPoint->isCtrlPt()) {
                mGPressedPoint->finishTransform();
            } else {
                for(const auto& anim : qAsConst(mGraphAnimators)) {
                    anim->graph_finishSelectedKeysTransform();
                }
            }
        } else {
            for(const auto& anim : qAsConst(mSelectedKeysAnimators)) {
                anim->anim_finishSelectedKeysTransform();
            }
        }
    }

    releaseMouseAndDontTrack();
}

TimelineMovable *KeysView::getRectangleMovableAtPos(
        const int pressX, const int pressY,
        const qreal pixelsPerFrame,
        const int minViewedFrame) {
    const auto& wids = mBoxesListWidget->visibleWidgets();
    for(const auto& container : wids) {
        const int containerTop = container->y();
        const int containerBottom = containerTop + container->height();
        if(containerTop > pressY || containerBottom < pressY) continue;
        const auto bsw = static_cast<BoxSingleWidget*>(container);
        return bsw->getRectangleMovableAtPos(pressX, pixelsPerFrame,
                                             minViewedFrame);
    }
    return nullptr;
}

void KeysView::mousePressEvent(QMouseEvent *e) {
    KFT_setFocus();
    const QPoint posU = e->pos() + QPoint(-eSizesUI::widget/2, 0);
    if(e->button() == Qt::MiddleButton) {
        if(mGraphViewed) gMiddlePress(posU);
        else middlePress(posU);
    } else if(e->button() == Qt::LeftButton) {
        if(mIsMouseGrabbing) return;
        mFirstMove = true;
        mLastPressPos = posU;
        if(mGraphViewed) graphMousePress(posU);
        else {
            mLastPressedKey = getKeyAtPos(posU.x(), posU.y(),
                                          mPixelsPerFrame,
                                          mMinViewedFrame);
            const bool shiftPressed = e->modifiers() & Qt::SHIFT;
            if(!mLastPressedKey) {
                mLastPressedMovable = getRectangleMovableAtPos(
                                            posU.x(), posU.y(),
                                            mPixelsPerFrame,
                                            mMinViewedFrame);
                if(!mLastPressedMovable) {
                    mSelecting = true;
                    const qreal posUXFrame = xToFrame(posU.x());
                    const QPointF xFramePos(posUXFrame, posU.y() + mViewedTop);
                    mSelectionRect.setTopLeft(xFramePos);
                    mSelectionRect.setBottomRight(xFramePos);
                } else {
                    mLastPressedMovable->pressed(shiftPressed);
                    mMovingRect = true;
                }
            } else {
                if(!shiftPressed && !mLastPressedKey->isSelected()) {
                    clearKeySelection();
                }
                if(shiftPressed && mLastPressedKey->isSelected()) {
                    removeKeyFromSelection(mLastPressedKey);
                } else {
                    clearHoveredKey();
                    addKeyToSelection(mLastPressedKey);

                    mMovingKeys = true;
                    mValueInput.setupMove();
                }
            }
        }
    } else {
        if (mMovingKeys) {
            cancelTransform();
        } else {
            auto movable = getRectangleMovableAtPos(posU.x(),
                                                    posU.y(),
                                                    mPixelsPerFrame,
                                                    mMinViewedFrame);
            if (!movable) {
            } else if (movable->isDurationRect()) {
                QMenu menu;
                const QString editStr = tr("Edit duration");
                const QString splitStr = tr("Split Clip");
                menu.addAction(QIcon::fromTheme("sequence"), editStr);
                menu.addAction(QIcon::fromTheme("cut"), splitStr);
                const auto selectedAction = menu.exec(e->globalPos());
                if (selectedAction) {
                    if (selectedAction->text() == editStr) {
                        const auto durRect = static_cast<DurationRectangle*>(movable);
                        if (!durRect) { return; }
                        const auto& instance = DialogsInterface::instance();
                        instance.showDurationSettingsDialog(durRect);
                    } else if (selectedAction->text() == splitStr) {
                        if (mCurrentScene) { mCurrentScene->splitAction(); }
                    }
                }
            }
        }
    }

    grabMouseAndTrack();
    mValueInput.clearAndDisableInput();
    Document::sInstance->actionFinished();
}

stdsptr<KeysClipboard> KeysView::getSelectedKeysClipboardContainer() {
    stdsptr<KeysClipboard> container =
            enve::make_shared<KeysClipboard>();
    for(const auto& anim : qAsConst(mSelectedKeysAnimators)) {
        QByteArray keyData;
        QBuffer buffer(&keyData);
        buffer.open(QIODevice::WriteOnly);
        eWriteStream writeStream(&buffer);
        anim->anim_writeSelectedKeys(writeStream);
        buffer.close();
        container->addTargetAnimator(anim, keyData);
    }
    return container;
}

bool KeysView::KFT_keyPressEvent(QKeyEvent *event)
{
#ifdef Q_OS_MAC
    if (event->type() == QEvent::ShortcutOverride) { return false; }
#endif
    bool inputHandled = false;
    if(mMovingKeys) {
        if(mValueInput.handleTransormationInputKeyEvent(event->key())) {
            inputHandled = true;
        }
    }
    const QPoint posU = mapFromGlobal(QCursor::pos()) +
            QPoint(-eSizesUI::widget/2, 0);
    if(inputHandled) {
        handleMouseMove(mLastMovePos, QApplication::mouseButtons());
    } else if(event->key() == Qt::Key_Escape) {
        if(!mIsMouseGrabbing) return false;
        cancelTransform();
    } else if(event->key() == Qt::Key_Return ||
              event->key() == Qt::Key_Enter) {
        finishTransform();
    } else if(graphProcessFilteredKeyEvent(event)) {
    } else if(event->modifiers() & Qt::ControlModifier &&
              event->key() == Qt::Key_V) {
        if(event->isAutoRepeat()) return false;
        const auto container = Document::sInstance->getKeysClipboard();
        if(!container) return false;
        clearKeySelection();
        container->paste(mCurrentScene->getCurrentFrame(), true,
                         [this](Key* key) { addKeyToSelection(key); });
    } else if(!mSelectedKeysAnimators.isEmpty()) {
        if(event->modifiers() & Qt::ControlModifier &&
           event->key() == Qt::Key_C) {
            if(event->isAutoRepeat()) return false;
            auto container = getSelectedKeysClipboardContainer();
            Document::sInstance->replaceClipboard(container);
        } else if(event->key() == Qt::Key_S) {
            if(!mMovingKeys) {
                mValueInput.setupScale();
                mScalingKeys = true;
                mMovingKeys = true;
                mFirstMove = true;
                mLastPressPos = posU;
                grabMouseAndTrack();
            }
        } else if(event->key() == Qt::Key_G) {
            if(!mMovingKeys) {
                mValueInput.setupMove();
                mMovingKeys = true;
                mFirstMove = true;
                mLastPressPos = posU;
                grabMouseAndTrack();
            }
        } else if(event->modifiers() & Qt::SHIFT &&
                  event->key() == Qt::Key_D) {
            auto container = getSelectedKeysClipboardContainer();
            int lowestKey = FrameRange::EMAX;
            for(const auto& anim : qAsConst(mSelectedKeysAnimators)) {
                const int animLowest = anim->anim_getLowestAbsFrameForSelectedKey();
                if(animLowest < lowestKey) {
                    lowestKey = animLowest;
                }
            }
            clearKeySelection();
            container->paste(lowestKey, false,
                             [this](Key* key) { addKeyToSelection(key); });

            mValueInput.setupMove();
            mMovingKeys = true;
            mFirstMove = true;
            mLastPressPos = posU;
            grabMouseAndTrack();
        } else if(event->modifiers() & Qt::CTRL &&
                  event->key() == Qt::Key_D) {
            auto container = getSelectedKeysClipboardContainer();
            clearKeySelection();
            container->paste(mCurrentScene->getCurrentFrame(), true,
                             [this](Key* key) { addKeyToSelection(key); });
         } else if(event->key() == Qt::Key_Delete) {
            if(mGraphViewed) {
                graphDeletePressed();
            } else {
                deleteSelectedKeys();
            }
            update();
        } else if(event->modifiers() & Qt::CTRL &&
                  event->key() == Qt::Key_Right) {
            for(const auto& anim : qAsConst(mSelectedKeysAnimators)) {
                anim->anim_incSelectedKeysFrame(1);
            }
        } else if(event->modifiers() & Qt::CTRL &&
                  event->key() == Qt::Key_Left) {
            for(const auto& anim : qAsConst(mSelectedKeysAnimators)) {
                anim->anim_incSelectedKeysFrame(-1);
            }
        } else return false;
    } else return false;
    return true;
}

#ifdef Q_OS_MAC
bool KeysView::event(QEvent *e)
{
    if (e->type() == QEvent::NativeGesture) {
        auto g = dynamic_cast<QNativeGestureEvent*>(e);
        if (g->gestureType() == Qt::ZoomNativeGesture ||
            g->gestureType() == Qt::SmartZoomNativeGesture) {
            const QPoint globalMousePos = QCursor::pos();
            const QPoint localMousePos = mapFromGlobal(globalMousePos);
            const QPoint posU = localMousePos + QPoint(-eSizesUI::widget/2, 0);
            const qreal currentHoverFrame = static_cast<qreal>(posU.x()) / mPixelsPerFrame + mMinViewedFrame;
            // qDebug() << "currentHoverFrame" << currentHoverFrame;
            emit nativeEventSignal(g, currentHoverFrame);
            return true;
        }
    }
    return QWidget::event(e);
}
#endif

void KeysView::drawKeys(QPainter * const p,
                        const qreal pixelsPerFrame,
                        const FrameRange &viewedFrameRange) {
    p->save();
    p->setPen(Qt::NoPen);
    const auto& wids = mBoxesListWidget->visibleWidgets();
    for(const auto& container : wids) {
        const auto bsw = static_cast<BoxSingleWidget*>(container);
        p->save();
        bsw->prp_drawTimelineControls(p, pixelsPerFrame, viewedFrameRange);
        p->restore();
        p->translate(0, container->height());
    }
    p->restore();
}

void KeysView::paintEvent(QPaintEvent *) {
    QPainter p(this);

    //if(mGraphViewed) p.fillRect(rect(), QColor(33, 33, 38));
    //else p.fillRect(rect(), QColor(33, 33, 38));
    p.fillRect(rect(), ThemeSupport::getThemeBaseColor());

    if(mPixelsPerFrame < 0.001) return;
    if(!mGraphViewed) {
        int currY = eSizesUI::widget;
        p.setPen(QPen(ThemeSupport::getThemeTimelineColor(), 2));
        while(currY < height()) {
            p.drawLine(0, currY, width(), currY);
            currY += eSizesUI::widget;
        }
    }
    p.translate(eSizesUI::widget/2, 0);

    p.setPen(QPen(ThemeSupport::getThemeTimelineColor(), 2));
    qreal xT = mPixelsPerFrame*0.5;
    int iInc = 1;
    bool mult5 = true;
    while(iInc*mPixelsPerFrame < eSizesUI::widget/2) {
        if(mult5) iInc *= 5;
        else iInc *= 2;
    }
    int minFrame = mMinViewedFrame;
    int maxFrame = mMaxViewedFrame;

    minFrame += qCeil((-xT)/mPixelsPerFrame);
    minFrame = minFrame - minFrame%iInc - 1;
    maxFrame += qFloor((width() - 40 - xT)/mPixelsPerFrame) - maxFrame%iInc;

    for(int i = minFrame; i <= maxFrame; i += iInc) {
        const qreal xTT = xT + (i - mMinViewedFrame + 1)*mPixelsPerFrame;
        p.drawLine(QPointF(xTT, 0), QPointF(xTT, height()));
    }

    // draw markers (and in/out)
    for (int i = minFrame; i <= maxFrame; i++) {
        bool hasIn = hasFrameIn(i+1);
        bool hasOut = hasFrameOut(i+1);
        bool hasMark = hasFrameMarker(i+1);
        if (!hasIn && !hasOut && !hasMark) { continue; }
        const QColor col = hasMark ? ThemeSupport::getThemeFrameMarkerColor() : ThemeSupport::getThemeColorGreen();
        p.setPen(QPen(col, 2, Qt::DotLine));
        const qreal xTT = xT + (i - mMinViewedFrame + 1)*mPixelsPerFrame;
        p.drawLine(QPointF(xTT, 0), QPointF(xTT, height()));
    }

    if (mCurrentScene) {
        if (mCurrentScene->getCurrentFrame() <= maxFrame &&
           mCurrentScene->getCurrentFrame() >= minFrame) {
            xT = (mCurrentScene->getCurrentFrame() - mMinViewedFrame)*mPixelsPerFrame + mPixelsPerFrame*0.5;
            p.setPen(QPen(ThemeSupport::getThemeHighlightColor(), 2));
            p.drawLine(QPointF(xT, 0), QPointF(xT, height()));
        }
    }

    p.setPen(QPen(Qt::black, 1));

    if(mGraphViewed) {
        p.save();
        graphPaint(&p);
        p.restore();
    } else {
        p.save();
        p.setRenderHint(QPainter::Antialiasing);
        const qreal transDFrame = 0.5*eSizesUI::widget/mPixelsPerFrame;
        const qreal frameAtZeroX = mMinViewedFrame - transDFrame;
        const int frameAtZeroXi = qFloor(frameAtZeroX);
        p.translate((frameAtZeroXi - mMinViewedFrame)*mPixelsPerFrame, 0);
        const int maxFrame = qCeil(mMaxViewedFrame + 3*transDFrame);
        const FrameRange viewedFrameRange{frameAtZeroXi, maxFrame};
        drawKeys(&p, mPixelsPerFrame, viewedFrameRange);
        p.restore();
        if(mSelecting) {
            p.setPen(QPen(Qt::white, 1.5, Qt::DotLine));
            p.setBrush(Qt::NoBrush);
            p.drawRect(QRectF((mSelectionRect.x() - mMinViewedFrame)*mPixelsPerFrame,
                              mSelectionRect.y() - mViewedTop,
                              mSelectionRect.width()*mPixelsPerFrame,
                              mSelectionRect.height()));
        }
    }

    p.resetTransform();
    if(mMovingKeys && mValueInput.inputEnabled())
        mValueInput.draw(&p, height() - eSizesUI::widget);
    if(hasFocus()) {
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(ThemeSupport::getThemeHighlightColor(), 4));
        p.drawRect(0, 0, width(), height());
    }

    p.end();
}

void KeysView::KFT_setFocusToWidget() {
    if(mCurrentScene) Document::sInstance->setActiveScene(mCurrentScene);
    setFocus();
    update();
}

void KeysView::KFT_clearFocus() {
    clearFocus();
    update();
}

Key *KeysView::getKeyAtPos(const int pressX, const int pressY,
                           const qreal pixelsPerFrame,
                           const int minViewedFrame) {
    const int remaining = pressY % eSizesUI::widget;
    if(remaining < (eSizesUI::widget - KEY_RECT_SIZE)/2 ||
       remaining > (eSizesUI::widget + KEY_RECT_SIZE)/2) return nullptr;
    const auto& wids = mBoxesListWidget->visibleWidgets();
    for(const auto& container : wids) {
        const int containerTop = container->y();
        const int containerBottom = containerTop + container->height();
        if(containerTop > pressY || containerBottom < pressY) continue;
        const auto bsw = static_cast<BoxSingleWidget*>(container);
        return bsw->getKeyAtPos(pressX, pixelsPerFrame, minViewedFrame);
    }
    return nullptr;
}

void KeysView::updateHovered(const QPoint &posU) {
    if(mGraphViewed) {
        clearHoveredKey();
        clearHoveredMovable();

        if(mGHoveredPoint) mGHoveredPoint->setHovered(false);
        mGHoveredPoint = graphGetPointAtPos(posU);
        if(mGHoveredPoint) mGHoveredPoint->setHovered(true);
        return;
    }
    if(mHoveredKey) mHoveredKey->setHovered(false);
    mHoveredKey = getKeyAtPos(posU.x(), posU.y(),
                              mPixelsPerFrame,
                              mMinViewedFrame);
    if(mHoveredKey) {
        mHoveredKey->setHovered(true);
        clearHoveredMovable();
    } else {
        const auto lastMovable = mHoveredMovable;
        mHoveredMovable = getRectangleMovableAtPos(
                            posU.x(), posU.y(),
                            mPixelsPerFrame,
                            mMinViewedFrame);
        if(lastMovable != mHoveredMovable) {
            if(lastMovable) lastMovable->setHovered(false);
            if(mHoveredMovable) {
                mHoveredMovable->setHovered(true);
                setCursor(mHoveredMovable->getHoverCursorShape());
            } else {
                setCursor(Qt::ArrowCursor);
            }
        }
        clearHoveredKey();
    }
}

void KeysView::clearHovered() {
    clearHoveredMovable();
    clearHoveredKey();
}

void KeysView::clearHoveredKey() {
    if(!mHoveredKey) return;
    mHoveredKey->setHovered(false);
    mHoveredKey = nullptr;
}

void KeysView::clearHoveredMovable() {
    if(!mHoveredMovable) return;
    mHoveredMovable->setHovered(false);
    mHoveredMovable = nullptr;
    setCursor(Qt::ArrowCursor);
}

void KeysView::scrollRight() {
    const int pixelInc = 2*eSizesUI::widget;
    const int inc = qMax(1, qFloor(pixelInc/mPixelsPerFrame));
    mMinViewedFrame += inc;
    mMaxViewedFrame += inc;
    emit changedViewedFrames({mMinViewedFrame, mMaxViewedFrame});
    if(mSelecting) {
        mSelectionRect.setBottomRight(mSelectionRect.bottomRight() +
                                      QPointF(inc, 0));
    } else if(mMovingKeys) {
        mLastPressPos.setX(mLastPressPos.x() - qRound(inc*mPixelsPerFrame));
        handleMouseMove(mLastMovePos, QApplication::mouseButtons());
    }
    update();
}

void KeysView::scrollLeft() {
    const int pixelInc = 2*eSizesUI::widget;
    const int inc = qMax(1, qFloor(pixelInc/mPixelsPerFrame));
    mMinViewedFrame -= inc;
    mMaxViewedFrame -= inc;
    emit changedViewedFrames({mMinViewedFrame, mMaxViewedFrame});
    if(mSelecting) {
        mSelectionRect.setBottomRight(mSelectionRect.bottomRight() -
                                      QPointF(inc, 0));
    } else if(mMovingKeys) {
        mLastPressPos.setX(mLastPressPos.x() + qRound(inc*mPixelsPerFrame));
        handleMouseMove(mLastMovePos, QApplication::mouseButtons());
    }
    update();
}

void KeysView::handleMouseMoveScroll(const QPoint &pos) {
    if(pos.x() < eSizesUI::widget/2) {
        if(!mScrollTimer->isActive()) {
            connect(mScrollTimer, &QTimer::timeout,
                    this, &KeysView::scrollLeft);
            mScrollTimer->start(300);
        }
    } else if(pos.x() > width() - eSizesUI::widget/2) {
        if(!mScrollTimer->isActive()) {
            connect(mScrollTimer, &QTimer::timeout,
                    this, &KeysView::scrollRight);
            mScrollTimer->start(300);
        }
    } else {
        mScrollTimer->disconnect();
        mScrollTimer->stop();
    }
}

void KeysView::handleMouseMove(const QPoint &pos,
                               const Qt::MouseButtons &buttons) {
    const QPoint posU = pos + QPoint(-eSizesUI::widget/2, 0);
    if(buttons & Qt::MiddleButton) {
        if (mGraphViewed) { graphMiddleMove(posU); }
        else { middleMove(posU); }
        emit changedViewedFrames({mMinViewedFrame, mMaxViewedFrame});
    } else if(mIsMouseGrabbing || buttons & Qt::LeftButton) {
        handleMouseMoveScroll(pos);
        qreal dFrame;
        if(mValueInput.inputEnabled()) {
            dFrame = mValueInput.getValue();
        } else {
            const qreal dX = posU.x() - mLastPressPos.x();
            dFrame = dX/mPixelsPerFrame;
            mValueInput.setDisplayedValue(dFrame);
        }
        const bool ctrlPt = mGPressedPoint && mGPressedPoint->isCtrlPt();
        if(!ctrlPt) dFrame = round(dFrame);
        const qreal dDFrame = dFrame - mMoveDFrame;
        const int iDDFrame = qRound(dDFrame);
        mMoveDFrame = dFrame;

        if(mGraphViewed && ctrlPt) {
            if(mFirstMove) mGPressedPoint->startTransform();
            qreal value;
            qreal frame;
            graphGetValueAndFrameFromPos(posU, value, frame);
            const int dY = mLastPressPos.y() - posU.y();
            const qreal dValue = mValueInput.xOnlyMode() ? 0 : dY/mPixelsPerValUnit;
            const qreal dFrameV = mValueInput.yOnlyMode() ? 0 : dFrame;
            const QPointF saved = mGPressedPoint->getSavedFrameAndValue();
            const qreal rawFrame = saved.x() + dFrameV;
            const qreal rawValue = saved.y() + dValue;
            const qreal newFrame = qBound(mMinMoveFrame, rawFrame, mMaxMoveFrame);
            const qreal newValue = qBound(mMinMoveVal, rawValue, mMaxMoveVal);
            mGPressedPoint->setFrameAndValue(newFrame, newValue,
                                             mPixelsPerFrame,
                                             mPixelsPerValUnit);
        } else if(mMovingKeys) {
            if(mFirstMove) {
                if(mGraphViewed) {
                    for(const auto& anim : qAsConst(mGraphAnimators)) {
                        anim->graph_startSelectedKeysTransform();
                    }
                } else {
                    for(const auto& anim : qAsConst(mSelectedKeysAnimators)) {
                        anim->anim_startSelectedKeysTransform();
                    }
                }
            }
            if(mScalingKeys) {
                qreal keysScale;
                if(mValueInput.inputEnabled()) {
                    keysScale = mValueInput.getValue();
                } else {
                    keysScale = 1 + (posU.x() - mLastPressPos.x())/150.;
                    mValueInput.setDisplayedValue(keysScale);
                }
                const int absFrame = mCurrentScene->getCurrentFrame();
                if(mGraphViewed) {
                    for(const auto& anim : qAsConst(mGraphAnimators)) {
                        anim->anim_scaleSelectedKeysFrame(absFrame, keysScale);
                    }
                } else {
                    for(const auto& anim : qAsConst(mSelectedKeysAnimators)) {
                        anim->anim_scaleSelectedKeysFrame(absFrame, keysScale);
                    }
                }
            } else {
                if(mGraphViewed) {
                    const int dY = mLastPressPos.y() - posU.y();
                    const qreal dValue = mValueInput.xOnlyMode() ? 0 :
                                dY/mPixelsPerValUnit;
                    const qreal dFrameV = mValueInput.yOnlyMode() ? 0 : dFrame;
                    for(const auto& anim : qAsConst(mGraphAnimators)) {
                        anim->graph_changeSelectedKeysFrameAndValue(
                                        {dFrameV, dValue});
                    }
                } else if(iDDFrame != 0) {
                    for(const auto& anim : qAsConst(mSelectedKeysAnimators)) {
                        anim->anim_incSelectedKeysFrame(iDDFrame);
                    }
                }
            }
        } else if(mMovingRect) {
            if(mFirstMove) {
                if(mLastPressedMovable) {
                    const bool shiftPressed = QApplication::keyboardModifiers() & Qt::SHIFT;
                    if(!mLastPressedMovable->isSelected()) {
                        mLastPressedMovable->selectionChangeTriggered(shiftPressed);
                    }
                    const auto childProp = mLastPressedMovable->getParentProperty();
                    mMoveAllSelected = true;
                    // mMoveAllSelected = shiftPressed && enve_cast<eBoxOrSound*>(childProp);
                    if(mMoveAllSelected) {
                        if(mLastPressedMovable->isDurationRect()) {
                            mCurrentScene->startDurationRectPosTransformForAllSelected();
                        } else if(mLastPressedMovable->isMaxFrame()) {
                            mCurrentScene->startMaxFramePosTransformForAllSelected();
                        } else if(mLastPressedMovable->isMinFrame()) {
                            mCurrentScene->startMinFramePosTransformForAllSelected();
                        }
                        const auto ebs = static_cast<eBoxOrSound*>(childProp);
                        if(!ebs->isSelected()) mLastPressedMovable->startPosTransform();
                    } else {
                        mLastPressedMovable->startPosTransform();
                    }
                }
            }

            if(iDDFrame != 0) {
                if(mLastPressedMovable) {
                    const auto childProp = mLastPressedMovable->getParentProperty();
                    if(mMoveAllSelected) {
                        if(mLastPressedMovable->isDurationRect()) {
                            mCurrentScene->moveDurationRectForAllSelected(iDDFrame);
                        } else if(mLastPressedMovable->isMaxFrame()) {
                            mCurrentScene->moveMaxFrameForAllSelected(iDDFrame);
                        } else if(mLastPressedMovable->isMinFrame()) {
                            mCurrentScene->moveMinFrameForAllSelected(iDDFrame);
                        }
                        const auto ebs = static_cast<eBoxOrSound*>(childProp);
                        if(!ebs->isSelected()) mLastPressedMovable->changeFramePosBy(iDDFrame);
                    } else {
                        mLastPressedMovable->changeFramePosBy(iDDFrame);
                    }
                }
//                        mLastPressedDurationRectangleMovable->changeFramePosBy(
//                                    dDFrame);
            }
        } else if(mSelecting) {
            if(mGraphViewed) {
                qreal value;
                qreal frame;
                graphGetValueAndFrameFromPos(posU, value, frame);
                mSelectionRect.setBottomRight({frame, value});
            } else {
                const qreal posUXFrame = posU.x()/mPixelsPerFrame + mMinViewedFrame;
                const QPointF br(posUXFrame, posU.y() + mViewedTop);
                mSelectionRect.setBottomRight(br);
            }
        }
        mFirstMove = false;
    } else updateHovered(posU);

    if(mIsMouseGrabbing) Document::sInstance->updateScenes();
    else update();
    mLastMovePos = pos;
}

void KeysView::mouseMoveEvent(QMouseEvent *event) {
    handleMouseMove(event->pos(), event->buttons());
}

void KeysView::mouseReleaseEvent(QMouseEvent *e) {
    if(mScrollTimer->isActive()) {
        mScrollTimer->disconnect();
        mScrollTimer->stop();
    }
    const bool shiftPressed = e->modifiers() & Qt::SHIFT;
    if(mGraphViewed && mGPressedPoint) {
        if(mGPressedPoint->isKeyPt() && mFirstMove && !shiftPressed) {
            clearKeySelection();
            addKeyToSelection(mGPressedPoint->getParentKey());
        } else if(!mFirstMove) finishTransform();
        mGPressedPoint->setSelected(false);
        mGPressedPoint = nullptr;

        graphConstrainAnimatorCtrlsFrameValues();
        //graphConstrainAnimatorCtrlsFrameValues();

        // needed ?
        graphUpdateDimensions();
    } else {
        if(e->button() == Qt::LeftButton) {
            if(mSelecting) {
                if(mSelectionRect.left() > mSelectionRect.right()) {
                    const qreal rightT = mSelectionRect.right();
                    const qreal leftT = mSelectionRect.left();
                    mSelectionRect.setLeft(rightT);
                    mSelectionRect.setRight(leftT);
                }
                if(mSelectionRect.top() > mSelectionRect.bottom()) {
                    const qreal bottomT = mSelectionRect.bottom();
                    const qreal topT = mSelectionRect.top();
                    mSelectionRect.setTop(bottomT);
                    mSelectionRect.setBottom(topT);
                }
                if(!shiftPressed) clearKeySelection();
                selectKeysInSelectionRect();
            } else if(mMovingKeys) {
                if(mFirstMove && mLastPressedKey) {
                    if(!shiftPressed) {
                        clearKeySelection();
                        addKeyToSelection(mLastPressedKey);
                    }
                }
                for(const auto& anim : qAsConst(mSelectedKeysAnimators)) {
                    anim->anim_finishSelectedKeysTransform();
                }
            } else if(mMovingRect) {
                if(mFirstMove) {
                    if(mLastPressedMovable) {
                        mLastPressedMovable->selectionChangeTriggered(shiftPressed);
                    }
                } else {
                    const auto childProp = mLastPressedMovable->getParentProperty();
                    if(mMoveAllSelected) {
                        if(mLastPressedMovable->isDurationRect()) {
                            mCurrentScene->finishDurationRectPosTransformForAllSelected();
                        } else if(mLastPressedMovable->isMinFrame()) {
                            mCurrentScene->finishMinFramePosTransformForAllSelected();
                        } else if(mLastPressedMovable->isMaxFrame()) {
                            mCurrentScene->finishMaxFramePosTransformForAllSelected();
                        }
                        const auto ebs = static_cast<eBoxOrSound*>(childProp);
                        if(!ebs->isSelected()) mLastPressedMovable->finishPosTransform();
                    } else {
                        mLastPressedMovable->finishPosTransform();
                    }
                }
            }
        } else if(e->button() == Qt::RightButton) {
            if(mMovingRect) {
                if(!mFirstMove) {
                    const auto childProp = mLastPressedMovable->getParentProperty();
                    if(mMoveAllSelected) {
                        if(mLastPressedMovable->isDurationRect()) {
                            mCurrentScene->cancelDurationRectPosTransformForAllSelected();
                        } else if(mLastPressedMovable->isMinFrame()) {
                            mCurrentScene->cancelMinFramePosTransformForAllSelected();
                        } else if(mLastPressedMovable->isMaxFrame()) {
                            mCurrentScene->cancelMaxFramePosTransformForAllSelected();
                        }
                        const auto ebs = static_cast<eBoxOrSound*>(childProp);
                        if(!ebs->isSelected()) mLastPressedMovable->cancelPosTransform();
                    } else {
                        mLastPressedMovable->cancelPosTransform();
                    }
                    mMovingRect = false;
                    mLastPressedMovable = nullptr;
                }
            }
        }
    }
    updateHovered(e->pos() + QPoint(-eSizesUI::widget/2, 0));
    releaseMouseAndDontTrack();

    mValueInput.clearAndDisableInput();
    Document::sInstance->actionFinished();
}

void KeysView::setFramesRange(const FrameRange& range) {
    mMinViewedFrame = range.fMin;
    mMaxViewedFrame = range.fMax;
    updatePixelsPerFrame();
    if(mGraphViewed) graphUpdateDimensions();
    update();
}

int KeysView::getMinViewedFrame() {
    return mMinViewedFrame;
}

int KeysView::getMaxViewedFrame() {
    return mMaxViewedFrame;
}

qreal KeysView::getPixelsPerFrame() {
    return mPixelsPerFrame;
}

void KeysView::updatePixelsPerFrame() {
    const qreal animWidth = width() - 2*eSizesUI::widget;
    const qreal dFrame = mMaxViewedFrame - mMinViewedFrame + 1;
    mPixelsPerFrame = animWidth/dFrame;
}

void KeysView::addKeyToSelection(Key * const key) {
    if(!key) return;
    QList<Animator*> toSelect;
    key->addToSelection(toSelect);
    for(const auto& anim : qAsConst(toSelect)) {
        connect(anim, &QObject::destroyed, this, [this, anim]() {
            mSelectedKeysAnimators.removeOne(anim);
        });
        mSelectedKeysAnimators << anim;
    }
}

void KeysView::removeKeyFromSelection(Key * const key) {
    QList<Animator*> toRemove;
    key->removeFromSelection(toRemove);
    for(const auto& anim : qAsConst(toRemove)) {
        disconnect(anim, &QObject::destroyed, this, nullptr);
        mSelectedKeysAnimators.removeOne(anim);
    }
}

void KeysView::clearKeySelection() {
    for(const auto& anim : qAsConst(mSelectedKeysAnimators)) {
        anim->anim_deselectAllKeys();
    }
    mSelectedKeysAnimators.clear();
}
