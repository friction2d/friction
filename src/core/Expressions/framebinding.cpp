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

#include "framebinding.h"

FrameBinding::FrameBinding(const Property* const context) :
    PropertyBindingBase(context) {}

qsptr<FrameBinding> FrameBinding::sCreate(const Property* const context) {
    const auto result = new FrameBinding(context);
    return qsptr<FrameBinding>(result);
}

QJSValue FrameBinding::getJSValue(QJSEngine& e) {
    Q_UNUSED(e)
    return relFrame();
}

QJSValue FrameBinding::getJSValue(QJSEngine& e, const qreal relFrame) {
    Q_UNUSED(e)
    return relFrame;
}

FrameRange FrameBinding::identicalRelRange(const int absFrame) {
    if(mContext) {
        const int relFrame = mContext->prp_absFrameToRelFrame(absFrame);
        return {relFrame, relFrame};
    }
    return FrameRange::EMINMAX;
}

FrameRange FrameBinding::nextNonUnaryIdenticalRelRange(const int absFrame) {
    Q_UNUSED(absFrame);
    if(mContext) return {FrameRange::EMAX/2, FrameRange::EMAX};
    return FrameRange::EMINMAX;
}
