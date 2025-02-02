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

#include "qcubicsegment1danimator.h"
#include "appsupport.h"

qCubicSegment1DAnimator::qCubicSegment1DAnimator(const QString &name) :
    InterpolationAnimatorT<qCubicSegment1D>(name) {}

void qCubicSegment1DAnimator::prp_readPropertyXEV_impl(
        const QDomElement& ele, const Friction::Core::XevImporter& imp) {
    Q_UNUSED(imp)
    readValuesXEV(ele, [](qCubicSegment1D& seg, const QStringRef& str) {
        const auto valueStrs = str.split(' ', Qt::SkipEmptyParts);
        if(valueStrs.count() == 4) {
            const qreal p0 = Friction::Core::XmlExportHelpers::stringToDouble(valueStrs[0]);
            const qreal c1 = Friction::Core::XmlExportHelpers::stringToDouble(valueStrs[1]);
            const qreal c2 = Friction::Core::XmlExportHelpers::stringToDouble(valueStrs[2]);
            const qreal p1 = Friction::Core::XmlExportHelpers::stringToDouble(valueStrs[3]);
            seg = qCubicSegment1D(p0, c1, c2, p1);
        } else RuntimeThrow("Invalid value count '" + str.toString() + "'");
    });
}

QDomElement qCubicSegment1DAnimator::prp_writePropertyXEV_impl(const Friction::Core::XevExporter& exp) const {
    auto result = exp.createElement("CubicSegment1D");

    writeValuesXEV(result, [](const qCubicSegment1D& seg) {
        return QString::number(seg.p0()) + ' ' +
               QString::number(seg.c1()) + ' ' +
               QString::number(seg.c2()) + ' ' +
               QString::number(seg.p1());
    });

    return result;
}
