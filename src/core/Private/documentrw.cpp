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

#include "Private/document.h"

#include "XML/xmlformat.h"
#include "ReadWrite/evformat.h"
#include "XML/xmlexporthelpers.h"
#include "Animators/gradient.h"
#include "Paint/brushescontext.h"
#include "simpletask.h"
#include "canvas.h"
#include "appsupport.h"

using namespace Friction;

void Document::writeBookmarked(eWriteStream &dst) const
{
    dst << fColors.count();
    for (const auto &col : fColors) { dst << col; }

    dst << fBrushes.count();
    for (const auto &brush : fBrushes) { dst << brush; }
}

void Document::writeScenes(eWriteStream& dst) const
{
    writeBookmarked(dst);
    dst.writeCheckpoint();

    const int nScenes = fScenes.count();
    dst.write(&nScenes, sizeof(int));
    for (const auto &scene : fScenes) {
        scene->writeBoundingBox(dst);
        dst.writeCheckpoint();
    }
}

void Document::readBookmarked(eReadStream &src)
{
    int nCol; src >> nCol;
    for (int i = 0; i < nCol; i++) {
        QColor col; src >> col;
        addBookmarkColor(col);
    }

    int nBrush; src >> nBrush;
    for (int i = 0; i < nBrush; i++) {
        SimpleBrushWrapper* brush; src >> brush;
        if (brush) { addBookmarkBrush(brush); }
    }
}

void Document::readGradients(eReadStream& src)
{
    int nGrads; src >> nGrads;
    for (int i = 0; i < nGrads; i++) {
        enve::make_shared<SceneBoundGradient>(nullptr)->read(src);
    }
}

void Document::readScenes(eReadStream& src)
{
    if (src.evFileVersion() > 1) {
        readBookmarked(src);
        src.readCheckpoint("Error reading bookmarks");
    }
    if (src.evFileVersion() <= 5) {
        readGradients(src);
        src.readCheckpoint("Error reading gradients");
    }

    int nScenes;
    src.read(&nScenes, sizeof(int));
    for (int i = 0; i < nScenes; i++) {
        Canvas* scene;
        if (src.evFileVersion() < EvFormat::betterSWTAbsReadWrite) { scene = createNewScene(); }
        else { scene = fScenes.at(fScenes.count() - nScenes + i).get(); }
        const auto block = scene->blockUndoRedo();
        scene->readBoundingBox(src);
        src.readCheckpoint("Error reading scene");
    }

    SimpleTask::sProcessAll();
}

void Document::writeDoxumentXML(QDomDocument& doc) const
{
    auto document = doc.createElement("Document");
    document.setAttribute("format-version",
                          Core::XmlFormat::version);

    auto bColors = doc.createElement("ColorBookmarks");
    for (const auto &col : fColors) {
        auto color = doc.createElement("Color");
        color.setAttribute("name", col.name());
        bColors.appendChild(color);
    }
    document.appendChild(bColors);

    // REMOVE
    /*auto bBrushes = doc.createElement("BrushBookmarks");
    for(const auto &b : fBrushes) {
        const auto brush = Core::XmlExportHelpers::brushToElement(b, doc);
        bBrushes.appendChild(brush);
    }
    document.appendChild(bBrushes);*/

    auto scenes = doc.createElement("Scenes");
    for (const auto &s : fScenes) {
        auto scene = doc.createElement("Scene");
        const qreal resolution = s->getResolution();
        scene.setAttribute("resolution", QString::number(resolution));
        scene.setAttribute("name", s->prp_getName());
        scene.setAttribute("frame", s->getCurrentFrame());
        scene.setAttribute("width", s->getCanvasWidth());
        scene.setAttribute("height", s->getCanvasHeight());
        scene.setAttribute("fps", s->getFps());
        scene.setAttribute("clip", s->clipToCanvas() ? "true" : "false");
        const auto range = s->getFrameRange();
        const auto rangeStr = QString("%1 %2").arg(range.fMin).arg(range.fMax);
        scene.setAttribute("frameRange", rangeStr);

        scenes.appendChild(scene);
    }
    document.appendChild(scenes);

    doc.appendChild(document);
}

void Document::writeScenesXML(const std::shared_ptr<Core::XmlZipFileSaver>& xevFileSaver,
                              const Core::RuntimeIdToWriteId& objListIdConv) const
{
    int id = 0;
    for (const auto &s : fScenes) {
        const QString path = "scenes/" + QString::number(id++) + "/";
        s->writeBoxOrSoundXML(xevFileSaver, objListIdConv, path);
    }
}

