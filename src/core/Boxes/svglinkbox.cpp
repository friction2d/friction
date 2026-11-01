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

#include "svglinkbox.h"

#include "fileshandler.h"
#include "svgimporter.h"
#include "Animators/gradient.h"
#include "appsupport.h"

SvgFileCacheHandler* svgFileHandlerGetter(const QString& path) {
    return FilesHandler::sInstance->getFileHandler<SvgFileCacheHandler>(path);
}

SvgLinkBox::SvgLinkBox() :
    SvgLinkBoxBase([](const QString& path) {
                       return svgFileHandlerGetter(path);
                   },
                   [this](SvgFileCacheHandler* obj) {
                       return fileHandlerAfterAssigned(obj);
                   },
                   [this](ConnContext& conn, SvgFileCacheHandler* obj) {
                       fileHandlerConnector(conn, obj);
                   }) {
    mType = eBoxType::svgLink;
}

void SvgLinkBox::changeSourceFile()
{
    const QString path = AppSupport::getOpenFile(nullptr,
                                                 tr("Change Source"),
                                                 getFilePath(),
                                                 tr("SVG Files (*.svg)"));
    if (!path.isEmpty()) { setFilePath(path); }
}

void SvgLinkBox::updateContent() {
    removeAllContained();
    mGradients.clear();
    const auto obj = fileHandler();
    if(obj) {
        const auto gradientCreator = [this]() {
            const auto grad = enve::make_shared<Gradient>();
            mGradients << grad;
            return grad.get();
        };
        const auto imported = ImportSVG::loadSVGFile(obj->path(), gradientCreator);
        if(imported) addContained(imported);
    }
}

void SvgLinkBox::fileHandlerConnector(ConnContext &conn, SvgFileCacheHandler *obj) {
    if(obj) {
        conn << connect(obj, &SvgFileCacheHandler::pathChanged,
                        this, &SvgLinkBox::updateContent);
        conn << connect(obj, &SvgFileCacheHandler::reloaded,
                        this, &SvgLinkBox::updateContent);
    }
}

void SvgLinkBox::fileHandlerAfterAssigned(SvgFileCacheHandler *obj) {
    Q_UNUSED(obj)
    updateContent();
}
