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

#include "pointhelpers.h"
#include "exceptions.h"

#include <QtMath>
#include <complex>
#include <QDebug>
#include <QMatrix>
#include <QList>
typedef std::complex<double> cmplx;

qCubicSegment1D xSeg(const qCubicSegment2D &seg) {
    return {seg.p0().x(), seg.c1().x(), seg.c2().x(), seg.p3().x()};
}

qCubicSegment1D ySeg(const qCubicSegment2D &seg) {
    return {seg.p0().y(), seg.c1().y(), seg.c2().y(), seg.p3().y()};
}

QPointF symmetricToPos(const QPointF& toMirror,
                       const QPointF& mirrorCenter) {
    const QPointF posDist = toMirror - mirrorCenter;
    return mirrorCenter - posDist;
}

QPointF symmetricToPosNewLen(const QPointF& toMirror,
                             const QPointF& mirrorCenter,
                             const qreal newLen) {
    const QPointF posDist = toMirror - mirrorCenter;
    return mirrorCenter - scalePointToNewLen(posDist, newLen);
}

QPointF gCubicValueAtT(const qCubicSegment2D &seg, const qreal t) {
    return QPointF(gCubicValueAtT(xSeg(seg), t),
                   gCubicValueAtT(ySeg(seg), t));
}

qreal gCubicValueAtT(const qCubicSegment1D &seg, const qreal t) {
    return qPow(1 - t, 3)*seg.p0() +
            3*qPow(1 - t, 2)*t*seg.c1() +
            3*(1 - t)*t*t*seg.c2() +
            t*t*t*seg.p1();
}

qreal gSolveForP2(const qreal p0, const qreal p1,
                  const qreal p3, const qreal t,
                  const qreal value) {
    if(isZero4Dec(t*t - t)) RuntimeThrow("Cannot solve with t*t == t.");
    qreal tm1 = t - 1.;
    return -(p0*tm1*tm1*tm1 - 3.*p1*t*tm1*tm1 - p3*t*t*t + value)/(3*tm1*t*t);
}

qreal gSolveForP1(const qreal p0, const qreal p2,
                  const qreal p3, const qreal t,
                  const qreal value) {
    if(isZero4Dec(t*t - t)) RuntimeThrow("Cannot solve with t*t == t.");
    qreal tm1 = t - 1.;
    return (p0*tm1*tm1*tm1 + 3.*p2*t*t*tm1 - p3*t*t*t + value)/(3*tm1*tm1*t);
}

qCubicSegment1D::Pair gDividedAtX(const qCubicSegment1D& seg,
                                  const qreal x, qreal* t) {
    const qreal min = qMin4(seg.p0(), seg.c1(), seg.c2(), seg.p1());
    const qreal max = qMax4(seg.p0(), seg.c1(), seg.c2(), seg.p1());
    if(x < min || isZero4Dec(x - min)) {
        if(t) *t = 0;
        return {qCubicSegment1D(seg.p0()), seg};
    }
    if(x > max || isZero4Dec(x - max)) {
        if(t) *t = 1;
        return {seg, qCubicSegment1D(seg.p1())};
    }
    qreal tTmp;
    if(!t) t = &tTmp;
    *t = gTFromX(seg, x);
    return seg.dividedAtT(*t);
}

qreal gTFromX(const qCubicSegment1D &seg,
             const qreal x) {
    qreal minT = 0.;
    qreal maxT = 1.;
    qreal xGuess;
    qreal guessT;
    do {
        guessT = (maxT + minT)*0.5;
        xGuess = gCubicValueAtT(seg, guessT);
        if(xGuess > x) {
            maxT = guessT;
        } else {
            minT = guessT;
        }
    } while(qAbs(xGuess - x) > 0.0001);
    return guessT;
}

bool gIsSymmetric(const QPointF &startPos,
                  const QPointF &centerPos,
                  const QPointF &endPos,
                  const qreal threshold) {
    const auto sC0 = symmetricToPos(startPos, centerPos);
    return pointToLen(sC0 - endPos) < threshold;
}

bool gIsSmooth(const QPointF &startPos,
               const QPointF &centerPos,
               const QPointF &endPos,
               const qreal threshold) {
    const auto sC0 = symmetricToPosNewLen(startPos, centerPos,
                                          pointToLen(endPos - centerPos));
    return pointToLen(sC0 - endPos) < threshold;
}

void gGetCtrlsSymmetricPos(const QPointF& startPos,
                           const QPointF& centerPos,
                           const QPointF& endPos,
                           QPointF &newStartPos,
                           QPointF &newEndPos) {
    const QPointF symEndPos = symmetricToPos(endPos, centerPos);
    const qreal len1 = pointToLen(symEndPos - centerPos);
    const qreal len2 = pointToLen(startPos - centerPos);
    const qreal lenSum = len1 + len2;
    if(isZero4Dec(lenSum)) {
        newStartPos = centerPos;
        newEndPos = centerPos;
        return;
    }
    newStartPos = (symEndPos*len1 + startPos*len2)/lenSum;
    newEndPos = symmetricToPos(newStartPos, centerPos);
}

void gGetCtrlsSmoothPos(const QPointF& startPos,
                        const QPointF& centerPos,
                        const QPointF& endPos,
                        QPointF &newStartPos,
                        QPointF &newEndPos) {
    const QPointF symEndPos = symmetricToPos(endPos, centerPos);
    const qreal len1 = pointToLen(endPos - centerPos);
    const qreal len2 = pointToLen(startPos - centerPos);
    const qreal lenSum = len1 + len2;
    if(isZero4Dec(lenSum)) {
        newStartPos = centerPos;
        newEndPos = centerPos;
        return;
    }
    const QPointF point1Rel = endPos - centerPos;
    const QPointF point2Rel = startPos - centerPos;
    const QPointF newStartDirection =
            scalePointToNewLen(
                (symEndPos*len1 + startPos*len2)/lenSum - centerPos, 1);
    const qreal startCtrlPtLen =
            qAbs(QPointF::dotProduct(point2Rel, newStartDirection));
    newStartPos = newStartDirection*startCtrlPtLen + centerPos;
    const qreal endCtrlPtLen =
            qAbs(QPointF::dotProduct(point1Rel, newStartDirection));
    newEndPos = -newStartDirection*endCtrlPtLen + centerPos;
}

