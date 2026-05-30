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

#ifndef INTERNALLINKBOXBASE_H
#define INTERNALLINKBOXBASE_H

#include "boundingbox.h"
#include "Timeline/durationrectangle.h"

template <typename BoxT>
class InternalLinkBoxBase : public BoxT {
protected:
    InternalLinkBoxBase(const QString& name,
                        const eBoxType type,
                        const bool innerLink) :
        BoxT(name, type), mInnerLink(innerLink) {}
public:
    bool isLink() const final { return true; }

    FrameRange prp_getIdenticalRelRange(const int relFrame) const override;
    FrameRange prp_relInfluenceRange() const override;
    int prp_getRelFrameShift() const override;

    void writeBoundingBox(eWriteStream& dst) const override
    { BoundingBox::writeBoundingBox(dst); }

    void readBoundingBox(eReadStream& src) override
    { BoundingBox::readBoundingBox(src); }

    bool relPointInsidePath(const QPointF &relPos) const override;
    QPointF getRelCenterPosition() override;

    stdsptr<BoxRenderData> createRenderData() override;

    SkBlendMode getBlendMode() const override;

    QTransform getRelativeTransformAtFrame(const qreal relFrame) const override;
    QTransform getInheritedTransformAtFrame(const qreal relFrame) const override;
    QTransform getTotalTransformAtFrame(const qreal relFrame) const override;

    bool isFrameInDurationRect(const int relFrame) const override;
    bool isFrameFInDurationRect(const qreal relFrame) const override;

    HardwareSupport hardwareSupport() const override;

    void blendSetup(ChildRenderData& data,
                    const int index, const qreal relFrame,
                    QList<ChildRenderData>& delayed) const override;
    void detachedBlendSetup(
            SkCanvas * const canvas,
            const SkFilterQuality filter, int& drawId,
            QList<BlendEffect::Delayed> &delayed) const override;

    void saveSVG(SvgExporter& exp, DomEleTask* const task) const override;

    qreal getOpacity(const qreal relFrame) const override;

    BoxT *getFinalTarget() const;
protected:
    ConnContext& assignLinkTarget(BoxT * const linkTarget);
    BoxT *getLinkTarget() const
    { return mLinkTarget; }

    const qsptr<BoxTargetProperty> mBoxTarget =
            enve::make_shared<BoxTargetProperty>("link target");
private:
    const bool mInnerLink;
    ConnContextQPtr<BoxT> mLinkTarget;
};

#define ILBB InternalLinkBoxBase<BoxT>

template <typename BoxT>
qreal ILBB::getOpacity(const qreal relFrame) const {
    const auto linkTarget = getLinkTarget();
    if(mInnerLink && linkTarget) {
        return linkTarget->getOpacity(relFrame);
    } else {
        return BoundingBox::getOpacity(relFrame);
    }
}

template <typename BoxT>
bool ILBB::relPointInsidePath(const QPointF &relPos) const {
    const auto linkTarget = getLinkTarget();
    if(!linkTarget) return false;
    return linkTarget->relPointInsidePath(relPos);
}

template <typename BoxT>
QPointF ILBB::getRelCenterPosition() {
    const auto linkTarget = getLinkTarget();
    if(!linkTarget) return QPointF();
    return linkTarget->getRelCenterPosition();
}

template <typename BoxT>
SkBlendMode ILBB::getBlendMode() const {
    const auto linkTarget = getLinkTarget();
    if(mInnerLink && linkTarget) {
        return linkTarget->getBlendMode();
    }
    return BoxT::getBlendMode();
}

template <typename BoxT>
HardwareSupport ILBB::hardwareSupport() const {
    const auto linkTarget = getLinkTarget();
    if(!linkTarget) return BoxT::hardwareSupport();
    return linkTarget->hardwareSupport();
}

template<typename BoxT>
ConnContext &ILBB::assignLinkTarget(BoxT * const linkTarget) {
    auto& conn = mLinkTarget.assign(linkTarget);
    if(linkTarget) {
        if(mInnerLink) {
            this->setVisible(linkTarget->isVisible());
            conn << QObject::connect(linkTarget, &eBoxOrSound::visibilityChanged,
                                     this, [this](const bool visible) {
                this->setVisible(visible);
            });
        } else {
            this->rename(linkTarget->prp_getName() + " Link 0");
        }
        conn << QObject::connect(linkTarget, &BoundingBox::prp_absFrameRangeChanged,
                                 this, [this, linkTarget](const FrameRange& targetAbs) {
            const auto relRange = linkTarget->prp_absRangeToRelRange(targetAbs);
            this->prp_afterChangedRelRange(relRange);
        });
    } else if(!mInnerLink) this->rename("Empty Link 0");
    return conn;
}

template <typename BoxT>
bool ILBB::isFrameInDurationRect(const int relFrame) const {
    const auto linkTarget = getLinkTarget();
    if(!linkTarget) return false;
    return BoxT::isFrameInDurationRect(relFrame) &&
           linkTarget->isFrameInDurationRect(relFrame);
}

