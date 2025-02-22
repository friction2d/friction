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

#include "canvas.h"

#include "eevent.h"

#include "Private/document.h"
#include "GUI/dialogsinterface.h"

#include "Boxes/circle.h"
#include "Boxes/rectangle.h"
#include "Boxes/imagebox.h"
#include "Boxes/textbox.h"
#include "Boxes/internallinkbox.h"
#include "Boxes/containerbox.h"
#include "Boxes/smartvectorpath.h"
//#include "Boxes/paintbox.h"
#include "Boxes/nullobject.h"

#include "pointtypemenu.h"
#include "pointhelpers.h"
#include "clipboardcontainer.h"

#include "PathEffects/patheffect.h"
#include "PathEffects/patheffectsinclude.h"
#include "RasterEffects/rastereffect.h"

#include "MovablePoints/smartnodepoint.h"
#include "MovablePoints/pathpivot.h"

#include <QDesktopWidget>
#include <QScreen>
#include <QMouseEvent>
#include <QMenu>
#include <QInputDialog>

void Canvas::handleMovePathMousePressEvent(const eMouseEvent& e) {
    mPressedBox = mCurrentContainer->getBoxAt(e.fPos);
    if(e.shiftMod()) return;
    if(mPressedBox ? !mPressedBox->isSelected() : true) {
        clearBoxesSelection();
    }
}

void Canvas::addActionsToMenu(QMenu *const menu)
{
    const auto clipboard = mDocument.getBoxesClipboard();
    if (clipboard) {
        QAction * const pasteAct = menu->addAction(tr("Paste"), this,
                                                   &Canvas::pasteAction);
        pasteAct->setShortcut(Qt::CTRL + Qt::Key_V);
    }

    QMenu * const linkCanvasMenu = menu->addMenu(QIcon::fromTheme("sequence"),
                                                 tr("Link Scene"));
    for (const auto& canvas : mDocument.fScenes) {
        const auto slot = [this, canvas]() {
            auto newLink = canvas->createLink(false);
            mCurrentContainer->addContained(newLink);
            newLink->centerPivotPosition();
        };
        QAction * const action = linkCanvasMenu->addAction(canvas->prp_getName(),
                                                           this,
                                                           slot);
        if (canvas == this) {
            action->setEnabled(false);
            action->setVisible(false);
        }
    }

    menu->addAction(QIcon::fromTheme("duplicate"),
                    tr("Duplicate Scene"), [this]() {
        const auto newScene = Document::sInstance->createNewScene();
        newScene->setCanvasSize(mWidth, mHeight);
        newScene->setFps(mFps);
        newScene->setFrameRange(mRange);
        BoxClipboard::sCopyAndPaste(this, newScene);
        newScene->prp_setNameAction(newScene->prp_getName() + " copy");
    });

    const auto parentWidget = menu->parentWidget();
    menu->addAction(QIcon::fromTheme("file_movie"),
                    tr("Map to Different Fps"), [this, parentWidget]() {
        bool ok;
        const qreal newFps = QInputDialog::getDouble(
                    parentWidget, "Map to Different Fps",
                    "New Fps:", mFps, 1, 999, 2, &ok);
        if (ok) { changeFpsTo(newFps); }
    });

    menu->addAction(QIcon::fromTheme("sequence"),
                    tr("Scene Properties"), [this]() {
        const auto& dialogs = DialogsInterface::instance();
        dialogs.showSceneSettingsDialog(this);
    });
}

