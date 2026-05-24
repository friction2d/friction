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
#include "Animators/transformanimator.h"
#include "Animators/qpointfanimator.h"
#include "ReadWrite/evformat.h"
#include "swt_abstraction.h"
#include "typemenu.h"
#include "XML/xevexporter.h"

#include <QInputDialog>
#include <QLoggingCategory>
#include <yaml-cpp/yaml.h>

Q_LOGGING_CATEGORY(lcSvgPivot, "friction.svgpivot", QtWarningMsg)

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

static void collectAnimationNodes(BoundingBox* box,
                                   QList<BoundingBox*>& result) {
    for (const auto& doc : box->getDescYaml()) {
        if (!doc.isYaml) continue;
        try {
            const auto node = YAML::Load(doc.content.toStdString());
            if (node["kind"] && node["kind"].as<std::string>() == "animation-node") {
                result << box;
                break;
            }
        } catch (...) {}
    }
    if (const auto container = enve_cast<ContainerBox*>(box)) {
        for (const auto& child : container->getContainedBoxes()) {
            collectAnimationNodes(child, result);
        }
    }
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
    if (!svgRoot) return;
    QList<BoundingBox*> animationNodes;
    collectAnimationNodes(svgRoot, animationNodes);
    for (BoundingBox* node : animationNodes) {
        const QString name = node->prp_getName();
        const bool exists = std::any_of(
            mElementTracks.begin(), mElementTracks.end(),
            [&name](const qsptr<SvgElementTrack>& t) {
                return t->prp_getName() == name;
            });
        if (!exists) addElementTrack(name);
    }
    for (const auto& track : mFlipbookTracks) {
        track->setPageMap({});
    }
    collectFlipbookDescs(svgRoot);
    collectPivotDescs(svgRoot);
    for (const auto& track : mFlipbookTracks) {
        track->resolveTargets(svgRoot);
        track->syncToTargets();
    }
}

void SvgLinkBox::collectFlipbookDescs(ContainerBox* container) {
    for (auto* box : container->getContainedBoxes()) {
        for (const auto& doc : box->getDescYaml()) {
            if (!doc.isYaml) continue;
            try {
                const auto node = YAML::Load(doc.content.toStdString());
                if (!node["kind"] || node["kind"].as<std::string>() != "flipbook") continue;
                if (!node["map"]) break;
                QMap<int, QString> pageMap;
                for (const auto& entry : node["map"])
                    pageMap[entry.first.as<int>()] =
                        QString::fromStdString(entry.second.as<std::string>());
                if (pageMap.isEmpty()) break;
                QString ownerId = box->property("svgElementId").toString();
                if (ownerId.isEmpty()) ownerId = box->prp_getName();
                SvgFlipbookTrack* existing = nullptr;
                for (const auto& track : mFlipbookTracks) {
                    if (track->prp_getName() == ownerId) { existing = track.get(); break; }
                }
                if (!existing) {
                    auto track = enve::make_shared<SvgFlipbookTrack>(ownerId);
                    wireFlipbookTrack(track);
                    existing = track.get();
                }
                existing->setPageMap(pageMap);
                break;
            } catch (...) {}
        }
        if (const auto sub = enve_cast<ContainerBox*>(box))
            collectFlipbookDescs(sub);
    }
}

