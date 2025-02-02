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

#ifndef CUSTOMIDENTIFIER_H
#define CUSTOMIDENTIFIER_H

#include "exceptions.h"

#include <QDomElement>
#include <QVector>

#include "ReadWrite/ereadstream.h"
#include "ReadWrite/ewritestream.h"

struct CORE_EXPORT CustomIdentifier {
    struct Version {
        uint fMajor;
        uint fMinor;
        uint fPatch;

        QString toString() const {
            return QString::number(fMajor) + "." +
                   QString::number(fMinor) + "." +
                   QString::number(fPatch);
        }

        static Version sFromString(const QString& str) {
            const auto sep = str.splitRef('.');
            if(sep.count() != 3) RuntimeThrow("Invalid Version format " + str);
            return {sep[0].toUInt(), sep[1].toUInt(), sep[2].toUInt()};
        }

        bool operator==(const Version& other) const {
            return fMajor == other.fMajor &&
                   fMinor == other.fMinor &&
                   fPatch == other.fPatch;
        }

        bool operator>(const Version& other) const {
            if(fMajor < other.fMajor) return false;
            if(fMajor > other.fMajor) return true;
            if(fMinor < other.fMinor) return false;
            if(fMinor > other.fMinor) return true;
            return fPatch > other.fPatch;
        }

        bool operator<(const Version& other) const {
            return other > *this;
        }
    };

    QString fEffectId;
    QString fEffectName;
    Version fVersion;

    QString toString() const;

    void write(eWriteStream& dst) const;
    void writeXML(QDomElement& ele) const;

    static CustomIdentifier sRead(eReadStream& src);
    static CustomIdentifier sReadXML(const QDomElement& ele);
};

#endif // CUSTOMIDENTIFIER_H
