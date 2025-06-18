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

#include "sceneboundgradient.h"
#include "Private/scene.h"

int SceneBoundGradient::sNextDocumentId = 0;

SceneBoundGradient::SceneBoundGradient(Scene * const scene) :
    mDocumentId(sNextDocumentId++), mScene(scene) {
    connect(this, &Property::prp_ancestorChanged, this, [this]() {
        if(!getParentScene()) setParent(mScene->getCurrentGroup().get());
    });
}

void SceneBoundGradient::prp_setInheritedFrameShift(
        const int shift, ComplexAnimator *parentAnimator) {
    Q_UNUSED(shift)
    if(!parentAnimator) return;
    const auto& keys = anim_getKeys();
    for(const auto &key : keys) {
        parentAnimator->ca_updateDescendatKeyFrame(key);
    }
}

void SceneBoundGradient::write(const int id, eWriteStream& dst) {
    mReadWriteId = id;
    dst << id;
    prp_writeProperty_impl(dst);
}

int SceneBoundGradient::read(eReadStream& src) {
    src >> mReadWriteId;
    prp_readProperty_impl(src);
    return mReadWriteId;
}
