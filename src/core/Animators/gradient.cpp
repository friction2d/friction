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

#include "gradient.h"
#include "Animators/coloranimator.h"
#include "Boxes/pathbox.h"
#include "svgexporthelpers.h"

Gradient::Gradient() : DynamicComplexAnimator<ColorAnimator>("gradient") {
    connect(this, &Property::prp_currentFrameChanged,
            this, &Gradient::updateQGradientStops);
}

void Gradient::prp_writeProperty_impl(eWriteStream &dst) const {
    DynamicComplexAnimator<ColorAnimator>::prp_writeProperty_impl(dst);
}

void Gradient::prp_readProperty_impl(eReadStream &src) {
    DynamicComplexAnimator<ColorAnimator>::prp_readProperty_impl(src);
}

QColor Gradient::getLastQGradientStopQColor() {
    if(mQGradientStops.isEmpty()) return Qt::black;
    return mQGradientStops.last().second;
}

QColor Gradient::getFirstQGradientStopQColor() {
    if(mQGradientStops.isEmpty()) return Qt::black;
    return mQGradientStops.first().second;
}

void Gradient::addColor(const QColor &color) {
    auto newColorAnimator = enve::make_shared<ColorAnimator>();
    newColorAnimator->setColor(color);
    addChild(newColorAnimator);
}

void Gradient::replaceColor(const int id, const QColor &color) {
    const auto colorAnim = getChild(id);
    colorAnim->setColor(color);
}

void Gradient::startColorIdTransform(const int id) {
    const auto color = getChild(id);
    color->prp_startTransform();
}

QColor Gradient::getColorAt(const int id) {
    const int nCols = ca_getNumberOfChildren();
    if(id < 0 || id >= nCols) return Qt::black;
    const auto color = getChild(id);
    return color->getColor();
}

QGradientStops Gradient::getQGradientStops(const qreal absFrame) {
    const int nCols = ca_getNumberOfChildren();
    QGradientStops stops;
    const qreal inc = 1./(nCols - 1);
    qreal cPos = 0.;
    for(int i = 0; i < nCols; i++) {
        const auto colorAnim = getChild(i);
        const auto color = colorAnim->getColor(absFrame);
        const auto stop = QPair<qreal, QColor>(clamp(cPos, 0, 1), color);
        stops.append(stop);
        cPos += inc;
    }
    return stops;
}

void Gradient::saveSVG(SvgExporter& exp) const {
    auto ele = exp.createElement("linearGradient");
    const auto baseGradId = SvgExportHelpers::ptrToStr(this);
    const int count = ca_getNumberOfChildren();
    if(count == 0) {
        auto stop = exp.createElement("stop");
        stop.setAttribute("offset", 0);
        stop.setAttribute("stop-color", "#000000");
        ele.appendChild(stop);
    } else {
        for(int i = 0; i < count; i++) {
            const auto color = getChild(i);
            auto stop = exp.createElement("stop");
            stop.setAttribute("offset", count == 1 ? 0. : qreal(i)/(count - 1));
            color->saveColorSVG(exp, stop, exp.fAbsRange, "stop-color");
            ele.appendChild(stop);
        }
    }
    ele.setAttribute("id", baseGradId);

    exp.addToDefs(ele);
}

void Gradient::updateQGradientStops() {
    const int nCols = ca_getNumberOfChildren();
    mQGradientStops.clear();
    const qreal inc = 1./(nCols - 1);
    qreal cPos = 0;
    for(int i = 0; i < nCols; i++) {
        const auto colorAnim = getChild(i);
        const auto color = colorAnim->getColor();
        const auto stop = QPair<qreal, QColor>(clamp(cPos, 0, 1), color);
        mQGradientStops.append(stop);
        cPos += inc;
    }
}
