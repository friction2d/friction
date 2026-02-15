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

#include "listofnodes.h"

ListOfNodes::ListOfNodes(ListOfNodes &&other) :
    mList(std::move(other.mList)) {}

ListOfNodes::ListOfNodes(const ListOfNodes &other)
{ deepCopyFrom(other); }

int ListOfNodes::append(const Node &nodeBlueprint) {
    const int id = mList.count();
    insert(id, nodeBlueprint);
    return id;
}

void ListOfNodes::insert(const int id, const Node &nodeBlueprint) {
    Node * const newNode = insertNewNode(id);
    *newNode = nodeBlueprint;
    newNode->setNodeId(id);
}

void ListOfNodes::removeAt(const int id) {
    mList.removeAt(id);
    updateNodeIds(id);
}

void ListOfNodes::moveNode(const int fromId, const int toId) {
    mList.move(fromId, toId);
    updateNodeIds(qMin(fromId, toId));
}

void ListOfNodes::deepCopyFrom(const ListOfNodes &other) {
    const int otherCount = other.count();
    const int thisCount = count();
    int i = 0;
    for(; i < otherCount && i < thisCount; i++) {
        *mList.at(i).get() = *other.at(i);
    }
    for(; i < otherCount; i++) {
        insert(i, *other.at(i));
    }
    for(; i < thisCount; i++) {
        removeAt(otherCount);
    }
}

void ListOfNodes::reverse() {
    const auto cpy = mList;
    mList.clear();
    for(const auto& node : cpy) {
        const auto oldC0 = node->c0();
        node->setC0(node->c2());
        node->setC2(oldC0);
        mList.prepend(node);
    }
    updateNodeIds();
}

void ListOfNodes::moveNodesToFrontStartingWith(const int first) {
    for(int i = first; i < mList.count(); i++)
        mList.prepend(mList.takeLast());
    updateNodeIds();
}

ListOfNodes ListOfNodes::detachNodesStartingWith(const int first) {
    ListOfNodes result;
    const int iniCount = mList.count();
    for(int i = first, j = 0; i < iniCount; i++, j++) {
        const auto node = mList.takeLast();
        node->setNodeId(j);
        result.mList.prepend(node);
    }
    return result;
}

void ListOfNodes::appendNodes(ListOfNodes &&src) {
    if(src.isEmpty()) return;
    const int oldCount = count();
    for(const auto& node : src)
        mList.append(node);
    updateNodeIds(oldCount);
}

void ListOfNodes::prependNodes(ListOfNodes &&src) {
    if(src.isEmpty()) return;
    for(int i = src.count() - 1; i >= 0; i--) {
        const auto node = src.atSPtr(i);
        mList.prepend(node);
    }
    updateNodeIds();
}

void ListOfNodes::applyTransform(const QMatrix &transform) {
    for(const auto& node : qAsConst(mList))
        node->applyTransform(transform);
}

void ListOfNodes::updateNodeIds(const int minId) {
    const int maxId = mList.count() - 1;
    updateNodeIds(minId, maxId);
}

void ListOfNodes::updateNodeIds(const int minId, const int maxId) {
    for(int i = minId; i <= maxId; i++)
        updateNodeId(i);
}

void ListOfNodes::updateNodeId(const int id) {
    mList.at(id)->setNodeId(id);
}

Node *ListOfNodes::insertNewNode(const int id) {
    const auto newNode = stdsptr<Node>(new Node);
    mList.insert(id, newNode);
    updateNodeIds(id);
    return newNode.get();
}
