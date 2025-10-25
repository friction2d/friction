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

#include "ReadWrite/xevformat.h"
#include "ReadWrite/evformat.h"
#include "XML/xmlexporthelpers.h"
#include "Animators/gradient.h"
#include "Paint/brushescontext.h"
#include "simpletask.h"
#include "canvas.h"
#include "gridcontroller.h"
#include "smartPointers/ememory.h"
#include "appsupport.h"
#include <QtGlobal>
#include <QColor>

using Friction::Core::GridSettings;

void Document::writeBookmarked(eWriteStream &dst) const {
    dst << fColors.count();
    for(const auto &col : fColors) {
        dst << col;
    }

    dst << fBrushes.count();
    for(const auto &brush : fBrushes) {
        dst << brush;
    }
}

void Document::writeGridSettings(eWriteStream &dst) const
{
    const auto& s = mGridController.settings;
    dst << s.sizeX;
    dst << s.sizeY;
    dst << s.originX;
    dst << s.originY;
    dst << s.snapThresholdPx;
    dst << s.enabled;
    dst << s.show;
    dst << s.majorEveryX;
    dst << s.majorEveryY;
    const QColor color = s.colorAnimator ? s.colorAnimator->getColor() : Friction::Core::GridSettings::defaults().colorAnimator->getColor();
    const QColor majorColor = s.majorColorAnimator ? s.majorColorAnimator->getColor() : Friction::Core::GridSettings::defaults().majorColorAnimator->getColor();
    dst << color;
    dst << majorColor;
}


void Document::writeScenes(eWriteStream& dst) const {
    writeGridSettings(dst);
    dst.writeCheckpoint();

    writeBookmarked(dst);
    dst.writeCheckpoint();

    const int nScenes = fScenes.count();
    dst.write(&nScenes, sizeof(int));
    for(const auto &scene : fScenes) {
        scene->writeBoundingBox(dst);
        dst.writeCheckpoint();
    }
}

void Document::readBookmarked(eReadStream &src) {
    int nCol; src >> nCol;
    for(int i = 0; i < nCol; i++) {
        QColor col; src >> col;
        addBookmarkColor(col);
    }

    int nBrush; src >> nBrush;
    for(int i = 0; i < nBrush; i++) {
        SimpleBrushWrapper* brush; src >> brush;
        if(brush) addBookmarkBrush(brush);
    }
}

void Document::readGridSettings(eReadStream &src)
{
    GridSettings settings = mGridController.settings;
    src >> settings.sizeX;
    src >> settings.sizeY;
    src >> settings.originX;
    src >> settings.originY;
    src >> settings.snapThresholdPx;
    bool enabled = mGridController.settings.enabled;
    bool show = mGridController.settings.show;
    src >> enabled;
    src >> show;
    const int fileVersion = src.evFileVersion();
    if (fileVersion >= EvFormat::grids) {
        src >> settings.majorEveryX;
        src >> settings.majorEveryY;
    }
    QColor color;
    src >> color;
    QColor majorColor = color;
    if (fileVersion >= EvFormat::grids) {
        src >> majorColor;
    }
    settings.enabled = enabled;
    settings.show = show;
    if (!settings.colorAnimator) {
        settings.colorAnimator = enve::make_shared<ColorAnimator>();
    }
    settings.colorAnimator->setColor(color);
    if (!settings.majorColorAnimator) {
        settings.majorColorAnimator = enve::make_shared<ColorAnimator>();
    }
    settings.majorColorAnimator->setColor(majorColor);
    applyGridSettings(settings, false, true);
}


void Document::readGradients(eReadStream& src) {
    int nGrads; src >> nGrads;
    for(int i = 0; i < nGrads; i++) {
        enve::make_shared<SceneBoundGradient>(nullptr)->read(src);
    }
}

void Document::readScenes(eReadStream& src) {
    if (src.evFileVersion() >= EvFormat::grids) {
        readGridSettings(src);
        src.readCheckpoint("Error reading grid settings");
    }
    if(src.evFileVersion() > 1) {
        readBookmarked(src);
        src.readCheckpoint("Error reading bookmarks");
    }
    if(src.evFileVersion() <= 5) {
        readGradients(src);
        src.readCheckpoint("Error reading gradients");
    }

    int nScenes;
    src.read(&nScenes, sizeof(int));
    for(int i = 0; i < nScenes; i++) {
        Canvas* scene;
        if(src.evFileVersion() < EvFormat::betterSWTAbsReadWrite) {
            scene = createNewScene();
        } else {
            scene = fScenes.at(fScenes.count() - nScenes + i).get();
        }
        const auto block = scene->blockUndoRedo();
        scene->readBoundingBox(src);
        src.readCheckpoint("Error reading scene");
    }

    SimpleTask::sProcessAll();
}

