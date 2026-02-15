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

#include "sumpatheffect.h"
#include "pathoperations.h"
#include "Boxes/pathbox.h"

SumPathEffect::SumPathEffect() :
    PathEffect("sum effect", PathEffectType::SUM) {
}

class SumEffectCaller : public PathEffectCaller {
public:
    SumEffectCaller() {}

    void apply(SkPath& path);
private:
};

void SumEffectCaller::apply(SkPath &path) {
    QList<SkPath> paths = gBreakApart(path);
    const auto srcFillType = path.getFillType();
    path.reset();
    path.setFillType(srcFillType);
    SkOpBuilder builder;
    for(const auto &subPath : qAsConst(paths)) {
        builder.add(subPath, SkPathOp::kUnion_SkPathOp);
    }
    builder.resolve(&path);
}

stdsptr<PathEffectCaller> SumPathEffect::getEffectCaller(
        const qreal relFrame, const qreal influence) const {
    Q_UNUSED(relFrame)
    Q_UNUSED(influence)
    return enve::make_shared<SumEffectCaller>();
}
