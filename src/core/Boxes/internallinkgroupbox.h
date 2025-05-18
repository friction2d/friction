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

#ifndef INTERNALLINKGROUPBOX_H
#define INTERNALLINKGROUPBOX_H
#include "containerbox.h"
#include "internallinkboxbase.h"
#include "Properties/boxtargetproperty.h"

class CORE_EXPORT InternalLinkGroupBox : public InternalLinkBoxBase<ContainerBox> {
    e_OBJECT
protected:
    InternalLinkGroupBox(ContainerBox * const linkTarget = nullptr,
                         const bool innerLink = false);
public:
    void SWT_setupAbstraction(SWT_Abstraction* abs,
                              const UpdateFuncs &funcs,
                              const int widgetId)
    { BoundingBox::SWT_setupAbstraction(abs, funcs, widgetId); }

    bool SWT_dropSupport(const QMimeData * const data)
    { return BoundingBox::SWT_dropSupport(data); }

    bool SWT_dropIntoSupport(const int index, const QMimeData * const data)
    { return BoundingBox::SWT_dropIntoSupport(index, data); }

    bool SWT_drop(const QMimeData * const data)
    { return BoundingBox::SWT_drop(data); }

    bool SWT_dropInto(const int index, const QMimeData * const data)
    { return BoundingBox::SWT_dropInto(index, data); }

    void setupRenderData(const qreal relFrame, const QMatrix& parentM,
                         BoxRenderData * const data,
                         Scene * const scene);

    bool localDifferenceInPathBetweenFrames(
            const int frame1, const int frame2) const;
    bool localDifferenceInOutlinePathBetweenFrames(
            const int frame1, const int frame2) const;
    bool localDifferenceInFillPathBetweenFrames(
            const int frame1, const int frame2) const;

    bool isFlipBook() const;
    iValueRange getContainedMinMax() const;

    void setLinkTarget(ContainerBox * const linkTarget);
protected:
    const qsptr<BoxTargetProperty> mBoxTarget =
            enve::make_shared<BoxTargetProperty>("link target");
private:
    void insertInnerLinkFor(const int id, eBoxOrSound* const obj);
};

#endif // INTERNALLINKGROUPBOX_H