void Document::readGridSettings(const QDomElement& element)
{
    if (element.isNull()) { return; }
    GridSettings settings = mGridController.settings;
    bool ok = false;
    settings.sizeX = element.attribute("sizeX", QString::number(settings.sizeX)).toDouble(&ok);
    settings.sizeY = element.attribute("sizeY", QString::number(settings.sizeY)).toDouble(&ok);
    settings.originX = element.attribute("originX", QString::number(settings.originX)).toDouble(&ok);
    settings.originY = element.attribute("originY", QString::number(settings.originY)).toDouble(&ok);
    settings.snapThresholdPx = element.attribute("snapThresholdPx", QString::number(settings.snapThresholdPx)).toInt();
    settings.enabled = element.attribute("enabled", settings.enabled ? "true" : "false") == "true";
    settings.show = element.attribute("show", settings.show ? "true" : "false") == "true";
    auto parseMajorAttr = [](const QDomElement& elem,
                             const QString& attribute,
                             int currentValue,
                             bool& applied)
    {
        applied = false;
        if (!elem.hasAttribute(attribute)) { return currentValue; }
        bool okValue = false;
        const int parsed = elem.attribute(attribute).toInt(&okValue);
        if (okValue && parsed > 0) {
            applied = true;
            return parsed;
        }
        return currentValue;
    };
    bool appliedMajorX = false;
    bool appliedMajorY = false;
    settings.majorEveryX = parseMajorAttr(element, "majorEveryX", settings.majorEveryX, appliedMajorX);
    settings.majorEveryY = parseMajorAttr(element, "majorEveryY", settings.majorEveryY, appliedMajorY);
    if ((!appliedMajorX || !appliedMajorY) && element.hasAttribute("majorEvery")) {
        bool okLegacy = false;
        const int legacy = element.attribute("majorEvery").toInt(&okLegacy);
        if (okLegacy && legacy > 0) {
            if (!appliedMajorX) { settings.majorEveryX = legacy; }
            if (!appliedMajorY) { settings.majorEveryY = legacy; }
        }
    }
    const QString colorStr = element.attribute("color");
    if (!colorStr.isEmpty()) {
        const QColor parsed(colorStr);
        if (parsed.isValid()) {
            if (!settings.colorAnimator) { settings.colorAnimator = enve::make_shared<ColorAnimator>(); }
            settings.colorAnimator->setColor(parsed);
        }
    }
    const QString majorColorStr = element.attribute("majorColor");
    if (!majorColorStr.isEmpty()) {
        const QColor parsed(majorColorStr);
        if (parsed.isValid()) {
            if (!settings.majorColorAnimator) { settings.majorColorAnimator = enve::make_shared<ColorAnimator>(); }
            settings.majorColorAnimator->setColor(parsed);
        }
    }
    applyGridSettings(settings, false, true);
}


