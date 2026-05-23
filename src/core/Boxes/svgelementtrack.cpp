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

#include "svgelementtrack.h"

#include "Boxes/containerbox.h"
#include "Boxes/boundingbox.h"
#include "typemenu.h"
#include "XML/xevexporter.h"
#include "XML/xevimporter.h"
#include "Animators/qrealanimator.h"
#include "Animators/complexanimator.h"
#include "ReadWrite/ewritestream.h"
#include "ReadWrite/ereadstream.h"
#include "ReadWrite/evformat.h"
#include "canvas.h"

#include <QPainter>
#include <QInputDialog>
#include <QBuffer>

SvgElementTrack::SvgElementTrack(const QString& targetId)
    : StaticComplexAnimator(targetId) {
    connect(this, &Property::prp_selectionChanged,
            this, [this]() {
                if (!prp_isSelected()) return;
                if (!mResolvedTarget) return;
                auto* scene = getParentScene();
                if (!scene) return;
                auto* parentGroup = mResolvedTarget->getParentGroup();
                if (!parentGroup) return;
                scene->setCurrentBoxesGroup(parentGroup);
                scene->addBoxToSelection(mResolvedTarget);
            });
}

void SvgElementTrack::setOrphaned(const bool orphaned) {
    if (mOrphaned == orphaned) return;
    mOrphaned = orphaned;
    emit orphanedChanged();
    SWT_scheduleContentUpdate(SWT_BoxRule::all);
}

static BoundingBox* findDescendantByName(ContainerBox* container,
                                         const QString& name) {
    for (auto* box : container->getContainedBoxes()) {
        if (box->prp_getName() == name) return box;
        if (box->property("svgElementId").toString() == name) return box;
        if (const auto sub = enve_cast<ContainerBox*>(box)) {
            if (auto* found = findDescendantByName(sub, name)) return found;
        }
    }
    return nullptr;
}

BoundingBox* SvgElementTrack::resolveTarget(ContainerBox* svgRoot) {
    disconnect(mTargetConn);
    BoundingBox* found = svgRoot ? findDescendantByName(svgRoot, prp_getName())
                                 : nullptr;
    mResolvedTarget = found;
    setOrphaned(!found);
    if (found) {
        mTargetConn = connect(found, &Property::prp_absFrameRangeChanged,
                              this, [this, found]() {
                                  if (!mSyncingToTarget) captureFromTarget(found);
                              });
    }
    return found;
}

// Uses public ca_getNumberOfChildren/ca_getChildAt to avoid protected access
static qsptr<Property> createMatchingProperty(Property* source) {
    if (enve_cast<QrealAnimator*>(source)) {
        return enve::make_shared<QrealAnimator>(source->prp_getName());
    }
    if (const auto c = enve_cast<ComplexAnimator*>(source)) {
        auto clone = enve::make_shared<StaticComplexAnimator>(source->prp_getName());
        const int n = c->ca_getNumberOfChildren();
        for (int i = 0; i < n; i++) {
            auto childClone = createMatchingProperty(c->ca_getChildAt(i));
            if (childClone) clone->ca_addChild(childClone);
        }
        return clone;
    }
    return nullptr;
}

void SvgElementTrack::reconcileWithTarget(BoundingBox* target) {
    if (!target) return;

    QMap<QString, Property*> targetByName;
    const int nTarget = target->ca_getNumberOfChildren();
    for (int i = 0; i < nTarget; i++) {
        auto* prop = target->ca_getChildAt(i);
        targetByName[prop->prp_getName()] = prop;
    }

    QSet<QString> myChildNames;
    for (const auto& child : ca_getChildren()) {
        myChildNames.insert(child->prp_getName());
        if (targetByName.contains(child->prp_getName())) {
            mOrphanedChildren.remove(child.get());
        } else {
            mOrphanedChildren.insert(child.get());
        }
    }

    for (int i = 0; i < nTarget; i++) {
        auto* prop = target->ca_getChildAt(i);
        if (!myChildNames.contains(prop->prp_getName())) {
            auto mirror = createMatchingProperty(prop);
            if (mirror) ca_addChild(mirror);
        }
    }
}