template <typename BoxT>
bool ILBB::isFrameFInDurationRect(const qreal relFrame) const {
    const auto linkTarget = getLinkTarget();
    if(!linkTarget) return false;
    return BoxT::isFrameFInDurationRect(relFrame) &&
           linkTarget->isFrameFInDurationRect(relFrame);
}

template <typename BoxT>
QTransform ILBB::getRelativeTransformAtFrame(const qreal relFrame) const {
    if(mInnerLink) {
        const auto linkTarget = getLinkTarget();
        if(!linkTarget) return QTransform();
        return linkTarget->getRelativeTransformAtFrame(relFrame);
    } else {
        return BoundingBox::getRelativeTransformAtFrame(relFrame);
    }
}

template <typename BoxT>
QTransform ILBB::getInheritedTransformAtFrame(const qreal relFrame) const {
    if(mInnerLink) {
        const auto parentGroup = this->getParentGroup();
        if(!parentGroup) return QTransform();
        return parentGroup->getTotalTransformAtFrame(relFrame);
    } else {
        return BoundingBox::getInheritedTransformAtFrame(relFrame);
    }
}

template <typename BoxT>
QTransform ILBB::getTotalTransformAtFrame(const qreal relFrame) const {
    if(mInnerLink) {
        const auto parentGroup = this->getParentGroup();
        const auto linkTarget = getLinkTarget();
        if(!linkTarget || !parentGroup) return QTransform();
        return linkTarget->getRelativeTransformAtFrame(relFrame)*
               parentGroup->getTotalTransformAtFrame(relFrame);
    } else {
        return BoundingBox::getTotalTransformAtFrame(relFrame);
    }
}

template <typename BoxT>
BoxT *ILBB::getFinalTarget() const {
    const auto linkTarget = getLinkTarget();
    if(!linkTarget) return nullptr;
    if(linkTarget->isLink()) {
        const auto targetAsLink = static_cast<ILBB*>(linkTarget);
        return targetAsLink->getFinalTarget();
    }
    return linkTarget;
}

template <typename BoxT>
FrameRange ILBB::prp_getIdenticalRelRange(const int relFrame) const {
    FrameRange range{FrameRange::EMIN, FrameRange::EMAX};
    const auto linkTarget = getLinkTarget();
    if(this->isVisible() && linkTarget)
        range *= BoxT::prp_getIdenticalRelRange(relFrame);
    else return range;
    auto targetRange = linkTarget->prp_getIdenticalRelRange(relFrame);
    return range*targetRange;
}

template <typename BoxT>
FrameRange ILBB::prp_relInfluenceRange() const {
    const auto linkTarget = getLinkTarget();
    FrameRange inflRange;
    const auto durRect = this->getDurationRectangle();
    if(durRect) inflRange = durRect->getRelFrameRange();
    else inflRange = ComplexAnimator::prp_relInfluenceRange();
    if(linkTarget) {
        return inflRange*linkTarget->prp_relInfluenceRange();
    } else return inflRange;
}

template <typename BoxT>
int ILBB::prp_getRelFrameShift() const {
    const auto linkTarget = getLinkTarget();
    if(linkTarget) {
        return linkTarget->prp_getRelFrameShift() +
               BoxT::prp_getRelFrameShift();
    } else return BoxT::prp_getRelFrameShift();
}

template <typename BoxT>
stdsptr<BoxRenderData> ILBB::createRenderData() {
    const auto linkTarget = getLinkTarget();
    if(!linkTarget) return nullptr;
    const auto renderData = linkTarget->createRenderData();
    if(!renderData) return nullptr;
    renderData->fParentBox = this;
    if(!mInnerLink) renderData->fBlendEffectIdentifier = this;
    return renderData;
}

template <typename BoxT>
void ILBB::blendSetup(ChildRenderData& data,
                      const int index, const qreal relFrame,
                      QList<ChildRenderData>& delayed) const {
    if(mInnerLink) {
        const auto linkTarget = getLinkTarget();
        if(!linkTarget) return;
        linkTarget->blendSetup(data, index, relFrame, delayed);
    } else BoxT::blendSetup(data, index, relFrame, delayed);
}

template <typename BoxT>
void ILBB::detachedBlendSetup(
        SkCanvas * const canvas,
        const SkFilterQuality filter, int& drawId,
        QList<BlendEffect::Delayed> &delayed) const {
    if(mInnerLink) {
        const auto linkTarget = getLinkTarget();
        if(!linkTarget) return;
        linkTarget->detachedBlendSetup(canvas, filter, drawId, delayed);
    } else BoxT::detachedBlendSetup(canvas, filter, drawId, delayed);
}

template<typename BoxT>
void ILBB::saveSVG(SvgExporter& exp, DomEleTask* const task) const {
    const auto linkTarget = getLinkTarget();
    if(linkTarget) linkTarget->saveSVG(exp, task);
}

#endif // INTERNALLINKBOXBASE_H
