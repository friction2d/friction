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

#ifndef IMPORTHANDLER_H
#define IMPORTHANDLER_H
#include <QFileInfo>
#include <memory>
#include "Boxes/boundingbox.h"

class CORE_EXPORT eImporter {
public:
    virtual ~eImporter();

    virtual bool supports(const QFileInfo& fileInfo) const = 0;
    //! @brief Can return nullptr.
    virtual qsptr<BoundingBox> import(const QFileInfo& fileInfo,
                                      Scene* const scene) const = 0;
private:
};

class CORE_EXPORT ImportHandler {
public:
    ImportHandler();

    static ImportHandler* sInstance;

    template<typename T>
    void addImporter() {
        mImporters << std::make_shared<T>();
    }

    qsptr<BoundingBox> import(const QString& path, Scene* const scene) const;
private:
    QList<std::shared_ptr<eImporter>> mImporters;
};

#endif // IMPORTHANDLER_H
