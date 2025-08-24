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

#include "boxtargetproperty.h"
#include "Animators/complexanimator.h"
#include "Boxes/boundingbox.h"
#include "Properties/emimedata.h"
#include "simpletask.h"

BoxTargetProperty::BoxTargetProperty(const QString &name) :
    Property(name) {}

BoundingBox* BoxTargetProperty::getTarget() const {
    return mTarget_d;
}

void BoxTargetProperty::setTargetAction(BoundingBox* const box)
{
    if (box == mTarget_d) { return; }
    const auto oldValue = mTarget_d.get();
    emit setActionStarted(oldValue, box);
    {
        prp_pushUndoRedoName(tr("Set Box Target"));
        UndoRedo ur;
        const qptr<BoundingBox> oldValue = mTarget_d.operator BoundingBox *();
        const qptr<BoundingBox> newValue = box;
        ur.fUndo = [this, oldValue]() { setTarget(oldValue); };
        ur.fRedo = [this, newValue]() { setTarget(newValue); };
        prp_addUndoRedo(ur);
    }

    setTarget(box);
    emit setActionFinished(oldValue, box);
}

void BoxTargetProperty::setTarget(BoundingBox* const box) {
    if(box == mTarget_d) return;

    mTarget_d.assign(box);
    if(box) {
        mTarget_d << connect(box, &BoundingBox::prp_ancestorChanged,
                             this, [this]() {
            SimpleTask::sScheduleContexted(this, [this]() {
                if(mTarget_d && mTarget_d->getParentScene()) return;
                setTargetAction(nullptr);
            });
        });
    }
    prp_afterWholeInfluenceRangeChanged();
    emit targetSet(box);
}

bool BoxTargetProperty::SWT_dropSupport(const QMimeData * const data) {
    if(!eMimeData::sHasType<BoundingBox>(data)) return false;
    const auto eData = static_cast<const eMimeData*>(data);
    const auto bData = static_cast<const eDraggedObjects*>(eData);
    const auto objs = bData->getObjects<BoundingBox>();
    if(objs.count() != 1) return false;
    const auto obj = objs.first();
    if(mValidator && !mValidator(obj)) return false;
    const auto parentBox = getFirstAncestor<BoundingBox>();
    if(parentBox == obj) return false;
    return true;
}

bool BoxTargetProperty::SWT_drop(const QMimeData * const data) {
    if(!SWT_dropSupport(data)) return false;
    const auto eData = static_cast<const eMimeData*>(data);
    const auto bData = static_cast<const eDraggedObjects*>(eData);
    const auto objs = bData->getObjects<BoundingBox>();
    setTargetAction(objs.first());
    return true;
}

void BoxTargetProperty::prp_writeProperty_impl(eWriteStream& dst) const {
    int targetWriteId = -1;
    int targetDocumentId = -1;

    if(mTarget_d) {
        targetWriteId = mTarget_d->getWriteId();
        targetDocumentId = mTarget_d->getDocumentId();
    }
    dst << targetWriteId;
    dst << targetDocumentId;
}

#include "simpletask.h"
void BoxTargetProperty::prp_readProperty_impl(eReadStream& src) {
    int targetReadId;
    src >> targetReadId;
    int targetDocumentId;
    src >> targetDocumentId;
    if(targetReadId == -1 && targetDocumentId == -1) return;
    src.addReadStreamDoneTask([this, targetReadId, targetDocumentId]
                              (eReadStream& src) {
        BoundingBox* targetBox = nullptr;
        if(targetReadId != -1)
            targetBox = src.getBoxByReadId(targetReadId);
         if(!targetBox && targetDocumentId != -1)
             targetBox = BoundingBox::sGetBoxByDocumentId(targetDocumentId);
         setTarget(targetBox);
    });
}

QDomElement BoxTargetProperty::prp_writePropertyXML_impl(const Friction::Core::XmlExporter& exp) const {
    auto result = exp.createElement("ObjectLink");

    int targetWriteId = -1;
    if(mTarget_d) targetWriteId = mTarget_d->getWriteId();
    result.setAttribute("targetId", targetWriteId);

    return result;
}

void BoxTargetProperty::prp_readPropertyXML_impl(
        const QDomElement& ele, const Friction::Core::XmlImporter& imp) {
    const int targetId = Friction::Core::XmlExportHelpers::stringToInt(ele.attribute("targetId"));
    if(targetId == -1) return;
    auto& handler = imp.getXevReadBoxesHandler();
    handler.addXevImporterDoneTask([this, targetId](const Friction::Core::XmlReadBoxesHandler& imp) {
        const auto targetObj = imp.getBoxByReadId(targetId);
        if(targetObj) setTarget(targetObj);
    });
}
