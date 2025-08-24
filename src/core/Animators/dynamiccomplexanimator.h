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

#ifndef DYNAMICCOMPLEXANIMATOR_H
#define DYNAMICCOMPLEXANIMATOR_H
#include <QApplication>
#include "complexanimator.h"
#include "typemenu.h"
#include "Properties/emimedata.h"
#include "namefixer.h"

template <class T>
class DynamicComplexAnimatorBase : public ComplexAnimator {
protected:
    DynamicComplexAnimatorBase(const QString &name) :
        ComplexAnimator(name) {}
public:
    virtual qsptr<T> createDuplicate(T* const src) = 0;

    bool SWT_dropSupport(const QMimeData* const data) {
        return eMimeData::sHasType<T>(data);
    }

    bool SWT_dropIntoSupport(const int index, const QMimeData* const data) {
        Q_UNUSED(index)
        return eMimeData::sHasType<T>(data);
    }

    bool SWT_drop(const QMimeData* const data) {
        return SWT_dropInto(0, data);
    }

    bool SWT_dropInto(const int index, const QMimeData* const data) {
        const auto eData = static_cast<const eMimeData*>(data);
        const auto bData = static_cast<const eDraggedObjects*>(eData);
        const bool duplicate = QApplication::queryKeyboardModifiers() & Qt::CTRL;
        const auto objects = bData->getObjects<T>();
        int i = index;
        for(const auto iObj : objects) {
            if(duplicate) insertChild(createDuplicate(iObj), i++);
            else insertChild(iObj->template ref<T>(), i++);
        }
        return true;
    }

    void prp_setupTreeViewMenu(PropertyMenu * const menu) {
        ComplexAnimator::prp_setupTreeViewMenu(menu);
        menu->addSeparator();

        const PropertyMenu::PlainSelectedOp<DynamicComplexAnimatorBase<T>> dOp =
        [](DynamicComplexAnimatorBase<T> * animTarget) {
            animTarget->ca_removeAllChildren();
        };
        menu->addPlainAction(QIcon::fromTheme("trash"), tr("Clear"), dOp)->setEnabled(this->ca_getNumberOfChildren());
    }

    void insertChild(const qsptr<T>& child,
                     const int index)
    {
        clearOldParent(child);
        ca_insertChild(child, index);

        {
            prp_pushUndoRedoName(tr("Insert ") + child->prp_getName());
            UndoRedo ur;
            ur.fUndo = [this, child]() { removeChild(child); };
            ur.fRedo = [this, child, index]() { insertChild(child, index); };
            prp_addUndoRedo(ur);
        }
    }

    void clear() {
        const int nChildren = ca_getNumberOfChildren();
        for(int i = nChildren - 1; i >= 0; i--) {
            takeChildAt(i);
        }
    }

    void addChild(const qsptr<T>& newChild) {
        insertChild(newChild, ca_getNumberOfChildren());
    }

    void removeChild(const qsptr<T>& child)
    {
        const int index = ca_getChildPropertyIndex(child.get());
        ca_removeChild(child);

        {
            prp_pushUndoRedoName(tr("Remove ") + child->prp_getName());
            UndoRedo ur;
            ur.fUndo = [this, child, index]() { insertChild(child, index); };
            ur.fRedo = [this, child]() { removeChild(child); };
            prp_addUndoRedo(ur);
        }
    }

    T* getChild(const int index) const {
        return ca_getChildAt<T>(index);
    }

    qsptr<T> takeChildAt(const int index) {
        const auto child = getChild(index)->template ref<T>();
        removeChild(child);
        return child;
    }


    void moveAllFrom(DynamicComplexAnimatorBase<T> * const from) {
        const int iMax = from->ca_getNumberOfChildren() - 1;
        for(int i = iMax; i >= 0; i--)
            addChild(from->takeChildAt(i));
    }

    void swapChildren(const int id1,
                      const int id2)
    {
        ca_swapChildren(id1, id2);

        {
            prp_pushUndoRedoName(tr("Swap"));
            UndoRedo ur;
            ur.fUndo = [this, id1, id2]() { swapChildren(id2, id1); };
            ur.fRedo = [this, id1, id2]() { swapChildren(id1, id2); };
            prp_addUndoRedo(ur);
        }
    }

    void saveOrder() {
        mSavedOrder = ca_getChildren();
    }

    void restoreOrder() {
        restoreOrder(mSavedOrder);
    }

    void finishOrder()
    {
        prp_pushUndoRedoName(tr("Reorder"));
        UndoRedo ur;
        const auto oldValue = mSavedOrder;
        const auto newValue = ca_getChildren();
        ur.fUndo = [this, oldValue]() { restoreOrder(oldValue); };
        ur.fRedo = [this, newValue]() { restoreOrder(newValue); };
        prp_addUndoRedo(ur);
    }

    void swapChildrenTemporary(const int id1, const int id2) {
        ca_swapChildren(id1, id2);
    }

    void allStartingWith(const QString &text,
                         QList<Property*> &result) {
        const auto& children = ca_getChildren();
        for(const auto &child : children) {
            const bool nameMatch = child->prp_getName().startsWith(text);
            if(nameMatch) result << child.get();
        }
    }

    QStringList allNamesStartingWith(const QString &text,
                                     Property * const skip) {
        QStringList result;
        QList<Property*> matches;
        allStartingWith(text, matches);
        for(const auto match : matches) {
            if(match == skip) continue;
            result << match->prp_getName();
        }
        return result;
    }