void Canvas::handleRightButtonMouseRelease(const eMouseEvent& e) {
    if(e.fMouseGrabbing) {
        cancelCurrentTransform();
        e.fReleaseMouse();
        mValueInput.clearAndDisableInput();
    } else {
        mPressedBox = mHoveredBox;
        mPressedPoint = mHoveredPoint_d;
        if(mPressedPoint) {
            QMenu qMenu;
            PointTypeMenu menu(&qMenu, this, e.fWidget);
            if(mPressedPoint->selectionEnabled()) {
                if(!mPressedPoint->isSelected()) {
                    if(!e.shiftMod()) clearPointsSelection();
                    addPointToSelection(mPressedPoint);
                }
                for(const auto& pt : mSelectedPoints_d) {
                    pt->canvasContextMenu(&menu);
                }
            } else {
                mPressedPoint->canvasContextMenu(&menu);
            }
            qMenu.exec(e.fGlobalPos);
        } else if(mPressedBox) {
            if(!mPressedBox->isSelected()) {
                if(!e.shiftMod()) clearBoxesSelection();
                addBoxToSelection(mPressedBox);
            }

            QMenu qMenu(e.fWidget);
            PropertyMenu menu(&qMenu, this, e.fWidget);
            for(const auto& box : mSelectedBoxes) {
                box->setupCanvasMenu(&menu);
            }
            qMenu.exec(e.fGlobalPos);
        } else {
            clearPointsSelection();
            clearBoxesSelection();
            QMenu menu(e.fWidget);
            addActionsToMenu(&menu);
            menu.exec(e.fGlobalPos);
        }
    }
    mDocument.actionFinished();
}

void Canvas::clearHoveredEdge() {
    mHoveredNormalSegment.reset();
}

void Canvas::handleMovePointMousePressEvent(const eMouseEvent& e) {
    if(mHoveredNormalSegment.isValid()) {
        if(e.ctrlMod()) {
            clearPointsSelection();
            mPressedPoint = mHoveredNormalSegment.divideAtAbsPos(e.fPos);
        } else {
            mCurrentNormalSegment = mHoveredNormalSegment;
            mCurrentNormalSegmentT = mCurrentNormalSegment.closestAbsT(e.fPos);
            clearPointsSelection();
            clearCurrentSmartEndPoint();
            clearLastPressedPoint();
        }
        clearHovered();
    } else if(mPressedPoint) {
        if(mPressedPoint->isSelected()) return;
        if(!e.shiftMod() && mPressedPoint->selectionEnabled()) {
            clearPointsSelection();
        }
        if(!mPressedPoint->selectionEnabled()) {
            addPointToSelection(mPressedPoint);
        }
    }
}


