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

#include "qcubicsegment2d.h"
#include "qcubicsegment1d.h"
#include "simplemath.h"
#include "exceptions.h"
#include "skia/skqtconversions.h"

qCubicSegment2D::qCubicSegment2D(const qCubicSegment1D &xSeg,
                                 const qCubicSegment1D &ySeg) {
    mP0 = QPointF(xSeg.p0(), ySeg.p0());
    mC1 = QPointF(xSeg.c1(), ySeg.c1());
    mC2 = QPointF(xSeg.c2(), ySeg.c2());
    mP3 = QPointF(xSeg.c1(), ySeg.c1());
}

SkPath qCubicSegment2D::toSkPath() const {
    SkPath path;
    path.moveTo(toSkPoint(mP0));
    path.cubicTo(toSkPoint(mC1), toSkPoint(mC2), toSkPoint(mP3));
    return path;
}

qCubicSegment1D qCubicSegment2D::xSeg() const {
    return qCubicSegment1D(mP0.x(), mC1.x(), mC2.x(), mP3.x());
}

qCubicSegment1D qCubicSegment2D::ySeg() const {
    return qCubicSegment1D(mP0.y(), mC1.y(), mC2.y(), mP3.y());
}

QPointF qCubicSegment2D::tanAtLength(const qreal len) const {
    return tanAtT(tAtLength(len));
}

QPointF qCubicSegment2D::tanAtT(const qreal t) const {
    const qreal oneMinusT = 1 - t;
    return 3 * oneMinusT * oneMinusT * (mC1 - mP0) +
           6 * t * oneMinusT * (mC2 - mC1) +
           3 * t * t * (mP3 - mC2);
}

PosAndTan qCubicSegment2D::posAndTanAtLength(const qreal len) const {
    return posAndTanAtT(tAtLength(len));
}

PosAndTan qCubicSegment2D::posAndTanAtT(const qreal t) const {
    return {posAtT(t), tanAtT(t)};
}

QPointF qCubicSegment2D::posAtLength(const qreal len) const {
    return posAtT(tAtLength(len));
}

QPointF qCubicSegment2D::posAtT(const qreal t) const {
    const qreal oneMinusT = 1 - t;
    return qPow(oneMinusT, 3)*p0() +
            3*qPow(oneMinusT, 2)*t*c1() +
            3*oneMinusT*t*t*c2() +
            t*t*t*p3();
}

qreal qCubicSegment2D::tAtPos(const QPointF &pos) const {
    qreal t;
    minDistanceTo(pos, &t);
    return t;
}

qreal qCubicSegment2D::length() const {
    if(!mLengthUpToDate) updateLength();
    return fLength;
}

qreal qCubicSegment2D::tAtLength(const qreal len) const {
    if(isZero6Dec(len) || len < 0) return 0;
    const qreal totLen = length();
    if(isZero6Dec(len - totLen) || len > totLen) return 1;
    return tAtLength(len, 0.01, 0, 1);
}

qreal qCubicSegment2D::lengthAtT(qreal t) const {
    t = CLAMP(t, 0, 1);
    if(isZero6Dec(qMax(0., t))) return 0;
    if(isZero6Dec(qMin(1., t) - 1)) return length();
    auto divSeg = dividedAtT(t);
    return divSeg.first.length();
}

qreal qCubicSegment2D::lengthFracAtT(qreal t) const {
    t = CLAMP(t, 0, 1);
    if(isZero6Dec(t)) return 0;
    if(isZero6Dec(t - 1)) return length();
    const qreal totLen = length();
    if(isZero6Dec(totLen)) return 1;
    return lengthAtT(t)/totLen;
}

qCubicSegment2D::Pair qCubicSegment2D::dividedAtT(qreal t) const {
    t = CLAMP(t, 0, 1);
    if(isZero6Dec(t)) return { qCubicSegment2D(mP0), *this };
    if(isOne6Dec(t)) return { *this, qCubicSegment2D(mP3) };
    const qreal oneMinusT = 1 - t;
    const QPointF P0_1 = p0()*oneMinusT + c1()*t;
    const QPointF P1_2 = c1()*oneMinusT + c2()*t;
    const QPointF P2_3 = c2()*oneMinusT + p3()*t;

    const QPointF P01_12 = P0_1*oneMinusT + P1_2*t;
    const QPointF P12_23 = P1_2*oneMinusT + P2_3*t;

    const QPointF P0112_1223 = P01_12*oneMinusT + P12_23*t;

    const qCubicSegment2D seg1(p0(), P0_1, P01_12, P0112_1223);
    const qCubicSegment2D seg2(P0112_1223, P12_23, P2_3, p3());

    return {seg1, seg2};
}

const QPointF &qCubicSegment2D::p0() const { return mP0; }

