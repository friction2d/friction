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

#include <fstream>
#include "Animators/qrealanimator.h"
#include "Animators/qpointfanimator.h"
#include "Animators/qvector3danimator.h"
#include "Animators/coloranimator.h"
#include "Animators/qstringanimator.h"
#include "Animators/transformanimator.h"
#include "Animators/paintsettingsanimator.h"
#include "Animators/qrealanimator.h"
#include "Animators/gradient.h"
#include "Properties/comboboxproperty.h"
#include "PathEffects/patheffectcollection.h"
#include "PathEffects/patheffect.h"
#include "Boxes/boundingbox.h"
#include "Boxes/pathbox.h"
#include "Boxes/containerbox.h"
#include "Boxes/rectangle.h"
#include "Boxes/circle.h"
#include "Boxes/imagebox.h"
#include "Boxes/videobox.h"
#include "Boxes/textbox.h"
#include "Boxes/imagesequencebox.h"
#include "Boxes/internallinkbox.h"
//#include "Boxes/paintbox.h"
//#include "GUI/BrushWidgets/brushsettingswidget.h"
#include "canvas.h"
#include "Timeline/durationrectangle.h"
#include "Animators/gradientpoints.h"
#include "MovablePoints/gradientpoint.h"
#include "Animators/qrealkey.h"
#include "GUI/mainwindow.h"
#include "GUI/canvaswindow.h"
#include "gradientwidgets/gradientwidget.h"
#include <QMessageBox>
#include "PathEffects/patheffectsinclude.h"
#include "Boxes/internallinkcanvas.h"
#include "Boxes/smartvectorpath.h"
#include "Sound/soundcomposition.h"
#include "RasterEffects/rastereffectcollection.h"
#include "ReadWrite/filefooter.h"
#include "GUI/timelinedockwidget.h"
#include "GUI/RenderWidgets/renderwidget.h"
#include "GUI/BoxesList/boxscrollwidget.h"
#include "ReadWrite/evformat.h"
#include "ReadWrite/ereadstream.h"
#include "ReadWrite/ewritestream.h"
#include "XML/runtimewriteid.h"

void MainWindow::loadEVFile(const QString &path)
{
    QFile file(path);
    if (!file.exists()) { RuntimeThrow("File does not exist " + path); }
    if (!file.open(QIODevice::ReadOnly)) {
        RuntimeThrow("Could not open file " + path);
    }
    try {
        const int evVersion = FileFooter::sReadEvFileVersion(&file);
        if (evVersion <= 0) { RuntimeThrow("Incompatible or incomplete data"); }

        if (evVersion > EvFormat::version) {
            QMessageBox::warning(this,
                                 tr("Unsupported project version"),
                                 tr("This project file (version %1) is not compatible"
                                    " with this version of Friction,"
                                    " max supported project version is %2.").arg(QString::number(evVersion),
                                                                                 QString::number(EvFormat::version)));
            file.close();
            return;
        }

        eReadStream readStream(evVersion, &file);
        readStream.setPath(path);

        const qint64 savedPos = file.pos();
        const qint64 pos = file.size() - FileFooter::sSize(evVersion) -
                qint64(sizeof(int));
        file.seek(pos);
        readStream.readFutureTable();
        file.seek(savedPos);
        readStream.readCheckpoint("File beginning pos mismatch");
        if(evVersion >= EvFormat::betterSWTAbsReadWrite) {
            int nScenes; readStream >> nScenes;
            for(int i = 0; i < nScenes; i++) {
                const bool beforeContent = (evVersion >= EvFormat::readSceneSettingsBeforeContent);
                const auto scene = mDocument.createNewScene(!beforeContent);
                if (beforeContent) {
                    scene->readSettings(readStream);
                    mDocument.sceneCreated(scene);
                }
            }
            mLayoutHandler->read(readStream);
            readStream.readCheckpoint("Error reading Layout");
        }
        mDocument.readScenes(readStream);
        readStream.readCheckpoint("Error reading Document");
        if(evVersion >= EvFormat::betterSWTAbsReadWrite) {
            //const auto renderWidget = mTimeline->getRenderWidget();
            mRenderWidget->read(readStream);
            readStream.readCheckpoint("Error reading Render Widget");
        }
    } catch(...) {
        file.close();
        RuntimeThrow("Error while reading from file " + path);
    }
    file.close();
    addRecentFile(path);
    mRenderWidget->updateRenderSettings();
}

