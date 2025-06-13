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

#include "internallinkgroupbox.h"
#include "Animators/transformanimator.h"
#include "Timeline/durationrectangle.h"
#include "layerboxrenderdata.h"
#include "BlendEffects/blendeffectboxshadow.h"

InternalLinkGroupBox::InternalLinkGroupBox(ContainerBox * const linkTarget,
                                           const bool innerLink) :
    InternalLinkBoxBase<ContainerBox>("Link 0", eBoxType::internalLinkGroup, innerLink) {
    mBoxTarget->setValidator<ContainerBox>();

    ca_prependChild(mTransformAnimator.data(), mBoxTarget);
    connect(mBoxTarget.data(), &BoxTargetProperty::targetSet,
            this, [this](BoundingBox* const target) {
        const auto cbTarget = enve_cast<ContainerBox*>(target);
        setLinkTarget(cbTarget);
    });
    mBoxTarget->setTarget(linkTarget);
}

void InternalLinkGroupBox::setupRenderData(const qreal relFrame,
                                           const QMatrix& parentM,
                                           BoxRenderData * const data,
                                           Scene * scene
) {
    const auto linkTarget = getLinkTarget();
    if(linkTarget) linkTarget->BoundingBox::setupRenderData(
                relFrame, parentM, data, scene);
    ContainerBox::setupRenderData(relFrame, parentM, data, scene);
}

bool InternalLinkGroupBox::localDifferenceInPathBetweenFrames(
        const int frame1, const int frame2) const {
    const bool diff1 = ContainerBox::localDifferenceInPathBetweenFrames(frame1, frame2);
    if(diff1) return true;
    if(const auto linkTarget = getLinkTarget()) {
        const bool diff2 = linkTarget->localDifferenceInPathBetweenFrames(frame1, frame2);
        return diff1 || diff2;
    } else return false;
}

bool InternalLinkGroupBox::localDifferenceInOutlinePathBetweenFrames(
        const int frame1, const int frame2) const {
    const bool diff1 = ContainerBox::localDifferenceInOutlinePathBetweenFrames(frame1, frame2);
    if(diff1) return true;
    if(const auto linkTarget = getLinkTarget()) {
        const bool diff2 = linkTarget->localDifferenceInOutlinePathBetweenFrames(frame1, frame2);
        return diff1 || diff2;
    } else return false;
}

bool InternalLinkGroupBox::localDifferenceInFillPathBetweenFrames(
        const int frame1, const int frame2) const {
    const bool diff1 = ContainerBox::localDifferenceInFillPathBetweenFrames(frame1, frame2);
    if(diff1) return true;
    if(const auto linkTarget = getLinkTarget()) {
        const bool diff2 = linkTarget->localDifferenceInFillPathBetweenFrames(frame1, frame2);
        return diff1 || diff2;
    } else return false;
}

bool InternalLinkGroupBox::isFlipBook() const {
    const auto linkTarget = getLinkTarget();
    if(linkTarget) return linkTarget->isFlipBook();
    else return false;
}

iValueRange InternalLinkGroupBox::getContainedMinMax() const {
    const auto linkTarget = getLinkTarget();
    if(linkTarget) return linkTarget->getContainedMinMax();
    else return ContainerBox::getContainedMinMax();
}

#include "Sound/eindependentsound.h"
#include "Sound/esoundlink.h"

void InternalLinkGroupBox::insertInnerLinkFor(
        const int id, eBoxOrSound* const obj) {
    if(const auto box = enve_cast<BoundingBox*>(obj)) {
        const auto newLink = box->createLink(true);
        insertContained(id, newLink);
    } else if(const auto sound = enve_cast<eIndependentSound*>(obj)) {
        const auto newLink = sound->createLink();
        insertContained(id, newLink);
    } else if(const auto bebs = enve_cast<BlendEffectBoxShadow*>(obj)) {
        const auto newLink = bebs->createLink();
        insertContained(id, newLink);
    }
}

void InternalLinkGroupBox::setLinkTarget(ContainerBox * const linkTarget) {
    removeAllContained();
    mBoxTarget->setTargetAction(linkTarget);
    auto& conn = assignLinkTarget(linkTarget);
    if(linkTarget) {
        conn << connect(linkTarget->getTransformAnimator(),
                        &Property::prp_absFrameRangeChanged,
                this, [this, linkTarget](const FrameRange& targetAbs) {
            const auto relRange = linkTarget->prp_absRangeToRelRange(targetAbs);
            mTransformAnimator->prp_afterChangedRelRange(relRange);
        });

        if(linkTarget->isLayer()) promoteToLayer();
        else demoteToGroup();
        conn << connect(linkTarget, &ContainerBox::switchedGroupLayer,
                        this, [this](const eBoxType type) {
            if(type == eBoxType::group) {
                demoteToGroup();
            } else if(type == eBoxType::layer) {
                promoteToLayer();
            }
        });

        conn << connect(linkTarget, &ContainerBox::insertedObject,
                this, &InternalLinkGroupBox::insertInnerLinkFor);
        conn << connect(linkTarget, &ContainerBox::removedObject,
                        this, &ContainerBox::removeContainedFromList);
        conn << connect(linkTarget, &ContainerBox::movedObject,
                        this, qOverload<int, int>(&ContainerBox::moveContainedInList));
        conn << connect(linkTarget, &ContainerBox::childPathsUpdated,
                        this, &InternalLinkGroupBox::updateAllChildPaths);

        const auto &boxesList = linkTarget->getContained();
        const int iMax = boxesList.count();
        for(int i = 0; i < iMax; i++) {
            const auto& child = boxesList.at(i);
            insertInnerLinkFor(i, child.get());
        }
    }
    planUpdate(UpdateReason::userChange);
}