void solveClosestToSegment(const qCubicSegment1D &seg,
                           const qreal vn,
                           QList<qreal> *list) {
    const cmplx v0 = cmplx(seg.p0(), 0.);
    const cmplx v1 = cmplx(seg.c1(), 0.);
    const cmplx v2 = cmplx(seg.c2(), 0.);
    const cmplx v3 = cmplx(seg.p1(), 0.);
    const cmplx v = cmplx(vn, 0.);
    const cmplx var1 = sqrt(pow(v1, 2.) + pow(v2, 2.) - v1*(v2 + v3) -
                                 v0*(v3 - v2));
    const cmplx var2 = v0 - 3.*v1 + 3.*v2 - v3;
    const cmplx var3 = v0 - 2.*v1 + v2;
    const cmplx sol1 = (var1 - var3)/-var2;
    const cmplx sol2 = (var1 + var3)/var2;

    const cmplx var4 =
            pow(-2.*pow(v1,3.) + 3.*v0*v1*v2 + 3.*pow(v1,2.)*v2 -
              6.*v0*pow(v2,2.) + 3.*v1*pow(v2,2.) - 2.*pow(v2,3.) +
              v*pow(v0 - 3.*v1 + 3.*v2 - v3,2.) - pow(v0,2.)*v3 + 3.*v0*v1*v3 -
              6.*pow(v1,2.)*v3 + 3.*v0*v2*v3 + 3.*v1*v2*v3 - v0*pow(v3,2.) +
              sqrt(pow(v0 - 3.*v1 + 3.*v2 - v3,2.)*
                (-3.*pow(v1,2.)*pow(v2,2.) + 4.*v0*pow(v2,3.) +
                  pow(v,2.)*pow(v0 - 3.*v1 + 3.*v2 - v3,2.) + 4.*pow(v1,3.)*v3 -
                  6.*v0*v1*v2*v3 + pow(v0,2.)*pow(v3,2.) -
                  2.*v*(2.*pow(v1,3.) + 2.*pow(v2,3.) -
                     3.*pow(v1,2.)*(v2 - 2.*v3) + pow(v0,2.)*v3 -
                     3.*v1*(v0 + v2)*(v2 + v3) +
                     v0*(6.*pow(v2,2.) - 3.*v2*v3 + pow(v3,2.))))),
             1./3.);

    const cmplx sol3 = (-2.*(v0 - 2.*v1 + v2) + (2.*v2to1div3*
                    (pow(v1,2.) + pow(v2,2.) + v0*(-v2 + v3) - v1*(v2 + v3)))/
                  var4 + v2to2div3*
                  var4)/(2.*(-v0 + 3.*v1 - 3.*v2 + v3));
    const cmplx sol4 =
            (-36.*(v0 - 2.*v1 + v2) - (cmplx(0.,18.)*v2to1div3*
                     (cmplx(0.,-1.) + sqrt(3.))*
                     (pow(v1,2.) + pow(v2,2.) + v0*(-v2 + v3) - v1*(v2 + v3)))/
                   var4 + cmplx(0,9)*v2to2div3*
                   (cmplx(0.,1.) + sqrt(3.))*
                   var4)/(36.*(-v0 + 3.*v1 - 3.*v2 + v3));

    const cmplx sol5 =
            (-36.*(v0 - 2.*v1 + v2) + (cmplx(0.,18.)*v2to1div3*
           (cmplx(0.,1.) + sqrt(3.))*
           (pow(v1,2.) + pow(v2,2.) + v0*(-v2 + v3) - v1*(v2 + v3)))/
         var4 - 9.*v2to2div3*
         (1. + cmplx(0.,1.)*sqrt(3.))*var4)/
            (36.*(-v0 + 3.*v1 - 3.*v2 + v3));
    if(sol1.real() > 0 && sol1.real() < 1) {
        list->append(sol1.real());
    }
    if(sol2.real() > 0 && sol2.real() < 1) {
        list->append(sol2.real());
    }
    if(sol3.real() > 0 && sol3.real() < 1) {
        list->append(sol3.real());
    }
    if(sol4.real() > 0 && sol4.real() < 1) {
        list->append(sol4.real());
    }
    if(sol5.real() > 0 && sol5.real() < 1) {
        list->append(sol5.real());
    }
    list->append(0);
    list->append(1);
}

void solveOnSegmnet(const qCubicSegment1D &seg,
                    const qreal xn,
                    QList<qreal> *list) {
    const cmplx i = cmplx(0., 1.);
    const cmplx x0 = cmplx(seg.p0(), 0.);
    const cmplx x1 = cmplx(seg.c1(), 0.);
    const cmplx x2 = cmplx(seg.c2(), 0.);
    const cmplx x3 = cmplx(seg.p1(), 0.);
    const cmplx x = cmplx(xn, 0.);
    const cmplx x0sq = pow(x0, 2.);
    const cmplx x1sq = pow(x1, 2.);
    const cmplx x2sq = pow(x2, 2.);
    const cmplx x3sq = pow(x3, 2.);
    const cmplx xsq = pow(x, 2.);
    const cmplx x1th = pow(x1, 3.);
    const cmplx x2th = pow(x2, 3.);
    const cmplx _3x2x3 = 3.*x2*x3;
    const cmplx _x0x1 = x0*x1;
    const cmplx var1 = pow(x0 - 3.*x1 + 3.*x2 - x3, 2.);
    const cmplx var2 = sqrt(var1*(-3.*x1sq*x2sq + 4.*x0*x2th + xsq*var1 +
                                4.*x1th*x3 - 2.*_x0x1*_3x2x3 + x0sq*x3sq -
                                2.*x*(2.*x1th + 2.*x2th - 3.*x1sq*(x2 - 2.*x3) +
                                x0sq*x3 - 3.*x1*(x0 + x2)*(x2 + x3) +
                                x0*(6.*x2sq - _3x2x3 + x3sq))) );
    const cmplx var3 = pow(-2.*x1th + 3.*_x0x1*x2 + 3.*x1sq*x2 -
                                6.*x0*x2sq + 3.*x1*x2sq - 2.*x2th + x*var1 -
                                x0sq*x3 + 3.*_x0x1*x3 - 6.*x1sq*x3 + x0*_3x2x3 +
                                x1*_3x2x3 - x0*x3sq + var2, 1./3.);
    const cmplx var4 = x1sq + x2sq + x0*(-x2 + x3) - x1*(x2 + x3);
    const cmplx var5 = -x0 + 3.*x1 - 3.*x2 + x3;
    const cmplx var6 = x0 - 2.*x1 + x2;
    const cmplx t1 = (-var6 + (v2to1div3*var4)/var3 + v2to2div3*var3*0.5) / var5;
    const cmplx t2 = (-4.*var6 - (2.*i*v2to1div3*(-i + sqrt3)*var4)/var3 +
                 i*v2to2div3*(i + sqrt3)*var3) / (4.*var5);
    const cmplx t3 = (-4.*var6 + (2.*i*v2to1div3*(i + sqrt3)*var4)/var3 -
                 v2to2div3*(1. + i*sqrt3)*var3 ) / (4.*var5);
    if(t1.real() > 0. && t1.real() < 1.) {
        list->append(t1.real());
    }
    if(t2.real() > 0. && t2.real() < 1.) {
        list->append(t2.real());
    }
    if(t3.real() > 0. && t3.real() < 1.) {
        list->append(t3.real());
    }
    list->append(0.);
    list->append(1.);
}

qreal gGetClosestTValueOnBezier(const qCubicSegment1D &seg,
                               const qreal p,
                               qreal *bestPosPtr,
                               qreal *errorPtr) {
    QList<qreal> values;

    solveClosestToSegment(seg, p, &values);
    qreal bestT = 0.;
    qreal bestPos = seg.p0();;
    qreal minErrorT = DBL_MAX;
    for(const qreal &val : qAsConst(values)) {
        qreal posT = gCubicValueAtT(seg, val);
        qreal errorT = qAbs(posT - p);
        if(errorT < minErrorT) {
            bestPos = posT;
            minErrorT = errorT;
            bestT = val;
        }
    }
    if(bestPosPtr) {
        *bestPosPtr = bestPos;
    }
    if(errorPtr) {
        *errorPtr = minErrorT;
    }
    return bestT;
}