void Canvas::handleLeftButtonMousePress(const eMouseEvent& e) {
    if(e.fMouseGrabbing) {
        //handleMouseRelease(event->pos());
        //releaseMouseAndDontTrack();
        return;
    }

    mDoubleClick = false;
    //mMovesToSkip = 2;
    mStartTransform = true;

    const qreal invScale = 1/e.fScale;
    mPressedPoint = getPointAtAbsPos(e.fPos, mCurrentMode, invScale);

    if(mRotPivot->isPointAtAbsPos(e.fPos, mCurrentMode, invScale)) {
        return mRotPivot->setSelected(true);
    }
    if(mCurrentMode == CanvasMode::boxTransform) {
        if(mHoveredPoint_d) {
            handleMovePointMousePressEvent(e);
        } else {
            handleMovePathMousePressEvent(e);
        }
    } else if(mCurrentMode == CanvasMode::pathCreate) {
        handleAddSmartPointMousePress(e);
    } else if(mCurrentMode == CanvasMode::pointTransform) {
        handleMovePointMousePressEvent(e);
    } else if(mCurrentMode == CanvasMode::drawPath) {
        const bool manual = mDocument.fDrawPathManual;
        bool start;
        if(manual) {
            start = mManualDrawPathState == ManualDrawPathState::none;
            if(mManualDrawPathState == ManualDrawPathState::drawn) {
                qreal dist;
                const int forceSplit = mDrawPath.nearestForceSplit(e.fPos, &dist);
                const int maxDist = 10;
                if(dist < maxDist) mDrawPath.removeForceSplit(forceSplit);
                else {
                    const int smoothPt = mDrawPath.nearestSmoothPt(e.fPos, &dist);
                    if(dist < maxDist) mDrawPath.addForceSplit(smoothPt);
                }
                mDrawPath.fit(DBL_MAX/5, false);
            }
        } else start = true;
        if(start) {
            mDrawPathFirst = getPointAtAbsPos(e.fPos, mCurrentMode, invScale);
            mDrawPathFit = 0;
            drawPathClear();
            mDrawPath.lineTo(e.fPos);
        }
    } else if (mCurrentMode == CanvasMode::pickFillStroke ||
               mCurrentMode == CanvasMode::pickFillStrokeEvent) {
        //mPressedBox = getBoxAtFromAllDescendents(e.fPos);
    } else if(mCurrentMode == CanvasMode::circleCreate) {
        const auto newPath = enve::make_shared<Circle>();
        newPath->planCenterPivotPosition();
        mCurrentContainer->addContained(newPath);
        newPath->setAbsolutePos(e.fPos);
        clearBoxesSelection();
        addBoxToSelection(newPath.get());

        mCurrentCircle = newPath.get();

    } else if(mCurrentMode == CanvasMode::nullCreate) {
        const auto newPath = enve::make_shared<NullObject>();
        newPath->planCenterPivotPosition();
        mCurrentContainer->addContained(newPath);
        newPath->setAbsolutePos(e.fPos);
        clearBoxesSelection();
        addBoxToSelection(newPath.get());
    } else if(mCurrentMode == CanvasMode::rectCreate) {
        const auto newPath = enve::make_shared<RectangleBox>();
        newPath->planCenterPivotPosition();
        mCurrentContainer->addContained(newPath);
        newPath->setAbsolutePos(e.fPos);
        clearBoxesSelection();
        addBoxToSelection(newPath.get());

        mCurrentRectangle = newPath.get();
    } else if (mCurrentMode == CanvasMode::textCreate) {
        if (enve_cast<TextBox*>(mHoveredBox)) {
            setCurrentBox(mHoveredBox);
            emit openTextEditor();
        } else {
            const auto newPath = enve::make_shared<TextBox>();
            newPath->planCenterPivotPosition();
            newPath->setFontFamilyAndStyle(mDocument.fFontFamily,
                                           mDocument.fFontStyle);
            newPath->setFontSize(mDocument.fFontSize);
            mCurrentContainer->addContained(newPath);
            newPath->setAbsolutePos(e.fPos);

            mCurrentTextBox = newPath.get();

            clearBoxesSelection();
            addBoxToSelection(newPath.get());
        }
    }
}

void Canvas::cancelCurrentTransform() {
    if(mCurrentMode == CanvasMode::pointTransform) {
        if(mCurrentNormalSegment.isValid()) {
            mCurrentNormalSegment.cancelPassThroughTransform();
        } else {
            cancelSelectedPointsTransform();
        }
    } else if(mCurrentMode == CanvasMode::boxTransform) {
        if(mRotPivot->isSelected()) {
            mRotPivot->cancelTransform();
        } else {
            cancelSelectedBoxesTransform();
        }
    } else if(mCurrentMode == CanvasMode::pathCreate) {

    } else if (mCurrentMode == CanvasMode::pickFillStroke ||
               mCurrentMode == CanvasMode::pickFillStrokeEvent) {
        //mCanvasWindow->setCanvasMode(MOVE_PATH);
    }
    mValueInput.clearAndDisableInput();
    mTransMode = TransformMode::none;
}