static void syncLeafAnimator(QrealAnimator* src, QrealAnimator* dst) {
    QBuffer buf;
    buf.open(QBuffer::ReadWrite);
    {
        eWriteStream ws(&buf);
        src->prp_writeProperty(ws);
        ws.writeFutureTable();
    }
    buf.seek(0);
    eReadStream rs(EvFormat::version, &buf);
    dst->prp_readProperty(rs);
}

static void syncByName(ComplexAnimator* trackCA, ComplexAnimator* targetCA) {
    QMap<QString, Property*> targetByName;
    const int n = targetCA->ca_getNumberOfChildren();
    for (int i = 0; i < n; i++) {
        auto* p = targetCA->ca_getChildAt(i);
        targetByName[p->prp_getName()] = p;
    }
    const int m = trackCA->ca_getNumberOfChildren();
    for (int i = 0; i < m; i++) {
        auto* sp = trackCA->ca_getChildAt(i);
        auto it = targetByName.find(sp->prp_getName());
        if (it == targetByName.end()) continue;
        auto* tp = it.value();
        if (const auto srcReal = enve_cast<QrealAnimator*>(sp)) {
            if (const auto dstReal = enve_cast<QrealAnimator*>(tp)) {
                syncLeafAnimator(srcReal, dstReal);
            }
        } else if (const auto srcCA = enve_cast<ComplexAnimator*>(sp)) {
            if (const auto dstCA = enve_cast<ComplexAnimator*>(tp)) {
                syncByName(srcCA, dstCA);
            }
        }
    }
}

void SvgElementTrack::syncToTarget(BoundingBox* target) {
    if (!target) return;
    mSyncingToTarget = true;

    QMap<QString, Property*> targetByName;
    const int n = target->ca_getNumberOfChildren();
    for (int i = 0; i < n; i++) {
        auto* p = target->ca_getChildAt(i);
        targetByName[p->prp_getName()] = p;
    }

    for (const auto& trackChild : ca_getChildren()) {
        if (mOrphanedChildren.contains(trackChild.get())) continue;
        auto it = targetByName.find(trackChild->prp_getName());
        if (it == targetByName.end()) continue;
        auto* tp = it.value();
        if (const auto srcReal = enve_cast<QrealAnimator*>(trackChild.get())) {
            if (const auto dstReal = enve_cast<QrealAnimator*>(tp)) {
                syncLeafAnimator(srcReal, dstReal);
            }
        } else if (const auto srcCA = enve_cast<ComplexAnimator*>(trackChild.get())) {
            if (const auto dstCA = enve_cast<ComplexAnimator*>(tp)) {
                syncByName(srcCA, dstCA);
            }
        }
    }
    mSyncingToTarget = false;
}

void SvgElementTrack::initFromTarget(BoundingBox* target) {
    if (!target) return;

    QMap<QString, Property*> targetByName;
    const int n = target->ca_getNumberOfChildren();
    for (int i = 0; i < n; i++) {
        auto* p = target->ca_getChildAt(i);
        targetByName[p->prp_getName()] = p;
    }

    for (const auto& trackChild : ca_getChildren()) {
        auto it = targetByName.find(trackChild->prp_getName());
        if (it == targetByName.end()) continue;
        auto* tp = it.value();
        if (const auto dstReal = enve_cast<QrealAnimator*>(trackChild.get())) {
            if (const auto srcReal = enve_cast<QrealAnimator*>(tp)) {
                syncLeafAnimator(srcReal, dstReal);
            }
        } else if (const auto dstCA = enve_cast<ComplexAnimator*>(trackChild.get())) {
            if (const auto srcCA = enve_cast<ComplexAnimator*>(tp)) {
                syncByName(srcCA, dstCA);
            }
        }
    }
}

static void captureLeafFromTarget(QrealAnimator* src, QrealAnimator* dst) {
    dst->setCurrentBaseValue(src->getEffectiveValue());
}

