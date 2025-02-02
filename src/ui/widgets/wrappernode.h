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

#ifndef WRAPPERNODE_H
#define WRAPPERNODE_H

#include "ui_global.h"

#include <QBoxLayout>
#include <QtCore>
#include <QDomElement>
#include "smartPointers/ememory.h"
#include "widgets/widgetstack.h"
#include "XML/runtimewriteid.h"

#include "ReadWrite/ereadstream.h"
#include "ReadWrite/ewritestream.h"
#include "XML/xmlimporter.h"

enum class UI_EXPORT WrapperNodeType {
    base,
    widget,
    splitH,
    splitV
};

class SplitWrapperNode;
class ParentWrapperNode;
class WidgetWrapperNode;
class Canvas;

class UI_EXPORT WrapperNode {
public:
    typedef std::function<WidgetWrapperNode*(Canvas*)> WidgetCreator;
    WrapperNode(const WrapperNodeType type,
                const WidgetCreator& creator) :
        fType(type), fCreator(creator) {}
    virtual ~WrapperNode() {}

    const WrapperNodeType fType;
    const WidgetCreator fCreator;
    ParentWrapperNode* fParent = nullptr;

    virtual QWidget* widget() = 0;

    void write(eWriteStream& dst) {
        dst.write(&fType, sizeof(WrapperNodeType));
        writeData(dst);
    }

    QDomElement writeXEV(QDomDocument& doc, Friction::Core::RuntimeIdToWriteId& objListIdConv);

    static WrapperNode *sRead(eReadStream& src,
                              const WidgetCreator& creator);
    static WrapperNode *sReadXEV(Friction::Core::XmlReadBoxesHandler& boxReadHandler,
                                 const QDomElement& ele,
                                 const WidgetCreator& creator,
                                 Friction::Core::RuntimeIdToWriteId& objListIdConv);
protected:
    virtual void readData(eReadStream& src) = 0;
    virtual void writeData(eWriteStream& dst) = 0;
    virtual void writeDataXEV(QDomElement& ele, QDomDocument& doc,
                              Friction::Core::RuntimeIdToWriteId& objListIdConv) = 0;
    virtual void readDataXEV(Friction::Core::XmlReadBoxesHandler& boxReadHandler,
                             const QDomElement& ele,
                             Friction::Core::RuntimeIdToWriteId& objListIdConv) = 0;
    virtual QString tagNameXEV() const = 0;
};

class UI_EXPORT ParentWrapperNode : public WrapperNode {
protected:
    using WrapperNode::WrapperNode;
public:
    virtual void closeChild(WrapperNode* const child) = 0;

    virtual void replaceChild(WrapperNode* const child,
                              WrapperNode* const with) = 0;
};

class UI_EXPORT BaseWrapperNode : public QWidget, public ParentWrapperNode {
public:
    BaseWrapperNode(const QString& tagName, const WidgetCreator& creator) :
        ParentWrapperNode(WrapperNodeType::base, creator),
        mTagName(tagName) {
        setLayout(new QVBoxLayout);
        layout()->setContentsMargins(0, 0, 0, 0);
    }

    WrapperNode* fChild = nullptr;

    void closeChild(WrapperNode* const child) {
        Q_UNUSED(child)
        return;
//        if(child == fChild) fChild.reset();
//        else Q_ASSERT(false);
    }

    void replaceChild(WrapperNode* const child,
                      WrapperNode* const with) {
        layout()->replaceWidget(child->widget(), with->widget());

        with->fParent = this;
        if(child == fChild) fChild = with;
        else Q_ASSERT(false);
    }

    QWidget* widget() { return this; }

    void reset(Canvas * const scene = nullptr);

    void readData(eReadStream& src) {
        const auto newChild = sRead(src, fCreator);
        replaceAndDeleteChild(newChild);
    }

    void writeData(eWriteStream& dst) {
        fChild->write(dst);
    }

    void writeDataXEV(QDomElement& ele, QDomDocument& doc,
                      Friction::Core::RuntimeIdToWriteId& objListIdConv) {
        const auto child = fChild->writeXEV(doc, objListIdConv);
        ele.appendChild(child);
    }

    void readDataXEV(Friction::Core::XmlReadBoxesHandler& boxReadHandler,
                     const QDomElement& ele,
                     Friction::Core::RuntimeIdToWriteId& objListIdConv) {
        const auto child = ele.firstChildElement();
        const auto childTag = child.tagName();
        const auto newChild = sReadXEV(boxReadHandler, child, fCreator, objListIdConv);
        replaceAndDeleteChild(newChild);
    }

    QString tagNameXEV() const { return mTagName; }
private:
    using WrapperNode::write;
    const QString mTagName;

    void replaceAndDeleteChild(WrapperNode* newChild) {
        newChild->fParent = this;
        if(fChild) {
            layout()->replaceWidget(fChild->widget(), newChild->widget());
            delete fChild;
        } else layout()->addWidget(newChild->widget());
        fChild = newChild;
    }
};

class UI_EXPORT SplitWrapperNode : public ParentWrapperNode {
protected:
    using ParentWrapperNode::ParentWrapperNode;
public:
    WrapperNode* fChild1 = nullptr;
    WrapperNode* fChild2 = nullptr;

    void closeChild(WrapperNode* const child) {
        Q_ASSERT(child == fChild1 || child == fChild2);
        const auto remChild = child == fChild1 ? fChild2 : fChild1;
        takeWidget(remChild->widget());
        fParent->replaceChild(this, remChild);
        delete this;
    }

