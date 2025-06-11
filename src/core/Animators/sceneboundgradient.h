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

#ifndef SCENEBOUNDGRADIENT_H
#define SCENEBOUNDGRADIENT_H
#include "gradient.h"

class CORE_EXPORT SceneBoundGradient : public Gradient {
public:
    SceneBoundGradient(Scene* const scene);

    void prp_setInheritedFrameShift(
            const int shift, ComplexAnimator* parentAnimator);

    int prp_getTotalFrameShift() const { return 0; }
    int prp_getInheritedFrameShift() const { return 0; }

    void write(const int id, eWriteStream &dst);
    int read(eReadStream &src);

    int getReadWriteId() const { return mReadWriteId; }
    int getDocumentId() const { return mDocumentId; }
    void clearReadWriteId() { mReadWriteId = -1; }
private:
    static int sNextDocumentId;
    const int mDocumentId;
    int mReadWriteId = -1;
    Scene* const mScene;
};

#endif // SCENEBOUNDGRADIENT_H
