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

#ifndef CUSTOMPROPERTIES_H
#define CUSTOMPROPERTIES_H
#include "dynamiccomplexanimator.h"

#include "Properties/namedproperty.h"

CORE_EXPORT
qsptr<Animator> readIdCreateCProperty(eReadStream& src);
CORE_EXPORT
void writeCPropertyType(Animator* const obj, eWriteStream& dst);

CORE_EXPORT
qsptr<Animator> readIdCreateCPropertyXML(const QDomElement& ele);
CORE_EXPORT
void writeCPropertyTypeXML(Animator* const obj, QDomElement& ele);

using CustomPropertiesBase =
    DynamicComplexAnimator<Animator,
                           writeCPropertyType,
                           readIdCreateCProperty,
                           writeCPropertyTypeXML,
                           readIdCreateCPropertyXML>;

class CORE_EXPORT CustomProperties : public CustomPropertiesBase {
    Q_OBJECT
    e_OBJECT
protected:
    CustomProperties();
public:
    void prp_setupTreeViewMenu(PropertyMenu * const menu) override;

    void prp_writeProperty_impl(eWriteStream& dst) const override;
    void prp_readProperty_impl(eReadStream& src) override;

    void addProperty(const qsptr<Animator>& prop);

    template <typename T>
    T* addPropertyOfType(const QString& name) {
        using PropType = NamedProperty<T>;
        const auto prop = enve::make_shared<PropType>(name);
        this->addProperty(prop);
        return prop.get();
    }
private:
    using CustomPropertiesBase::addChild;
    using CustomPropertiesBase::insertChild;
};

#endif // CUSTOMPROPERTIES_H
