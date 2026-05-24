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

#ifndef SVGLINKBOX_H
#define SVGLINKBOX_H

#include "externallinkboxt.h"
#include "Boxes/containerbox.h"
#include "FileCacheHandlers/svgfilecachehandler.h"
#include "Boxes/svgelementtrack.h"
#include "Boxes/svgflipbooktrack.h"

using SvgLinkBoxBase =
    ExternalLinkBoxT<ContainerBox,
                     SvgFileCacheHandler>;

class CORE_EXPORT SvgLinkBox : public SvgLinkBoxBase {
public:
    SvgLinkBox();

    void changeSourceFile();

    void addElementTrack(const QString& targetId);
    void removeElementTrack(SvgElementTrack* track);
    void removeFlipbookTrack(SvgFlipbookTrack* track);

    void anim_setAbsFrame(const int frame) override;

    void writeBoundingBox(eWriteStream& dst) const override;
    void readBoundingBox(eReadStream& src) override;

    void SWT_setupAbstraction(SWT_Abstraction* abstraction,
                              const UpdateFuncs& updateFuncs,
                              int visiblePartWidgetId) override;

    void prp_setupTreeViewMenu(PropertyMenu* menu) override;

protected:
    QDomElement prp_writePropertyXEV_impl(const XevExporter& exp) const override;
    void prp_readPropertyXEV_impl(const QDomElement& ele,
                                  const XevImporter& imp) override;

private:
    void updateContent();
    void resolveElementTracks();
    void wireTrack(const qsptr<SvgElementTrack>& track);
    void wireFlipbookTrack(const qsptr<SvgFlipbookTrack>& track);
    void collectFlipbookDescs(ContainerBox* container);
    void fileHandlerConnector(ConnContext &conn, SvgFileCacheHandler *obj);
    void fileHandlerAfterAssigned(SvgFileCacheHandler *obj);

    QList<qsptr<Gradient>> mGradients;
    QList<qsptr<SvgElementTrack>> mElementTracks;
    QList<qsptr<SvgFlipbookTrack>> mFlipbookTracks;
};

#endif // SVGLINKBOX_H