void Document::writeXML(const std::shared_ptr<Core::XmlZipFileSaver>& xevFileSaver,
                        const Core::RuntimeIdToWriteId& objListIdConv) const
{
    auto& fileSaver = xevFileSaver->fileSaver();
    fileSaver.processText("document.xml", [&](QTextStream& stream) {
        QDomDocument document;
        writeDoxumentXML(document);
        stream << document.toString();
    });
    writeScenesXML(xevFileSaver, objListIdConv);
}

void Document::readDocumentXML(Core::ZipFileLoader& fileLoader,
                               QList<Canvas*>& scenes)
{
    fileLoader.process("document.xml", [&](QIODevice* const src) {
        QDomDocument document;
        document.setContent(src);
        readDocumentXML(document, scenes);
    });
}

void Document::readDocumentXML(const QDomDocument& doc,
                               QList<Canvas*>& scenes)
{
    const auto document = doc.firstChildElement("Document");
    const QString versionStr = document.attribute("format-version", "");
    if (versionStr.isEmpty()) { RuntimeThrow("No format version specified"); }
//    const int version = Core::XmlExportHelpers::stringToInt(versionStr);

    auto bColors = document.firstChildElement("ColorBookmarks");
    const auto colors = bColors.elementsByTagName("Color");
    const int nColors = colors.count();
    for (int i = 0; i < nColors; i++) {
        const auto color = colors.at(i);
        if (!color.isElement()) { continue; }
        const auto colorEle = color.toElement();
        const QString name = colorEle.attribute("name");
        if (name.isEmpty()) { continue; }
        addBookmarkColor(QColor(name));
    }

    // REMOVE
    /*auto bBrushes = document.firstChildElement("BrushBookmarks");
    const auto brushes = bBrushes.elementsByTagName("Brush");
    const int nBrushes = brushes.count();
    for(int i = 0; i < nBrushes; i++) {
        const auto brush = brushes.at(i);
        if(!brush.isElement()) continue;
        const auto brushEle = brush.toElement();
        const auto brushPtr = Core::XmlExportHelpers::brushFromElement(brushEle);
        if(brushPtr) addBookmarkBrush(brushPtr);
    }*/

    auto scenesE = document.firstChildElement("Scenes");
    const auto sceneEles = scenesE.elementsByTagName("Scene");
    const int nScenes = sceneEles.count();
    for (int i = 0; i < nScenes; i++) {
        const auto sceneNode = sceneEles.at(i);
        if (!sceneNode.isElement()) { continue; }
        const auto sceneEle = sceneNode.toElement();

        const auto resStr = sceneEle.attribute("resolution");
        const qreal res = Core::XmlExportHelpers::stringToDouble(resStr);
        const int frame = Core::XmlExportHelpers::stringToInt(sceneEle.attribute("frame"));
        const int width = Core::XmlExportHelpers::stringToInt(sceneEle.attribute("width"));
        const int height = Core::XmlExportHelpers::stringToInt(sceneEle.attribute("height"));
        const qreal fps = Core::XmlExportHelpers::stringToDouble(sceneEle.attribute("fps"));
        const bool clip = sceneEle.attribute("clip") == "true";
        const auto rangeStr = sceneEle.attribute("frameRange", "0 200");
        const auto rangeStrs = rangeStr.split(' ', Qt::SkipEmptyParts);
        if (rangeStrs.count() != 2) { RuntimeThrow("Invalid frame range " + rangeStr); }
        const int rangeMin = Core::XmlExportHelpers::stringToInt(rangeStrs[0]);
        const int rangeMax = Core::XmlExportHelpers::stringToInt(rangeStrs[1]);

        const auto newScene = createNewScene();
        newScene->setResolution(res);
        newScene->prp_setName(sceneEle.attribute("name"));
        newScene->anim_setAbsFrame(frame);
        newScene->setCanvasSize(width, height);
        newScene->setFps(fps);
        newScene->setClipToCanvas(clip);
        newScene->setFrameRange(FrameRange{rangeMin, rangeMax}, false);

        scenes << newScene;
    }
}

void Document::readScenesXML(Core::XmlReadBoxesHandler& boxReadHandler,
                             Core::ZipFileLoader& fileLoader,
                             const QList<Canvas*>& scenes,
                             const Core::RuntimeIdToWriteId& objListIdConv)
{
    int id = 0;
    for (const auto& scene : scenes) {
        const auto block = scene->blockUndoRedo();
        const QString path = "scenes/" + QString::number(id++) + "/";
        scene->readBoxOrSoundXML(boxReadHandler, fileLoader,
                                 path, objListIdConv);
    }
}
