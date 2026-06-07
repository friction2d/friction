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

#ifndef QCUBICSEGMENT2D_H
#define QCUBICSEGMENT2D_H
#include "../skia/skqtconversions.h"
#include <QPointF>
#include <QPainterPath>
#include "../simplemath.h"
#include "../core_global.h"

typedef std::pair<qreal, qreal> qrealPair;
struct qCubicSegment1D;

struct CORE_EXPORT PosAndTan {
    QPointF fPos;
    QPointF fTan;
};

struct CORE_EXPORT PosAndT {
    qreal fT;
    QPointF fPos;
};

struct CORE_EXPORT qCubicSegment2D {
    typedef std::pair<qCubicSegment2D, qCubicSegment2D> Pair;
    qCubicSegment2D(const QPointF& p0, const QPointF& c1,
                    const QPointF& c2, const QPointF& p1) {
        mP0 = p0; mC1 = c1; mC2 = c2; mP3 = p1;
    }

    qCubicSegment2D(const QPointF& p) {
        mP0 = p; mC1 = p; mC2 = p; mP3 = p;
    }

    qCubicSegment2D() : qCubicSegment2D(QPointF(0, 0)) {}

    qCubicSegment2D(const qCubicSegment1D& xSeg,
                    const qCubicSegment1D& ySeg);

    static qCubicSegment2D sFromLine(const QLineF& line) {
        return sFromLine(line.p1(), line.p2());
    }

    static qCubicSegment2D sFromLine(const QPointF& p0, const QPointF& p1) {
        return qCubicSegment2D(p0, p0*(2./3) + p1*(1./3),
                               p0*(1./3) + p1*(2./3), p1);
    }

    static qCubicSegment2D sFromConic(const QPointF& p0, const QPointF& c,
                                      const QPointF& p1, const qreal weight) {
        qreal u = 4*weight/(3*(1 + weight));
        return qCubicSegment2D(p0, p0*(1 - u) + c*u, p1*(1 - u) + c*u, p1);
    }

    static qCubicSegment2D sFromQuad(const QPointF& p0, const QPointF& c,
                                     const QPointF& p1) {
        return qCubicSegment2D(p0, p0 + 2*(c - p0)/3, p1 + 2*(c - p1)/3, p1);
    }

    SkPath toSkPath() const;

    QRectF ptsBoundingRect() const {
        return qRectF4Points(mP0, mC1, mC2, mP3);
    }

    qCubicSegment1D xSeg() const;
    qCubicSegment1D ySeg() const;

    QPointF tanAtLength(const qreal len) const;
    QPointF tanAtT(const qreal t) const;

    PosAndTan posAndTanAtLength(const qreal len) const;
    PosAndTan posAndTanAtT(const qreal t) const;

    QPointF posAtT(const qreal t) const;
    QPointF posAtLength(const qreal len) const;
    qreal tAtPos(const QPointF& pos) const;

    qreal length() const;
    qreal tAtLength(const qreal len) const;
    qreal lengthAtT(qreal t) const;
    qreal lengthFracAtT(qreal t) const;

    Pair dividedAtT(qreal t) const;

    const QPointF &p0() const;
    const QPointF &c1() const;
    const QPointF &c2() const;
    const QPointF &p3() const;

    void setP0(const QPointF& p0);
    void setC1(const QPointF& c1);
    void setC2(const QPointF& c2);
    void setP3(const QPointF& p3);

    qreal tValueForPointClosestTo(const QPointF& p);
    PosAndT closestPosAndT(const QPointF& p);
    qreal minDistanceTo(const QPointF &p,
                        qreal * const pBestT = nullptr,
                        QPointF * const pBestPos = nullptr) const;
    qreal minDistanceTo(const QPointF &p,
                        const qreal minT,
                        const qreal maxT,
                        qreal * const pBestT = nullptr,
                        QPointF * const pBestPos = nullptr) const;

    void reverse();

    qCubicSegment2D rotated(const qreal deg) const;

    void transform(const QTransform& transform);

    void rotate(const qreal deg);

    void makePassThroughRel(const QPointF &relPos, const qreal t);

    qCubicSegment2D randomDisplaced(const qreal displ);

    void randomDisplace(const qreal displ);

    //! @brief -90 is y direction 0 is x direction
    qreal tFurthestInDirection(const qreal deg) const;

    qCubicSegment2D tFragment(qreal minT, qreal maxT) const;
    qCubicSegment2D lenFragment(const qreal minLen, const qreal maxLen) const;
    qCubicSegment2D lenFracFragment(const qreal minLenFrac,
                                    const qreal maxLenFrac) const;

    bool isLine() const;

    bool isNull() const;
private:
    qreal tAtLength(const qreal length, const qreal maxLenErr,
                    const qreal minT, const qreal maxT) const;

    void updateLength() const;
    mutable bool mLengthUpToDate = false;
    mutable qreal fLength;

    QPointF mP0;
    QPointF mC1;
    QPointF mC2;
    QPointF mP3;
};

#endif // QCUBICSEGMENT2D_H
