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

#include "svgflipbooktrack.h"

#include "Boxes/containerbox.h"
#include "Boxes/boundingbox.h"
#include "Animators/eboxorsound.h"
#include "Animators/intanimator.h"
#include "typemenu.h"
#include "ReadWrite/ewritestream.h"
#include "ReadWrite/ereadstream.h"

#include <QPainter>
#include <QtMath>

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

SvgFlipbookTrack::SvgFlipbookTrack(const QString& ownerElementId)
    : StaticComplexAnimator(ownerElementId) {
    mIndex = enve::make_shared<IntAnimator>(0, -9999, 9999, 1, "index");
    ca_addChild(mIndex);
}

void SvgFlipbookTrack::setPageMap(const QMap<int, QString>& pageMap) {
    mPageMap = pageMap;
    mResolvedPages.clear();
}

void SvgFlipbookTrack::resolveTargets(ContainerBox* svgRoot) {
    mResolvedPages.clear();
    bool anyResolved = false;
    for (auto it = mPageMap.begin(); it != mPageMap.end(); ++it) {
        BoundingBox* found = svgRoot ? findDescendantByName(svgRoot, it.value())
                                     : nullptr;
        if (found) {
            mResolvedPages[it.key()] = found;
            anyResolved = true;
        }
    }
    mOrphaned = mPageMap.isEmpty() || !anyResolved;
}

void SvgFlipbookTrack::syncToTargets() {
    const int idx = mIndex->getEffectiveIntValue();
    for (auto it = mResolvedPages.begin(); it != mResolvedPages.end(); ++it) {
        if (BoundingBox* box = it.value()) {
            box->setVisibleFromAnimation(it.key() == idx);
        }
    }
}

void SvgFlipbookTrack::writeTrack(eWriteStream& dst) const {
    dst << prp_getName();
    mIndex->prp_writeProperty(dst);
}

void SvgFlipbookTrack::readTrack(eReadStream& src) {
    QString name; src >> name;
    prp_setName(name);
    mIndex->prp_readProperty(src);
}

void SvgFlipbookTrack::prp_drawTimelineControls(
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

void SvgFlipbookTrack::prp_setupTreeViewMenu(PropertyMenu* menu) {
    if (menu->hasActionsForType<SvgFlipbookTrack>()) return;
    menu->addedActionsForType<SvgFlipbookTrack>();

    const PropertyMenu::PlainSelectedOp<SvgFlipbookTrack> deleteOp =
    [](SvgFlipbookTrack* track) {
        emit track->deleteRequested();
    };
    menu->addPlainAction(QIcon::fromTheme("edit-delete"),
                         QObject::tr("Delete Track"), deleteOp);

    menu->addSeparator();
    StaticComplexAnimator::prp_setupTreeViewMenu(menu);
}