void Canvas::handleMovePointMouseRelease(const eMouseEvent &e) {
    if(mRotPivot->isSelected()) {
        mRotPivot->setSelected(false);
    } else if(mTransMode == TransformMode::rotate ||
              mTransMode == TransformMode::scale) {
        finishSelectedPointsTransform();
        mTransMode = TransformMode::none;
    } else if(mSelecting) {
        mSelecting = false;
        if(!e.shiftMod()) clearPointsSelection();
        moveSecondSelectionPoint(e.fPos);
        selectAndAddContainedPointsToSelection(mSelectionRect);
    } else if(mStartTransform) {
        if(mPressedPoint) {
            if(mPressedPoint->isCtrlPoint()) {
                removePointFromSelection(mPressedPoint);
            } else if(e.shiftMod()) {
                if(mPressedPoint->isSelected()) {
                    removePointFromSelection(mPressedPoint);
                } else {
                    addPointToSelection(mPressedPoint);
                }
            } else {
                selectOnlyLastPressedPoint();
            }
        } else {
            mPressedBox = mCurrentContainer->getBoxAt(e.fPos);
            if(mPressedBox ? !!enve_cast<ContainerBox*>(mPressedBox) : true) {
                const auto pressedBox = getBoxAtFromAllDescendents(e.fPos);
                if(!pressedBox) {
                    if(!e.shiftMod()) {
                        clearPointsSelectionOrDeselect();
                    }
                } else {
                    clearPointsSelection();
                    clearCurrentSmartEndPoint();
                    clearLastPressedPoint();
                    setCurrentBoxesGroup(pressedBox->getParentGroup());
                    addBoxToSelection(pressedBox);
                    mPressedBox = pressedBox;
                }
            }
            if(mPressedBox) {
                if(e.shiftMod()) {
                    if(mPressedBox->isSelected()) {
                        removeBoxFromSelection(mPressedBox);
                    } else {
                        addBoxToSelection(mPressedBox);
                    }
                } else {
                    clearPointsSelection();
                    clearCurrentSmartEndPoint();
                    clearLastPressedPoint();
                    selectOnlyLastPressedBox();
                }
            }
        }
    } else {
        finishSelectedPointsTransform();
        if(mPressedPoint) {
            if(!mPressedPoint->selectionEnabled()) {
                removePointFromSelection(mPressedPoint);
            }
        }
    }
}

void Canvas::handleMovePathMouseRelease(const eMouseEvent &e) {
    if(mRotPivot->isSelected()) {
        if(!mStartTransform) mRotPivot->finishTransform();
        mRotPivot->setSelected(false);
    } else if(mTransMode == TransformMode::rotate) {
        pushUndoRedoName("Rotate Objects");
        finishSelectedBoxesTransform();
    } else if(mTransMode == TransformMode::scale) {
        pushUndoRedoName("Scale Objects");
        finishSelectedBoxesTransform();
    } else if(mStartTransform) {
        mSelecting = false;
        if(e.shiftMod() && mPressedBox) {
            if(mPressedBox->isSelected()) {
                removeBoxFromSelection(mPressedBox);
            } else {
                addBoxToSelection(mPressedBox);
            }
        } else {
            selectOnlyLastPressedBox();
        }
    } else if(mSelecting) {
        moveSecondSelectionPoint(e.fPos);
        mCurrentContainer->addContainedBoxesToSelection(mSelectionRect);
        mSelecting = false;
    } else {
        pushUndoRedoName("Move Objects");
        finishSelectedBoxesTransform();
    }
}

SmartNodePoint* drawPathAppend(const QList<qCubicSegment2D>& fitted,
                               SmartNodePoint* endPoint) {
    for(int i = 0; i < fitted.count(); i++) {
        const auto& seg = fitted.at(i);
        endPoint->moveC2ToAbsPos(seg.c1());
        endPoint = endPoint->actionAddPointAbsPos(seg.p3());
        endPoint->moveC0ToAbsPos(seg.c2());
    }
    return endPoint;
}

