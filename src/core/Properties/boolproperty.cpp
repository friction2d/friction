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

#include "boolproperty.h"

BoolProperty::BoolProperty(const QString &name) :
    Property(name) {}

void BoolProperty::prp_writeProperty_impl(eWriteStream& dst) const {
    dst << mValue;
}

void BoolProperty::prp_readProperty_impl(eReadStream& src) {
    src >> mValue;
}

QDomElement BoolProperty::prp_writePropertyXML_impl(const Friction::Core::XmlExporter& exp) const {
    auto result = exp.createElement("Combo");
    result.setAttribute("value", mValue ? "true" : "false");
    return result;
}

void BoolProperty::prp_readPropertyXML_impl(
        const QDomElement& ele, const Friction::Core::XmlImporter& imp) {
    Q_UNUSED(imp)
    const auto valueStr = ele.attribute("value");
    mValue = valueStr == "true";
}

bool BoolProperty::getValue() {
    return mValue;
}

void BoolProperty::setValue(const bool value)
{
    if (mValue == value) { return; }
    {
        prp_pushUndoRedoName(value ? tr("Enable Property") : tr("Disable Property"));
        UndoRedo ur;
        const auto oldValue = mValue;
        const auto newValue = value;
        ur.fUndo = [this, oldValue]() { setValue(oldValue); };
        ur.fRedo = [this, newValue]() { setValue(newValue); };
        prp_addUndoRedo(ur);
    }
    mValue = value;
    prp_afterWholeInfluenceRangeChanged();
    emit valueChanged(value);
}