//void VectorPathAnimator::bezierLeastSquareV1V2(const qreal v0,
//                                               qreal &v1, qreal &v2,
//                                               const qreal v3,
//                                               const QList<qreal> &vs,
//                                               const QList<qreal> &ts) {
//    qreal v1Inc = 0.;
//    qreal v1Dec = 0.;
//    qreal v2Inc = 0.;
//    qreal v2Dec = 0.;
//    for(int i = 0; i < vs.count(); i++) {
//        const qreal& val = vs.at(i);
//        const qreal& t = ts.at(i);
//        v1Inc += pow(1. - t, 2.)*t*(-v0*pow(1. - t, 3.) - 3.*v2*(1. - t)*pow(t, 2.) -
//                 v3*pow(t, 3.) + val);
//        v1Dec += (1. - t)*pow(t, 2.)*(3.*t*pow(1. - t, 2.));
//        v2Inc += (1. - t)*pow(t, 2.)*(-v0*pow(1. - t, 3.) - 3.*v1*pow(1. - t, 2.)*t -
//                 v3*pow(t, 3.) + val);
//        v2Dec += (1. - t)*pow(t, 2.)*(3.*(1. - t)*pow(t, 2.));
//    }
//    v1 = v1Inc/v1Dec;
//    v2 = v2Inc/v2Dec;
//}

qCubicSegment2D gBezierLeastSquareV1V2(const qCubicSegment2D &seg,
                                       const QList<QPointF> &vs,
                                       const QList<qreal> &ts) {
    qreal v1XInc = 0.;
    qreal v1Dec = 0.;
    qreal v2XInc = 0.;
    qreal v2Dec = 0.;
    qreal v1YInc = 0.;
    qreal v2YInc = 0.;
    for(int i = 0; i < vs.count(); i++) {
        const QPointF &val = vs.at(i);
        const qreal& t = ts.at(i);
        v1XInc += pow(1. - t, 2.)*t*(-seg.p0().x()*pow(1. - t, 3.) - 3.*seg.c2().x()*(1. - t)*pow(t, 2.) -
                 seg.p3().x()*pow(t, 3.) + val.x());
        v2XInc += (1. - t)*pow(t, 2.)*(-seg.p0().x()*pow(1. - t, 3.) - 3.*seg.c1().x()*pow(1. - t, 2.)*t -
                 seg.p3().x()*pow(t, 3.) + val.x());
        v1YInc += pow(1. - t, 2.)*t*(-seg.p0().y()*pow(1. - t, 3.) - 3.*seg.c2().y()*(1. - t)*pow(t, 2.) -
                 seg.p3().y()*pow(t, 3.) + val.y());
        v2YInc += (1. - t)*pow(t, 2.)*(-seg.p0().y()*pow(1. - t, 3.) - 3.*seg.c1().y()*pow(1. - t, 2.)*t -
                 seg.p3().y()*pow(t, 3.) + val.y());
        v1Dec = (1. - t)*pow(t, 2.)*(3.*t*pow(1. - t, 2.));
        v2Dec = (1. - t)*pow(t, 2.)*(3.*(1. - t)*pow(t, 2.));
    }
    return qCubicSegment2D(seg.p0(), QPointF(v1XInc/v1Dec, v1YInc/v1Dec),
                           QPointF(v2XInc/v2Dec, v2YInc/v2Dec), seg.p3());
}

qCubicSegment2D gBezierLeastSquareV1V2(
        const qCubicSegment2D &seg,
        const QList<QPointF> &vs,
        const int minVs,
        const int maxVs) {
    QPointF v1, v2;
    for(int j = 0; j < 50; j++) {
        qreal v1XInc = 0.;
        qreal v1Dec = 0.;
        qreal v2XInc = 0.;
        qreal v2Dec = 0.;
        qreal v1YInc = 0.;
        qreal v2YInc = 0.;
        for(int i = minVs; i <= maxVs; i++) {
            const QPointF &val = vs.at(i);
            qreal t = (static_cast<qreal>(i) - minVs)/(maxVs - minVs);
            //qreal t = getClosestTValueBezier2D(seg.p0, seg.p1, seg.p2, seg.p3, val);
            v1XInc += pow(1. - t, 2.)*t*(-seg.p0().x()*pow(1. - t, 3.) - 3.*seg.c2().x()*(1. - t)*pow(t, 2.) -
                     seg.p3().x()*pow(t, 3.) + val.x());
            v2XInc += (1. - t)*pow(t, 2.)*(-seg.p0().x()*pow(1. - t, 3.) - 3.*seg.c1().x()*pow(1. - t, 2.)*t -
                     seg.p3().x()*pow(t, 3.) + val.x());
            v1YInc += pow(1. - t, 2.)*t*(-seg.p0().y()*pow(1. - t, 3.) - 3.*seg.c2().y()*(1. - t)*pow(t, 2.) -
                     seg.p3().y()*pow(t, 3.) + val.y());
            v2YInc += (1. - t)*pow(t, 2.)*(-seg.p0().y()*pow(1. - t, 3.) - 3.*seg.c1().y()*pow(1. - t, 2.)*t -
                     seg.p3().y()*pow(t, 3.) + val.y());
            v1Dec += pow(1. - t, 2.)*t*(3.*t*pow(1. - t, 2.));
            v2Dec += (1. - t)*pow(t, 2.)*(3.*(1. - t)*pow(t, 2.));
        }
        v1 = QPointF(v1XInc/v1Dec, v1YInc/v1Dec);
        v2 = QPointF(v2XInc/v2Dec, v2YInc/v2Dec);
    }
    return {seg.p0(), v1, v2, seg.p3()};
}

qreal get1DAccuracyValue(const qreal x0,
                         const qreal x1,
                         const qreal x2,
                         const qreal x3) {
    return qMax4(x0, x1, x2, x3) - qMin4(x0, x1, x2, x3);
}

QPointF gGetClosestPointOnLineSegment(const QPointF &a,
                                      const QPointF &b,
                                      const QPointF &p) {
    QPointF AP(p - a); //Vector from A to P
    QPointF AB(b - a); //Vector from A to B

    qreal magnitudeAB = pow2(AB.x()) + pow2(AB.y()); //Magnitude of AB vector (it's length squared)
    qreal ABAPproduct = AP.x()*AB.x() + AP.y()*AB.y(); //The DOT product of a_to_p and a_to_b
    qreal distance = ABAPproduct / magnitudeAB; //The normalized "distance" from a to your closest point

    if(distance < 0) { //Check if P projection is over vectorAB
        return a;
    } else if(distance > 1) {
        return b;
    } else {
        return a + (AB * distance);
    }
}

QPointF gClosestPointOnRect(const QRectF &rect,
                           const QPointF &point,
                           qreal *dist) {
    qreal minDist = DBL_MAX;
    QPointF bestPos;
    if(point.y() > rect.bottom()) {
        // check bottom
        QPointF pt = gGetClosestPointOnLineSegment(rect.bottomLeft(),
                                                  rect.bottomRight(),
                                                  point);
        const qreal dist = pointToLen(pt - point);
        if(dist < minDist) {
            minDist = dist;
            bestPos = pt;
        }
    } else if(point.y() < rect.top()) {
        // check top
        QPointF pt = gGetClosestPointOnLineSegment(rect.topLeft(),
                                                   rect.topRight(),
                                                   point);
        const qreal dist = pointToLen(pt - point);
        if(dist < minDist) {
            minDist = dist;
            bestPos = pt;
        }
    }

    if(point.x() > rect.right()) {
        // check right
        QPointF pt = gGetClosestPointOnLineSegment(rect.topRight(),
                                                   rect.bottomRight(),
                                                   point);
        const qreal dist = pointToLen(pt - point);
        if(dist < minDist) {
            minDist = dist;
            bestPos = pt;
        }
    } else if(point.y() < rect.left()) {
        // check left
        QPointF pt = gGetClosestPointOnLineSegment(rect.bottomLeft(),
                                                  rect.topLeft(),
                                                  point);
        const qreal dist = pointToLen(pt - point);
        if(dist < minDist) {
            minDist = dist;
            bestPos = pt;
        }
    }
    if(dist) *dist = minDist;
    return bestPos;
}

