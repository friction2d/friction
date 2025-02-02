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

#ifndef QSTRINGANIMATOR_H
#define QSTRINGANIMATOR_H
#include "Animators/steppedanimator.h"

typedef KeyT<QString> QStringKey;

class CORE_EXPORT QStringAnimator : public SteppedAnimator<QString> {
    e_OBJECT
protected:
    QStringAnimator(const QString& name);

    void prp_readPropertyXEV_impl(const QDomElement& ele, const Friction::Core::XevImporter& imp);
    QDomElement prp_writePropertyXEV_impl(const Friction::Core::XevExporter& exp) const;
public:
    using PropSetter = std::function<void(QDomElement&)>;
    void saveSVG(SvgExporter& exp, QDomElement& parent,
                 const PropSetter& propSetter) const;
};

#endif // QSTRINGANIMATOR_H
