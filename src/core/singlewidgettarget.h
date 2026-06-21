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

#ifndef SINGLEWIDGETTARGET_H
#define SINGLEWIDGETTARGET_H

#include <QList>
#include <map>
#include <QDomElement>

#include "swt_abstraction.h"
#include "swt_rulescollection.h"
#include "XML/xevexporter.h"
#include "XML/xevimporter.h"

#include "ReadWrite/ereadstream.h"
#include "ReadWrite/ewritestream.h"

class QMenu;
class QAction;
class QMimeData;

// Properties
class Property;
// Animators
class Animator;
class ComplexAnimator;
// Boxes
class eBoxOrSound;
class BoundingBox;
class ContainerBox;
class Canvas;
class InternalLinkBox;
class PathBox;
//class PaintBox;
class SmartVectorPath;
class BlendEffectBoxShadow;
// Sound
class eSound;
class eIndependentSound;

class CORE_EXPORT SingleWidgetTarget : public SelfRef {
    Q_OBJECT
public:
    SingleWidgetTarget() {}
    virtual void SWT_setupAbstraction(SWT_Abstraction*,
                                      const UpdateFuncs &,
                                      const int) {}

    virtual bool SWT_shouldBeVisible(const SWT_RulesCollection &rules,
                                     const bool parentSatisfies,
                                     const bool parentMainTarget) const {
        Q_UNUSED(rules)
        return parentSatisfies && !parentMainTarget;
    }

    virtual bool SWT_shouldPassThrough(const SWT_RulesCollection &rules) const {
        Q_UNUSED(rules)
        return false;
    }

    virtual QMimeData *SWT_createMimeData() {
        return nullptr;
    }

    virtual bool SWT_dropSupport(const QMimeData* const data) {
        Q_UNUSED(data)
        return false;
    }

    virtual bool SWT_drop(const QMimeData* const data) {
        Q_UNUSED(data)
        return false;
    }

    virtual bool SWT_dropIntoSupport(const int index,
                                     const QMimeData* const data) {
        Q_UNUSED(index)
        Q_UNUSED(data)
        return false;
    }

    virtual bool SWT_dropInto(const int index,
                              const QMimeData* const data) {
        Q_UNUSED(index)
        Q_UNUSED(data)
        return false;
    }

    virtual void SWT_setChildrenAncestorDisabled(const bool bT) {
        Q_UNUSED(bT)
    }

    virtual bool SWT_visibleOnlyIfParentDescendant() const {
        return true;
    }    

    // Properties
    e_DECLARE_TYPE_FUNCTION(Property)
    // Animators
    e_DECLARE_TYPE_FUNCTION(Animator)
    e_DECLARE_TYPE_FUNCTION(ComplexAnimator)
    // Boxes
    e_DECLARE_TYPE_FUNCTION(eBoxOrSound)
    e_DECLARE_TYPE_FUNCTION(BoundingBox)
    e_DECLARE_TYPE_FUNCTION(ContainerBox)
    e_DECLARE_TYPE_FUNCTION(Canvas)
    e_DECLARE_TYPE_FUNCTION(InternalLinkBox)
    e_DECLARE_TYPE_FUNCTION(PathBox)
    //e_DECLARE_TYPE_FUNCTION(PaintBox)
    e_DECLARE_TYPE_FUNCTION(SmartVectorPath)
    e_DECLARE_TYPE_FUNCTION(BlendEffectBoxShadow)
    // Sound
    e_DECLARE_TYPE_FUNCTION(eSound)
    e_DECLARE_TYPE_FUNCTION(eIndependentSound)

    void SWT_addChild(SingleWidgetTarget * const child);
    void SWT_addChildAt(SingleWidgetTarget * const child, const int id);
    void SWT_removeChild(SingleWidgetTarget * const child);

    SWT_Abstraction *SWT_createAbstraction(const UpdateFuncs &updateFuncs,
                                           const int visiblePartWidgetId);

    void SWT_removeAbstractionForWidget(const int visiblePartWidgetId);

    SWT_Abstraction* SWT_getAbstractionForWidget(
            const int visiblePartWidgetId) const;

    SWT_Abstraction* SWT_abstractionForWidget(
            const UpdateFuncs &updateFuncs,
            const int visiblePartWidgetId);

    void SWT_scheduleContentUpdate(const SWT_BoxRule rule);

    void SWT_scheduleSearchContentUpdate();

    void SWT_moveChildTo(SingleWidgetTarget * const target, const int id);

    bool SWT_isVisible() const { return SWT_mVisible; }

    void SWT_hide() { SWT_setVisible(false); }
    void SWT_show() { SWT_setVisible(true); }
    void SWT_setVisible(const bool bT);

    void SWT_setEnabled(const bool enabled)
    { SWT_setDisabled(!enabled); }

    void SWT_setDisabled(const bool disable);

    void SWT_disable()
    { SWT_setDisabled(true); }

    void SWT_enable()
    { SWT_setDisabled(false); }

    bool SWT_isEnabled() const
    { return !SWT_isDisabled(); }

    bool SWT_isDisabled() const
    { return SWT_mDisabled || SWT_mAncestorDisabled; }

    void SWT_setAncestorDisabled(const bool disabled);
protected:
    void SWT_writeAbstraction(eWriteStream& dst) const;
    void SWT_readAbstraction(eReadStream& src) const;

    void SWT_writeAbstractionXEV(QDomElement& ele, const XevExporter& exp) const;
    void SWT_readAbstractionXEV(const QDomElement& ele, const XevImporter& imp) const;
signals:
    void SWT_changedDisabled(bool);
private:
    bool SWT_mAncestorDisabled = false;
    bool SWT_mVisible = true;
    bool SWT_mDisabled = false;
    std::map<int, stdsptr<SWT_Abstraction>> SWT_mAllAbstractions;
};

#endif // SINGLEWIDGETTARGET_H