void gDrawCosmeticEllipse(QPainter *p,
                          const QPointF &absPos,
                          const qreal rX, const qreal rY) {
    const QTransform &transform = p->transform();
    p->drawEllipse(absPos,
                   rX/transform.m11(),
                   rY/transform.m22());
}

void gGetSmoothAbsCtrlsForPtBetween(
        const SkPoint &prevP,
        const SkPoint &currP,
        const SkPoint &nextP,
        SkPoint &c1, SkPoint &c2,
        const float smoothness) {
//    SkPoint sLastP = prevP - currP;
//    sLastP.setLength(1);
//    SkPoint sNextP = nextP - currP;
//    sNextP.setLength(1);
//    SkPoint vectP = (sLastP + sNextP)*0.5f;
//    vectP.set(vectP.y(), -vectP.x());
//    if(vectP.dot(prevP - currP) > 0) vectP.negate();

    const SkPoint vectP = (nextP - prevP)*0.5f*smoothness;

    const float prevDist = (currP - prevP).length()*0.5f;
    SkPoint prevVectP = vectP;
    prevVectP.negate();
    if(prevVectP.length() > prevDist) {
        prevVectP.setLength(prevDist);
    }
    c1 = currP + prevVectP;

    const float nextDist = (currP - nextP).length()*0.5f;
    SkPoint nextVectP = vectP;
    if(nextVectP.length() > nextDist) {
        nextVectP.setLength(nextDist);
    }

    c2 = currP + nextVectP;
}

void gGetMaxSmoothAbsCtrlsForPtBetween(
        const SkPoint &lastP,
        const SkPoint &currP,
        const SkPoint &nextP,
        SkPoint &c1, SkPoint &c2) {
    SkPoint sLastP = lastP - currP;
    sLastP.setLength(1);
    SkPoint sNextP = nextP - currP;
    sNextP.setLength(1);
    SkPoint vectP = (sLastP + sNextP)*0.5f;
    vectP.set(vectP.y(), -vectP.x());

    if(vectP.dot(lastP - currP) > 0) vectP.negate();

    const float nextDist = (currP - nextP).length()*0.5f;
    vectP.setLength(nextDist);

    c2 = currP + vectP;

    vectP.negate();
    const float lastDist = (currP - lastP).length()*0.5f;
    vectP.setLength(lastDist);
    c1 = currP + vectP;
}

void gGetMaxSmoothAbsCtrlsForPtBetween(
        const QPointF &lastP,
        const QPointF &currP,
        const QPointF &nextP,
        QPointF &c1, QPointF &c2) {
    QPointF sLastP = lastP - currP;
    sLastP = scalePointToNewLen(sLastP, 1);
    QPointF sNextP = nextP - currP;
    sNextP = scalePointToNewLen(sNextP, 1);
    QPointF vectP = (sLastP + sNextP)*0.5;
    vectP = QPointF(vectP.y(), -vectP.x());

    if(QPointF::dotProduct(vectP, lastP - currP) > 0) vectP = -vectP;

    const qreal nextDist = pointToLen(currP - nextP)*0.5;
    vectP = scalePointToNewLen(vectP, nextDist);

    c2 = currP + vectP;

    vectP = -vectP;
    const qreal lastDist = pointToLen(currP - lastP)*0.5;
    vectP = scalePointToNewLen(vectP, lastDist);
    c1 = currP + vectP;
}

template <typename T1, typename T2>
void smoothyAbsCtrlsForPtBetween(const T1 &lastP, const T1 &currP,
                                 const T1 &nextP, T1 &c1, T1 &c2,
                                 T2 smoothness) {
    smoothness = CLAMP(smoothness, -1, 1);

    if(smoothness > 0) {
        T1 maxC1; T1 maxC2;
        gGetMaxSmoothAbsCtrlsForPtBetween(lastP, currP, nextP, maxC1, maxC2);
        c1 = currP + (c1 - currP)*(1 - smoothness) + (maxC1 - currP)*smoothness;
        c2 = currP + (c2 - currP)*(1 - smoothness) + (maxC2 - currP)*smoothness;
    } else {
        const T2 smoothnessDec = smoothness + 1;
        c1 = currP + (c1 - currP)*smoothnessDec;
        c2 = currP + (c2 - currP)*smoothnessDec;
    }
}

void gSmoothyAbsCtrlsForPtBetween(const QPointF &lastP,
                                  const QPointF &currP,
                                  const QPointF &nextP,
                                  QPointF &c1, QPointF &c2,
                                  qreal smoothness) {
    smoothyAbsCtrlsForPtBetween(lastP, currP, nextP, c1, c2, smoothness);
}

void gSmoothyAbsCtrlsForPtBetween(const SkPoint &lastP,
                                  const SkPoint &currP,
                                  const SkPoint &nextP,
                                  SkPoint &c1, SkPoint &c2,
                                  float smoothness) {
    smoothyAbsCtrlsForPtBetween(lastP, currP, nextP, c1, c2, smoothness);
}

SkPath gPathToPolyline(const SkPath& path) {
    SkPath result;
    QPointF lastMovePos;
    QPointF lastPos;
    SkPath::Iter iter(path, false);
    for(;;) {
        qCubicSegment2D seg;
        SkPoint pts[4];
        switch(iter.next(pts)) {
        case SkPath::kLine_Verb: {
            const QPointF pt1 = toQPointF(pts[1]);
            result.lineTo(pts[1]);
            lastPos = pt1;
            continue;
        }
        case SkPath::kQuad_Verb: {
            const QPointF pt2 = toQPointF(pts[2]);
            seg = qCubicSegment2D::sFromQuad(lastPos, toQPointF(pts[1]), pt2);
            lastPos = pt2;
        } break;
        case SkPath::kConic_Verb: {
            const QPointF pt2 = toQPointF(pts[2]);
            seg = qCubicSegment2D::sFromConic(lastPos, toQPointF(pts[1]), pt2,
                                             toQreal(iter.conicWeight()));
            lastPos = pt2;
        } break;
        case SkPath::kCubic_Verb: {
            const QPointF pt3 = toQPointF(pts[3]);
            seg = qCubicSegment2D(lastPos, toQPointF(pts[1]),
                                  toQPointF(pts[2]), pt3);
            lastPos = pt3;
        } break;
        case SkPath::kClose_Verb: {
            result.close();
            continue;
        }
        case SkPath::kMove_Verb: {
            result.moveTo(pts[0]);
            lastMovePos = toQPointF(pts[0]);
            lastPos = lastMovePos;
            continue;
        }
        case SkPath::kDone_Verb:
            return result;
        }
        if(!seg.isLine()) {
            for(qreal len = 10; len < seg.length(); len += 10) {
                result.lineTo(toSkPoint(seg.posAtLength(len)));
            }
        }
        result.lineTo(toSkPoint(seg.p3()));
    }
}