qsptr<SmartVectorPath> drawPathNew(QList<qCubicSegment2D>& fitted) {
    const QPointF& begin = fitted.first().p0();
    const QPointF& end = fitted.last().p3();
    const qreal beginEndDist = pointToLen(end - begin);
    const bool close = beginEndDist < 7 && fitted.count() > 1;
    if(close) fitted.last().setP3(begin);
    const auto newPath = enve::make_shared<SmartVectorPath>();
    CubicList fittedList(fitted);
    newPath->loadSkPath(fittedList.toSkPath());
    newPath->planCenterPivotPosition();
    return newPath;
}

void Canvas::drawPathClear() {
    mManualDrawPathState = ManualDrawPathState::none;
    mDrawPathFirst.clear();
    mDrawPath.clear();
    mDrawPathTmp.reset();
}

void Canvas::drawPathFinish(const qreal invScale) {
    mDrawPath.smooth(mDocument.fDrawPathSmooth);
    const bool manual = mDocument.fDrawPathManual;
    const qreal error = manual ? DBL_MAX/5 :
                                 mDocument.fDrawPathMaxError;
    mDrawPath.fit(error, !manual);

    auto& fitted = mDrawPath.getFitted();
    if(!fitted.isEmpty()) {
        const QPointF& begin = fitted.first().p0();
        const QPointF& end = fitted.last().p3();
        const auto beginHover = getPointAtAbsPos(begin, mCurrentMode, invScale);
        const auto beginNode = enve_cast<SmartNodePoint*>(beginHover);
        const auto endHover = getPointAtAbsPos(end, mCurrentMode, invScale);
        const auto endNode = enve_cast<SmartNodePoint*>(endHover);
        const bool beginEndPoint = beginNode ? beginNode->isEndPoint() : false;
        const bool endEndPoint = endNode ? endNode->isEndPoint() : false;
        bool createNew = false;

        if(beginNode && endNode && beginNode != endNode) {
            const auto beginParent = beginNode->getTargetAnimator();
            const auto endParent = endNode->getTargetAnimator();
            const bool sampeParent = beginParent == endParent;

            if(sampeParent) {
                const auto transform = beginNode->getTransform();
                const auto matrix = transform->getTotalTransform();
                const auto invMatrix = matrix.inverted();
                std::for_each(fitted.begin(), fitted.end(),
                              [&invMatrix](qCubicSegment2D& seg) {
                    seg.transform(invMatrix);
                });
                const int beginId = beginNode->getNodeId();
                const int endId = endNode->getNodeId();
                beginParent->actionReplaceSegments(beginId, endId, fitted);
            } else if(beginEndPoint && endEndPoint) {
                const bool reverse = endNode->hasNextPoint();

                const auto orderedBegin = reverse ? endNode : beginNode;
                const auto orderedEnd = reverse ? beginNode : endNode;

                if(orderedEnd->hasNextPoint() || !endNode->hasNextPoint()) {
                    std::reverse(fitted.begin(), fitted.end());
                    std::for_each(fitted.begin(), fitted.end(),
                                  [](qCubicSegment2D& seg) { seg.reverse(); });
                }

                const auto& lastSeg = fitted.last();
                const auto mid = fitted.mid(0, fitted.count() - 1);
                const auto last = drawPathAppend(mid, orderedEnd);
                last->moveC2ToAbsPos(lastSeg.c1());
                orderedBegin->moveC0ToAbsPos(lastSeg.c2());
                last->actionConnectToNormalPoint(orderedBegin);
            } else createNew = true;
        } else if(beginNode && beginEndPoint) {
            drawPathAppend(fitted, beginNode);
        } else if(endNode && endEndPoint) {
            drawPathAppend(fitted, endNode);
        } else createNew = true;
        if(createNew) {
            const auto matrix = mCurrentContainer->getTotalTransform();
            const auto invMatrix = matrix.inverted();
            std::for_each(fitted.begin(), fitted.end(),
                          [&invMatrix](qCubicSegment2D& seg) {
                seg.transform(invMatrix);
            });
            const auto newPath = drawPathNew(fitted);
            mCurrentContainer->addContained(newPath);
            clearBoxesSelection();
            addBoxToSelection(newPath.get());
        }
    }

    drawPathClear();
}

