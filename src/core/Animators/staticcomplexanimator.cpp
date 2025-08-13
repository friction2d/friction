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

#include "staticcomplexanimator.h"
#include "ReadWrite/evformat.h"
#include "XML/xmlexporthelpers.h"

StaticComplexAnimator::StaticComplexAnimator(const QString &name) :
    ComplexAnimator(name) {}

void StaticComplexAnimator::prp_writeProperty_impl(eWriteStream &dst) const {
    const auto& children = ca_getChildren();
    for(const auto& prop : children)
        prop->prp_writeProperty(dst);
}

void StaticComplexAnimator::prp_readProperty_impl(eReadStream &src)
{
    const auto& children = ca_getChildren();
    const auto SVGProperties = QStringList() << "begin event" << "end event";
    const bool isSubPathEffect = prp_getName() == "sub-path effect";

    for (const auto& prop : children) {
        if (src.evFileVersion() < EvFormat::svgBeginEnd &&
            SVGProperties.contains(prop->prp_getName())) { continue; }
        if (src.evFileVersion() < EvFormat::subPathOffset &&
            isSubPathEffect && prop->prp_getName() == "offset") { continue; }
        prop->prp_readProperty(src);
    }
}

void StaticComplexAnimator::prp_readPropertyXML_impl(
        const QDomElement& ele, const Friction::Core::XmlImporter& imp) {
    const auto& children = ca_getChildren();
    for(const auto& c : children) {
        const QString tagName = c->prp_tagNameXML();
        const auto path = tagName + "/";
        const auto impc = imp.withAssetsPath(path);
        Friction::Core::XmlExportHelpers::readProperty(ele, impc, tagName, c.get());
    }
}

void StaticComplexAnimator::writeChildPropertiesXML(
        QDomElement& prop, const Friction::Core::XmlExporter& exp) const {
    const auto& children = ca_getChildren();
    for(const auto& c : children) {
        const QString tagName = c->prp_tagNameXML();
        const auto path = tagName + "/";
        const auto expc = exp.withAssetsPath(path);
        Friction::Core::XmlExportHelpers::writeProperty(prop, *expc, tagName, c.get());
    }
}

QDomElement StaticComplexAnimator::prp_writePropertyXML_impl(const Friction::Core::XmlExporter& exp) const {
    auto result = exp.createElement(prp_tagNameXML());
    writeChildPropertiesXML(result, exp);
    return result;
}
