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

#include "simplemath.h"
#include "skia/skqtconversions.h"
#include <cmath>

qreal signedSquare(const qreal val) {
    return val*val*SIGN(val);
}

qreal distSign(const QPointF& distPt) {
    const qreal val = signedSquare(distPt.x()) + signedSquare(distPt.y());
    if(val > 0) return sqrt(val);
    else return -sqrt(-val);
}

qreal gRandF(const qreal fMin, const qreal fMax) {
    qreal f = static_cast<qreal>(QRandomGenerator::global()->generate())/RAND_MAX;
    return fMin + f * (fMax - fMin);
}

float gSkRandF(const float fMin, const float fMax) {
    float f = static_cast<float>(QRandomGenerator::global()->generate())/(float)RAND_MAX;
    return fMin + f * (fMax - fMin);
}

qreal pointToLen(QPointF point) {
    return qSqrt(point.x()*point.x() + point.y()*point.y());
}

float pointToLen(const SkPoint &point) {
    return SkScalarSqrt(point.x()*point.x() + point.y()*point.y());
}

QPointF scalePointToNewLen(const QPointF& point,
                           const qreal newLen) {
    const qreal currLen = pointToLen(point);
    if(isZero4Dec(currLen)) return QPointF(0, 0);
    return point * newLen / currLen;
}

int clamp(const int val, const int min, const int max) {
    if(val > max) return max;
    if(val < min) return min;
    return val;
}

bool isZero4Dec(const float val) {
    return abs(val) < 0.0001f;
}

bool isZero4Dec(const double val) {
    return qAbs(val) < 0.0001;
}

bool isInteger4Dec(const double val) {
    return isZero4Dec(val - qRound(val));
}

double floor4Dec(const double val) {
    const double roundVal = qRound(val);
    if(isZero4Dec(val - roundVal)) return roundVal;
    return qFloor(val);
}

bool isZero6Dec(const double val) {
    return qAbs(val) < 0.000001;
}

bool isOne4Dec(const double val) {
    return isZero4Dec(val - 1);
}

bool isOne6Dec(const double val) {
    return isZero6Dec(val - 1);
}

bool isZero6Dec(const float val) {
    return qAbs(val) < 0.000001f;
}

bool isOne4Dec(const float val) {
    return isZero4Dec(val - 1);
}

bool isOne6Dec(const float val) {
    return isZero6Dec(val - 1);
}

bool isZeroOrOne6Dec(const double val) {
    if(isZero6Dec(val)) return true;
    if(isOne6Dec(val)) return true;
    return false;
}

bool isZero2Dec(const qreal val) {
    return qAbs(val) < 0.01;
}

bool isZero2Dec(const float val) {
    return qAbs(val) < 0.01f;
}

bool isZero1Dec(qreal val) {
    return qAbs(val) < 0.1;
}

bool isPointZero(QPointF pos) {
    return pointToLen(pos) < 0.0001;
}

bool isPointFinite(const QPointF& point) {
    return std::isfinite(point.x()) && std::isfinite(point.y());
}

bool isNonZero(const float val) {
    return val > 0.0001f || val < - 0.0001f;
}

bool isNonZero(const double val) {
    return val > 0.0001 || val < - 0.0001;
}

void rotate(float rad_t, float *x_t, float *y_t) {
    float cos_hue = cosf(rad_t);
    float sin_hue = sinf(rad_t);
    float x_rotated_t = *x_t*cos_hue - *y_t*sin_hue;
    float y_rotated_t = *x_t*sin_hue + *y_t*cos_hue;
    *x_t = x_rotated_t;
    *y_t = y_rotated_t;
}

void rotate(qreal rad_t, qreal *x_t, qreal *y_t) {
    qreal cos_hue = cos(rad_t);
    qreal sin_hue = sin(rad_t);
    qreal x_rotated_t = *x_t*cos_hue - *y_t*sin_hue;
    qreal y_rotated_t = *x_t*sin_hue + *y_t*cos_hue;
    *x_t = x_rotated_t;
    *y_t = y_rotated_t;
}

unsigned char truncateU8(const int val) {
    if(val > 255) {
        return 255;
    } else if(val < 0) {
        return 0;
    }
    return static_cast<unsigned char>(val);
}

int clampInt(int val, int min, int max) {
    if(val > max) {
        return max;
    } else if(val < min) {
        return min;
    } else {
        return val;
    }
}

float clamp(float val, float min_t, float max_t) {
    if(val > max_t)
    {
        return max_t;
    }
    if(val < min_t)
    {
        return min_t;
    }
    return val;
}

double getAngleF(double x1, double y1, double x2, double y2) {
    double dot = x1*x2 + y1*y2;
    double det = x1*y2 - y1*x2;
    return atan2(det, dot)*RadToF + 0.5;
}

double getAngleDeg(double x1, double y1, double x2, double y2)
{
    double dot = x1*x2 + y1*y2;
    double det = x1*y2 - y1*x2;
    return atan2(det, dot)*RadToDeg + 180;
}

void normalize(qreal *x_t, qreal *y_t, qreal dest_len) {
    qreal x_val = *x_t;
    qreal y_val = *y_t;
    qreal curr_len = sqrt(x_val*x_val + y_val*y_val);
    *x_t = x_val*dest_len/curr_len;
    *y_t = y_val*dest_len/curr_len;
}



qreal qMin4(qreal v1, qreal v2, qreal v3, qreal v4) {
    return qMin(v1, qMin(v2, qMin(v3, v4) ) );
}

qreal qMax4(qreal v1, qreal v2, qreal v3, qreal v4) {
    return qMax(v1, qMax(v2, qMax(v3, v4) ) );
}

QRectF qRectF4Points(QPointF p1, QPointF c1,
                     QPointF c2, QPointF p2) {
    return QRectF(QPointF(qMin4(p1.x(), c1.x(), c2.x(), p2.x()),
                          qMin4(p1.y(), c1.y(), c2.y(), p2.y())),
                  QPointF(qMax4(p1.x(), c1.x(), c2.x(), p2.x()),
                          qMax4(p1.y(), c1.y(), c2.y(), p2.y())));
}

QPointF rotateVector90Degrees(const QPointF &pt) {
    return QPointF(-pt.y(), pt.x()); // y is downwards
}

qreal degreesBetweenVectors(const QPointF &pt1,
                            const QPointF &pt2) {
    return radiansBetweenVectors(pt1, pt2)*180./M_PI;
}

qreal radiansBetweenVectors(const QPointF &pt1,
                            const QPointF &pt2) {
    qreal dot = pt1.x()*pt2.x() + pt1.y()*pt2.y();
    qreal det = pt1.x()*pt2.y() - pt1.y()*pt2.x();
    return atan2(det, dot);
}

QPointF gRotPt(const QPointF &pt, const qreal deg) {
    return {pt.x() * cos(deg*PI/180) - pt.y() * sin(deg*PI/180),
            pt.x() * sin(deg*PI/180) + pt.y() * cos(deg*PI/180)};
}

QPointF gQPointFDisplace(const QPointF& pt, const qreal displ) {
    return QPointF(pt.x() + gRandF(-displ, displ),
                   pt.y() + gRandF(-displ, displ));
}

bool nearlyEqual(double lhs,
                 double rhs)
{
    constexpr double eps = 1e-6;
    return std::abs(lhs - rhs) <= eps;
}
