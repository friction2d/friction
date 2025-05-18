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

#ifndef IMAGEBOX_H
#define IMAGEBOX_H
#include "Boxes/boundingbox.h"
#include "skia/skiaincludes.h"
#include "FileCacheHandlers/imagecachehandler.h"
#include "imagerenderdata.h"
#include "FileCacheHandlers/filehandlerobjref.h"

struct CORE_EXPORT ImageBoxRenderData : public ImageContainerRenderData {
    ImageBoxRenderData(ImageFileHandler * const cacheHandler,
                       BoundingBox * const parentBox) :
        ImageContainerRenderData(parentBox),
        fSrcCacheHandler(cacheHandler) {}

    void loadImageFromHandler();

    const qptr<ImageFileHandler> fSrcCacheHandler;
};

class CORE_EXPORT ImageBox : public BoundingBox {
    e_OBJECT
protected:
    ImageBox();
    ImageBox(const QString &filePath);

    void prp_readPropertyXEV_impl(const QDomElement& ele, const XevImporter& imp);
    QDomElement prp_writePropertyXEV_impl(const XevExporter& exp) const;
public:
    void setupCanvasMenu(PropertyMenu * const menu);

    void setupRenderData(const qreal relFrame, const QMatrix& parentM,
                         BoxRenderData * const data,
                         Scene * const scene);
    stdsptr<BoxRenderData> createRenderData();

    void writeBoundingBox(eWriteStream& dst) const;
    void readBoundingBox(eReadStream& src);

    void saveSVG(SvgExporter& exp, DomEleTask* const eleTask) const;

    void changeSourceFile();
    void setFilePath(const QString &path);

    void reload();
private:
    void setFilePathNoRename(const QString &path);

    void fileHandlerConnector(ConnContext& conn, ImageFileHandler* obj);
    void fileHandlerAfterAssigned(ImageFileHandler* obj);

    FileHandlerObjRef<ImageFileHandler> mFileHandler;
    QString mPath;
};

#endif // IMAGEBOX_H
