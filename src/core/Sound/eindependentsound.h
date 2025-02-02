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

#ifndef EINDEPENDENTSOUND_H
#define EINDEPENDENTSOUND_H

#include "esoundobjectbase.h"

#include "ReadWrite/ereadstream.h"
#include "ReadWrite/ewritestream.h"

class CORE_EXPORT eIndependentSound : public eSoundObjectBase {
    e_OBJECT
    e_DECLARE_TYPE(eIndependentSound)
protected:
    eIndependentSound();
public:
    bool SWT_shouldBeVisible(const SWT_RulesCollection &rules,
                             const bool parentSatisfies,
                             const bool parentMainTarget) const;

    void prp_setupTreeViewMenu(PropertyMenu * const menu);

    void prp_writeProperty_impl(eWriteStream& dst) const;
    void prp_readProperty_impl(eReadStream& src);
protected:
    void prp_readPropertyXEV_impl(const QDomElement& ele, const Friction::Core::XmlImporter& imp);
    QDomElement prp_writePropertyXEV_impl(const Friction::Core::XmlExporter& exp) const;

    void updateDurationRectLength();
public:
    void setFilePath(const QString &path);

private:
    void setFilePathNoRename(const QString &path);

    void fileHandlerConnector(ConnContext &conn, SoundFileHandler *obj);
    void fileHandlerAfterAssigned(SoundFileHandler *obj);

    FileHandlerObjRef<SoundFileHandler> mFileHandler;
};

#endif // EINDEPENDENTSOUND_H
