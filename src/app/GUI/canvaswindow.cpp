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

#include "canvaswindow.h"
#include "canvas.h"

#include <QComboBox>
#include <QApplication>

#include "mainwindow.h"
#include "GUI/BoxesList/boxscroller.h"
#include "swt_abstraction.h"
#include "dialogs/renderoutputwidget.h"
#include "Sound/soundcomposition.h"
#include "GUI/global.h"
#include "renderinstancesettings.h"
#include "svgimporter.h"
#include "filesourcescache.h"
#include "videoencoder.h"
#include "memorychecker.h"
#include "memoryhandler.h"
#include "simpletask.h"
#include "eevent.h"
#include "glhelpers.h"
#include "themesupport.h"

CanvasWindow::CanvasWindow(Document &document,
                           QWidget * const parent)
    : GLWindow(parent)
    , mDocument(document)
    , mActions(*Actions::sInstance)
    , mBlockInput(false)
    , mMouseGrabber(false)
    , mFitToSizeBlocked(false)
{
    connect(&mDocument, &Document::canvasModeSet,
            this, &CanvasWindow::setCanvasMode);

    setAcceptDrops(true);
    setMouseTracking(true);

    KFT_setFocus();
}

CanvasWindow::~CanvasWindow()
{
    setCurrentCanvas(nullptr);
}

BaseCanvas *CanvasWindow::getCurrentCanvas()
{
    return mCurrentCanvas;
}

void CanvasWindow::setCurrentCanvas(const int id)
{
    if (id < 0 || id >= mDocument.fScenes.count()) {
        setCurrentCanvas(nullptr);
    } else {
        setCurrentCanvas(mDocument.fScenes.at(id).get());
    }
}

void CanvasWindow::setCurrentCanvas(BaseCanvas * const canvas)
{
    if (mCurrentCanvas == canvas) { return; }
    if (mCurrentCanvas) {
        if (isVisible()) { mDocument.removeVisibleScene(mCurrentCanvas); }
    }
    const bool hadScene = mCurrentCanvas;
    auto& conn = mCurrentCanvas.assign(canvas);
    if (KFT_hasFocus()) { mDocument.setActiveScene(mCurrentCanvas);}
    if (mCurrentCanvas) {
        if (isVisible()) { mDocument.addVisibleScene(mCurrentCanvas); }
        // this signal is never connected to anything ... ???
        emit changeCanvasFrameRange(canvas->getFrameRange());
        updatePivotIfNeeded();
        conn << connect(mCurrentCanvas, &Canvas::requestUpdate,
                        this, qOverload<>(&CanvasWindow::update));
        conn << connect(mCurrentCanvas, &Canvas::destroyed,
                        this, [this]() { setCurrentCanvas(nullptr); });
    }

    if (hadScene) { fitCanvasToSize(); }
    updateFix();

    emit currentSceneChanged(canvas);
}

void CanvasWindow::finishAction()
{
    updatePivotIfNeeded();
    update();
    Document::sInstance->actionFinished();
}

void CanvasWindow::queTasksAndUpdate()
{
    updatePivotIfNeeded();
    update();
    Document::sInstance->updateScenes();
}

bool CanvasWindow::hasNoCanvas()
{
    return !mCurrentCanvas;
}

void CanvasWindow::renderSk(SkCanvas * const canvas)
{
    qreal pixelRatio = this->devicePixelRatioF();
    if (mCurrentCanvas) {
        canvas->save();
        mCurrentCanvas->repaint(canvas);
        canvas->restore();
    }

    if (KFT_hasFocus()) {
        SkPaint paint;
        paint.setColor(ThemeSupport::getThemeHighlightSkColor());
        paint.setStrokeWidth(pixelRatio*4);
        paint.setStyle(SkPaint::kStroke_Style);
        canvas->drawRect(SkRect::MakeWH(width() * pixelRatio,
                                        height() * pixelRatio),
                         paint);
    }
}