void gForEverySegmentInPath(
        const SkPath& path,
        const std::function<void(const qCubicSegment2D&)>& func) {
    QPointF lastMovePos;
    QPointF lastPos;
    SkPath::Iter iter(path, false);
    for(;;) {
        SkPoint pts[4];
        switch(iter.next(pts)) {
        case SkPath::kLine_Verb: {
            const QPointF pt1 = toQPointF(pts[1]);
            func(qCubicSegment2D(lastPos, lastPos, pt1, pt1));
            lastPos = pt1;
        } break;
        case SkPath::kQuad_Verb: {
            const QPointF pt2 = toQPointF(pts[2]);
            func(qCubicSegment2D::sFromQuad(lastPos, toQPointF(pts[1]), pt2));
            lastPos = pt2;
        } break;
        case SkPath::kConic_Verb: {
            const QPointF pt2 = toQPointF(pts[2]);
            func(qCubicSegment2D::sFromConic(lastPos, toQPointF(pts[1]), pt2,
                                            toQreal(iter.conicWeight())));
            lastPos = pt2;
        } break;
        case SkPath::kCubic_Verb: {
            const QPointF pt3 = toQPointF(pts[3]);
            func(qCubicSegment2D(lastPos, toQPointF(pts[1]),
                                 toQPointF(pts[2]), pt3));
            lastPos = pt3;
        } break;
        case SkPath::kClose_Verb: {
            if(!isZero2Dec(pointToLen(lastPos - lastMovePos))) {
                func({lastPos, lastPos, lastMovePos, lastMovePos});
                lastPos = lastMovePos;
            }
        } break;
        case SkPath::kMove_Verb: {
            lastMovePos = toQPointF(pts[0]);
            lastPos = lastMovePos;
        } break;
        case SkPath::kDone_Verb:
            return;
        }
    }
}

void gForEverySegmentInPath(
        const SkPath& path,
        const std::function<void(const SkPath&)>& func) {
    SkPoint lastMovePos{0, 0};
    SkPoint lastPos{0, 0};
    SkPath::Iter iter(path, false);
    for(;;) {
        SkPoint pts[4];
        switch(iter.next(pts)) {
        case SkPath::kLine_Verb: {
            const SkPoint pt1 = pts[1];
            SkPath seg;
            seg.moveTo(lastPos);
            seg.lineTo(pt1);
            func(seg);
            lastPos = pt1;
        } break;
        case SkPath::kQuad_Verb: {
            const SkPoint pt2 = pts[2];
            SkPath seg;
            seg.moveTo(lastPos);
            seg.quadTo(pts[1], pt2);
            func(seg);
            lastPos = pt2;
        } break;
        case SkPath::kConic_Verb: {
            const SkPoint pt2 = pts[2];
            SkPath seg;
            seg.moveTo(lastPos);
            seg.conicTo(pts[1], pt2, iter.conicWeight());
            func(seg);
            lastPos = pt2;
        } break;
        case SkPath::kCubic_Verb: {
            const SkPoint pt3 = pts[3];
            SkPath seg;
            seg.moveTo(lastPos);
            seg.cubicTo(pts[1], pts[2], pt3);
            func(seg);
            lastPos = pt3;
        } break;
        case SkPath::kClose_Verb: {
            if(!isZero2Dec(pointToLen(lastPos - lastMovePos))) {
                SkPath seg;
                seg.moveTo(lastPos);
                seg.lineTo(lastMovePos);
                func(seg);
            }
        } break;
        case SkPath::kMove_Verb: {
            lastMovePos = pts[0];
            lastPos = lastMovePos;
        } break;
        case SkPath::kDone_Verb:
            return;
        }
    }
}

static void RotateCCW(const SkPoint& src, SkPoint* dst) {
    // use a tmp in case src == dst
    const SkScalar tmp = src.fX;
    dst->fX = src.fY;
    dst->fY = -tmp;
}

static void RotateCCW(SkPoint* pt) {
    RotateCCW(*pt, pt);
}

static void Perterb(SkPoint * const p,
                    const SkVector& tangent,
                    const float scale) {
    SkVector normal = tangent;
    RotateCCW(&normal);
    normal.setLength(scale);
    *p += normal;
}

//! @brief Return random in range [0, 1)
float randFloat01(const qreal baseSeed) {
    if(isInteger4Dec(baseSeed)) {
        QRandomGenerator rand(static_cast<quint32>(qRound(baseSeed)));
        return toSkScalar(rand.generateDouble());
    } else {
        QRandomGenerator fRand(static_cast<quint32>(qFloor(baseSeed)));
        QRandomGenerator cRand(static_cast<quint32>(qCeil(baseSeed)));
        const qreal cRandWeight = baseSeed - qFloor(baseSeed);
        return toSkScalar(fRand.generateDouble()*(1 - cRandWeight) +
                cRand.generateDouble()*cRandWeight);
    }
}

//! @brief Return random in range [-1, 1)
float randFloat(const qreal baseSeed) {
    return randFloat01(baseSeed)*2 - 1;
}

//! @brief Return random in range [min, max)
float randFloat(const qreal baseSeed,
                const float min, const float max) {
    return static_cast<float>(randFloat01(baseSeed))*(max - min) + min;
}

SkPoint randPt(const qreal baseSeed, const SkPoint& pt,
               const float min, const float max) {
    const auto xR = randFloat(baseSeed, min, max);
    const auto yR = randFloat(baseSeed, min, max);
    return {pt.x() + xR, pt.y() + yR};
}

SkPoint randPt(const qreal baseSeed,
               const SkPoint& pt, const float dev) {
    return randPt(baseSeed, pt, -dev, dev);
}

SkPoint randPt(const qreal baseSeed, const float dev) {
    const auto xR = randFloat(baseSeed, -dev, dev);
    const auto yR = randFloat(baseSeed, -dev, dev);
    return SkPoint::Make(xR, yR);
}

void gAtomicDisplaceFilterPath(const qreal baseSeed,
                               SkPath* const dst,
                               const SkPath& src,
                               const float maxDev) {
    if(isZero4Dec(maxDev)) {
        *dst = src;
        return;
    }
    dst->reset();
    dst->setFillType(src.getFillType());

    qreal seedContourInc = 0;
    SkPath::Iter iter(src, false);
    SkPath::Iter nextIter(src, false);
    SkPoint pts[4];
    nextIter.next(pts);
    SkPoint prevDisp{0, 0};
    SkPoint firstDisp{0, 0};
    int i = 0;
    for(;;) {
        const auto nextVerb = nextIter.next(pts);
        const auto verb = iter.next(pts);
        if(verb == SkPath::kMove_Verb) {
            seedContourInc += 1000.;
            i = 0;
        }
        const auto disp = nextVerb == SkPath::kClose_Verb ?
                    firstDisp : randPt(baseSeed + (i++) + seedContourInc, maxDev);

        switch(verb) {
        case SkPath::kLine_Verb: {
            dst->lineTo(pts[1] + disp);
        } break;
        case SkPath::kQuad_Verb: {
            dst->quadTo(pts[1] + (prevDisp + disp)*0.5f, pts[2] + disp);
        } break;
        case SkPath::kConic_Verb: {
            dst->conicTo(pts[1] + prevDisp, pts[2] + disp, iter.conicWeight());
        } break;
        case SkPath::kCubic_Verb: {
            dst->cubicTo(pts[1] + prevDisp, pts[2] + disp, pts[3] + disp);
        } break;
        case SkPath::kClose_Verb: {
            dst->close();
        } break;
        case SkPath::kMove_Verb: {
            dst->moveTo(pts[0] + disp);
            firstDisp = disp;
        } break;
        case SkPath::kDone_Verb:
            return;
        }
        prevDisp = disp;
    }
}

