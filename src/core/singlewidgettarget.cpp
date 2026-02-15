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

#include "singlewidgettarget.h"
#include "swt_abstraction.h"

SWT_Abstraction* SingleWidgetTarget::SWT_createAbstraction(
        const UpdateFuncs& updateFuncs,
        const int visiblePartWidgetId) {
    const auto curr = SWT_getAbstractionForWidget(visiblePartWidgetId);
    if(curr) return curr;
    const auto abs = enve::make_shared<SWT_Abstraction>(
                this, updateFuncs, visiblePartWidgetId);
    SWT_setupAbstraction(abs.get(), updateFuncs, visiblePartWidgetId);
    SWT_mAllAbstractions[visiblePartWidgetId] = abs;
    return abs.get();
}

void SingleWidgetTarget::SWT_removeAbstractionForWidget(const int visiblePartWidgetId) {
    SWT_mAllAbstractions.erase(visiblePartWidgetId);
}

SWT_Abstraction *SingleWidgetTarget::SWT_getAbstractionForWidget(
        const int visiblePartWidgetId) const {
    const auto it = SWT_mAllAbstractions.find(visiblePartWidgetId);
    if(it == SWT_mAllAbstractions.end()) return nullptr;
    return it->second.get();
}

SWT_Abstraction *SingleWidgetTarget::SWT_abstractionForWidget(
        const UpdateFuncs &updateFuncs, const int visiblePartWidgetId) {
    const auto curr = SWT_getAbstractionForWidget(visiblePartWidgetId);
    if(curr) return curr;
    return SWT_createAbstraction(updateFuncs, visiblePartWidgetId);
}

void SingleWidgetTarget::SWT_addChild(SingleWidgetTarget * const child) {
    for(const auto& abs : SWT_mAllAbstractions) {
        abs.second->addChild(child);
    }
}

void SingleWidgetTarget::SWT_addChildAt(
        SingleWidgetTarget * const child, const int id) {
    for(const auto& abs : SWT_mAllAbstractions) {
        abs.second->addChildAt(child, id);
    }
}

void SingleWidgetTarget::SWT_scheduleContentUpdate(const SWT_BoxRule rule) {
    for(const auto& abs : SWT_mAllAbstractions) {
        abs.second->scheduleContentUpdate(rule);
    }
}

void SingleWidgetTarget::SWT_scheduleSearchContentUpdate() {
    for(const auto& abs : SWT_mAllAbstractions) {
        abs.second->scheduleSearchContentUpdate();
    }
}

void SingleWidgetTarget::SWT_removeChild(
        SingleWidgetTarget * const child) {
    for(const auto& abs : SWT_mAllAbstractions) {
        abs.second->removeChild(child);
    }
}

void SingleWidgetTarget::SWT_moveChildTo(
        SingleWidgetTarget * const child, const int id) {
    for(const auto& abs : SWT_mAllAbstractions) {
        abs.second->moveChildTo(child, id);
    }
}

void SingleWidgetTarget::SWT_setVisible(const bool bT) {
    if(SWT_mVisible == bT) return;
    SWT_mVisible = bT;
    for(const auto &swa : SWT_mAllAbstractions) {
        swa.second->afterContentVisibilityChanged();
    }
}

void SingleWidgetTarget::SWT_setDisabled(const bool disable) {
    if(SWT_mDisabled == disable) return;
    SWT_mDisabled = disable;
    SWT_setChildrenAncestorDisabled(SWT_isDisabled());
    emit SWT_changedDisabled(SWT_isDisabled());
}

void SingleWidgetTarget::SWT_setAncestorDisabled(const bool disabled) {
    if(SWT_mAncestorDisabled == disabled) return;
    SWT_mAncestorDisabled = disabled;
    SWT_setChildrenAncestorDisabled(SWT_isDisabled());
    emit SWT_changedDisabled(SWT_isDisabled());
}

void SingleWidgetTarget::SWT_writeAbstraction(eWriteStream& dst) const {
    QVector<int> absOpen;
    absOpen.reserve(SWT_mAllAbstractions.size());
    const auto& objListIdConv = dst.objListIdConv();
    for(const auto& abs : SWT_mAllAbstractions) {
        const int writeId = objListIdConv.runtimeIdToWriteId(abs.first);
        if(writeId == -1) continue;
        const bool vis = abs.second->contentVisible();
        if(vis) absOpen << writeId;
    }
    dst << absOpen.count();
    for(const int& open : qAsConst(absOpen)) dst << open;
}

void SingleWidgetTarget::SWT_readAbstraction(eReadStream& src) const {
    int count; src >> count;
    if(count <= 0) return;
    QVector<int> absOpen;
    absOpen.reserve(count);
    for(int i = 0; i < count; i++) {
        int id; src >> id;
        absOpen << id;
    }

    const auto& objListIdConv = src.objListIdConv();
    for(const auto& abs : SWT_mAllAbstractions) {
        const int writeId = objListIdConv.runtimeIdToWriteId(abs.first);
        if(writeId == -1) continue;
        const bool contVisible = absOpen.contains(writeId);
        if(contVisible) abs.second->setContentVisible(true);
    }
}

void SingleWidgetTarget::SWT_writeAbstractionXEV(
        QDomElement& ele, const XevExporter& exp) const {
    QString absOpen;
    const auto& objListIdConv = exp.objListIdConv();
    for(const auto& abs : SWT_mAllAbstractions) {
        const int writeId = objListIdConv.runtimeIdToWriteId(abs.first);
        if(writeId == -1) continue;
        const bool vis = abs.second->contentVisible();
        if(vis) {
            if(!absOpen.isEmpty()) absOpen += ' ';
            absOpen += QString::number(writeId);
        }
    }
    if(!absOpen.isEmpty()) ele.setAttribute("open", absOpen);
}

void SingleWidgetTarget::SWT_readAbstractionXEV(
        const QDomElement& ele, const XevImporter& imp) const {
    const QString absOpenStr = ele.attribute("open");
    if(absOpenStr.isEmpty()) return;
    const auto absOpenStrs = absOpenStr.splitRef(' ');
    QList<int> open;
    for(const auto& val : absOpenStrs) {
        open << XmlExportHelpers::stringToInt(val);
    }

    const auto& objListIdConv = imp.objListIdConv();
    for(const auto& abs : SWT_mAllAbstractions) {
        const int writeId = objListIdConv.runtimeIdToWriteId(abs.first);
        if(writeId == -1) continue;
        const bool contVisible = open.contains(writeId);
        if(contVisible) abs.second->setContentVisible(true);
    }
}
