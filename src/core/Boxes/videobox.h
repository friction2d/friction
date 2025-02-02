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

#ifndef VIDEOBOX_H
#define VIDEOBOX_H
#include <QString>
#include <unordered_map>
#include "Boxes/animationbox.h"
#include "FileCacheHandlers/videocachehandler.h"

class eVideoSound;

class CORE_EXPORT VideoBox : public AnimationBox {
    e_OBJECT
protected:
    VideoBox();

    void prp_readPropertyXML_impl(const QDomElement& ele, const Friction::Core::XmlImporter& imp);
    QDomElement prp_writePropertyXML_impl(const Friction::Core::XmlExporter& exp) const;
public:
    struct VideoSpecs {
        QSize dim;
        qreal fps;
        FrameRange range;
    };
    void changeSourceFile();

    void writeBoundingBox(eWriteStream& dst) const;
    void readBoundingBox(eReadStream& src);

    void setStretch(const qreal stretch);

    eVideoSound* sound() const
    { return mSound.get(); }
    void setFilePath(const QString& path);
    QString getFilePath();
    const VideoSpecs getSpecs();
private:
    void setFilePathNoRename(const QString &path);

    void soundDataChanged();
    void fileHandlerConnector(ConnContext& conn, VideoFileHandler* obj);
    void fileHandlerAfterAssigned(VideoFileHandler* obj);

    qsptr<eVideoSound> mSound;
    FileHandlerObjRef<VideoFileHandler> mFileHandler;
};

#endif // VIDEOBOX_H