const QPointF &qCubicSegment2D::c1() const { return mC1; }

const QPointF &qCubicSegment2D::c2() const { return mC2; }

const QPointF &qCubicSegment2D::p3() const { return mP3; }

void qCubicSegment2D::setP0(const QPointF &p0) {
    mP0 = p0;
    mLengthUpToDate = false;
}

void qCubicSegment2D::setC1(const QPointF &c1) {
    mC1 = c1;
    mLengthUpToDate = false;
}

void qCubicSegment2D::setC2(const QPointF &c2) {
    mC2 = c2;
    mLengthUpToDate = false;
}

void qCubicSegment2D::setP3(const QPointF &p1) {
    mP3 = p1;
    mLengthUpToDate = false;
}

qreal qCubicSegment2D::tValueForPointClosestTo(const QPointF &p) {
    qreal bestT;
    minDistanceTo(p, 0, 1, &bestT);
    return bestT;
}

PosAndT qCubicSegment2D::closestPosAndT(const QPointF &p) {
    qreal bestT;
    QPointF bestPos;
    minDistanceTo(p, 0, 1, &bestT, &bestPos);
    return {bestT, bestPos};
}

qreal qCubicSegment2D::minDistanceTo(const QPointF &p,
                                     qreal * const pBestT,
                                     QPointF * const pBestPos) const {
    return minDistanceTo(p, 0, 1, pBestT, pBestPos);
}

qreal qCubicSegment2D::minDistanceTo(const QPointF &p,
                                     const qreal minT,
                                     const qreal maxT,
                                     qreal * const pBestT,
                                     QPointF * const pBestPos) const {
    const qreal maxLen = lengthAtT(maxT);
    qreal bestT = 0;
    QPointF bestPt = p0();
    qreal minError = DBL_MAX;
    bool last = false;
    qreal len = lengthAtT(minT);
    while(!last) { // t ? [0., 1.]
        if(len > maxLen) {
            len = maxLen;
            last = true;
        }
        qreal t = tAtLength(len);
        QPointF pt = posAtT(t);
        qreal dist = pointToLen(pt - p);
        if(dist < minError) {
            bestT = t;
            bestPt = pt;
            minError = dist;
            if(minError < 1) {
                const qreal& p0x = p0().x();
                const qreal& p0y = p0().y();
                const qreal& c1x = c1().x();
                const qreal& c1y = c1().y();
                const qreal& c2x = c2().x();
                const qreal& c2y = c2().y();
                const qreal& p3x = p3().x();
                const qreal& p3y = p3().y();
                while(dist < minError) {
                    bestT = t;
                    bestPt = pt;
                    minError = dist;
                    const qreal tMinusOne = t - 1;
                    const qreal pow2TMinusOne = pow2(tMinusOne);
                    const qreal pow3TMinusOne = pow3(tMinusOne);

                    const qreal v0 = 3*c2x - 3*c2x*t + p3x*t;
                    const qreal v1 = 3*c2y - 3*c2y*t + p3y*t;

                    const qreal v2 = 3*c1x*pow2TMinusOne + t*v0;
                    const qreal v3 = t*(3*c1y*pow2TMinusOne + t*v1);

                    const qreal v4 = -1 + 4*t - 3*pow2(t);

                    const qreal num = pow2(p.x() + p0x*pow3TMinusOne - t*v2) +
                                      pow2(p.y() + p0y*pow3TMinusOne - v3);
                    const qreal den = -6*(p0x*pow2TMinusOne + t*(-2*c2x + 3*c2x*t - p3x*t) +
                                    c1x*v4)*
                                  (-p.x() - p0x*pow3TMinusOne +  t*v2) -
                                 6*(p0y*pow2TMinusOne + t*(-2*c2y + 3*c2y*t - p3y*t) +
                                    c1y*v4)*
                                  (-p.y() - p0y*pow3TMinusOne + v3);
                    if(isZero6Dec(den)) {
//                        if(first && minError > 0.01 && !isZeroOrOne6Dec(t)) {
//                            t = gCubicTimeAtLength(seg, len + dist*0.5);
//                            pt = gCalcCubicBezierVal(seg, t);
//                            dist = pointToLen(pt - p);
//                            continue;
//                        }
                        break;
                    }
                    const qreal newT = t - num/den;
                    pt = posAtT(newT);
                    dist = pointToLen(pt - p);
                    t = newT;
                }
            }
            if(minError < 0.01) break;
        }

        len += dist*0.8;
    }
    if(pBestPos) *pBestPos = bestPt;
    if(pBestT) *pBestT = bestT;
    return minError;
}

void qCubicSegment2D::reverse() {
    std::swap(mP0, mP3);
    std::swap(mC1, mC2);
}

