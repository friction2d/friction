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

#ifndef LISTOFNODES_H
#define LISTOFNODES_H

#include "smartPointers/stdselfref.h"
#include "node.h"

class CORE_EXPORT ListOfNodes {
public:
    ListOfNodes() = default;
    ListOfNodes(ListOfNodes&& other);
    ListOfNodes(const ListOfNodes& other);

    ListOfNodes& operator=(const ListOfNodes& other) {
        deepCopyFrom(other);
        return *this;
    }

    ListOfNodes& operator=(ListOfNodes&& other) {
        mList = std::move(other.mList);
        return *this;
    }

    bool isEmpty() const { return mList.isEmpty(); }

    int append(const Node& nodeBlueprint);
    void insert(const int id, const Node& nodeBlueprint);
    void removeAt(const int id);
    void moveNode(const int fromId, const int toId);

    void clear() { mList.clear(); }
    void swap(ListOfNodes& other) { mList.swap(other.mList); }

    Node* at(const int id) const {
        if(id < 0 || id >= count()) RuntimeThrow("Index out of range.");
        return mList.at(id).get();
    }

    stdsptr<Node> atSPtr(const int id) const {
        if(id < 0 || id >= count()) RuntimeThrow("Index out of range.");
        return mList.at(id);
    }

    Node* first() const
    { return at(0); }

    Node* last() const
    { return at(mList.count() - 1); }

    int count() const
    { return mList.count(); }

    void deepCopyFrom(const ListOfNodes& other);

    Node * operator[](const int id) const
    { return at(id); }

    void reverse();

    void moveNodesToFrontStartingWith(const int first);

    ListOfNodes detachNodesStartingWith(const int first);

    void appendNodes(ListOfNodes&& src);
    void prependNodes(ListOfNodes&& src);

    void applyTransform(const QTransform &transform);

    typedef QList<stdsptr<Node>>::const_iterator const_iterator;
    typedef QList<stdsptr<Node>>::iterator iterator;

    const_iterator begin() const
    { return mList.begin(); }

    const_iterator end() const
    { return mList.end(); }
private:
    void updateNodeIds() { updateNodeIds(0); }
    void updateNodeIds(const int minId);
    void updateNodeIds(const int minId, const int maxId);
    void updateNodeId(const int id);

    Node* insertNewNode(const int id);

    QList<stdsptr<Node>> mList;
};
#endif // LISTOFNODES_H
