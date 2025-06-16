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

#include "scenechooser.h"
#include "Private/document.h"
#include "Private/scene.h"

SceneChooser::SceneChooser(Document& document, const bool active,
                           QWidget* const parent) :
    QMenu("none", parent), mDocument(document) {
    if(active) {
        const auto active = addAction("Active");
        active->setCheckable(true);
        addSeparator();
        connect(active, &QAction::toggled,
                this, [this](const bool active) {
            if(active) {
                connect(&mDocument, &Document::activeSceneSet,
                        this, qOverload<Scene*>(&SceneChooser::setCurrentScene));
                setCurrentScene(mDocument.fActiveScene);
            } else {
                disconnect(&mDocument, &Document::activeSceneSet,
                           this, qOverload<Scene*>(&SceneChooser::setCurrentScene));
            }
        });
    }
    for(const auto& scene : mDocument.fScenes)
        addScene(scene.get());

    connect(&mDocument, qOverload<Scene*>(&Document::sceneRemoved),
            this, &SceneChooser::removeScene);
    connect(&mDocument, &Document::sceneCreated,
            this, &SceneChooser::addScene);

    if(isEmpty()) setDisabled(true);
}

void SceneChooser::addScene(Scene * const scene) {
    if(!scene) return;
    if(isEmpty()) setEnabled(true);
    const auto act = addAction(scene->name());
    act->setCheckable(true);
    connect(act, &QAction::triggered, this,
            [this, scene, act]() {
        setCurrentScene(scene, act);
        Document::sInstance->actionFinished();
    });
    connect(scene, &Scene::nameChanged, act,
            [this, scene, act](const QString& name) {
        if(scene == mCurrentScene) setTitle(name);
        act->setText(name);
    });
    mSceneToAct.insert({scene, act});
    //if(!mCurrentScene) setCurrentScene(scene, act);
}

void SceneChooser::removeScene(Scene * const scene) {
    const auto removeIt = mSceneToAct.find(scene);
    if(removeIt == mSceneToAct.end()) return;
    removeAction(removeIt->second);
    delete removeIt->second;
    mSceneToAct.erase(removeIt);
    if(mCurrentScene == scene) {
        const auto newIt = mSceneToAct.begin();
        if(newIt == mSceneToAct.end()) setCurrentScene(nullptr);
        else setCurrentScene(newIt->first, newIt->second);
    }
    if(isEmpty()) setDisabled(true);
}

void SceneChooser::setCurrentScene(Scene * const scene) {
    if(scene == mCurrentScene) return;
    if(!scene) return setCurrentScene(nullptr, nullptr);
    const auto it = mSceneToAct.find(scene);
    if(it == mSceneToAct.end()) return;
    setCurrentScene(scene, it->second);
}

void SceneChooser::setCurrentScene(Scene * const scene, QAction * const act) {
    if(scene == mCurrentScene) return;
    if(act) {
        act->setChecked(true);
        act->setDisabled(true);
    }
    if(mCurrentScene) {
        const auto currIt = mSceneToAct.find(mCurrentScene);
        if(currIt != mSceneToAct.end()) {
            const auto currAct = currIt->second;
            currAct->setChecked(false);
            currAct->setEnabled(true);
        }
    }
    setTitle(scene ? scene->name() : "none");
    mCurrentScene = scene;
    emit currentChanged(mCurrentScene);
}
