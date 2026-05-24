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

#include "hddcachablecachehandler.h"

Q_LOGGING_CATEGORY(lcCacheHandler, "friction.cachehandler", QtWarningMsg)
#include "pointhelpers.h"
#include "themesupport.h"

void HddCachableCacheHandler::drawCacheOnTimeline(QPainter * const p,
                                                  const QRectF& drawRect,
                                                  const int startFrame,
                                                  const int endFrame,
                                                  const qreal unit,
                                                  const int maxX) const {
    if(startFrame > endFrame) return;
    p->save();
    const qreal quStartFrame = startFrame/unit;
    const int uStartFrame = isInteger4Dec(quStartFrame) ?
                qRound(quStartFrame): qFloor(quStartFrame);
    const qreal pixelsPerFrame = drawRect.width()/(endFrame - startFrame + 1);
//    if(!isOne4Dec(unit))
//        p->translate((uStartFrame - quStartFrame)*pixelsPerFrame, 0);

    p->setBrush(ThemeSupport::getThemeHighlightColor(55));
    p->setPen(Qt::NoPen);

    int lastDrawnFrame = uStartFrame;
    int lastDrawX = qRound(drawRect.x());
    //bool lastStoresInMemory = true;
    for(auto it = mConts.atOrAfterFrame(uStartFrame); it != mConts.end(); it++) {
        const auto cont = it->second.get();
        const int minFrame = qMax(startFrame - 1,
                                  qRound(cont->getRangeMin()*unit));
        if(minFrame > endFrame + 1) break;
        const int afterMaxFrame = qMin(endFrame + 1,
                                       qRound((cont->getRangeMax() + 1)*unit));
        if(afterMaxFrame < startFrame) continue;

        const int dFrame = minFrame - startFrame;
        int x = qRound(dFrame*pixelsPerFrame + drawRect.x());

        int width = qRound(pixelsPerFrame*(afterMaxFrame - minFrame));
        if(lastDrawnFrame == minFrame) {
            width += x - lastDrawX;
            x = lastDrawX;
        }
        if(x > maxX) break;
        const bool storesInMemory = cont->storesDataInMemory();
        //if(storesInMemory != lastStoresInMemory) {
            if(storesInMemory) {
                if(cont->inUse()) p->setBrush(ThemeSupport::getThemeHighlightColor(110));
                else p->setBrush(ThemeSupport::getThemeHighlightColor(55));
            } else {
                p->setBrush(ThemeSupport::getThemeHighlightColor(55));
            }
            //lastStoresInMemory = storesInMemory;
        //}
        p->drawRect(x, qRound(drawRect.top()),
                    qMin(maxX, x + width) - x, /*qRound(drawRect.height())*/4);
        lastDrawnFrame = afterMaxFrame;
        lastDrawX = x + width;
    }
    p->restore();
}
