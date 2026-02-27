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

#include "videobox.h"
#include "ReadWrite/evformat.h"

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavutil/imgutils.h>
}

#include <QDebug>
#include <QInputDialog>

#include "Sound/evideosound.h"
#include "canvas.h"
#include "Sound/soundcomposition.h"
#include "filesourcescache.h"
#include "fileshandler.h"
#include "typemenu.h"

VideoFileHandler* videoFileHandlerGetter(const QString& path) {
    return FilesHandler::sInstance->getFileHandler<VideoFileHandler>(path);
}

VideoBox::VideoBox() : AnimationBox("Video", eBoxType::video),
    mFileHandler(this,
                 [](const QString& path) {
                     return videoFileHandlerGetter(path);
                 },
                 [this](VideoFileHandler* obj) {
                     return fileHandlerAfterAssigned(obj);
                 },
                 [this](ConnContext& conn, VideoFileHandler* obj) {
                     fileHandlerConnector(conn, obj);
                 }) {
    const auto flar = getDurationRectangle()->ref<FixedLenAnimationRect>();
    mSound = enve::make_shared<eVideoSound>(flar);
    ca_addChild(mSound);
    //mSound->hide(); // should be on by default
    mSound->SWT_hide();

    connect(this, &eBoxOrSound::parentChanged,
            mSound.get(), &eBoxOrSound::setParentGroup);
    connect(this, &eBoxOrSound::parentChanged, this, &VideoBox::setCorrectFps);
}

void VideoBox::fileHandlerConnector(ConnContext &conn, VideoFileHandler *obj) {
    const auto newDataHandler = obj ? obj->getFrameHandler() : nullptr;
    if(newDataHandler) {
        const auto frameHandler = enve::make_shared<VideoFrameHandler>(newDataHandler);
        setAnimationFramesHandler(frameHandler);
        const auto cacheHandler = &newDataHandler->getCacheHandler();
        getAnimationDurationRect()->setRasterCacheHandler(cacheHandler);
        conn << connect(obj, &VideoFileHandler::pathChanged,
                        this, [this, obj]() {
            fileHandlerAfterAssigned(obj);
        });
        conn << connect(obj, &VideoFileHandler::reloaded,
                        this, &ImageBox::prp_afterWholeInfluenceRangeChanged);
        conn << connect(newDataHandler, &VideoDataHandler::frameCountUpdated,
                        this, &VideoBox::updateAnimationRange);
    }
}

void VideoBox::fileHandlerAfterAssigned(VideoFileHandler *obj) {
    const auto newDataHandler = obj ? obj->getFrameHandler() : nullptr;
    qsptr<AnimationFrameHandler> frameHandler;
    const HddCachableCacheHandler* cacheHandler;
    if(newDataHandler) {
        frameHandler = enve::make_shared<VideoFrameHandler>(newDataHandler);
        cacheHandler = &newDataHandler->getCacheHandler();
    } else cacheHandler = nullptr;
    setAnimationFramesHandler(frameHandler);
    getAnimationDurationRect()->setRasterCacheHandler(cacheHandler);

    soundDataChanged();
    animationDataChanged();
}

void VideoBox::writeBoundingBox(eWriteStream& dst) const {
    AnimationBox::writeBoundingBox(dst);
    dst.writeFilePath(mFileHandler->path());
    dst << getStretch();
}

void VideoBox::readBoundingBox(eReadStream& src) {
    AnimationBox::readBoundingBox(src);
    const QString path = src.readFilePath();
    setFilePathNoRename(path);
    if (src.evFileVersion() >= EvFormat::avStretch) {
        qreal stretch;
        src >> stretch;
        setStretch(stretch);
    }
}

QDomElement VideoBox::prp_writePropertyXEV_impl(const XevExporter& exp) const {
    auto result = AnimationBox::prp_writePropertyXEV_impl(exp);
    const QString& absSrc = mFileHandler.path();
    XevExportHelpers::setAbsAndRelFileSrc(absSrc, result, exp);
    return result;
}

void VideoBox::prp_readPropertyXEV_impl(const QDomElement& ele,
                                        const XevImporter& imp) {
    AnimationBox::prp_readPropertyXEV_impl(ele, imp);
    const QString absSrc = XevExportHelpers::getAbsAndRelFileSrc(ele, imp);
    setFilePathNoRename(absSrc);
}

#include "GUI/edialogs.h"
void VideoBox::changeSourceFile() {
    const QString path = eDialogs::openFile(
                "Change Source", getFilePath(),
                "Video Files (" + FileExtensions::videoFilters() + ")");
    if(!path.isEmpty()) setFilePath(path);
}

void VideoBox::setStretch(const qreal stretch) {
    AnimationBox::setStretch(stretch);
    mSound->setStretch(stretch);
}

void VideoBox::setFilePathNoRename(const QString &path) {
    mFileHandler.assign(path);
}

void VideoBox::setFilePath(const QString &path) {
    setFilePathNoRename(path);
    rename(QFileInfo(path).completeBaseName());
}

QString VideoBox::getFilePath() {
    return mFileHandler.path();
}

const VideoBox::VideoSpecs VideoBox::getSpecs()
{
    VideoSpecs specs;
    const auto h = mFileHandler->getFrameHandler();
    if (!h) { return specs; }
    int frames = h->getFrameCount();
    specs.range = FrameRange{0, frames <= 1 ? 0 : frames - 1};
    specs.fps = h->getFps();
    specs.dim = h->getDim();
    return specs;
}

void VideoBox::soundDataChanged()
{
    const auto pScene = getParentScene();
    const auto soundHandler = mFileHandler ?
                mFileHandler->getSoundHandler() : nullptr;
    const auto durRect = getDurationRectangle();
    if (soundHandler) {
        if (!mSound->SWT_isVisible()) {
            if (pScene) { pScene->getSoundComposition()->addSound(mSound); }
        }
        durRect->setSoundCacheHandler(&soundHandler->getCacheHandler());
    } else {
        if (mSound->SWT_isVisible()) {
            if (pScene) { pScene->getSoundComposition()->removeSound(mSound); }
        }
        durRect->setSoundCacheHandler(nullptr);
    }
    mSound->setSoundDataHandler(soundHandler);
    mSound->SWT_setVisible(soundHandler);

    // why? this does not make any sense to me
    //mSound->setVisible(soundHandler);
}
