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

#ifndef CANVASBASE_H
#define CANVASBASE_H
#include <functional>
#include <QList>
class Property;
class BoundingBox;

#include "Boxes/containerbox.h"
#include "MovablePoints/movablepoint.h"
#include "conncontextobjlist.h"

class CORE_EXPORT CanvasBase : public ContainerBox {
    friend class Canvas;
private:
    CanvasBase();
public:
    template <class T = BoundingBox>
    void execOpOnSelectedBoxes(const std::function<void(const QList<T*>&)> &op) {
        QList<T*> all;
        for(const auto& box : qAsConst(mSelectedBoxes)) {
            const auto boxT = enve_cast<T*>(box);
            if(boxT) all << boxT;
        }
        op(all);
    }

    template <class T = BoundingBox>
    void execOpOnSelectedBoxes(const std::function<void(T*)> &op) {
        for(const auto& box : qAsConst(mSelectedBoxes)) {
            const auto boxT = enve_cast<T*>(box);
            if(boxT) op(boxT);
        }
    }

    template <class T = MovablePoint>
    void execOpOnSelectedPoints(const std::function<void(const QList<T*>&)> &op) {
        QList<T*> all;
        for(const auto& pt : qAsConst(mSelectedPoints_d)) {
            const auto ptT = enve_cast<T*>(pt);
            if(ptT) all << ptT;
        }
        op(all);
    }

    template <class T = MovablePoint>
    void execOpOnSelectedPoints(const std::function<void(T*)> &op) {
        if(mPressedPoint) {
            if(!mPressedPoint->selectionEnabled()) {
                const auto ptT = enve_cast<T*>(mPressedPoint.data());
                if(ptT) {
                    op(ptT);
                    //if(ptT->selectionEnabled()) addPointToSelection(ptT);
                    return;
                }
            }
        }
        for(const auto& pt : qAsConst(mSelectedPoints_d)) {
            const auto ptT = enve_cast<T*>(pt);
            if(ptT) {
                op(ptT);
                //if(!ptT->selectionEnabled()) removePointFromSelection(ptT);
            }
        }
    }

    template <class T = Property>
    void execOpOnSelectedProperties(const std::function<void(T*)> &op) {
        for(const auto prop : qAsConst(mSelectedProps)) {
            const auto t = enve_cast<T*>(prop);
            if(t) op(t);
        }
    }

    void addToSelectedProps(Property* const prop) {
        auto& conn = mSelectedProps.addObj(prop);
        conn << connect(prop, &Property::prp_parentChanged,
                        this, [this, prop]() { removeFromSelectedProps(prop); });
        prop->prp_setSelected(true);
    }

    void removeFromSelectedProps(Property* const prop) {
        mSelectedProps.removeObj(prop);
        prop->prp_setSelected(false);
    }

    void clearSelectedProps() {
        const auto selected = mSelectedProps.getList();
        for(const auto prop : selected) {
            removeFromSelectedProps(prop);
        }
    }
protected:
    QList<MovablePoint*> mSelectedPoints_d;
    stdptr<MovablePoint> mPressedPoint;

    ConnContextObjList<BoundingBox*> mSelectedBoxes;
    QPointer<BoundingBox> mLastSelectedBox;

    ConnContextObjList<Property*> mSelectedProps;
};

#endif // CANVASBASE_H
