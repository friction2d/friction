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

#include "scene.h"

#include "CacheHandlers/sceneframecontainer.h"
#include "Animators/sceneboundgradient.h"
#include "ReadWrite/ereadstream.h"
#include "ReadWrite/ewritestream.h"
#include "Sound/soundcomposition.h"
#include "framerange.h"


Scene::Scene(
    QString sceneName,
    qreal canvasWidth,
    qreal canvasHeight,
    qreal fps,
    ContainerBox defaultGroup = ContainerBox(sceneName)
    ) : _currentGroup(defaultGroup)
      , _name(sceneName)
      , _canvasWidth(canvasWidth)
      , _canvasHeight(canvasHeight)
      , _fps(fps) {};

Scene::~Scene() {
    emit destroyed();
};

void Scene::setCurrentGroupParentAsCurrentGroup()
{
    auto currentGroup = getCurrentGroup();

    setCurrentGroup(currentGroup->getParentGroup());
}

void Scene::saveSVG(SvgExporter& exp, DomEleTask* const eleTask) const {
    _currentGroup->saveSVG(exp, eleTask);
};

void Scene::writeBoxOrSoundXEV(const stdsptr<XevZipFileSaver>& xevFileSaver,
                        const RuntimeIdToWriteId& objListIdConv,
                        const QString& path) const {
    _currentGroup->writeBoxOrSoundXEV(xevFileSaver, objListIdConv, path);
};

void Scene::readBoxOrSoundXEV(XevReadBoxesHandler& boxReadHandler,
                       ZipFileLoader& fileLoader, const QString& path,
                       const RuntimeIdToWriteId& objListIdConv) {
    _currentGroup->readBoxOrSoundXEV(boxReadHandler, fileLoader, path, objListIdConv);
};

void Scene::writeAllContained(eWriteStream &dst) const {
    return _currentGroup->writeAllContained(dst);
};

void Scene::writeAllContainedXEV(const stdsptr<XevZipFileSaver>& fileSaver,
                          const RuntimeIdToWriteId& objListIdConv,
                          const QString& path) const {
    _currentGroup->writeAllContainedXEV(fileSaver, objListIdConv, path);
};

QString Scene::makeNameUniqueForDescendants(
        const QString &name, eBoxOrSound * const skip) {
    return NameFixer::makeNameUnique(
                name, [this, skip](const QString& name) {
        return getCurrentGroup()->allDescendantsNamesStartingWith(name, skip);
    });
}

QString ContainerBox::makeNameUniqueForContained(
        const QString &name, eBoxOrSound * const skip) {
    return NameFixer::makeNameUnique(
                name, [this, skip](const QString& name) {
        return allContainedNamesStartingWith(name, skip);
    });
}
