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

#ifndef INTERNALLINKBOX_H
#define INTERNALLINKBOX_H
#include "Boxes/internallinkboxbase.h"
#include "Properties/boxtargetproperty.h"
#include "Properties/boolproperty.h"

class eSound;

class CORE_EXPORT InternalLinkBox : public InternalLinkBoxBase<BoundingBox> {
    e_OBJECT
    e_DECLARE_TYPE(InternalLinkBox)
protected:
    InternalLinkBox(BoundingBox * const linkTarget,
                    const bool innerLink);
public:
    void setupRenderData(const qreal relFrame, const QMatrix& parentM,
                         BoxRenderData * const data);

    void setLinkTarget(BoundingBox * const linkTarget);
protected:
    const qsptr<BoxTargetProperty> mBoxTarget =
            enve::make_shared<BoxTargetProperty>("link target");
private:
    qsptr<eSound> mSound;
};

#endif // INTERNALLINKBOX_H
