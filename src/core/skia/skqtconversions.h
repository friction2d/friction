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

#ifndef SKQTCONVERSIONS_H
#define SKQTCONVERSIONS_H

#include <QRectF>
#include <QMatrix>
#include <QMatrix4x4>
#include <QColor>
#include <QFont>

#include "skiaincludes.h"
#include "../core_global.h"

extern inline qreal toQreal(const float ss) {
    return static_cast<qreal>(ss);
}

extern inline float toSkScalar(const qreal qr) {
    return static_cast<float>(qr);
}

CORE_EXPORT
extern SkFontStyle::Slant toSkSlant(const QFont::Style& style);
CORE_EXPORT
extern int QFontWeightToSkFontWeight(const int qWeight);
CORE_EXPORT
extern QFont toQFont(const SkFont& skfont, const int skPPI, const int qPPI);
CORE_EXPORT
extern QFont toQFont(const SkFont& skfont);
CORE_EXPORT
extern SkFont toSkFont(const QFont& qfont, const int qPPI, const int skPPI);
CORE_EXPORT
extern SkFont toSkFont(const QFont& qfont);

CORE_EXPORT
extern SkColor toSkColor(const QColor& qcol);

CORE_EXPORT
extern QRect toQRect(const SkIRect &rect);
CORE_EXPORT
extern SkIRect toSkIRect(const QRect &rect);

CORE_EXPORT
extern QRectF toQRectF(const SkRect &rect);
CORE_EXPORT
extern SkRect toSkRect(const QRectF &rect);
// m11 - scaleX
// m12 - skewY
// m21 - skewX
// m22 - scaleY
CORE_EXPORT
extern QMatrix toQMatrix(const SkMatrix &matrix);

CORE_EXPORT
extern SkMatrix toSkMatrix(const QMatrix &matrix);

CORE_EXPORT
extern QMatrix4x4 toQMatrix4x4(const SkMatrix44 &matrix);

CORE_EXPORT
extern SkMatrix44 toSkMatrix44(const QMatrix4x4 &matrix);

extern inline QPointF toQPointF(const SkPoint &point) {
    return QPointF(toQreal(point.x()),
                   toQreal(point.y()));
}

extern inline QVector3D toQVector3D(const SkPoint3 &point) {
    return QVector3D(toQreal(point.x()),
                   toQreal(point.y()),
                   toQreal(point.z()));
}

extern inline SkPoint toSkPoint(const QPointF &point) {
    return SkPoint::Make(toSkScalar(point.x()),
                         toSkScalar(point.y()));
}

extern inline SkPoint toSkPoint(const QPoint &point) {
    return SkPoint::Make(point.x(), point.y());
}

extern inline SkPoint3 toSkPoint3(const QPointF &point) {
    return SkPoint3::Make(toSkScalar(point.x()),
                          toSkScalar(point.y()),
                          0);
}

extern inline SkPoint3 toSkPoint3(const QPoint &point) {
    return SkPoint3::Make(point.x(), point.y(), 0);
}

extern inline SkPoint3 toSkPoint3(const QVector3D &point) {
    return SkPoint3::Make(point.x(), point.y(), point.z());
}

CORE_EXPORT
extern SkPaint::Cap QCapToSkCap(const Qt::PenCapStyle &cap);

CORE_EXPORT
extern SkPaint::Join QJoinToSkJoin(const Qt::PenJoinStyle &join);

CORE_EXPORT
extern SkPath toSkPath(const QPainterPath &qPath);
CORE_EXPORT
extern QPainterPath toQPainterPath(const SkPath &path);

CORE_EXPORT
extern sk_sp<SkImage> toSkImage(const QImage& qImg);

CORE_EXPORT
extern void switchSkQ(const QPointF& qPos, SkPoint& skPos);
CORE_EXPORT
extern void switchSkQ(const SkPoint& skPos, QPointF& qPos);
CORE_EXPORT
extern void switchSkQ(const qreal q, float& sk);
CORE_EXPORT
extern void switchSkQ(const float sk, qreal& q);

#endif // SKQTCONVERSIONS_H