static void captureByName(ComplexAnimator* trackCA, ComplexAnimator* targetCA) {
    QMap<QString, Property*> targetByName;
    const int n = targetCA->ca_getNumberOfChildren();
    for (int i = 0; i < n; i++) {
        auto* p = targetCA->ca_getChildAt(i);
        targetByName[p->prp_getName()] = p;
    }
    const int m = trackCA->ca_getNumberOfChildren();
    for (int i = 0; i < m; i++) {
        auto* sp = trackCA->ca_getChildAt(i);
        auto it = targetByName.find(sp->prp_getName());
        if (it == targetByName.end()) continue;
        auto* tp = it.value();
        if (const auto dstReal = enve_cast<QrealAnimator*>(sp)) {
            if (const auto srcReal = enve_cast<QrealAnimator*>(tp)) {
                captureLeafFromTarget(srcReal, dstReal);
            }
        } else if (const auto dstCA = enve_cast<ComplexAnimator*>(sp)) {
            if (const auto srcCA = enve_cast<ComplexAnimator*>(tp)) {
                captureByName(dstCA, srcCA);
            }
        }
    }
}

void SvgElementTrack::captureFromTarget(BoundingBox* target) {
    if (!target) return;

    QMap<QString, Property*> targetByName;
    const int n = target->ca_getNumberOfChildren();
    for (int i = 0; i < n; i++) {
        auto* p = target->ca_getChildAt(i);
        targetByName[p->prp_getName()] = p;
    }

    for (const auto& trackChild : ca_getChildren()) {
        if (mOrphanedChildren.contains(trackChild.get())) continue;
        auto it = targetByName.find(trackChild->prp_getName());
        if (it == targetByName.end()) continue;
        auto* tp = it.value();
        if (const auto dstReal = enve_cast<QrealAnimator*>(trackChild.get())) {
            if (const auto srcReal = enve_cast<QrealAnimator*>(tp)) {
                captureLeafFromTarget(srcReal, dstReal);
            }
        } else if (const auto dstCA = enve_cast<ComplexAnimator*>(trackChild.get())) {
            if (const auto srcCA = enve_cast<ComplexAnimator*>(tp)) {
                captureByName(dstCA, srcCA);
            }
        }
    }
}

static void writeTrackProperty(eWriteStream& dst, Property* prop) {
    if (enve_cast<QrealAnimator*>(prop)) {
        dst << (qint32)0;
        dst << prop->prp_getName();
        prop->prp_writeProperty(dst);
    } else if (const auto c = enve_cast<ComplexAnimator*>(prop)) {
        dst << (qint32)1;
        dst << prop->prp_getName();
        const int n = c->ca_getNumberOfChildren();
        dst << (qint32)n;
        for (int i = 0; i < n; i++) {
            writeTrackProperty(dst, c->ca_getChildAt(i));
        }
    }
}

static qsptr<Property> readTrackProperty(eReadStream& src) {
    qint32 type; src >> type;
    QString name; src >> name;
    if (type == 0) {
        auto anim = enve::make_shared<QrealAnimator>(name);
        anim->prp_readProperty(src);
        return anim;
    } else {
        auto node = enve::make_shared<StaticComplexAnimator>(name);
        qint32 count; src >> count;
        for (int i = 0; i < count; i++) {
            auto child = readTrackProperty(src);
            if (child) node->ca_addChild(child);
        }
        return node;
    }
}

void SvgElementTrack::writeTrack(eWriteStream& dst) const {
    dst << prp_getName();
    const auto& children = ca_getChildren();
    dst << (qint32)children.count();
    for (const auto& child : children) {
        writeTrackProperty(dst, child.get());
    }
}

void SvgElementTrack::readTrack(eReadStream& src) {
    QString name; src >> name;
    prp_setName(name);
    qint32 count; src >> count;
    for (int i = 0; i < count; i++) {
        auto child = readTrackProperty(src);
        if (child) ca_addChild(child);
    }
}

