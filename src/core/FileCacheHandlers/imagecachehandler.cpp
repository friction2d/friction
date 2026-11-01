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

#include "imagecachehandler.h"

#include "filecachehandler.h"
//#include "Ora/oraimporter.h"

#include "appsupport.h"
#include "filesourcescache.h"

ImageFileDataHandler::ImageFileDataHandler() {}

void ImageFileDataHandler::afterSourceChanged()
{
    const QFileInfo info(getFilePath());
    const auto suffix = info.suffix();
    if (suffix == "ora") {
        mType = Type::ora;
    } else {
        mType = Type::image;
    }
}

void ImageFileDataHandler::clearCache() {
    mImage.reset();
    mImageLoader.reset();
}

eTask *ImageFileDataHandler::scheduleLoad()
{
    if (mImage) {
        const auto task = mImage->scheduleLoadFromTmpFile();
        if (task) { return task; }
    }
    if (mImageLoader) { return mImageLoader.get(); }
    switch (mType) {
    /*case Type::ora:
        mImageLoader = enve::make_shared<OraLoader>(mFilePath, this);
        break;*/
    case Type::image:
        mImageLoader = enve::make_shared<ImageLoader>(mFilePath, this);
        break;
    default:
        return nullptr;
    }
    if (mImageLoader) { mImageLoader->queTask(); }
    return mImageLoader.get();
}

bool ImageFileDataHandler::hasImage() const
{
    if (!mImage) { return false; }
    return mImage->hasImage();
}

sk_sp<SkImage> ImageFileDataHandler::getImage() const
{
    if (!mImage) { return nullptr; }
    return mImage->getImage();
}

void ImageFileDataHandler::replaceImage(const sk_sp<SkImage> &img)
{
    if (img) {
        mImage = enve::make_shared<ImageCacheContainerX>(img, this);
    } else { mImage.reset(); }
    mImageLoader.reset();
}

ImageLoader::ImageLoader(const QString &filePath,
                         ImageFileDataHandler * const handler)
    : mTargetHandler(handler)
    , mFilePath(filePath) {}

void ImageLoader::process()
{
    const sk_sp<SkData> data = SkData::MakeFromFileName(mFilePath.toUtf8().data());
    mImage = SkImage::MakeFromEncoded(data);
}

void ImageLoader::afterProcessing()
{
    if (mTargetHandler) { mTargetHandler->replaceImage(mImage); }
}

void ImageLoader::afterCanceled()
{
    if (mTargetHandler) { mTargetHandler->replaceImage(mImage); }
}

/*void OraLoader::process()
{
    mImage = ImportORA::loadMergedORAFile(mFilePath, true);
}*/

void ImageFileHandler::replace()
{
    const auto importPath = AppSupport::getOpenFile(nullptr,
                                                    tr("Change Source"),
                                                    path(),
                                                    tr("Image Files (%1)").arg(FileExtensions::imageFilters()));
    if (!importPath.isEmpty()) { setPath(importPath); }
}