qCubicSegment2D qCubicSegment2D::rotated(const qreal deg) const {
    qCubicSegment2D result(*this);
    result.rotate(deg);
    return result;
}

void qCubicSegment2D::transform(const QTransform& transform) {
    mP0 = transform.map(p0());
    mC1 = transform.map(c1());
    mC2 = transform.map(c2());
    mP3 = transform.map(p3());
    mLengthUpToDate = false;
}

void qCubicSegment2D::rotate(const qreal deg) {
    if(isZero6Dec(deg)) return;
    mP0 = gRotPt(p0(), deg);
    mC1 = gRotPt(c1(), deg);
    mC2 = gRotPt(c2(), deg);
    mP3 = gRotPt(p3(), deg);
    mLengthUpToDate = false;
}

void qCubicSegment2D::makePassThroughRel(const QPointF& relPos, const qreal t) {
    if(t < 0.001 || t > 0.999) return;
    const qreal oneMinusT = 1 - t;
    QPointF dPos = relPos - posAtT(t);
    for(int i = 0; i < 100; i++) {
        setC1(c1() + oneMinusT*dPos);
        setC2(c2() + t*dPos);
        dPos = relPos - posAtT(t);
        if(pointToLen(dPos) < 1) break;
    }
}

qCubicSegment2D qCubicSegment2D::randomDisplaced(const qreal displ) {
    qCubicSegment2D result(*this);
    result.randomDisplace(displ);
    return result;
}

void qCubicSegment2D::randomDisplace(const qreal displ) {
    setP0(gQPointFDisplace(p0(), displ));
    setC1(gQPointFDisplace(c1(), displ));
    setC2(gQPointFDisplace(c2(), displ));
    setP3(gQPointFDisplace(p3(), displ));
    mLengthUpToDate = false;
}

qreal qCubicSegment2D::tFurthestInDirection(const qreal deg) const {
    auto rotSeg = rotated(deg);
    return rotSeg.xSeg().tWithBiggestValue();
}

qCubicSegment2D qCubicSegment2D::tFragment(qreal minT, qreal maxT) const {
    maxT = CLAMP01(maxT);
    minT = CLAMP01(minT);
    if(isZero6Dec(minT - maxT)) return qCubicSegment2D(posAtT(minT));
    if(minT > maxT)
        RuntimeThrow("Wrong t range. Min value larger than max.");
    if(isZero6Dec(minT - 1)) return qCubicSegment2D(mP3);
    qCubicSegment2D div1 = dividedAtT(minT).second;
    if(isZero6Dec(maxT - 1)) return div1;
    qreal mappedMaxT = (maxT - minT)/(1 - minT);
    return div1.dividedAtT(mappedMaxT).first;
}

qCubicSegment2D qCubicSegment2D::lenFragment(const qreal minLen,
                                             const qreal maxLen) const {
    return tFragment(tAtLength(minLen), tAtLength(maxLen));
}

qCubicSegment2D qCubicSegment2D::lenFracFragment(
        const qreal minLenFrac, const qreal maxLenFrac) const {
    return lenFragment(minLenFrac*length(), maxLenFrac*length());
}

bool qCubicSegment2D::isLine() const {
    const qreal arr1 = mP0.x()*(mC1.y() - mC2.y()) +
                       mC1.x()*(mC2.y() - mP0.y()) +
                       mC2.x()*(mP0.y() - mC1.y());
    if(!isZero2Dec(arr1)) return false;
    const qreal arr2 = mP3.x()*(mC1.y() - mC2.y()) +
                       mC1.x()*(mC2.y() - mP3.y()) +
                       mC2.x()*(mP3.y() - mC1.y());
    if(!isZero2Dec(arr2)) return false;
    return true;
}

bool qCubicSegment2D::isNull() const {
    if(!isZero2Dec(pointToLen(mP0 - mP3))) return false;
    if(!isZero2Dec(pointToLen(mP0 - mC1))) return false;
    if(!isZero2Dec(pointToLen(mP0 - mC2))) return false;
    return true;
}

qreal qCubicSegment2D::tAtLength(const qreal length,
                                 const qreal maxLenErr,
                                 const qreal minT,
                                 const qreal maxT) const {
    qreal guessT = (maxT + minT)*0.5;
    qreal lenAtGuess = lengthAtT(guessT);
    if(abs(lenAtGuess - length) < maxLenErr) return guessT;
    if(lenAtGuess > length) {
        return tAtLength(length, maxLenErr, minT, guessT);
    } else {
        return tAtLength(length, maxLenErr, guessT, maxT);
    }
}

void qCubicSegment2D::updateLength() const {
    mLengthUpToDate = true;
    QPainterPath path;
    path.moveTo(p0());
    path.cubicTo(c1(), c2(), p3());
    fLength = path.length();
}