void Document::writeDoxumentXEV(QDomDocument& doc) const {
    auto document = doc.createElement("Document");
    document.setAttribute("format-version", XevFormat::version);

    auto bColors = doc.createElement("ColorBookmarks");
    for(const auto &col : fColors) {
        auto color = doc.createElement("Color");
        color.setAttribute("name", col.name());
        bColors.appendChild(color);
    }
    document.appendChild(bColors);

    auto bBrushes = doc.createElement("BrushBookmarks");
    for(const auto &b : fBrushes) {
        const auto brush = XevExportHelpers::brushToElement(b, doc);
        bBrushes.appendChild(brush);
    }
    document.appendChild(bBrushes);

    auto gridSettings = doc.createElement("GridSettings");
    const auto& grid = mGridController.settings;
    gridSettings.setAttribute("sizeX", QString::number(grid.sizeX));
    gridSettings.setAttribute("sizeY", QString::number(grid.sizeY));
    gridSettings.setAttribute("originX", QString::number(grid.originX));
    gridSettings.setAttribute("originY", QString::number(grid.originY));
    gridSettings.setAttribute("snapThresholdPx", QString::number(grid.snapThresholdPx));
    gridSettings.setAttribute("enabled", grid.enabled ? "true" : "false");
    gridSettings.setAttribute("show", grid.show ? "true" : "false");
    gridSettings.setAttribute("majorEveryX", QString::number(grid.majorEveryX));
    gridSettings.setAttribute("majorEveryY", QString::number(grid.majorEveryY));
    // Legacy attribute for older consumers expecting a unified value.
    gridSettings.setAttribute("majorEvery", QString::number(grid.majorEveryX));
    const QColor gridColor = grid.colorAnimator ? grid.colorAnimator->getColor() : Friction::Core::GridSettings::defaults().colorAnimator->getColor();
    const QColor gridMajorColor = grid.majorColorAnimator ? grid.majorColorAnimator->getColor() : Friction::Core::GridSettings::defaults().majorColorAnimator->getColor();
    gridSettings.setAttribute("color", gridColor.name(QColor::HexArgb));
    gridSettings.setAttribute("majorColor", gridMajorColor.name(QColor::HexArgb));
    document.appendChild(gridSettings);

    auto scenes = doc.createElement("Scenes");
    for(const auto &s : fScenes) {
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

void Document::writeScenesXEV(const std::shared_ptr<XevZipFileSaver>& xevFileSaver,
                              const RuntimeIdToWriteId& objListIdConv) const {
    int id = 0;
    for(const auto &s : fScenes) {
        const QString path = "scenes/" + QString::number(id++) + "/";
        s->writeBoxOrSoundXEV(xevFileSaver, objListIdConv, path);
    }
}

void Document::writeXEV(const std::shared_ptr<XevZipFileSaver>& xevFileSaver,
                        const RuntimeIdToWriteId& objListIdConv) const {
    auto& fileSaver = xevFileSaver->fileSaver();
    fileSaver.processText("document.xml", [&](QTextStream& stream) {
        QDomDocument document;
        writeDoxumentXEV(document);
        stream << document.toString();
    });
    writeScenesXEV(xevFileSaver, objListIdConv);
}

void Document::readDocumentXEV(ZipFileLoader& fileLoader,
                               QList<Canvas*>& scenes) {
    fileLoader.process("document.xml", [&](QIODevice* const src) {
        QDomDocument document;
        document.setContent(src);
        readDocumentXEV(document, scenes);
    });
}

void Document::readDocumentXEV(const QDomDocument& doc,
                               QList<Canvas*>& scenes) {
    const auto document = doc.firstChildElement("Document");
    const QString versionStr = document.attribute("format-version", "");
    if(versionStr.isEmpty()) RuntimeThrow("No format version specified");
//    const int version = XmlExportHelpers::stringToInt(versionStr);

    const auto gridElement = document.firstChildElement("GridSettings");
    if (!gridElement.isNull()) {
        readGridSettings(gridElement);
    }

    auto bColors = document.firstChildElement("ColorBookmarks");
    const auto colors = bColors.elementsByTagName("Color");
    const int nColors = colors.count();
    for(int i = 0; i < nColors; i++) {
        const auto color = colors.at(i);
        if(!color.isElement()) continue;
        const auto colorEle = color.toElement();
        const QString name = colorEle.attribute("name");
        if(name.isEmpty()) continue;
        addBookmarkColor(QColor(name));
    }

    auto bBrushes = document.firstChildElement("BrushBookmarks");
    const auto brushes = bBrushes.elementsByTagName("Brush");
    const int nBrushes = brushes.count();
    for(int i = 0; i < nBrushes; i++) {
        const auto brush = brushes.at(i);
        if(!brush.isElement()) continue;
        const auto brushEle = brush.toElement();
        const auto brushPtr = XevExportHelpers::brushFromElement(brushEle);
        if(brushPtr) addBookmarkBrush(brushPtr);
    }

    auto scenesE = document.firstChildElement("Scenes");
    const auto sceneEles = scenesE.elementsByTagName("Scene");
    const int nScenes = sceneEles.count();
    for(int i = 0; i < nScenes; i++) {
        const auto sceneNode = sceneEles.at(i);
        if(!sceneNode.isElement()) continue;
        const auto sceneEle = sceneNode.toElement();

        const auto resStr = sceneEle.attribute("resolution");
        const qreal res = XmlExportHelpers::stringToDouble(resStr);
        const int frame = XmlExportHelpers::stringToInt(sceneEle.attribute("frame"));
        const int width = XmlExportHelpers::stringToInt(sceneEle.attribute("width"));
        const int height = XmlExportHelpers::stringToInt(sceneEle.attribute("height"));
        const qreal fps = XmlExportHelpers::stringToDouble(sceneEle.attribute("fps"));
        const bool clip = sceneEle.attribute("clip") == "true";
        const auto rangeStr = sceneEle.attribute("frameRange", "0 200");
        const auto rangeStrs = rangeStr.split(' ', Qt::SkipEmptyParts);
        if(rangeStrs.count() != 2) RuntimeThrow("Invalid frame range " + rangeStr);
        const int rangeMin = XmlExportHelpers::stringToInt(rangeStrs[0]);
        const int rangeMax = XmlExportHelpers::stringToInt(rangeStrs[1]);

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

void Document::readScenesXEV(XevReadBoxesHandler& boxReadHandler,
                             ZipFileLoader& fileLoader,
                             const QList<Canvas*>& scenes,
                             const RuntimeIdToWriteId& objListIdConv) {
    int id = 0;
    for(const auto& scene : scenes) {
        const auto block = scene->blockUndoRedo();
        const QString path = "scenes/" + QString::number(id++) + "/";
        scene->readBoxOrSoundXEV(boxReadHandler, fileLoader,
                                 path, objListIdConv);
    }
}