const QColor Canvas::pickPixelColor(const QPoint &pos)
{
    // try the "safe" option first
    if (QApplication::activeWindow()) {
        const auto nPos = QApplication::activeWindow()->mapFromGlobal(pos);
        return QApplication::activeWindow()->grab(QRect(QPoint(nPos.x(), nPos.y()),
                                                        QSize(1, 1))).toImage().pixel(0, 0);
    }

    // "insecure" fallback (will not work in a sandbox or wayland)
    // will prompt for permissions on macOS
    // Windows and X11 don't care
    QScreen *screen = QApplication::screenAt(pos);
    if (!screen) { return QColor(); }
    WId wid = QApplication::desktop()->winId();
    const auto pix = screen->grabWindow(wid, pos.x(), pos.y(), 1, 1);
    return QColor(pix.toImage().pixel(0, 0));
}

void Canvas::applyPixelColor(const QColor &color,
                             const bool &fill)
{
    if (!color.isValid()) { return; }
    for (const auto& box : mSelectedBoxes) {
        if (fill) {
            auto settings = box->getFillSettings();
            if (settings) {
                if (settings->getPaintType() == PaintType::NOPAINT) {
                    settings->setPaintType(PaintType::FLATPAINT);
                }
                settings->setCurrentColor(color, true);
                box->fillStrokeSettingsChanged();
            }
        } else {
            auto settings = box->getStrokeSettings();
            if (settings) {
                if (settings->getPaintType() == PaintType::NOPAINT) {
                    settings->setPaintType(PaintType::FLATPAINT);
                }
                settings->setCurrentColor(color, true);
                box->fillStrokeSettingsChanged();
            }
        }
    }
}

void Canvas::handleLeftMouseRelease(const eMouseEvent &e) {
    if(e.fMouseGrabbing) e.fReleaseMouse();
    if(mCurrentNormalSegment.isValid()) {
        if(!mStartTransform) mCurrentNormalSegment.finishPassThroughTransform();
        mHoveredNormalSegment = mCurrentNormalSegment;
        mHoveredNormalSegment.generateSkPath();
        mCurrentNormalSegment.reset();
        return;
    }
    if(mDoubleClick) return;
    if(mCurrentMode == CanvasMode::pointTransform) {
        handleMovePointMouseRelease(e);
    } else if(mCurrentMode == CanvasMode::boxTransform) {
        if(!mPressedPoint) {
            handleMovePathMouseRelease(e);
        } else {
            handleMovePointMouseRelease(e);
            clearPointsSelection();
        }
    } else if(mCurrentMode == CanvasMode::pathCreate) {
        handleAddSmartPointMouseRelease(e);
    } else if(mCurrentMode == CanvasMode::drawPath) {
        const bool manual = mDocument.fDrawPathManual;
        if(manual) {
            mManualDrawPathState = ManualDrawPathState::drawn;
        } else {
            drawPathFinish(1/e.fScale);
        }
    } else if (mCurrentMode == CanvasMode::pickFillStrokeEvent) {
        emit currentPickedColor(pickPixelColor(e.fGlobalPos));
    }
    mValueInput.clearAndDisableInput();
    mTransMode = TransformMode::none;
}

QPointF Canvas::getMoveByValueForEvent(const eMouseEvent &e) {
    if(mValueInput.inputEnabled())
        return mValueInput.getPtValue();
    const QPointF moveByPoint = e.fPos - e.fLastPressPos;
    mValueInput.setDisplayedValue(moveByPoint);
    if(mValueInput.yOnlyMode()) return {0, moveByPoint.y()};
    else if(mValueInput.xOnlyMode()) return {moveByPoint.x(), 0};
    return moveByPoint;
}