void SvgLinkBox::collectPivotDescs(ContainerBox* container) {
    for (auto* box : container->getContainedBoxes()) {
        qCDebug(lcSvgPivot) << "collectPivotDescs: visiting box"
                            << box->prp_getName()
                            << "svgElementId=" << box->property("svgElementId").toString()
                            << "mCenterPivotPlanned=" << box->isCenterPivotPlanned()
                            << "hasSvgPivotPos=" << box->property("svgPivotPos").isValid();
        const QVariant pivotVar = box->property("svgPivotPos");
        if (pivotVar.isValid()) {
            const QPointF pivot = pivotVar.toPointF();
            const QString boxName = box->prp_getName();
            const QString svgId = box->property("svgElementId").toString();
            qCDebug(lcSvgPivot) << "collectPivotDescs: found pivot" << pivot
                                << "on box" << boxName << "svgId=" << svgId;
            bool found = false;
            for (const auto& track : mElementTracks) {
                const QString tName = track->prp_getName();
                if (tName == boxName || (!svgId.isEmpty() && tName == svgId)) {
                    found = true;
                    auto* target = track->resolvedTarget();
                    if (!target) {
                        qCDebug(lcSvgPivot) << "collectPivotDescs: track" << tName
                                            << "has no resolved target";
                        break;
                    }
                    auto* transformAdv = enve_cast<AdvancedTransformAnimator*>(
                        target->getTransformAnimator());
                    if (!transformAdv) {
                        qCDebug(lcSvgPivot) << "collectPivotDescs: no AdvancedTransformAnimator on"
                                            << tName;
                        break;
                    }
                    qCDebug(lcSvgPivot) << "collectPivotDescs: target=" << target->prp_getName()
                                        << "mCenterPivotPlanned=" << target->isCenterPivotPlanned()
                                        << "pivot before=" << transformAdv->getPivotAnimator()->getEffectiveValue();
                    target->cancelPlannedCenterPivot();
                    transformAdv->getPivotAnimator()->setBaseValue(pivot);
                    qCDebug(lcSvgPivot) << "collectPivotDescs: set pivot on" << tName
                                        << "=" << pivot
                                        << "pivot after=" << transformAdv->getPivotAnimator()->getEffectiveValue()
                                        << "mCenterPivotPlanned after cancel=" << target->isCenterPivotPlanned();
                    break;
                }
            }
            if (!found) {
                qCDebug(lcSvgPivot) << "collectPivotDescs: no element track found for"
                                    << boxName << "(svgId=" << svgId << ")";
            }
        }
        if (const auto sub = enve_cast<ContainerBox*>(box))
            collectPivotDescs(sub);
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

void SvgLinkBox::wireFlipbookTrack(const qsptr<SvgFlipbookTrack>& track) {
    mFlipbookTracks << track;
    track->setParent(this);
    connect(track.get(), &SvgFlipbookTrack::deleteRequested,
            this, [this, t = track.get()]() { removeFlipbookTrack(t); });
    const int swtId = ca_getNumberOfChildren()
                      + mElementTracks.count()
                      + mFlipbookTracks.count() - 1;
    SWT_addChildAt(track.get(), swtId);
}

void SvgLinkBox::removeFlipbookTrack(SvgFlipbookTrack* track) {
    for (int i = 0; i < mFlipbookTracks.count(); i++) {
        if (mFlipbookTracks[i].get() == track) {
            SWT_removeChild(track);
            mFlipbookTracks.removeAt(i);
            return;
        }
    }
}

void SvgLinkBox::anim_setAbsFrame(const int frame) {
    SvgLinkBoxBase::anim_setAbsFrame(frame);
    qCDebug(lcSvgElementTrack) << "anim_setAbsFrame" << frame
                               << "tracks:" << mElementTracks.count();
    for (const auto& track : mElementTracks) {
        track->anim_setAbsFrame(frame);
        auto* target = track->resolvedTarget();
        qCDebug(lcSvgElementTrack) << "  track:" << track->prp_getName()
                                   << "resolvedTarget:" << (target ? target->prp_getName() : "(null)");
        if (target) {
            track->syncToTarget(target);
        }
    }
    for (const auto& track : mFlipbookTracks) {
        track->anim_setAbsFrame(frame);
        track->syncToTargets();
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
    dst << (qint32)mFlipbookTracks.count();
    for (const auto& track : mFlipbookTracks) {
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
    if (src.evFileVersion() >= EvFormat::svgLinkFlipbookTracks) {
        qint32 count; src >> count;
        for (int i = 0; i < count; i++) {
            auto track = enve::make_shared<SvgFlipbookTrack>("");
            track->readTrack(src);
            wireFlipbookTrack(track);
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
    for (const auto& track : mFlipbookTracks) {
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
    if (!mFlipbookTracks.isEmpty()) {
        auto tracksEle = exp.createElement("FlipbookTracks");
        for (const auto& track : mFlipbookTracks) {
            auto trackEle = exp.createElement("FlipbookTrack");
            trackEle.setAttribute("ownerId", track->prp_getName());
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
    const auto flipbookTracksEle = ele.firstChildElement("FlipbookTracks");
    if (!flipbookTracksEle.isNull()) {
        auto trackEle = flipbookTracksEle.firstChildElement("FlipbookTrack");
        while (!trackEle.isNull()) {
            const QString id = trackEle.attribute("ownerId");
            if (!id.isEmpty()) {
                auto track = enve::make_shared<SvgFlipbookTrack>(id);
                wireFlipbookTrack(track);
            }
            trackEle = trackEle.nextSiblingElement("FlipbookTrack");
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
