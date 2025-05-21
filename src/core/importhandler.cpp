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

#include "importhandler.h"

#include "exceptions.h"

ImportHandler* ImportHandler::sInstance = nullptr;

eImporter::~eImporter() {}

ImportHandler::ImportHandler() {
    Q_ASSERT(!sInstance);
    sInstance = this;
}

qsptr<BoundingBox> ImportHandler::import(const QString &path,
                                         Scene* const scene) const {
    {
        const QFile file(path);
        if(!file.exists()) RuntimeThrow("File does not exist");
    }
    const QFileInfo info(path);
    for(const auto& importer : mImporters) {
        if(importer->supports(info)) {
            try {
                return importer->import(info, scene);
            } catch(...) {
                const auto imp = importer.get();
                RuntimeThrow("Importer " + typeid(*imp).name() + " failed.");
            }
        }
    }
    RuntimeThrow("Unsupported file format:\n'" + path + "'\n");
}
