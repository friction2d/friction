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

#include "smartpathcollection.h"
#include "MovablePoints/pathpointshandler.h"
#include "Animators/transformanimator.h"

SmartPathCollection::SmartPathCollection() :
    SmartPathCollectionBase("paths") {
    connect(this, &ComplexAnimator::ca_childRemoved,
            this, &SmartPathCollection::updatePathColors);
    connect(this, &ComplexAnimator::ca_childAdded,
            this, &SmartPathCollection::updatePathColors);
    connect(this, &ComplexAnimator::ca_childMoved,
            this, &SmartPathCollection::updatePathColors);
    connect(this, &ComplexAnimator::ca_childRemoved,
            this, [this](Property* const child) {
        const auto path = static_cast<SmartPathAnimator*>(child);
        disconnect(path, &SmartPathAnimator::emptied, this, nullptr);
        disconnect(child, &Property::prp_selectionChanged,
                   this, &SmartPathCollection::updateVisibleChildren);
    });
    connect(this, &ComplexAnimator::ca_childAdded,
            this, [this](Property* const child) {
        const auto path = static_cast<SmartPathAnimator*>(child);
        connect(path, &SmartPathAnimator::emptied, this, [this, path]() {
            removeChild(enve::shared<SmartPathAnimator>(path));
        });
        connect(child, &Property::prp_selectionChanged,
                this, &SmartPathCollection::updateVisibleChildren);
    });
}

void SmartPathCollection::prp_writeProperty_impl(eWriteStream &dst) const {
    SmartPathCollectionBase::prp_writeProperty_impl(dst);
    dst.write(&mFillType, sizeof(SkPathFillType));
}

void SmartPathCollection::prp_readProperty_impl(eReadStream &src) {
    SmartPathCollectionBase::prp_readProperty_impl(src);
    src.read(&mFillType, sizeof(SkPathFillType));
}

void SmartPathCollection::savePathsSVG(SvgExporter& exp,
                                       QDomElement& parent,
                                       const EffectApplier& applier,
                                       const bool forceDumbIncrement,
                                       const FrameRange& visRange,
                                       QList<Animator*> const extInfl)
{
    if (mFillType == SkPathFillType::kEvenOdd) {
        parent.setAttribute("fill-rule", "evenodd");
    }
    if(!forceDumbIncrement && ca_getNumberOfChildren() == 1) {
        const auto path0 = getChild(0);
        path0->graph_saveSVG(exp, parent, visRange, "d",
                             [path0, &applier](const qreal relFrame) {
            auto path = path0->getPathAtRelFrame(relFrame);
            if(applier) applier(relFrame, path);
            SkString pathStr;
            SkParsePath::ToSVGString(path, &pathStr);
            if(pathStr.isEmpty()) return QString("M0 0");
            return QString(pathStr.c_str());
        });
    } else {
        Animator::saveSVG(exp, parent, visRange, "d",
                          [this, &applier](const qreal relFrame) {
            auto path = getPathAtRelFrame(relFrame);
            if(applier) applier(relFrame, path);
            SkString pathStr;
            SkParsePath::ToSVGString(path, &pathStr);
            if(pathStr.isEmpty()) return QString("M0 0");
            return QString(pathStr.c_str());
        }, "discrete", extInfl);
    }
}

SmartNodePoint *SmartPathCollection::createNewSubPathAtRelPos(const QPointF &relPos) {
    const auto newPath = createNewPath();
    const auto handler = newPath->getPointsHandler();
    const auto pathHandler = static_cast<PathPointsHandler*>(handler);
    return pathHandler->addNewAtEnd(relPos);
}

SmartNodePoint *SmartPathCollection::createNewSubPathAtPos(const QPointF &absPos) {
    const auto trans = getTransformAnimator();
    const auto relPos = trans ? trans->mapAbsPosToRel(absPos) : absPos;
    return createNewSubPathAtRelPos(relPos);
}