    QString makeNameUnique(const QString &name,
                           Property * const skip = nullptr) {
        return NameFixer::makeNameUnique(
                    name, [this, skip](const QString& name) {
            return allNamesStartingWith(name, skip);
        });
    }
private:
    using ComplexAnimator::ca_addChild;
    using ComplexAnimator::ca_insertChild;
    using ComplexAnimator::ca_prependChild;

    using ComplexAnimator::ca_removeChild;
    using ComplexAnimator::ca_replaceChild;
    using ComplexAnimator::ca_takeChildAt;

    using ComplexAnimator::ca_removeAllChildren;

    using ComplexAnimator::ca_swapChildren;
    using ComplexAnimator::ca_moveChildInList;
    using ComplexAnimator::ca_moveChildBelow;
    using ComplexAnimator::ca_moveChildAbove;

    void restoreOrder(const QList<qsptr<Property>>& order) {
        ca_removeAllChildren();
        for(const auto& child : order)
            ca_addChild(child);
    }

    void clearOldParent(const qsptr<T>& futureChild) {
        const auto oldParent = futureChild->template getParent
                <DynamicComplexAnimatorBase<T>>();
        if(oldParent) oldParent->removeChild(futureChild);
    }

    QList<qsptr<Property>> mSavedOrder;
};

template <class T>
qsptr<T> TCreateOnly(eReadStream& src) {
    Q_UNUSED(src)
    return enve::make_shared<T>();
}

template <class T>
void TNoWriteType(T* const obj, eWriteStream& dst) {
    Q_UNUSED(obj)
    Q_UNUSED(dst)
}

template <class T>
qsptr<T> TCreateOnlyXML(const QDomElement& ele) {
    Q_UNUSED(ele)
    return enve::make_shared<T>();
}

template <class T>
void TNoWriteTypeXML(T* const obj, QDomElement& ele) {
    Q_UNUSED(obj)
    Q_UNUSED(ele)
}

template <class T,
          void (&TWriteType)(T* const obj, eWriteStream& dst) = TNoWriteType<T>,
          qsptr<T> (&TReadTypeAndCreate)(eReadStream& src) = TCreateOnly<T>,
          void (&TWriteTypeXML)(T* const obj, QDomElement& ele) = TNoWriteTypeXML<T>,
          qsptr<T> (&TReadTypeAndCreateXML)(const QDomElement& ele) = TCreateOnlyXML<T>>
class CORE_EXPORT DynamicComplexAnimator : public DynamicComplexAnimatorBase<T> {
    e_OBJECT
protected:
    DynamicComplexAnimator(const QString &name) :
        DynamicComplexAnimatorBase<T>(name) {}
public:
    qsptr<T> createDuplicate(T* const src) override {
        Clipboard clipboard(ClipboardType::property);
        clipboard.write([src](eWriteStream& dst) {
            TWriteType(src, dst);
            src->prp_writeProperty(dst);
        });
        qsptr<T> duplicate;
        clipboard.read([&duplicate](eReadStream& src) {
            duplicate = TReadTypeAndCreate(src);
            duplicate->prp_readProperty(src);
        });
        return duplicate;
    }

    void prp_writeProperty_impl(eWriteStream& dst) const override {
        const auto& children = this->ca_getChildren();
        const int nProps = children.count();
        dst << nProps;
        for(const auto& prop : children) {
            const auto futureId = dst.planFuturePos();
            const auto TProp = static_cast<T*>(prop.get());
            TWriteType(TProp, dst);
            TProp->prp_writeProperty(dst);
            dst.assignFuturePos(futureId);
        }
    }

    void prp_readProperty_impl(eReadStream& src) override {
        int nProps;
        src >> nProps;
        for(int i = 0; i < nProps; i++) {
            const auto futurePos = src.readFuturePos();
            try {
                const auto prop = TReadTypeAndCreate(src);
                prop->prp_readProperty(src);
                this->addChild(prop);
            } catch(const std::exception& e) {
                src.seek(futurePos);
                gPrintExceptionCritical(e);
            }
        }
    }
protected:
    QDomElement prp_writePropertyXML_impl(const Friction::Core::XmlExporter& exp) const override {
        if(!this->ca_hasChildren()) return QDomElement();
        auto result = exp.createElement(this->prp_tagNameXML());
        const auto& children = this->ca_getChildren();
        int id = 0;
        for(const auto& c : children) {
            const auto TProp = static_cast<T*>(c.get());
            const auto path = QString::number(id++) + "/";
            const auto expc = exp.withAssetsPath(path);
            auto child = c->prp_writePropertyXML(*expc);
            TWriteTypeXML(TProp, child);
            result.appendChild(child);
        }
        return result;
    }

    void prp_readPropertyXML_impl(const QDomElement& ele, const Friction::Core::XmlImporter& imp) override {
        const auto childNodes = ele.childNodes();
        const int count = childNodes.count();
        for(int i = 0; i < count; i++) {
            const auto node = childNodes.at(i);
            if(!node.isElement()) continue;
            const auto element = node.toElement();
            try {
                const auto prop = TReadTypeAndCreateXML(element);
                this->addChild(prop);
                const auto path = QString::number(i) + "/";
                prop->prp_readPropertyXML(element, imp.withAssetsPath(path));
            } catch(const std::exception& e) {
                gPrintExceptionCritical(e);
            }
        }
    }
};

#endif // DYNAMICCOMPLEXANIMATOR_H
