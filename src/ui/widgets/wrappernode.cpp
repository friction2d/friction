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

#include "wrappernode.h"
#include "widgets/widgetwrappernode.h"
#include "exceptions.h"

WrapperNode* createForType(const WrapperNodeType type,
                           const WrapperNode::WidgetCreator &creator) {
    switch(type) {
    case WrapperNodeType::widget: return creator(nullptr);
    case WrapperNodeType::splitH: return new HWidgetStackNode(creator);
    case WrapperNodeType::splitV: return new VWidgetStackNode(creator);
    default: RuntimeThrow("Invalid WrapperNodeType, data corrupted");
    }
}

QDomElement WrapperNode::writeXML(QDomDocument& doc,
                                  Friction::Core::RuntimeIdToWriteId& objListIdConv) {
    auto result = doc.createElement(tagNameXML());
    writeDataXML(result, doc, objListIdConv);
    return result;
}

WrapperNode* WrapperNode::sRead(eReadStream &src,
                                const WidgetCreator &creator) {
    WrapperNodeType type;
    src.read(&type, sizeof(WrapperNodeType));
    const auto wid = createForType(type, creator);
    wid->readData(src);
    return wid;
}

WrapperNode* WrapperNode::sReadXML(Friction::Core::XmlReadBoxesHandler& boxReadHandler,
                                   const QDomElement& ele,
                                   const WidgetCreator& creator,
                                   Friction::Core::RuntimeIdToWriteId& objListIdConv) {
    WrapperNodeType type;
    const auto tag = ele.tagName();
    if(tag == "HSplit") type = WrapperNodeType::splitH;
    else if(tag == "VSplit") type = WrapperNodeType::splitV;
    else type = WrapperNodeType::widget;
    const auto wid = createForType(type, creator);
    wid->readDataXML(boxReadHandler, ele, objListIdConv);
    return wid;
}

void BaseWrapperNode::reset(Canvas* const scene) {
    if(fChild) delete fChild;
    fChild = fCreator(scene);
    fChild->fParent = this;
    layout()->addWidget(fChild->widget());
}