#include <QApplication>
#include "MovablePoints/smartctrlpoint.h"
#include "MovablePoints/pathpointshandler.h"
#include "Boxes/smartvectorpath.h"
void Canvas::handleMovePointMouseMove(const eMouseEvent &e) {
    if(mRotPivot->isSelected()) {
        if(mStartTransform) mRotPivot->startTransform();
        mRotPivot->moveByAbs(getMoveByValueForEvent(e));
    } else if(mTransMode == TransformMode::scale) {
        scaleSelected(e);
    } else if(mTransMode == TransformMode::rotate) {
        rotateSelected(e);
    } else if(mCurrentNormalSegment.isValid()) {
        if(mStartTransform) mCurrentNormalSegment.startPassThroughTransform();
        mCurrentNormalSegment.makePassThroughAbs(e.fPos, mCurrentNormalSegmentT);
    } else {
        if(mPressedPoint) {
            addPointToSelection(mPressedPoint);
            const auto mods = QGuiApplication::queryKeyboardModifiers();
            if(mPressedPoint->isSmartNodePoint()) {
                if(mods & Qt::ControlModifier) {
                    const auto nodePt = static_cast<SmartNodePoint*>(mPressedPoint.data());
                    if(nodePt->isDissolved()) {
                        const int selId = nodePt->moveToClosestSegment(e.fPos);
                        const auto handler = nodePt->getHandler();
                        const auto dissPt = handler->getPointWithId<SmartNodePoint>(selId);
                        if(nodePt->getNodeId() != selId) {
                            removePointFromSelection(nodePt);
                            addPointToSelection(dissPt);
                        }
                        mPressedPoint = dissPt;
                        return;
                    }
                } else if(mods & Qt::ShiftModifier) {
                    const auto nodePt = static_cast<SmartNodePoint*>(mPressedPoint.data());
                    const auto nodePtAnim = nodePt->getTargetAnimator();
                    if(nodePt->isNormal()) {
                        SmartNodePoint* closestNode = nullptr;
                        qreal minDist = 10/e.fScale;
                        for(const auto& sBox : mSelectedBoxes) {
                            if(!enve_cast<SmartVectorPath*>(sBox)) continue;
                            const auto sPatBox = static_cast<SmartVectorPath*>(sBox);
                            const auto sAnim = sPatBox->getPathAnimator();
                            for(int i = 0; i < sAnim->ca_getNumberOfChildren(); i++) {
                                const auto sPath = sAnim->getChild(i);
                                if(sPath == nodePtAnim) continue;
                                const auto sHandler = static_cast<PathPointsHandler*>(sPath->getPointsHandler());
                                const auto node = sHandler->getClosestNode(e.fPos, minDist);
                                if(node) {
                                    closestNode = node;
                                    minDist = pointToLen(closestNode->getAbsolutePos() - e.fPos);
                                }
                            }
                        }
                        if(closestNode) {
                            const bool reverse = mods & Qt::ALT;

                            const auto sC0 = reverse ? closestNode->getC2Pt() : closestNode->getC0Pt();
                            const auto sC2 = reverse ? closestNode->getC0Pt() : closestNode->getC2Pt();

                            nodePt->setCtrlsMode(closestNode->getCtrlsMode());
                            nodePt->setC0Enabled(sC0->enabled());
                            nodePt->setC2Enabled(sC2->enabled());
                            nodePt->setAbsolutePos(closestNode->getAbsolutePos());
                            nodePt->getC0Pt()->setAbsolutePos(sC0->getAbsolutePos());
                            nodePt->getC2Pt()->setAbsolutePos(sC2->getAbsolutePos());
                        } else {
                            if(mStartTransform) mPressedPoint->startTransform();
                            mPressedPoint->moveByAbs(getMoveByValueForEvent(e));
                        }
                        return;
                    }
                }
            }

            if(!mPressedPoint->selectionEnabled()) {
                if(mStartTransform) mPressedPoint->startTransform();
                mPressedPoint->moveByAbs(getMoveByValueForEvent(e));
                return;
            }
        }
        moveSelectedPointsByAbs(getMoveByValueForEvent(e),
                                mStartTransform);
    }
}

