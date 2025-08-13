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

#include "Boxes/imagebox.h"

#include <QMenu>

#include "FileCacheHandlers/imagecachehandler.h"
#include "fileshandler.h"
#include "filesourcescache.h"
#include "GUI/edialogs.h"
#include "typemenu.h"
//#include "paintbox.h"
#include "svgexporter.h"
#include "svgexporthelpers.h"

ImageFileHandler* imageFileHandlerGetter(const QString& path) {
    return FilesHandler::sInstance->getFileHandler<ImageFileHandler>(path);
}

ImageBox::ImageBox() : BoundingBox("Image", eBoxType::image),
    mFileHandler(this,
                 [](const QString& path) {
                     return imageFileHandlerGetter(path);
                 },
                 [this](ImageFileHandler* obj) {
                     return fileHandlerAfterAssigned(obj);
                 },
                 [this](ConnContext& conn, ImageFileHandler* obj) {
                     fileHandlerConnector(conn, obj);
                 }) {
}

ImageBox::ImageBox(const QString &filePath) : ImageBox() {
    setFilePath(filePath);
}

void ImageBox::fileHandlerConnector(ConnContext &conn, ImageFileHandler *obj) {
    conn << connect(obj, &ImageFileHandler::pathChanged,
                    this, &ImageBox::prp_afterWholeInfluenceRangeChanged);
    conn << connect(obj, &ImageFileHandler::reloaded,
                    this, &ImageBox::prp_afterWholeInfluenceRangeChanged);
}

void ImageBox::fileHandlerAfterAssigned(ImageFileHandler *obj) {
    Q_UNUSED(obj);
}

void ImageBox::writeBoundingBox(eWriteStream& dst) const {
    BoundingBox::writeBoundingBox(dst);
    dst.writeFilePath(mFileHandler->path());
}

void ImageBox::readBoundingBox(eReadStream& src) {
    BoundingBox::readBoundingBox(src);
    const QString path = src.readFilePath();
    setFilePathNoRename(path);
}

QDomElement ImageBox::prp_writePropertyXML_impl(const Friction::Core::XmlExporter& exp) const {
    auto result = BoundingBox::prp_writePropertyXML_impl(exp);
    const QString& absSrc = mFileHandler.path();
    Friction::Core::XmlExportHelpers::setAbsAndRelFileSrc(absSrc, result, exp);
    return result;
}

void ImageBox::prp_readPropertyXML_impl(const QDomElement& ele, const Friction::Core::XmlImporter& imp) {
    BoundingBox::prp_readPropertyXML_impl(ele, imp);
    const QString absSrc = Friction::Core::XmlExportHelpers::getAbsAndRelFileSrc(ele, imp);
    setFilePathNoRename(absSrc);
}

void ImageBox::setFilePathNoRename(const QString &path) {
    mPath = path;
    mFileHandler.assign(path);
    prp_afterWholeInfluenceRangeChanged();
}

void ImageBox::setFilePath(const QString &path) {
    setFilePathNoRename(path);
    rename(QFileInfo(path).completeBaseName());
}

void ImageBox::reload() {
    if(mFileHandler) mFileHandler->reloadAction();
}

void ImageBox::setupCanvasMenu(PropertyMenu * const menu)
{
    if (menu->hasActionsForType<ImageBox>()) { return; }
    menu->addedActionsForType<ImageBox>();

    const PropertyMenu::PlainSelectedOp<ImageBox> reloadOp =
    [](ImageBox * box) { box->reload(); };
    menu->addPlainAction(QIcon::fromTheme("loop"), tr("Reload"), reloadOp);

    const PropertyMenu::PlainSelectedOp<ImageBox> setSrcOp =
    [](ImageBox * box) { box->changeSourceFile(); };
    menu->addPlainAction(QIcon::fromTheme("document-new"), tr("Set Source File"), setSrcOp);

    menu->addSeparator();

    BoundingBox::setupCanvasMenu(menu);
}

void ImageBox::changeSourceFile() {
    const QString filters = FileExtensions::imageFilters();
    QString importPath = eDialogs::openFile("Change Source", mFileHandler.path(),
                                            "Image Files (" + filters + " *.ora)");
    if(!importPath.isEmpty()) setFilePath(importPath);
}

void ImageBox::setupRenderData(const qreal relFrame, const QMatrix& parentM,
                               BoxRenderData * const data,
                               Canvas* const scene)
{
    if (!mFileHandler) { mFileHandler.assign(mPath); }
    BoundingBox::setupRenderData(relFrame, parentM, data, scene);
    const auto imgData = static_cast<ImageBoxRenderData*>(data);
    if (mFileHandler->hasImage()) {
        imgData->setContainer(mFileHandler->getImageContainer());
    } else {
        const auto loader = mFileHandler->scheduleLoad();
        if (loader) { loader->addDependent(imgData); }
    }
}

stdsptr<BoxRenderData> ImageBox::createRenderData()
{
    if (!mFileHandler) { mFileHandler.assign(mPath); }
    return enve::make_shared<ImageBoxRenderData>(mFileHandler, this);
}

void ImageBox::saveSVG(SvgExporter& exp, DomEleTask* const eleTask) const {
    const QString imageId = SvgExportHelpers::ptrToStr(mFileHandler.data());
    const auto expPtr = &exp;
    const auto generate = [expPtr, eleTask, imageId](const sk_sp<SkImage>& image) {
        if(!image) return;
        SvgExportHelpers::defImage(*expPtr, image, imageId);
        auto& use = eleTask->initialize("use");
        use.setAttribute("href", "#" + imageId);
    };
    if(mFileHandler->hasImage()) {
        const auto image = mFileHandler->getImage();
        generate(image);
    } else {
        const auto task = mFileHandler->scheduleLoad();
        if(!task) return;
        const qptr<const ImageBox> thisPtr = this;
        const stdptr<DomEleTask> eleTaskPtr = eleTask;
        task->addDependent(
        {[thisPtr, eleTaskPtr, imageId, generate]() {
             if(!eleTaskPtr || !thisPtr) return;
             const auto image = thisPtr->mFileHandler->getImage();
             generate(image);
         }, nullptr});
        task->addDependent(eleTask);
    }
}

void ImageBoxRenderData::loadImageFromHandler() {
    if(fSrcCacheHandler) {
        setContainer(fSrcCacheHandler->getImageContainer());
    }
}
