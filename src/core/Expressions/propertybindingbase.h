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

#ifndef PROPERTYBINDINGBASE_H
#define PROPERTYBINDINGBASE_H

#include <QObject>

#include "framerange.h"
#include "Properties/property.h"

#include <QJSValue>

class CORE_EXPORT PropertyBindingBase : public QObject {
    Q_OBJECT
protected:
    PropertyBindingBase(const Property* const context);
public:
    virtual QJSValue getJSValue(QJSEngine& e) = 0;
    virtual QJSValue getJSValue(QJSEngine& e, const qreal relFrame) = 0;
    virtual QJSValue getJSValueAtAbsFrame(QJSEngine& e,
                                          const qreal absFrame);
    virtual FrameRange identicalRelRange(const int absFrame) = 0;
    virtual FrameRange nextNonUnaryIdenticalRelRange(const int absFrame) = 0;
    virtual QString path() const = 0;
    virtual bool dependsOn(const Property* const prop) {
        Q_UNUSED(prop)
        return false;
    }
    virtual bool isValid() const { return true; }

    bool setAbsFrame(const int absFrame);
signals:
    void relRangeChanged(const FrameRange& range);
    void currentValueChanged();
protected:
    qreal relFrame() const { return mRelFrame; }

    const qptr<const Property> mContext;
private:
    bool mValueUpToDate = false;
    qreal mRelFrame = 123456789.123456789;
};

#endif // PROPERTYBINDINGBASE_H
