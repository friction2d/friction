/*
#
# Friction - https://friction.graphics
#
# Copyright (c) Ole-André Rodlie and contributors
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3.
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

#include "svgo.h"

#include <QDebug>
#include <QRegularExpression>

using namespace Friction::Core;

QString SVGO::optimize(const QString &svg)
{
    QDomDocument doc;
    QString errorMsg;

    if (!doc.setContent(svg, &errorMsg)) {
        qWarning() << "Failed to read SVG:" << errorMsg;
        return svg;
    }

    removeProcessingInstructions(doc);
    removeUselessDefs(doc);

    bool changed = true;
    QDomElement root = doc.documentElement();

    if (root.isNull()) { return svg; }

    while (changed) { changed = recursiveOptimize(root); }

    return minify(doc.toString());
}

bool SVGO::recursiveOptimize(QDomElement &element)
{
    bool changed = false;
    QDomNode child = element.firstChild();

    while (!child.isNull()) {
        QDomNode nextChild = child.nextSibling();
        if (child.isElement()) {
            QDomElement childElem = child.toElement();
            if (recursiveOptimize(childElem)) { changed = true; }
        }
        child = nextChild;
    }

    if (element.tagName() == "g") {
        QDomNode parent = element.parentNode();
        if (parent.isNull() || !parent.isElement()) { return changed; }

        const int elementChildCount = countElementChildren(element);

        if (elementChildCount == 0) {
            parent.removeChild(element);
            return true;
        }

        if (elementChildCount != 1) { return changed; }

        QDomElement singleChild = element.firstChildElement();

        if (singleChild.tagName() != "g") { return changed; }
        if (element.hasAttribute("id")) { return changed; }

        QDomElement childGroup = singleChild;

        if (childGroup.elementsByTagName("animateTransform").count() > 0) {
            return changed;
        }

        QDomNamedNodeMap oldAttrs = element.attributes();

        for (int i = 0; i < oldAttrs.count(); ++i) {
            QDomAttr attr = oldAttrs.item(i).toAttr();
            QString attrName = attr.name();

            if (attrName == "transform" && childGroup.hasAttribute("transform")) {
                QString mergedTransform = attr.value() + " " + childGroup.attribute("transform");
                childGroup.setAttribute("transform", mergedTransform.trimmed());
            } else {
                if (!childGroup.hasAttribute(attrName)) {
                    childGroup.setAttribute(attrName, attr.value());
                }
            }
        }

        parent.insertBefore(childGroup, element);
        parent.removeChild(element);

        return true;
    }
    return changed;
}

void SVGO::removeUselessDefs(QDomDocument &doc)
{
    QDomElement root = doc.documentElement();
    if (root.isNull()) { return; }

    QDomElement defs = root.firstChildElement("defs");
    if (defs.isNull()) { return; }

    QDomNode defChild = defs.firstChild();
    while (!defChild.isNull()) {
        QDomNode nextDefChild = defChild.nextSibling();

        if (defChild.isElement()) {
            QDomElement defElem = defChild.toElement();
            QString id = defElem.attribute("id");

            if (id.isEmpty()) {
                defChild = nextDefChild;
                continue;
            }

            QString searchId = "#" + id;
            bool isReferenced = false;

            QDomNodeList allElements = doc.elementsByTagName("*");
            for (int i = 0; i < allElements.count(); ++i) {
                QDomElement elem = allElements.at(i).toElement();
                QDomNamedNodeMap attrs = elem.attributes();
                for (int j = 0; j < attrs.count(); ++j) {
                    QDomAttr attr = attrs.item(j).toAttr();
                    if (attr.value().contains(searchId)) {
                        isReferenced = true;
                        break;
                    }
                }
                if (isReferenced) { break; }
            }

            if (!isReferenced) { defs.removeChild(defChild); }
        }
        defChild = nextDefChild;
    }

    if (!defs.hasChildNodes()) { root.removeChild(defs); }
}

void SVGO::removeProcessingInstructions(QDomDocument &doc)
{
    QDomNode node = doc.firstChild();
    while (!node.isNull()) {
        QDomNode nextNode = node.nextSibling();
        if (node.isProcessingInstruction()) {
            doc.removeChild(node);
        }
        node = nextNode;
    }
}

int SVGO::countElementChildren(const QDomElement &element)
{
    int count = 0;
    QDomNode child = element.firstChild();
    while (!child.isNull()) {
        if (child.isElement()) { count++; }
        child = child.nextSibling();
    }
    return count;
}

QString SVGO::minify(const QString &xml)
{
    static QRegularExpression regex(">\\s+<");
    QString minified = xml;
    minified.replace(regex, "><");
    return minified.trimmed();
}
