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
#include "ReadWrite/evformat.h"
#include "swt_abstraction.h"
#include "typemenu.h"
#include "XML/xevexporter.h"

#include <QInputDialog>

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

#include "GUI/edialogs.h"
void SvgLinkBox::changeSourceFile() {
    const QString path = eDialogs::openFile(
                "Change Source", getFilePath(),
                "SVG Files (*.svg)");
    if(!path.isEmpty()) setFilePath(path);
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
    resolveElementTracks();
}

void SvgLinkBox::resolveElementTracks() {
    ContainerBox* svgRoot = nullptr;
    const auto& contained = getContainedBoxes();
    if (!contained.isEmpty()) {
        svgRoot = enve_cast<ContainerBox*>(contained.first());
    }
    for (const auto& track : mElementTracks) {
        BoundingBox* targetBox = track->resolveTarget(svgRoot);
        if (targetBox) {
            track->reconcileWithTarget(targetBox);
            track->syncToTarget(targetBox);
        }
    }
}

void SvgLinkBox::addElementTrack(const QString& targetId) {
    auto track = enve::make_shared<SvgElementTrack>(targetId);
    wireTrack(track);
    ContainerBox* svgRoot = nullptr;
    const auto& contained = getContainedBoxes();
    if (!contained.isEmpty()) svgRoot = enve_cast<ContainerBox*>(contained.first());
    BoundingBox* targetBox = track->resolveTarget(svgRoot);
    if (targetBox) {
        track->reconcileWithTarget(targetBox);
        track->initFromTarget(targetBox);
    }
}

void SvgLinkBox::wireTrack(const qsptr<SvgElementTrack>& track) {
    mElementTracks << track;
    track->setParent(this);
    connect(track.get(), &Property::prp_nameChanged,
            this, [this](const QString&) { resolveElementTracks(); });
    connect(track.get(), &SvgElementTrack::deleteRequested,
            this, [this, t = track.get()]() { removeElementTrack(t); });
    const int swtId = ca_getNumberOfChildren() + mElementTracks.count() - 1;
    SWT_addChildAt(track.get(), swtId);
}

void SvgLinkBox::anim_setAbsFrame(const int frame) {
    SvgLinkBoxBase::anim_setAbsFrame(frame);
    for (const auto& track : mElementTracks) {
        track->anim_setAbsFrame(frame);
        if (auto* target = track->resolvedTarget()) {
            track->syncToTarget(target);
        }
    }
}

void SvgLinkBox::removeElementTrack(SvgElementTrack* track) {
    for (int i = 0; i < mElementTracks.count(); i++) {
        if (mElementTracks[i].get() == track) {
            SWT_removeChild(track);
            mElementTracks.removeAt(i);
            return;
        }
    }
}

void SvgLinkBox::writeBoundingBox(eWriteStream& dst) const {
    dst << (qint32)mElementTracks.count();
    for (const auto& track : mElementTracks) {
        track->writeTrack(dst);
    }
    SvgLinkBoxBase::writeBoundingBox(dst);
}

void SvgLinkBox::readBoundingBox(eReadStream& src) {
    if (src.evFileVersion() >= EvFormat::svgLinkElementTrackData) {
        qint32 count; src >> count;
        for (int i = 0; i < count; i++) {
            auto track = enve::make_shared<SvgElementTrack>("");
            track->readTrack(src);
            wireTrack(track);
        }
    } else if (src.evFileVersion() >= EvFormat::svgLinkElementTracks) {
        qint32 count; src >> count;
        for (int i = 0; i < count; i++) {
            QString id; src >> id;
            addElementTrack(id);
        }
    }
    SvgLinkBoxBase::readBoundingBox(src);
}

void SvgLinkBox::SWT_setupAbstraction(SWT_Abstraction* abstraction,
                                       const UpdateFuncs& updateFuncs,
                                       const int visiblePartWidgetId) {
    SvgLinkBoxBase::SWT_setupAbstraction(abstraction, updateFuncs, visiblePartWidgetId);
    for (const auto& track : mElementTracks) {
        auto abs = track->SWT_abstractionForWidget(updateFuncs, visiblePartWidgetId);
        abstraction->addChildAbstraction(abs->ref<SWT_Abstraction>());
    }
}

void SvgLinkBox::prp_setupTreeViewMenu(PropertyMenu* menu) {
    if (!menu->hasActionsForType<SvgLinkBox>()) {
        menu->addedActionsForType<SvgLinkBox>();

        const auto parentWidget = menu->getParentWidget();
        menu->addPlainAction(QIcon::fromTheme("plus"),
                             QObject::tr("Add Element Track"),
                             [this, parentWidget]() {
                                 bool ok;
                                 const QString id = QInputDialog::getText(
                                             parentWidget,
                                             QObject::tr("Add Element Track"),
                                             QObject::tr("SVG element id:"),
                                             QLineEdit::Normal, "", &ok);
                                 if (ok && !id.isEmpty()) addElementTrack(id);
                             });

        menu->addSeparator();
    }
    SvgLinkBoxBase::prp_setupTreeViewMenu(menu);
}

QDomElement SvgLinkBox::prp_writePropertyXEV_impl(const XevExporter& exp) const {
    auto result = SvgLinkBoxBase::prp_writePropertyXEV_impl(exp);
    if (!mElementTracks.isEmpty()) {
        auto tracksEle = exp.createElement("ElementTracks");
        for (const auto& track : mElementTracks) {
            auto trackEle = exp.createElement("Track");
            trackEle.setAttribute("targetId", track->prp_getName());
            tracksEle.appendChild(trackEle);
        }
        result.appendChild(tracksEle);
    }
    return result;
}

void SvgLinkBox::prp_readPropertyXEV_impl(const QDomElement& ele,
                                           const XevImporter& imp) {
    const auto tracksEle = ele.firstChildElement("ElementTracks");
    if (!tracksEle.isNull()) {
        auto trackEle = tracksEle.firstChildElement("Track");
        while (!trackEle.isNull()) {
            const QString id = trackEle.attribute("targetId");
            if (!id.isEmpty()) addElementTrack(id);
            trackEle = trackEle.nextSiblingElement("Track");
        }
    }
    SvgLinkBoxBase::prp_readPropertyXEV_impl(ele, imp);
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
