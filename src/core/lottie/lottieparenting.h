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
*/

#ifndef LOTTIEPARENTING_H
#define LOTTIEPARENTING_H

#include "framerange.h"

#include <QJsonObject>

class BoundingBox;

namespace LottieParenting {

BoundingBox* target(const BoundingBox* const box);
QJsonObject transform(const BoundingBox* const box,
                      const BoundingBox* const parent,
                      const FrameRange& frameRange);

}

#endif // LOTTIEPARENTING_H