// Here we pass the mousePressEvent method to BaseCanvas so it knows what to do...
void CanvasWindow::mousePressEvent(QMouseEvent *event)
{
    const auto button = event->button();
    bool leftAndAltPressed = (button == Qt::LeftButton) &&
                             QGuiApplication::keyboardModifiers().testFlag(Qt::AltModifier);
    if (button == Qt::MiddleButton ||
        button == Qt::RightButton ||
        leftAndAltPressed) {
        if (button == Qt::MiddleButton || leftAndAltPressed) {
            QApplication::setOverrideCursor(Qt::ClosedHandCursor);
        }
        return;
    }
    KFT_setFocus();
    if (!mCurrentCanvas || mBlockInput) { return; }
    if (mMouseGrabber && button == Qt::LeftButton) { return; }
    const auto pos = mapToCanvasCoord(event->pos());
    mCurrentCanvas->mousePressEvent(eMouseEvent(pos,
                                                pos,
                                                pos,
                                                mMouseGrabber,
                                                mViewTransform.m11(),
                                                event,
                                                [this]() { releaseMouse(); },
                                                [this]() { grabMouse(); },
                                                this));
    queTasksAndUpdate();
    mPrevMousePos = pos;
    if (button == Qt::LeftButton) {
        mPrevPressPos = pos;
        //const auto mode = mDocument.fCanvasMode;
        /*if(mode == CanvasMode::paint && !mValidPaintTarget) {
            updatePaintModeCursor();
        }*/
    }
}

// Here we pass the mouseReleaseEvent method to BaseCanvas so it knows what to do...
void CanvasWindow::mouseReleaseEvent(QMouseEvent *event)
{
    const auto button = event->button();
    bool leftAndAltPressed = (button == Qt::LeftButton) &&
                             QGuiApplication::keyboardModifiers().testFlag(Qt::AltModifier);
    if (button == Qt::MiddleButton || leftAndAltPressed) {
        QApplication::restoreOverrideCursor();
    } else if ((button == Qt::RightButton) &&
               QApplication::overrideCursor()) {
        QApplication::restoreOverrideCursor();
        return;
    }
    if (!mCurrentCanvas || mBlockInput) { return; }
    const auto pos = mapToCanvasCoord(event->pos());
    mCurrentCanvas->mouseReleaseEvent(eMouseEvent(pos,
                                                  mPrevMousePos,
                                                  mPrevPressPos,
                                                  mMouseGrabber,
                                                  mViewTransform.m11(),
                                                  event,
                                                  [this]() { releaseMouse(); },
                                                  [this]() { grabMouse(); },
                                                  this));
    if (button == Qt::LeftButton) { releaseMouse(); }
    finishAction();
}

void CanvasWindow::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Alt &&
        QApplication::overrideCursor()) {
        QApplication::restoreOverrideCursor();
    }
}

void CanvasWindow::mouseMoveEvent(QMouseEvent *event)
{
    if (!mCurrentCanvas || mBlockInput) { return; }
    auto pos = mapToCanvasCoord(event->pos());
    bool leftAndAltPressed = (event->buttons() & Qt::LeftButton) &&
                             QGuiApplication::keyboardModifiers().testFlag(Qt::AltModifier);
    if (event->buttons() & Qt::MiddleButton ||
        event->buttons() & Qt::RightButton ||
        leftAndAltPressed) {
        if (!QApplication::overrideCursor()) {
            QApplication::setOverrideCursor(Qt::ClosedHandCursor);
        }
        translateView(pos - mPrevMousePos);
        pos = mPrevMousePos;
    }
    mCurrentCanvas->mouseMoveEvent(eMouseEvent(pos,
                                               mPrevMousePos,
                                               mPrevPressPos,
                                               mMouseGrabber,
                                               mViewTransform.m11(),
                                               event,
                                               [this]() { releaseMouse(); },
                                               [this]() { grabMouse(); },
                                               this));

    if (mDocument.fCanvasMode == CanvasMode::paint) { update(); }
    else if (isMouseGrabber()) { queTasksAndUpdate(); }
    else { update(); }
    mPrevMousePos = pos;
}

void CanvasWindow::wheelEvent(QWheelEvent *event)
{
#ifdef Q_OS_MAC
    const bool alt = event->modifiers() & Qt::AltModifier;
    if (!alt && event->phase() != Qt::NoScrollPhase) {
        if (event->phase() == Qt::ScrollUpdate ||
            event->phase() == Qt::ScrollMomentum) {
            auto pos = mPrevMousePos;
            const qreal zoom = mViewTransform.m11() * 1.5;
            if (event->angleDelta().y() != 0) {
                pos.setY(pos.y() + event->angleDelta().y() / zoom);
            }
            if (event->angleDelta().x() != 0) {
                pos.setX(pos.x() + event->angleDelta().x() / zoom);
            }
            translateView(pos - mPrevMousePos);
            mPrevMousePos = pos;
            update();
        }
        return;
    }
    if (event->angleDelta().y() == 0 &&
        event->phase() != Qt::NoScrollPhase) { return; }
#endif
    if (!mCurrentCanvas) { return; }
    const auto ePos = event->position();
    if (event->angleDelta().y() > 0) {
        zoomView(1.1, ePos);
    } else {
        zoomView(0.9, ePos);
    }
    update();
}

