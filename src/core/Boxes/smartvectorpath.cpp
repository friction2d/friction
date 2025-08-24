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

#include "smartvectorpath.h"
#include <QPainter>
#include "canvas.h"
#include <QDebug>
#include "undoredo.h"
#include "MovablePoints/pathpivot.h"
#include "pointhelpers.h"
#include "Animators/SmartPath/smartpathcollection.h"
#include "Animators/gradientpoints.h"
#include "Animators/transformanimator.h"
#include "MovablePoints/segment.h"
#include "Animators/SmartPath/smartpathanimator.h"
#include "PathEffects/patheffectcollection.h"
#include "Animators/qpointfanimator.h"
#include "svgexporter.h"

SmartVectorPath::SmartVectorPath() :
    PathBox("Path", eBoxType::vectorPath) {
    mPathAnimator = enve::make_shared<SmartPathCollection>();
    connect(mPathAnimator.get(), &Property::prp_currentFrameChanged,
            this, [this](const UpdateReason reason) {
        setPathsOutdated(reason);
    });
    ca_prependChild(mPathEffectsAnimators.data(), mPathAnimator);
}

bool SmartVectorPath::differenceInEditPathBetweenFrames(
        const int frame1, const int frame2) const {
    return mPathAnimator->prp_differencesBetweenRelFrames(frame1, frame2);
}

void SmartVectorPath::saveSVG(SvgExporter& exp, DomEleTask* const task) const {
    const bool baseEffects = hasBasePathEffects();
    const bool outlineBaseEffects = hasOutlineBaseEffects();
    const bool outlineEffects = hasOutlineEffects();
    const bool fillEffects = hasFillEffects();
    const bool splitFillStroke = fillEffects ||
                                 outlineBaseEffects ||
                                 outlineEffects;
    if(splitFillStroke) {
        auto& ele = task->initialize("g");

        auto fill = exp.createElement("path");
        SmartPathCollection::EffectApplier fillApplier;
        if(baseEffects || fillEffects) {
            fillApplier = [this](const int relFrame, SkPath& path) {
                applyBasePathEffects(relFrame, path);
                applyFillEffects(relFrame, path);
            };
        };
        QList<Animator*> fillExtInfl;
        if(baseEffects) fillExtInfl << mPathEffectsAnimators.get();
        if(fillEffects) fillExtInfl << mFillPathEffectsAnimators.get();
        mPathAnimator->savePathsSVG(exp, fill, fillApplier,
                                    baseEffects || fillEffects,
                                    task->visRange(), fillExtInfl);
        saveFillSettingsSVG(exp, fill, task->visRange());
        fill.setAttribute("stroke", "none");
        ele.appendChild(fill);
        switch(mPathAnimator->getFillType()) {
        case SkPathFillType::kEvenOdd:
            fill.setAttribute("fill-rule", "evenodd");
            break;
        default:
            fill.setAttribute("fill-rule", "nonzero");
        }

        auto stroke = exp.createElement("path");
        SmartPathCollection::EffectApplier strokeApplier;
        if(baseEffects || outlineBaseEffects || outlineEffects) {
            strokeApplier = [this, outlineEffects](const int relFrame, SkPath& path) {
                applyBasePathEffects(relFrame, path);
                applyOutlineBaseEffects(relFrame, path);
                if(!outlineEffects) return;
                const auto strokeSettings = getStrokeSettings();
                SkStroke stroker;
                strokeSettings->setStrokerSettingsForRelFrameSk(relFrame, &stroker);
                stroker.strokePath(path, &path);
                applyOutlineEffects(relFrame, path);
            };
        };
        QList<Animator*> strokeExtInfl;
        if(baseEffects) fillExtInfl << mPathEffectsAnimators.get();
        if(outlineBaseEffects) fillExtInfl << mOutlineBasePathEffectsAnimators.get();
        if(outlineEffects) fillExtInfl << mOutlinePathEffectsAnimators.get();
        const bool forceDumb = baseEffects || outlineBaseEffects || outlineEffects;
        mPathAnimator->savePathsSVG(exp, stroke, strokeApplier,
                                    forceDumb, task->visRange(),
                                    strokeExtInfl);
        saveStrokeSettingsSVG(exp, stroke, task->visRange(), outlineEffects);
        stroke.setAttribute(outlineEffects ? "stroke" : "fill", "none");
        if(outlineEffects) stroke.setAttribute("fill-rule", "nonzero");

        ele.appendChild(stroke);
    } else {
        auto& ele = task->initialize("path");
        SmartPathCollection::EffectApplier applier;
        if(baseEffects) {
            applier = [this](const int relFrame, SkPath& path) {
                applyBasePathEffects(relFrame, path);
            };
        };
        QList<Animator*> extInfl;
        if(baseEffects) extInfl << mPathEffectsAnimators.get();
        mPathAnimator->savePathsSVG(exp, ele, applier, baseEffects,
                                    task->visRange(), extInfl);
        savePathBoxSVG(exp, ele, task->visRange());
    }
}

void SmartVectorPath::loadSkPath(const SkPath &path) {
    mPathAnimator->loadSkPath(path);
}

SmartPathCollection *SmartVectorPath::getPathAnimator() {
    return mPathAnimator.data();
}

#include "typemenu.h"
void SmartVectorPath::setupCanvasMenu(PropertyMenu * const menu)
{
    if (menu->hasActionsForType<SmartVectorPath>()) { return; }
    menu->addedActionsForType<SmartVectorPath>();
    PathBox::setupCanvasMenu(menu);
    PropertyMenu::PlainSelectedOp<SmartVectorPath> op = [](SmartVectorPath * box) {
        box->applyCurrentTransform();
    };
    menu->addSeparator();
    menu->addPlainAction(QIcon::fromTheme("loop2"), tr("Apply Transformation"), op);
}

void SmartVectorPath::applyCurrentTransform()
{
    prp_pushUndoRedoName(tr("Apply Transform"));
    mNReasonsNotToApplyUglyTransform++;
    const auto transform = mTransformAnimator->getRotScaleShearTransform();
    mPathAnimator->applyTransform(transform);
    getFillSettings()->applyTransform(transform);
    getStrokeSettings()->applyTransform(transform);
    mTransformAnimator->startRotScaleShearTransform();
    mTransformAnimator->resetRotScaleShear();
    mTransformAnimator->prp_finishTransform();
    mNReasonsNotToApplyUglyTransform--;
}

SkPath SmartVectorPath::getRelativePath(const qreal relFrame) const {
     return mPathAnimator->getPathAtRelFrame(relFrame);
}

void SmartVectorPath::getMotionBlurProperties(QList<Property*> &list) const {
    PathBox::getMotionBlurProperties(list);
    list.append(mPathAnimator.get());
}

QList<qsptr<SmartVectorPath>> SmartVectorPath::breakPathsApart_k() {
    QList<qsptr<SmartVectorPath>> result;
    const int iMax = mPathAnimator->ca_getNumberOfChildren() - 1;
    if(iMax < 1) return result;
    for(int i = iMax; i >= 0; i--) {
        const auto srcPath = mPathAnimator->takeChildAt(i);
        const auto newPath = enve::make_shared<SmartVectorPath>();
        copyPathBoxDataTo(newPath.get());
        newPath->getPathAnimator()->addChild(srcPath);
        result.append(newPath);
    }
    removeFromParent_k();
    return result;
}
