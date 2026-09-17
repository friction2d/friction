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

#ifndef LOTTIEEXPORTER_H
#define LOTTIEEXPORTER_H

#include "Private/Tasks/complextask.h"
#include "framerange.h"

class Canvas;

class CORE_EXPORT LottieExporter : public ComplexTask
{
public:
    LottieExporter(const QString& path,
                   Canvas* const scene,
                   const FrameRange& frameRange,
                   const qreal fps,
                   const bool background,
                   const bool embedImages = true,
                   const bool svgRendererFix = false,
                   const bool nativeText = false);

    void nextStep() override;

private:
    void finish();

    const QString mPath;
    Canvas* const mScene;
    const FrameRange mFrameRange;
    const qreal mFps;
    const bool mBackground;
    const bool mEmbedImages;
    const bool mSvgRendererFix;
    const bool mNativeText;
};

#endif // LOTTIEEXPORTER_H
