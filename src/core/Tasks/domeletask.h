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

#ifndef DOMELETASK_H
#define DOMELETASK_H

#include "updatable.h"

#include "framerange.h"
#include "svgexporter.h"

#include <QDomElement>

class CORE_EXPORT DomEleTask : public eCpuTask {
public:
    DomEleTask(SvgExporter& exp, const FrameRange& visRange);

    void process() {}

    QDomElement& initialize(const QString& tag);
    QDomElement& element();

    const FrameRange& visRange() const { return mVisRange; }
    const SvgExporter::FrameMapping& frameMapping() const { return mFrameMapping; }
private:
    SvgExporter& mExp;
    QDomElement mEle;
    const FrameRange mVisRange;
    const SvgExporter::FrameMapping mFrameMapping;
};

#endif // DOMELETASK_H