#include "randomgrid.h"

void gSpatialDisplaceFilterPath(const qreal baseSeed,
                                const qreal gridSize,
                                SkPath* const dst,
                                const SkPath& src,
                                const float maxDev) {
    if(isZero4Dec(maxDev)) {
        *dst = src;
        return;
    }
    dst->reset();
    dst->setFillType(src.getFillType());

    SkPath::Iter iter(src, false);
    SkPoint pts[4];
    SkPoint prevDisp{0, 0};
    for(;;) {
        const auto verb = iter.next(pts);
        SkPoint targetPt{0, 0};
        switch(verb) {
        case SkPath::kLine_Verb: {
           targetPt = pts[1];
        } break;
        case SkPath::kQuad_Verb:
        case SkPath::kConic_Verb: {
            targetPt = pts[2];
        } break;
        case SkPath::kCubic_Verb: {
            targetPt = pts[3];
        } break;
        case SkPath::kClose_Verb: {
        } break;
        case SkPath::kMove_Verb: {
            targetPt = pts[0];
        } break;
        case SkPath::kDone_Verb:
            return;
        }

        const float randX = toSkScalar(RandomGrid::sGetRandomValue(
                    -1, 1, baseSeed, gridSize, toQPointF(targetPt)));
        const float randY = toSkScalar(RandomGrid::sGetRandomValue(
                    -1, 1, baseSeed + 100, gridSize, toQPointF(targetPt)));

        const auto disp = SkPoint::Make(randX*maxDev, randY*maxDev);

        switch(verb) {
        case SkPath::kLine_Verb: {
            dst->lineTo(pts[1] + disp);
        } break;
        case SkPath::kQuad_Verb: {
            dst->quadTo(pts[1] + (prevDisp + disp)*0.5f, pts[2] + disp);
        } break;
        case SkPath::kConic_Verb: {
            dst->conicTo(pts[1] + prevDisp, pts[2] + disp, iter.conicWeight());
        } break;
        case SkPath::kCubic_Verb: {
            dst->cubicTo(pts[1] + prevDisp, pts[2] + disp, pts[3] + disp);
        } break;
        case SkPath::kClose_Verb: {
            dst->close();
        } break;
        case SkPath::kMove_Verb: {
            dst->moveTo(pts[0] + disp);
        } break;
        case SkPath::kDone_Verb:
            return;
        }
        prevDisp = disp;
    }
}

template <typename GenDispDataOp>
void displaceClosedPath(const qreal baseSeed,
                        SkPathMeasure &meas,
                        SkPath* const dst,
                        const float segLen,
                        const float maxDev,
                        const float smoothness,
                        GenDispDataOp genDispDataOp) {
    const bool zeroSmooth = smoothness < 0.001f;
    const float length = meas.getLength();

    const int nTot = SkScalarCeilToInt(length / segLen);
    if(nTot < 2) return;

    const float remLen = length - (nTot - 1)*segLen;
    const float endPtFrac = remLen/segLen;
    QVector<SkPoint> pts(nTot);
    QVector<SkPoint> cPts(zeroSmooth ? 1 : nTot*2);

    genDispDataOp(baseSeed, nTot, pts, cPts, meas,
                  segLen, maxDev, smoothness);

    if(!zeroSmooth) {
        const auto& prevPrevPt = pts[nTot - 2];
        const int lastId = nTot - 1;
        const auto& lastPt = pts[lastId];
        const auto& secondPt = pts[1];
        cPts[lastId*2 + 1] = lastPt;
        cPts[0] = lastPt;
        gGetSmoothAbsCtrlsForPtBetween(prevPrevPt, lastPt, secondPt,
                                       cPts[lastId*2], cPts[1], smoothness);
    }

    {
        const int beforeCurrLastId = nTot - 4 < 0 ? 2*nTot - 4 : nTot - 4;
        const int currLastId = nTot - 3 < 0 ? 2*nTot - 3 : nTot - 3;
        const int fragId = nTot - 2; // nTot !< 2
        const int lastId = nTot - 1;

        const SkPoint& currLast = pts[currLastId];
        const SkPoint& last = pts[lastId];

        SkPoint currLastCtrlAfter;
        SkPoint currLastCtrlBefore;
        if(zeroSmooth) {
            currLastCtrlAfter = currLast;
            currLastCtrlBefore = currLast;
        } else {
            gGetSmoothAbsCtrlsForPtBetween(
                        pts[beforeCurrLastId], currLast, last,
                        currLastCtrlBefore, currLastCtrlAfter, smoothness);
        }

        SkPoint lastCtrlBefore;
        SkPoint lastCtrlAfter;
        if(zeroSmooth) {
            lastCtrlBefore = last;
        } else {
            gGetSmoothAbsCtrlsForPtBetween(
                        currLast, last, pts[1],
                        lastCtrlBefore, lastCtrlAfter, smoothness);
        }

        const qCubicSegment2D seg(toQPointF(currLast),
                                  toQPointF(currLastCtrlAfter),
                                  toQPointF(lastCtrlBefore),
                                  toQPointF(last));
        const auto div = seg.dividedAtT(0.5 + 0.5*static_cast<double>(1 - endPtFrac));

        pts[fragId] = pts[fragId]*endPtFrac +
                toSkPoint(div.first.p3())*(1 - endPtFrac);
        if(!zeroSmooth) {
            cPts[currLastId*2] = (cPts[currLastId*2]*endPtFrac +
                    currLastCtrlBefore*(1 - endPtFrac))*smoothness +
                    pts[currLastId]*(1 - smoothness);
            cPts[currLastId*2 + 1] = (cPts[currLastId*2 + 1]*endPtFrac +
                    toSkPoint(div.first.c1())*(1 - endPtFrac))*smoothness +
                    pts[currLastId]*(1 - smoothness);

            cPts[fragId*2] = (cPts[fragId*2]*endPtFrac +
                    toSkPoint(div.first.c2())*(1 - endPtFrac))*smoothness +
                    pts[fragId]*(1 - smoothness);
            cPts[fragId*2 + 1] = (cPts[fragId*2 + 1]*endPtFrac +
                    toSkPoint(div.second.c1())*(1 - endPtFrac))*smoothness +
                    pts[fragId]*(1 - smoothness);

            cPts[lastId*2] = (cPts[lastId*2]*endPtFrac +
                    toSkPoint(div.second.c2())*(1 - endPtFrac))*smoothness +
                    pts[lastId]*(1 - smoothness);
            cPts[1] = (cPts[1]*endPtFrac +
                    lastCtrlAfter*(1 - endPtFrac))*smoothness +
                    pts[lastId]*(1 - smoothness);
        }
    }

    auto currIt = pts.begin();
    if(zeroSmooth) {
        for(int i = 0; i < nTot; i++, currIt++) {
            if(i == 0) dst->moveTo(pts[i]);
            else dst->lineTo(pts[i]);
        }
    } else {
        for(int i = 0; i < nTot; i++, currIt++) {
            if(i == 0) dst->moveTo(pts[i]);
            else dst->cubicTo(cPts[i*2 - 1], cPts[i*2], pts[i]);
        }
    }

    dst->close();
}