void SvgElementTrack::prp_drawTimelineControls(
        QPainter* p, const qreal pixelsPerFrame,
        const FrameRange& absFrameRange, const int rowHeight) {
    if (mOrphaned) {
        p->save();
        p->setOpacity(0.25);
        p->fillRect(0, 0,
                    qCeil(absFrameRange.span() * pixelsPerFrame),
                    rowHeight, Qt::red);
        p->restore();
    }
    Animator::prp_drawTimelineControls(p, pixelsPerFrame, absFrameRange, rowHeight);
}

void SvgElementTrack::prp_setupTreeViewMenu(PropertyMenu* menu) {
    if (menu->hasActionsForType<SvgElementTrack>()) return;
    menu->addedActionsForType<SvgElementTrack>();

    const auto parentWidget = menu->getParentWidget();
    const PropertyMenu::PlainSelectedOp<SvgElementTrack> editOp =
    [parentWidget](SvgElementTrack* track) {
        bool ok;
        const QString newId = QInputDialog::getText(
                    parentWidget,
                    QObject::tr("Edit Target Id"),
                    QObject::tr("SVG element id:"),
                    QLineEdit::Normal,
                    track->prp_getName(), &ok);
        if (ok && !newId.isEmpty()) {
            track->prp_setName(newId);
        }
    };
    menu->addPlainAction(QIcon::fromTheme("document-edit"),
                         QObject::tr("Edit Target Id"), editOp);

    const PropertyMenu::PlainSelectedOp<SvgElementTrack> deleteOp =
    [](SvgElementTrack* track) {
        emit track->deleteRequested();
    };
    menu->addPlainAction(QIcon::fromTheme("edit-delete"),
                         QObject::tr("Delete Track"), deleteOp);

    menu->addSeparator();
    StaticComplexAnimator::prp_setupTreeViewMenu(menu);
}

static void writeTrackPropertyXEV(QDomElement& parent,
                                   const XevExporter& exp,
                                   Property* prop) {
    auto mirror = exp.createElement("Mirror");
    mirror.setAttribute("name", prop->prp_getName());
    if (enve_cast<QrealAnimator*>(prop)) {
        mirror.setAttribute("type", "0");
        mirror.appendChild(prop->prp_writePropertyXEV(exp));
    } else if (const auto c = enve_cast<ComplexAnimator*>(prop)) {
        mirror.setAttribute("type", "1");
        const int n = c->ca_getNumberOfChildren();
        for (int i = 0; i < n; i++) {
            writeTrackPropertyXEV(mirror, exp, c->ca_getChildAt(i));
        }
    }
    parent.appendChild(mirror);
}

static qsptr<Property> readTrackPropertyXEV(const QDomElement& mirrorEle,
                                              const XevImporter& imp) {
    const QString name = mirrorEle.attribute("name");
    const int type = mirrorEle.attribute("type", "0").toInt();
    if (type == 0) {
        auto anim = enve::make_shared<QrealAnimator>(name);
        const auto dataEle = mirrorEle.firstChildElement();
        if (!dataEle.isNull()) anim->prp_readPropertyXEV(dataEle, imp);
        return anim;
    } else {
        auto node = enve::make_shared<StaticComplexAnimator>(name);
        auto child = mirrorEle.firstChildElement("Mirror");
        while (!child.isNull()) {
            auto childProp = readTrackPropertyXEV(child, imp);
            if (childProp) node->ca_addChild(childProp);
            child = child.nextSiblingElement("Mirror");
        }
        return node;
    }
}

QDomElement SvgElementTrack::prp_writePropertyXEV_impl(
        const XevExporter& exp) const {
    auto result = exp.createElement("Track");
    result.setAttribute("targetId", prp_getName());
    for (const auto& child : ca_getChildren()) {
        writeTrackPropertyXEV(result, exp, child.get());
    }
    return result;
}

void SvgElementTrack::prp_readPropertyXEV_impl(
        const QDomElement& ele, const XevImporter& imp) {
    const QString id = ele.attribute("targetId");
    if (!id.isEmpty()) prp_setName(id);
    auto mirror = ele.firstChildElement("Mirror");
    while (!mirror.isNull()) {
        auto prop = readTrackPropertyXEV(mirror, imp);
        if (prop) ca_addChild(prop);
        mirror = mirror.nextSiblingElement("Mirror");
    }
}