void Canvas::scaleSelected(const eMouseEvent& e) {
    const auto absPos = mRotPivot->getAbsolutePos();
    const auto distMoved = e.fPos - e.fLastPressPos;

    qreal scaleBy;
    if(mValueInput.inputEnabled()) {
        scaleBy = mValueInput.getValue();
    } else {
        scaleBy = 1 + distSign(distMoved)*0.003;
    }
    qreal scaleX;
    qreal scaleY;
    if(mValueInput.xOnlyMode()) {
        scaleX = scaleBy;
        scaleY = 1;
    } else if(mValueInput.yOnlyMode()) {
        scaleX = 1;
        scaleY = scaleBy;
    } else {
        scaleX = scaleBy;
        scaleY = scaleBy;
    }

    if(mCurrentMode == CanvasMode::boxTransform) {
        scaleSelectedBy(scaleX, scaleY, absPos, mStartTransform);
    } else {
        scaleSelectedPointsBy(scaleX, scaleY, absPos, mStartTransform);
    }

    if(!mValueInput.inputEnabled())
        mValueInput.setDisplayedValue({scaleX, scaleY});
    mRotPivot->setMousePos(e.fPos);
}

void Canvas::rotateSelected(const eMouseEvent& e) {
    const QPointF absPos = mRotPivot->getAbsolutePos();
    qreal rot;
    if(mValueInput.inputEnabled()) {
        rot = mValueInput.getValue();
    } else {
        const QLineF dest_line(absPos, e.fPos);
        const QLineF prev_line(absPos, e.fLastPressPos);
        qreal d_rot = dest_line.angleTo(prev_line);
        if(d_rot > 180) d_rot -= 360;

        if(mLastDRot - d_rot > 90) {
            mRotHalfCycles += 2;
        } else if(mLastDRot - d_rot < -90) {
            mRotHalfCycles -= 2;
        }
        mLastDRot = d_rot;
        rot = d_rot + mRotHalfCycles*180;
    }

    if(mCurrentMode == CanvasMode::boxTransform) {
        rotateSelectedBy(rot, absPos, mStartTransform);
    } else {
        rotateSelectedPointsBy(rot, absPos, mStartTransform);
    }

    if(!mValueInput.inputEnabled())
        mValueInput.setDisplayedValue(rot);
    mRotPivot->setMousePos(e.fPos);
}

void Canvas::handleMovePathMouseMove(const eMouseEvent& e) {
    if(mRotPivot->isSelected()) {
        if(mStartTransform) mRotPivot->startTransform();
        mRotPivot->moveByAbs(getMoveByValueForEvent(e));
    } else if(mTransMode == TransformMode::scale) {
        scaleSelected(e);
    } else if(mTransMode == TransformMode::rotate) {
        rotateSelected(e);
    } else {
        if(mPressedBox) {
            addBoxToSelection(mPressedBox);
            mPressedBox = nullptr;
        }

        const auto moveBy = getMoveByValueForEvent(e);
        moveSelectedBoxesByAbs(moveBy, mStartTransform);
    }
}

void Canvas::updateTransformation(const eKeyEvent &e) {
    if(mSelecting) {
        moveSecondSelectionPoint(e.fPos);
    } else if(mCurrentMode == CanvasMode::pointTransform) {
        handleMovePointMouseMove(e);
    } else if(mCurrentMode == CanvasMode::boxTransform) {
        if(!mPressedPoint) {
            handleMovePathMouseMove(e);
        } else {
            handleMovePointMouseMove(e);
        }
    } else if(mCurrentMode == CanvasMode::pathCreate) {
        handleAddSmartPointMouseMove(e);
    }
}
