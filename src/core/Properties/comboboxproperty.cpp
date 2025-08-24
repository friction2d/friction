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

#include "comboboxproperty.h"
#include "XML/xmlexporthelpers.h"

ComboBoxProperty::ComboBoxProperty(const QString &name,
                                   const QStringList &list) :
    Property(name) {
    mValueNames = list;
}

void ComboBoxProperty::prp_writeProperty_impl(eWriteStream& dst) const {
    dst << mCurrentValue;
}

void ComboBoxProperty::prp_readProperty_impl(eReadStream& src) {
    src >> mCurrentValue;
}

QDomElement ComboBoxProperty::prp_writePropertyXML_impl(const Friction::Core::XmlExporter& exp) const {
    auto result = exp.createElement("Combo");
    result.setAttribute("value", mCurrentValue);
    return result;
}

void ComboBoxProperty::prp_readPropertyXML_impl(
        const QDomElement& ele, const Friction::Core::XmlImporter& imp) {
    Q_UNUSED(imp)
    const auto valueStr = ele.attribute("value");
    mCurrentValue = Friction::Core::XmlExportHelpers::stringToInt(valueStr);
}

QString ComboBoxProperty::getCurrentValueName() {
    if(mCurrentValue >= mValueNames.count()) return "null";
    return mValueNames.at(mCurrentValue);
}

void ComboBoxProperty::setCurrentValue(const int id)
{
    if (mCurrentValue == id) { return; }
    {
        prp_pushUndoRedoName(tr("Set ") + mValueNames.at(id));
        UndoRedo ur;
        const auto oldValue = mCurrentValue;
        const auto newValue = id;
        ur.fUndo = [this, oldValue]() { setCurrentValue(oldValue); };
        ur.fRedo = [this, newValue]() { setCurrentValue(newValue); };
        prp_addUndoRedo(ur);
    }
    mCurrentValue = id;
    emit valueChanged(id);
    prp_afterWholeInfluenceRangeChanged();
}