    void replaceChild(WrapperNode* const child,
                      WrapperNode* const with) {
        with->fParent = this;
        if(child == fChild1) fChild1 = with;
        else if(child == fChild2) fChild2 = with;
        else Q_ASSERT(false);
    }
protected:
    virtual QWidget* takeWidget(QWidget* const wid) = 0;

    void readData(eReadStream& src) {
        fChild1 = sRead(src, fCreator);
        fChild2 = sRead(src, fCreator);

        fChild1->fParent = this;
        fChild2->fParent = this;
    }

    void writeData(eWriteStream& dst) {
        fChild1->write(dst);
        fChild2->write(dst);
    }

    void writeDataXEV(QDomElement& ele, QDomDocument& doc,
                      Friction::Core::RuntimeIdToWriteId& objListIdConv) {
        const auto child1 = fChild1->writeXEV(doc, objListIdConv);
        const auto child2 = fChild2->writeXEV(doc, objListIdConv);

        ele.appendChild(child1);
        ele.appendChild(child2);
    }

    void readDataXEV(Friction::Core::XmlReadBoxesHandler& boxReadHandler,
                     const QDomElement& ele,
                     Friction::Core::RuntimeIdToWriteId& objListIdConv) {
        const auto child1 = ele.firstChildElement();
        const auto child2 = ele.lastChildElement();

        fChild1 = sReadXEV(boxReadHandler, child1, fCreator, objListIdConv);
        fChild2 = sReadXEV(boxReadHandler, child2, fCreator, objListIdConv);

        fChild1->fParent = this;
        fChild2->fParent = this;
    }
};

class UI_EXPORT VWidgetStackNode : public VWidgetStack, public SplitWrapperNode {
public:
    VWidgetStackNode(const WidgetCreator& creator) :
        SplitWrapperNode(WrapperNodeType::splitV, creator) {}

    QWidget* widget() { return this; }

    void replaceChild(WrapperNode* const child,
                      WrapperNode* const with) {
        replaceWidget(child->widget(), with->widget());
        SplitWrapperNode::replaceChild(child, with);
    }
protected:
    QWidget* takeWidget(QWidget* const wid) {
        return VWidgetStack::takeWidget(wid);
    }

    void readData(eReadStream& src) {
        SplitWrapperNode::readData(src);
        qreal child2frac; src >> child2frac;
        appendWidget(fChild1->widget());
        appendWidget(fChild2->widget(), child2frac);
    }

    void writeData(eWriteStream& dst) {
        SplitWrapperNode::writeData(dst);
        dst << percentAt(1);
    }

    void writeDataXEV(QDomElement& ele, QDomDocument& doc,
                      Friction::Core::RuntimeIdToWriteId& objListIdConv) {
        SplitWrapperNode::writeDataXEV(ele, doc, objListIdConv);
        ele.setAttribute("proportions", percentAt(1));
    }

    void readDataXEV(Friction::Core::XmlReadBoxesHandler& boxReadHandler,
                     const QDomElement& ele,
                     Friction::Core::RuntimeIdToWriteId& objListIdConv) {
        SplitWrapperNode::readDataXEV(boxReadHandler, ele, objListIdConv);
        const QString child2fracStr = ele.attribute("proportions");
        const qreal child2frac = Friction::Core::XmlExportHelpers::stringToDouble(child2fracStr);
        appendWidget(fChild1->widget());
        appendWidget(fChild2->widget(), child2frac);
    }

    QString tagNameXEV() const { return "VSplit"; };
};

class UI_EXPORT HWidgetStackNode : public HWidgetStack, public SplitWrapperNode {
public:
    HWidgetStackNode(const WidgetCreator& creator) :
        SplitWrapperNode(WrapperNodeType::splitH, creator) {}

    QWidget* widget() { return this; }

    void replaceChild(WrapperNode* const child,
                      WrapperNode* const with) {
        replaceWidget(child->widget(), with->widget());
        SplitWrapperNode::replaceChild(child, with);
    }
protected:
    QWidget* takeWidget(QWidget* const wid) {
        return HWidgetStack::takeWidget(wid);
    }

    void readData(eReadStream& src) {
        SplitWrapperNode::readData(src);
        qreal child2frac; src >> child2frac;
        appendWidget(fChild1->widget());
        appendWidget(fChild2->widget(), child2frac);
    }

    void writeData(eWriteStream& dst) {
        SplitWrapperNode::writeData(dst);
        dst << percentAt(1);
    }

    void writeDataXEV(QDomElement& ele, QDomDocument& doc,
                      Friction::Core::RuntimeIdToWriteId& objListIdConv) {
        SplitWrapperNode::writeDataXEV(ele, doc, objListIdConv);
        ele.setAttribute("proportions", percentAt(1));
    }

    void readDataXEV(Friction::Core::XmlReadBoxesHandler& boxReadHandler,
                     const QDomElement& ele,
                     Friction::Core::RuntimeIdToWriteId& objListIdConv) {
        SplitWrapperNode::readDataXEV(boxReadHandler, ele, objListIdConv);
        const QString child2fracStr = ele.attribute("proportions");
        const qreal child2frac = Friction::Core::XmlExportHelpers::stringToDouble(child2fracStr);
        appendWidget(fChild1->widget());
        appendWidget(fChild2->widget(), child2frac);
    }

    QString tagNameXEV() const { return "HSplit"; };
};

#endif // WRAPPERNODE_H