void genSpatialClosedDisplData(const qreal baseSeed,
                               const qreal gridSize,
                               const int nTot,
                               QVector<SkPoint>& pts,
                               QVector<SkPoint>& cPts,
                               SkPathMeasure &meas,
                               const float segLen,
                               const float maxDev,
                               const float smoothness) {
    const bool zeroSmooth = smoothness < 0.001f;
    float dist = 0;
    for(int i = 0; i < nTot; i++) {
        auto& pt = pts[i];
        if(i == nTot - 1) pt = pts[0];
        else {
            SkVector v;
            if(meas.getPosTan(dist += segLen, &pt, &v)) {
                const qreal rand = RandomGrid::sGetRandomValue(
                            -1, 1, baseSeed, gridSize, toQPointF(pt));
                Perterb(&pt, v, toSkScalar(rand) * maxDev);
            }
        }
        if(i > 1 && !zeroSmooth) {
            const auto& prevPrevPt = pts[i - 2];
            const int prevId = i - 1;
            const auto& prevPt = pts[prevId];
            const auto& nextPt = pt;
            gGetSmoothAbsCtrlsForPtBetween(prevPrevPt, prevPt, nextPt,
                                           cPts[prevId*2], cPts[prevId*2 + 1],
                                           smoothness);
        }
    }
}

void genSpatialOpenedDisplData(const qreal baseSeed,
                               const qreal gridSize,
                               const float halfLength,
                               const float endPtFrac,
                               const int nSide,
                               const int nTot,
                               QVector<SkPoint>& pts,
                               QVector<SkPoint>& cPts,
                               SkPathMeasure &meas,
                               const float segLen,
                               const float maxDev,
                               const float smoothness) {
     const bool zeroSmooth = smoothness < 0.001f;
     float rand1 = 0;
     float randNTotM2 = 0;
     const int middleId = nSide;
     for(int i = 0; i <= nSide; i++) {
         for(int j = -1; j < 2 && (i != 0 || j == -1); j += 2) {
             const int ptId = middleId + i*j;
             auto& pt = pts[ptId];
             SkVector v;
             if(meas.getPosTan(halfLength + j*i*segLen, &pt, &v)) {
                 const qreal qRand = RandomGrid::sGetRandomValue(
                             -1, 1, baseSeed, gridSize, toQPointF(pt));
                 float rand = toSkScalar(qRand);
                 if(ptId == 1) rand1 = rand;
                 else if(ptId == nTot - 2) randNTotM2 = rand;
                 else if(ptId == 0) rand = rand*endPtFrac + rand1*(1 - endPtFrac);
                 else if(ptId == nTot - 1) rand = rand*endPtFrac + randNTotM2*(1 - endPtFrac);
                 Perterb(&pt, v, rand * maxDev);
             }
             if(i > 0 && !zeroSmooth) {
                 const int currId = ptId - 1*j;
                 const auto& prevPt = pts[currId - 1];
                 const auto& currPt = pts[currId];
                 const auto& nextPt = pts[currId + 1];
                 gGetSmoothAbsCtrlsForPtBetween(prevPt, currPt, nextPt,
                                                cPts[currId*2], cPts[currId*2 + 1], smoothness);
             }
         }
     }
}

void genAtomicClosedDisplData(const qreal baseSeed,
                              const int nTot,
                              QVector<SkPoint>& pts,
                              QVector<SkPoint>& cPts,
                              SkPathMeasure &meas,
                              const float segLen,
                              const float maxDev,
                              const float smoothness) {
    const bool zeroSmooth = smoothness < 0.001f;
    float dist = 0;
    for(int i = 0; i < nTot; i++) {
        auto& pt = pts[i];
        if(i == nTot - 1) pt = pts[0];
        else {
            SkVector v;
            if(meas.getPosTan(dist += segLen, &pt, &v)) {
                Perterb(&pt, v, randFloat(baseSeed + i*100) * maxDev);
            }
        }
        if(i > 1 && !zeroSmooth) {
            const auto& prevPrevPt = pts[i - 2];
            const int prevId = i - 1;
            const auto& prevPt = pts[prevId];
            const auto& nextPt = pt;
            gGetSmoothAbsCtrlsForPtBetween(prevPrevPt, prevPt, nextPt,
                                           cPts[prevId*2], cPts[prevId*2 + 1],
                                           smoothness);
        }
    }
}

void genAtomicOpenedDisplData(const qreal baseSeed,
                              const float halfLength,
                              const float endPtFrac,
                              const int nSide,
                              const int nTot,
                              QVector<SkPoint>& pts,
                              QVector<SkPoint>& cPts,
                              SkPathMeasure &meas,
                              const float segLen,
                              const float maxDev,
                              const float smoothness) {
    const bool zeroSmooth = smoothness < 0.001f;
    float rand1 = 0;
    float randNTotM2 = 0;
    const int middleId = nSide;
    for(int i = 0; i <= nSide; i++) {
        for(int j = -1; j < 2 && (i != 0 || j == -1); j += 2) {
            const int ptId = middleId + i*j;
            auto& pt = pts[ptId];
            SkVector v;
            if(meas.getPosTan(halfLength + j*i*segLen, &pt, &v)) {
                float randV = randFloat(baseSeed + i*100);
                if(ptId == 1) rand1 = randV;
                else if(ptId == nTot - 2) randNTotM2 = randV;
                else if(ptId == 0) randV = randV*endPtFrac + rand1*(1 - endPtFrac);
                else if(ptId == nTot - 1) randV = randV*endPtFrac + randNTotM2*(1 - endPtFrac);
                Perterb(&pt, v, randV * maxDev);
            }
            if(i > 0 && !zeroSmooth) {
                const int currId = ptId - 1*j;
                const auto& prevPt = pts[currId - 1];
                const auto& currPt = pts[currId];
                const auto& nextPt = pts[currId + 1];
                gGetSmoothAbsCtrlsForPtBetween(prevPt, currPt, nextPt,
                                               cPts[currId*2], cPts[currId*2 + 1], smoothness);
            }
        }
    }
}

template <typename GenDispDataOp>
void displaceOpenedPath(const qreal baseSeed,
                        SkPathMeasure &meas,
                        SkPath* const dst,
                        const float segLen,
                        const float maxDev,
                        const float smoothness,
                        GenDispDataOp genDispDataOp) {
    const bool zeroSmooth = smoothness < 0.001f;
    const float length = meas.getLength();
    const float halfLength = length * 0.5f;

    const int nSide = SkScalarCeilToInt(halfLength / segLen);
    const int nTot = nSide + 1 + nSide;
    if(nTot < 2) return;

    const float remLen = halfLength - (nSide - 1)*segLen;
    const float endPtFrac = remLen/segLen;

    QVector<SkPoint> pts(nTot);
    QVector<SkPoint> cPts(zeroSmooth ? 1 : nTot*2);

    genDispDataOp(baseSeed, halfLength, endPtFrac, nSide,
                  nTot, pts, cPts, meas,
                  segLen, maxDev, smoothness);

    if(!zeroSmooth) {
        for(int j = -1; j < 2; j += 2) {
            const int ptId = j == -1 ? 0 : nTot - 1;
            const auto& currPt = pts[ptId];
            const auto& prevPt = j == -1 ? currPt : pts[ptId - 1];
            const auto& nextPt = j == -1 ? pts[ptId + 1] : currPt;

            gGetSmoothAbsCtrlsForPtBetween(prevPt, currPt, nextPt,
                                           cPts[2*ptId], cPts[2*ptId + 1],
                                           smoothness);
        }
    }

    auto currIt = pts.begin();
    if(zeroSmooth) {
        for(int i = 0; i < nTot; i++, currIt++) {
            if(i == 0) dst->moveTo(pts[i]);
            else dst->lineTo(pts[i]);
        }
    } else {
        for(int i = 0; i < nTot; i++, currIt++) {
            if(i == 0) dst->moveTo(pts[i]);
            else dst->cubicTo(cPts[i*2 - 1], cPts[i*2], pts[i]);
        }
    }
}

