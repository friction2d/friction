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
*/

#ifndef LOTTIEREALKEYFRAMES_H
#define LOTTIEREALKEYFRAMES_H

#include "framerange.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QPointF>

#include <functional>

class QPointFAnimator;
class QrealAnimator;
class ColorAnimator;

namespace LottieRealKeyframes {

using ScalarValue = std::function<qreal(qreal)>;
using PointValue = std::function<QJsonArray(const QPointF&, qreal)>;

QJsonObject scalar(QrealAnimator* const animator,
                   const FrameRange& frameRange,
                   const ScalarValue& value = nullptr);
QJsonObject point(QPointFAnimator* const animator,
                  const FrameRange& frameRange,
                  const PointValue& value);
QJsonObject color(ColorAnimator* const animator,
                  const FrameRange& frameRange,
                  const bool alpha);

}

#endif // LOTTIEREALKEYFRAMES_H
