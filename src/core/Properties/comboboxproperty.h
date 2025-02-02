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

#ifndef COMBOBOXPROPERTY_H
#define COMBOBOXPROPERTY_H

#include "property.h"

class CORE_EXPORT ComboBoxProperty : public Property {
    Q_OBJECT
    e_OBJECT
protected:
    ComboBoxProperty(const QString& name,
                     const QStringList &list);

    QDomElement prp_writePropertyXEV_impl(const Friction::Core::XevExporter& exp) const;
    void prp_readPropertyXEV_impl(const QDomElement& ele, const Friction::Core::XevImporter& imp);
public:
    void prp_writeProperty_impl(eWriteStream& dst) const;
    void prp_readProperty_impl(eReadStream& src);

    const QStringList &getValueNames() { return mValueNames; }
    QString getCurrentValueName();
    int getCurrentValue() { return mCurrentValue; }
    void setCurrentValue(const int id);
signals:
    void valueChanged(int);
private:
    int mCurrentValue = 0;
    QStringList mValueNames;
};

#endif // COMBOBOXPROPERTY_H