void MainWindow::saveToFile(const QString &path,
                            const bool addRecent)
{
    QFile file(path);
    if (file.exists()) { file.remove(); }

    // check if folder exists first
    QFileInfo info(path);
    QDir dir = info.absoluteDir();
    if (!dir.exists()) {
        if (!dir.mkpath(dir.absolutePath())) {
            RuntimeThrow(tr("Unable to create directory: %1").arg(dir.absolutePath()));
        }
    }

    if (!file.open(QIODevice::WriteOnly)) {
        RuntimeThrow("Could not open file for writing " + path + ".");
    }
    eWriteStream writeStream(&file);
    writeStream.setPath(path);
    try {
        writeStream.writeCheckpoint();
        const auto& scenes = mDocument.fScenes;
        writeStream << scenes.count();
        for (const auto& scene : scenes) {
            scene->writeSettings(writeStream);
        }
        mLayoutHandler->write(writeStream);
        writeStream.writeCheckpoint();
        mDocument.writeScenes(writeStream);
        writeStream.writeCheckpoint();
        //const auto renderWidget = mTimeline->getRenderWidget();
        mRenderWidget->write(writeStream);
        writeStream.writeCheckpoint();

        writeStream.writeFutureTable();
        FileFooter::sWrite(writeStream);
    } catch(...) {
        file.close();
        RuntimeThrow("Error while writing to file " + path);
    }
    file.close();

    BoundingBox::sClearWriteBoxes();
    if (addRecent) { addRecentFile(path); }
}

#include "XML/xevzipfilesaver.h"

void MainWindow::saveToFileXEV(const QString &path) {
    try {
        const auto xevfileSaver = std::make_shared<XevZipFileSaver>();
        xevfileSaver->setPath(path);
        auto& fileSaver = xevfileSaver->fileSaver();

        fileSaver.processText("mimetype", [](QTextStream& stream) {
            stream << "application/enve";
        }, false);

        fileSaver.process("Thumbnails/thumbnail.png", [this](QIODevice* const dst) {
            const qreal scale = 256./width();
            QImage img(qRound(width()*scale),
                       qRound(height()*scale),
                       QImage::Format_RGB888);
            QPainter p(&img);
            p.scale(scale, scale);
            render(&p);
            img.save(dst, "PNG");
        }, false);

        RuntimeIdToWriteId objListIdConv;
        objListIdConv.assign(mObjectSettingsWidget->getId());
        fileSaver.processText("UI/layouts.xml", [&](QTextStream& stream) {
            QDomDocument doc;
            auto ele = doc.createElement("Layouts");
            mLayoutHandler->writeXEV(ele, doc, objListIdConv);
            doc.appendChild(ele);
            stream << doc.toString();
        });

        mDocument.writeXEV(xevfileSaver, objListIdConv);
    } catch(...) {
        RuntimeThrow("Error while writing to file " + path);
    }

    BoundingBox::sClearWriteBoxes();
    addRecentFile(path);
}

void MainWindow::loadXevFile(const QString &path) {
    try {
        ZipFileLoader fileLoader;
        fileLoader.setZipPath(path);

        QList<Canvas*> scenes;
        mDocument.readDocumentXEV(fileLoader, scenes);

        XevReadBoxesHandler boxReadHandler;
        RuntimeIdToWriteId objListIdConv;
        objListIdConv.assign(mObjectSettingsWidget->getId());
        fileLoader.process("UI/layouts.xml", [&](QIODevice* const src) {
            QDomDocument doc;
            doc.setContent(src);
            const auto ele = doc.firstChildElement("Layouts");
            mLayoutHandler->readXEV(boxReadHandler, ele, objListIdConv);
        });

        mDocument.readScenesXEV(boxReadHandler, fileLoader, scenes, objListIdConv);
    } catch(...) {
        RuntimeThrow("Error while reading from file " + path);
    }

    addRecentFile(path);
}
