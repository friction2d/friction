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

#ifndef SKIAINCLUDES_H
#define SKIAINCLUDES_H

#include <qglobal.h>

#undef foreach

QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wattributes")

#ifdef _MSC_VER
#pragma warning(disable: 5030)
#endif

#include "skiadefines.h"

#include "include/gpu/GrBackendSurface.h"
#include "include/gpu/GrContext.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkSurface.h"
#include "include/core/SkPixelRef.h"
#include "include/core/SkTypeface.h"
#include "include/core/SkPathMeasure.h"
#include "include/core/SkFont.h"
#include "include/core/SkImageFilter.h"
#include "include/core/SkMaskFilter.h"
#include "include/core/SkColorFilter.h"
#include "include/core/SkFontMetrics.h"
#include "include/core/SkMatrix44.h"
#include "include/core/SkPoint3.h"

#include "include/pathops/SkPathOps.h"

#include "include/utils/SkRandom.h"
#include "include/utils/SkTextUtils.h"
#include "include/utils/SkParsePath.h"
#include "include/utils/SkBase64.h"

#include "include/core/SkStrokeRec.h"
#include "include/effects/SkDashPathEffect.h"
#include "include/effects/SkDiscretePathEffect.h"

#include "include/effects/SkGradientShader.h"
#include "include/effects/SkBlurImageFilter.h"
#include "include/effects/SkDropShadowImageFilter.h"
#include "include/effects/SkImageFilters.h"

#include "include/gpu/gl/GrGLTypes.h"
#include "include/gpu/gl/GrGLFunctions.h"
#include "include/gpu/gl/GrGLInterface.h"

#include <QtGui/qopengl.h>

#include "src/gpu/gl/GrGLDefines.h"

#include "src/core/SkStroke.h"
#include "src/core/SkGeometry.h" // for SkAutoConicToQuads

#ifdef _MSC_VER
#pragma warning(default: 5030)
#endif

QT_WARNING_POP

#endif // SKIAINCLUDES_H
