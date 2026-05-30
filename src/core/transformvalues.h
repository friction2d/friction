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

#ifndef TRANSFORMVALUES_H
#define TRANSFORMVALUES_H

#include <QTransform>

#include "core_global.h"

struct CORE_EXPORT TransformValues {
    qreal fPivotX = 0;
    qreal fPivotY = 0;
    qreal fMoveX = 0;
    qreal fMoveY = 0;
    qreal fRotation = 0;
    qreal fScaleX = 1;
    qreal fScaleY = 1;
    qreal fShearX = 0;
    qreal fShearY = 0;

    QTransform calculate() const;
};

#endif // TRANSFORMVALUES_H