bool gSpatialDisplaceFilterPath(const qreal baseSeed,
                                const qreal gridSize,
                                SkPath* const dst,
                                const SkPath& src,
                                const float maxDev,
                                const float segLen,
                                const float smoothness) {
    dst->reset();
    dst->setFillType(src.getFillType());

    SkPathMeasure meas(src, false);
    do {
        if(meas.isClosed()) {
            const auto op = [gridSize](
                           const qreal baseSeed,
                           const int nTot,
                           QVector<SkPoint>& pts,
                           QVector<SkPoint>& cPts,
                           SkPathMeasure &meas,
                           const float segLen,
                           const float maxDev,
                           const float smoothness) {
                genSpatialClosedDisplData(baseSeed, gridSize,
                                          nTot, pts, cPts, meas,
                                          segLen, maxDev, smoothness);
            };
            displaceClosedPath(baseSeed, meas, dst,
                               segLen, maxDev, smoothness, op);
        } else {
            const auto op = [gridSize](
                           const qreal baseSeed,
                           const float halfLength,
                           const float endPtFrac,
                           const int nSide,
                           const int nTot,
                           QVector<SkPoint>& pts,
                           QVector<SkPoint>& cPts,
                           SkPathMeasure &meas,
                           const float segLen,
                           const float maxDev,
                           const float smoothness) {
                genSpatialOpenedDisplData(baseSeed, gridSize,
                                          halfLength, endPtFrac, nSide,
                                          nTot, pts, cPts, meas,
                                          segLen, maxDev, smoothness);
            };
            displaceOpenedPath(baseSeed, meas, dst,
                               segLen, maxDev, smoothness, op);
        }
    } while(meas.nextContour());
    return true;
}

bool gAtomicDisplaceFilterPath(const qreal baseSeed,
                               SkPath* const dst,
                               const SkPath& src,
                               const float maxDev,
                               const float segLen,
                               const float smoothness) {
    dst->reset();
    dst->setFillType(src.getFillType());

    SkPathMeasure meas(src, false);

    qreal seedContourInc = 0;
    do {
        if(meas.isClosed()) {
            displaceClosedPath(baseSeed + seedContourInc,
                               meas, dst, segLen, maxDev, smoothness,
                               genAtomicClosedDisplData);
        } else {
            displaceOpenedPath(baseSeed + seedContourInc,
                               meas, dst, segLen, maxDev, smoothness,
                               genAtomicOpenedDisplData);
        }

        seedContourInc += 1000.;
    } while(meas.nextContour());
    dst->setFillType(src.getFillType());
    return true;
}

qreal gMapTToFragment(const qreal minAbsT,
                      const qreal maxAbsT,
                      const qreal absT) {
    const qreal tFrag = maxAbsT - minAbsT;
    if(isZero6Dec(tFrag)) RuntimeThrow("Cannot map to zero range");
    if(tFrag < 0) RuntimeThrow("Cannot map to negative range");
    return (absT - minAbsT)/tFrag;
}

qreal gMapTFromFragment(const qreal minAbsT,
                        const qreal maxAbsT,
                        const qreal relT) {
    const qreal tFrag = maxAbsT - minAbsT;
    if(isZero6Dec(tFrag)) return minAbsT;
    if(tFrag < 0) RuntimeThrow("Cannot map to negative range");
    return minAbsT + relT*tFrag;
}

void gGetValuesForNodeInsertion(
        const QPointF &prevP1, QPointF &prevC2,
        QPointF &newC0, QPointF &newP1, QPointF &newC2,
        QPointF &nextC0, const QPointF &nextP1, qreal t) {
    t = CLAMP(t, 0, 1);
    const qreal oneMinusT = 1 - t;
    const QPointF P1_2 = prevC2*oneMinusT + nextC0*t;

    const QPointF newPrevC2 = prevP1*oneMinusT + prevC2*t;
    const QPointF newNextC0 = nextC0*oneMinusT + nextP1*t;

    newC0 = newPrevC2*oneMinusT + P1_2*t;
    newC2 = P1_2*oneMinusT + newNextC0*t;
    newP1 = newC0*oneMinusT + newC2*t;

    prevC2 = newPrevC2;
    nextC0 = newNextC0;
}

void gGetValuesForNodeRemoval(
        const QPointF &prevP1, QPointF &prevC2,
        const QPointF &pC0, const QPointF &pP1, const QPointF &pC2,
        QPointF &nextC0, const QPointF &nextP1,
        qreal t) {
    Q_UNUSED(pP1)
    t = CLAMP(t, 0, 1);
    if(isZero6Dec(t)) {
        prevC2 = pC2;
        return;
    }
    if(isOne6Dec(t)) {
        nextC0 = pC0;
        return;
    }
    const qreal tMinus1 = t - 1;
    prevC2 = (prevC2 + prevP1*tMinus1)/t;
    nextC0 = (nextP1*t - nextC0)/tMinus1;
}

QList<SkPath> gBreakApart(const SkPath &src) {
    QList<SkPath> result;

    SkPath::Iter iter(src, false);
    SkPath current;
    for(;;) {
        SkPoint pts[4];
        switch(iter.next(pts)) {
        case SkPath::kLine_Verb: {
            current.lineTo(pts[1]);
        } break;
        case SkPath::kQuad_Verb: {
            current.quadTo(pts[1], pts[2]);
        } break;
        case SkPath::kConic_Verb: {
            current.conicTo(pts[1], pts[2], iter.conicWeight());
        } break;
        case SkPath::kCubic_Verb: {
            current.cubicTo(pts[1], pts[2], pts[3]);
        } break;
        case SkPath::kClose_Verb: {
            current.close();
        } break;
        case SkPath::kMove_Verb: {
            if(!current.isEmpty()) {
                result << current;
                current.reset();
            }
            current.moveTo(pts[0]);
        } break;
        case SkPath::kDone_Verb: {
            if(!current.isEmpty()) {
                result << current;
                current.reset();
            }
            return result;
        }
        }
    }
}

CtrlsMode gGuessCtrlsMode(const QPointF& c0,
                          const QPointF& p1,
                          const QPointF& c2,
                          const bool c0Enabled,
                          const bool c2Enabled) {
    if(!c0Enabled || !c2Enabled ||
       isZero2Dec(pointToLen(c0 - p1)) ||
       isZero2Dec(pointToLen(c2 - p1))) {
        return CtrlsMode::corner;
    }
    if(gIsSymmetric(c0, p1, c2)) return CtrlsMode::symmetric;
    else if(gIsSmooth(c0, p1, c2)) return CtrlsMode::smooth;
    else return CtrlsMode::corner;
}

bool gIsClockwise(const QList<qCubicSegment2D>& segs) {
    if(segs.isEmpty()) return false;
    QPointF prevPos = segs.first().p0();
    qreal sum = 0;
    const auto lineTo = [&prevPos, &sum](const QPointF& pos) {
        sum += (pos.x() - prevPos.x()) * (pos.y() + prevPos.y());
        prevPos = pos;
    };
    for(const auto& seg : segs) {
        lineTo(seg.c1());
        lineTo(seg.c2());
        lineTo(seg.p3());
    }
    return sum > 0;
}
