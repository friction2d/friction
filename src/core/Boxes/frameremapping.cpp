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

#include "frameremapping.h"
#include "Animators/qrealkey.h"

FrameRemappingBase::FrameRemappingBase() : QrealAnimator("frame") {}

void FrameRemappingBase::disableAction() {
    setEnabled(false);
    anim_setRecording(false);
}

void FrameRemappingBase::setFrameCount(const int count) {
    setValueRange(0, count - 1);
}

void FrameRemappingBase::prp_readProperty_impl(eReadStream &src) {
    bool enabled; src >> enabled;
    setEnabled(enabled);
    QrealAnimator::prp_readProperty_impl(src);
}

void FrameRemappingBase::prp_writeProperty_impl(eWriteStream &dst) const {
    dst << mEnabled;
    QrealAnimator::prp_writeProperty_impl(dst);
}

QDomElement FrameRemappingBase::prp_writePropertyXML_impl(const Friction::Core::XmlExporter& exp) const {
    auto result = QrealAnimator::prp_writePropertyXML_impl(exp);
    result.setAttribute("enabled", mEnabled ? "true" : "false");
    return result;
}

void FrameRemappingBase::prp_readPropertyXML_impl(const QDomElement& ele, const Friction::Core::XmlImporter& imp) {
    QrealAnimator::prp_readPropertyXML_impl(ele, imp);
    const auto enabled = ele.attribute("enabled");
    setEnabled(enabled == "true");
}

void FrameRemappingBase::enableAction(const int minFrame,
                                      const int maxFrame,
                                      const int animStartRelFrame)
{
    if (mEnabled) { return; }
    prp_pushUndoRedoName(tr("Enable Frame Remapping"));
    setValueRange(minFrame, maxFrame);
    if (maxFrame > minFrame) {
        const int firstValue = minFrame;
        const int firstFrame = animStartRelFrame + minFrame;
        const auto firstFrameKey = enve::make_shared<QrealKey>(firstValue,
                                                               firstFrame,
                                                               this);
        anim_appendKey(firstFrameKey);
        const int lastValue = maxFrame;
        const int lastFrame = animStartRelFrame + maxFrame;
        const auto lastFrameKey = enve::make_shared<QrealKey>(lastValue,
                                                              lastFrame,
                                                              this);
        anim_appendKey(lastFrameKey);
    } else {
        setCurrentBaseValue(0);
    }
    setEnabled(true);
}

void FrameRemappingBase::setEnabled(const bool enabled)
{
    {
        prp_pushUndoRedoName(tr("Set Frame Remapping"));
        UndoRedo ur;
        const auto oldValue = mEnabled;
        const auto newValue = enabled;
        ur.fUndo = [this, oldValue]() { setEnabled(oldValue); };
        ur.fRedo = [this, newValue]() { setEnabled(newValue); };
        prp_addUndoRedo(ur);
    }
    SWT_setVisible(enabled);
    mEnabled = enabled;
    prp_afterWholeInfluenceRangeChanged();
    emit enabledChanged(mEnabled);
}

IntFrameRemapping::IntFrameRemapping() {
    setNumberDecimals(0);
}

int IntFrameRemapping::frame(const qreal relFrame) const {
    if(!enabled()) return qRound(relFrame);
    return qRound(getEffectiveValue(relFrame));
}

QrealFrameRemapping::QrealFrameRemapping() {}

qreal QrealFrameRemapping::frame(const qreal relFrame) const {
    if(!enabled()) return relFrame;
    return getEffectiveValue(relFrame);
}

