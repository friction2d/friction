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

#include "zigzagpatheffect.h"
#include "Animators/qrealanimator.h"

ZigZagPathEffect::ZigZagPathEffect() :
    PathEffect("zigzag effect", PathEffectType::ZIGZAG) {
    mAngle = enve::make_shared<QrealAnimator>(0, -9999, 9999, 1, "angle");
    mDistance = enve::make_shared<QrealAnimator>(10, 1, 9999, 1, "distance");

    ca_addChild(mAngle);
    ca_addChild(mDistance);
}

class ZigZagEffectCaller : public PathEffectCaller {
public:
    ZigZagEffectCaller(const qreal angle, const qreal dist) :
        mAngle(angle), mDist(dist) {}

    void apply(SkPath& path);
private:
    const qreal mAngle;
    const qreal mDist;
};

void ZigZagEffectCaller::apply(SkPath &path) {
    auto segLists = CubicList::sMakeFromSkPath(path);
    const QRectF pathBounds = toQRectF(path.getBounds());
    const auto srcFillType = path.getFillType();
    path.reset();
    path.setFillType(srcFillType);
    QTransform rotate;
    const QPointF pivot = pathBounds.center();
    rotate.translate(pivot.x(), pivot.y());
    rotate.rotate(mAngle);
    rotate.translate(-pivot.x(), -pivot.y());
    const QPolygonF linesBBPolygon = rotate.map(QPolygonF(pathBounds));
    const QRectF secondBB = linesBBPolygon.boundingRect();
    const QPolygonF secondLinesBB = rotate.map(QPolygonF(secondBB));
    const QLineF firstLine(secondLinesBB.at(0), secondLinesBB.at(1));
    const QLineF sideLine(secondLinesBB.at(1), secondLinesBB.at(2));
    const int nLines = qCeil(sideLine.length()/mDist);
//    const QLineF transVec = QLineF::fromPolar(mDist, mAngle - 90);
    QLineF transVec = sideLine;
    transVec.setLength(mDist);
    const QPointF transPt(transVec.dx(), transVec.dy());
    QList<QLineF> prevLines;
    for(int i = 0; i < nLines; i++) {
        const QLineF iLine = firstLine.translated(i*transPt);
        QList<QPointF> intersections;
        for(auto& seg : segLists) {
//            intersections.append(iLine.p1());
//            intersections.append(iLine.p2());
            seg.lineIntersections(iLine, intersections);
        }

        const int jMin = 0;
        const int jMax = intersections.count() - 2;
        QList<QLineF> currLines;
        for(int j = jMin; j <= jMax; j += 2) {
            const QLineF line(intersections.at(j), intersections.at(j + 1));
            currLines << line;
        }

        if(prevLines.count() == 1 && currLines.count() == 1) {
            const auto& currLine = currLines.first();
            path.lineTo(toSkPoint(currLine.p1()));
            path.lineTo(toSkPoint(currLine.p2()));
        } else {
            for(const auto& line : qAsConst(currLines)) {
                path.moveTo(toSkPoint(line.p1()));
                path.lineTo(toSkPoint(line.p2()));
            }
        }

        prevLines = currLines;
    }
}

stdsptr<PathEffectCaller> ZigZagPathEffect::getEffectCaller(
        const qreal relFrame, const qreal influence) const {
    Q_UNUSED(influence)
    const qreal degAngle = mAngle->getEffectiveValue(relFrame);
    const qreal distInc = mDistance->getEffectiveValue(relFrame);
    return enve::make_shared<ZigZagEffectCaller>(degAngle, distInc);
}
