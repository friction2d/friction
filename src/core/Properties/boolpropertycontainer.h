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

#ifndef BOOLPROPERTYCONTAINER_H
#define BOOLPROPERTYCONTAINER_H

#include "../Animators/staticcomplexanimator.h"

class CORE_EXPORT BoolPropertyContainer : public StaticComplexAnimator {
    Q_OBJECT
    e_OBJECT
protected:
    BoolPropertyContainer(const QString& name);

    QDomElement prp_writePropertyXEV_impl(const XevExporter& exp) const;
    void prp_readPropertyXEV_impl(const QDomElement& ele, const XevImporter& imp);
public:
    void prp_writeProperty_impl(eWriteStream& dst) const;
    void prp_readProperty_impl(eReadStream& src);

    void setValue(const bool value);
    bool getValue();

    using ComplexAnimator::ca_addChild;
    using ComplexAnimator::ca_insertChild;
    using ComplexAnimator::ca_prependChild;
signals:
    void valueChanged(bool);
private:
    bool mValue = false;
};

#endif // BOOLPROPERTYCONTAINER_H