SkPath SmartPathCollection::getPathAtRelFrame(const qreal relFrame) const {
    SkPath result;
    const auto& children = ca_getChildren();
    for(const auto& child : children) {
        const auto path = static_cast<SmartPathAnimator*>(child.get());
        const auto mode = path->getMode();
        if(mode == SmartPathAnimator::Mode::normal)
            result.addPath(path->getPathAtRelFrame(relFrame));
        else {
            SkPathOp op{SkPathOp::kUnion_SkPathOp};
            switch(mode) {
                case(SmartPathAnimator::Mode::normal):
                case(SmartPathAnimator::Mode::add):
                    op = SkPathOp::kUnion_SkPathOp;
                    break;
                case(SmartPathAnimator::Mode::remove):
                    op = SkPathOp::kDifference_SkPathOp;
                    break;
                case(SmartPathAnimator::Mode::removeReverse):
                    op = SkPathOp::kReverseDifference_SkPathOp;
                    break;
                case(SmartPathAnimator::Mode::intersect):
                    op = SkPathOp::kIntersect_SkPathOp;
                    break;
                case(SmartPathAnimator::Mode::exclude):
                    op = SkPathOp::kXOR_SkPathOp;
                    break;
                case(SmartPathAnimator::Mode::divide):
                    const SkPath skPath = path->getPathAtRelFrame(relFrame);
                    SkPath intersect;
                    op = SkPathOp::kIntersect_SkPathOp;
                    if(!Op(result, skPath, op, &intersect))
                        RuntimeThrow("Operation Failed");
                    op = SkPathOp::kDifference_SkPathOp;
                    if(!Op(result, skPath, op, &result))
                        RuntimeThrow("Operation Failed");
                    result.addPath(intersect);
                    continue;
            }
            if(!Op(result, path->getPathAtRelFrame(relFrame), op, &result))
                RuntimeThrow("Operation Failed");
        }
    }
    result.setFillType(mFillType);
    return result;
}

void SmartPathCollection::applyTransform(const QMatrix &transform) const {
    const int iMax = ca_getNumberOfChildren() - 1;
    for(int i = 0; i <= iMax; i++) {
        const auto path = ca_getChildAt<SmartPathAnimator>(i);
        path->applyTransform(transform);
    }
}

void SmartPathCollection::loadSkPath(const SkPath &path) {
    const QList<SkPath> paths = gBreakApart(path);
    for(const auto& path : paths) {
        if(path.countVerbs() > 1) createNewPath(path);
    }
}

void SmartPathCollection::setFillType(const SkPathFillType fillType) {
    if(mFillType == fillType) return;
    {
        UndoRedo ur;
        const auto oldValue = mFillType;
        const auto newValue = fillType;
        ur.fUndo = [this, oldValue]() {
            setFillType(oldValue);
        };
        ur.fRedo = [this, newValue]() {
            setFillType(newValue);
        };
        prp_addUndoRedo(ur);
    }
    mFillType = fillType;
    prp_afterWholeInfluenceRangeChanged();
    emit fillTypeChanged(fillType);
}

SmartPathAnimator *SmartPathCollection::createNewPath() {
    const auto newPath = enve::make_shared<SmartPathAnimator>();
    addChild(newPath);
    return newPath.get();
}

SmartPathAnimator *SmartPathCollection::createNewPath(const SkPath &path) {
    const auto newPath = enve::make_shared<SmartPathAnimator>(path);
    addChild(newPath);
    return newPath.get();
}

const QList<QColor> gPathColors =
    { QColor(255, 255, 255),
      QColor(0, 255, 255), QColor(255, 155, 0),
      QColor(255, 0, 255), QColor(0, 255, 0),
      QColor(255, 0, 0)};

void SmartPathCollection::updatePathColors() {
    const auto& children = ca_getChildren();
    int i = 0;
    for(const auto& child : children) {
        const auto path = static_cast<SmartPathAnimator*>(child.get());
        path->setPathColor(gPathColors[(i++) % gPathColors.length()]);
    }
}

void SmartPathCollection::updateVisibleChildren() {
    bool allVisible = true;
    const auto& children = ca_getChildren();
    for(const auto& child : children) {
        if(child->prp_isSelected()) {
            allVisible = false;
            break;
        }
    }
    for(const auto& child : children) {
        const bool enabled = allVisible || child->prp_isSelected();
        child->prp_setDrawingOnCanvasEnabled(enabled);
    }
}
