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

#ifndef LOTTIEIMPORTER_H
#define LOTTIEIMPORTER_H

#include "smartPointers/selfref.h"

#include <QSize>
#include <QStringList>

class BoundingBox;
class Canvas;

namespace ImportLottie {

enum class SceneDurationMode {
    dontModify,
    extendIfNeeded,
    fitImportedLottie
};

struct CORE_EXPORT Analysis {
    int totalLayers = 0;
    int supportedLayers = 0;
    int totalShapes = 0;
    int supportedShapes = 0;
    int firstFrame = 0;
    int lastFrame = 0;
    qreal fps = 0;
    qreal duration = 0;
    QSize size;
    QStringList unsupported;
};

CORE_EXPORT Analysis analyzeFile(const QString& filename);

CORE_EXPORT qsptr<BoundingBox> loadFile(const QString& filename,
                                        Canvas* scene,
                                        SceneDurationMode durationMode =
                                            SceneDurationMode::extendIfNeeded);

}

#endif // LOTTIEIMPORTER_H