void CanvasWindow::KFT_setFocusToWidget()
{
    if (mCurrentCanvas) { mDocument.setActiveScene(mCurrentCanvas); }
    setFocus();
    update();
}

void CanvasWindow::writeState(eWriteStream &dst) const
{
    if (mCurrentCanvas) {
        dst << mCurrentCanvas->getWriteId();
        dst << mCurrentCanvas->getDocumentId();
    } else {
        dst << -1;
        dst << -1;
    }
    dst << mViewTransform;
}

void CanvasWindow::readState(eReadStream &src)
{
    int sceneReadId; src >> sceneReadId;
    int sceneDocumentId; src >> sceneDocumentId;

    src.addReadStreamDoneTask([this, sceneReadId, sceneDocumentId]
                              (eReadStream& src) {
        BoundingBox* sceneBox = nullptr;
        if (sceneReadId != -1) {
            sceneBox = src.getBoxByReadId(sceneReadId);
        }
        if (!sceneBox && sceneDocumentId != -1) {
            sceneBox = BoundingBox::sGetBoxByDocumentId(sceneDocumentId);
        }

        setCurrentCanvas(enve_cast<BaseCanvas*>(sceneBox));
    });

    src >> mViewTransform;
    mFitToSizeBlocked = true;
}

void CanvasWindow::readStateXEV(XevReadBoxesHandler& boxReadHandler,
                                const QDomElement& ele)
{
    const auto sceneIdStr = ele.attribute("sceneId");
    const int sceneId = XmlExportHelpers::stringToInt(sceneIdStr);

    boxReadHandler.addXevImporterDoneTask(
                [this, sceneId](const XevReadBoxesHandler& imp) {
        const auto sceneBox = imp.getBoxByReadId(sceneId);
        const auto scene = enve_cast<BaseCanvas*>(sceneBox);
        setCurrentCanvas(scene);
    });

    const auto viewTransformStr = ele.attribute("viewTransform");
    mViewTransform = XmlExportHelpers::stringToMatrix(viewTransformStr);

    mFitToSizeBlocked = true;
}

void CanvasWindow::writeStateXEV(QDomElement& ele,
                                 QDomDocument& doc) const
{
    Q_UNUSED(doc)
    const int sceneId = mCurrentCanvas ? mCurrentCanvas->getWriteId() : -1;
    ele.setAttribute("sceneId", sceneId);
    const auto viewTransformStr = XmlExportHelpers::matrixToString(mViewTransform);
    ele.setAttribute("viewTransform", viewTransformStr);
}

void CanvasWindow::setResolution(const qreal fraction)
{
    if (!mCurrentCanvas) { return; }
    mCurrentCanvas->setResolution(fraction);
    finishAction();
}

/* TODO: Need to think what to do with these methods ... */

void CanvasWindow::updatePivotIfNeeded()
{
    if (!mCurrentCanvas) { return; }
    mCurrentCanvas->updatePivotIfNeeded();
}

void CanvasWindow::schedulePivotUpdate()
{
    if (!mCurrentCanvas) { return; }
    mCurrentCanvas->schedulePivotUpdate();
}

ContainerBox *CanvasWindow::getCurrentGroup()
{
    if (!mCurrentCanvas) { return nullptr; }
    return mCurrentCanvas->getCurrentGroup();
}

int CanvasWindow::getCurrentFrame()
{
    if (!mCurrentCanvas) { return 0; }
    return mCurrentCanvas->getCurrentFrame();
}

int CanvasWindow::getMaxFrame()
{
    if (!mCurrentCanvas) { return 0; }
    return mCurrentCanvas->getMaxFrame();
}

/* =============== */

void CanvasWindow::dropEvent(QDropEvent *event)
{
    const QPointF pos = mapToCanvasCoord(event->posF());
    mActions.handleDropEvent(event, pos);
}

void CanvasWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls() ||
        event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist"))
    {
        event->acceptProposedAction();
        KFT_setFocus();
    }
}

void CanvasWindow::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void CanvasWindow::grabMouse()
{
    mMouseGrabber = true;
#ifndef QT_DEBUG
    QWidget::grabMouse();
#endif
    Actions::sInstance->startSmoothChange();
}

void CanvasWindow::releaseMouse()
{
    mMouseGrabber = false;
#ifndef QT_DEBUG
    QWidget::releaseMouse();
#endif
    Actions::sInstance->finishSmoothChange();
}

bool CanvasWindow::isMouseGrabber()
{
    return mMouseGrabber;
}
