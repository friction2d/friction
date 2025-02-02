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

#include "rastereffect.h"
#include "Animators/dynamiccomplexanimator.h"
#include "typemenu.h"

RasterEffect::RasterEffect(const QString &name,
                           const HardwareSupport hwSupport,
                           const bool hwInterchangeable,
                           const RasterEffectType type) :
    eEffect(name), mType(type),
    mTypeHwSupport(hwSupport),
    mHwInterchangeable(hwInterchangeable) {
    if(hwInterchangeable ||
       hwSupport == HardwareSupport::cpuOnly ||
       hwSupport == HardwareSupport::gpuOnly) {
        mInstHwSupport = hwSupport;
    } else if(hwSupport == HardwareSupport::cpuPreffered) {
        mInstHwSupport = HardwareSupport::cpuOnly;
    } else if(hwSupport == HardwareSupport::gpuPreffered) {
        mInstHwSupport = HardwareSupport::gpuOnly;
    } else Q_ASSERT(false);
}

void RasterEffect::writeIdentifier(eWriteStream &dst) const {
    dst.write(&mType, sizeof(RasterEffectType));
}

void RasterEffect::writeIdentifierXML(QDomElement& ele) const {
    ele.setAttribute("type", static_cast<int>(mType));
}

void RasterEffect::prp_setupTreeViewMenu(PropertyMenu * const menu) {
    eEffect::prp_setupTreeViewMenu(menu);
    const PropertyMenu::PlainSelectedOp<RasterEffect> dOp =
    [](RasterEffect* const eff) {
        const auto parent = eff->getParent<DynamicComplexAnimatorBase<RasterEffect>>();
        parent->removeChild(eff->ref<RasterEffect>());
    };
    menu->addPlainAction(QIcon::fromTheme("trash"), tr("Delete Effect(s)"), dOp);
}

QMimeData *RasterEffect::SWT_createMimeData() {
    return new eMimeData(QList<RasterEffect*>() << this);
}

void RasterEffect::switchInstanceHwSupport() {
    if(mTypeHwSupport == HardwareSupport::cpuOnly) return;
    if(mTypeHwSupport == HardwareSupport::gpuOnly) return;
    if(mInstHwSupport == HardwareSupport::cpuOnly) {
        if(mHwInterchangeable) mInstHwSupport = mTypeHwSupport;
        else mInstHwSupport = HardwareSupport::gpuOnly;
    } else if(mInstHwSupport == HardwareSupport::gpuOnly) {
        mInstHwSupport = HardwareSupport::cpuOnly;
    } else mInstHwSupport = HardwareSupport::gpuOnly;
    if(!mHwInterchangeable) prp_afterWholeInfluenceRangeChanged();
    emit hardwareSupportChanged();
}
