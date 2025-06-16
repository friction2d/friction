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

#include "renderinstancesettings.h"
#include "Private/scene.h"
#include "simpletask.h"

RenderInstanceSettings::RenderInstanceSettings(Scene* scene) {
    setTargetScene(scene);
}

RenderInstanceSettings::RenderInstanceSettings(const RenderInstanceSettings &src) :
    QObject() {
    setTargetScene(src.getTargetScene());
    mOutputDestination = src.mOutputDestination;
    mOutputSettingsProfile = src.mOutputSettingsProfile;
    mRenderSettings = src.mRenderSettings;
    mOutputSettings = src.mOutputSettings;
}

QString RenderInstanceSettings::getName() {
    return mTargetScene ? mTargetScene->name() : "-none-";
}

void RenderInstanceSettings::setOutputDestination(
        const QString &outputDestination) {
    mOutputDestination = outputDestination;
}

const QString &RenderInstanceSettings::getOutputDestination() const {
    return mOutputDestination;
}

void RenderInstanceSettings::setTargetScene(
        Scene *scene, const bool copySceneSettings) {
    const auto oldScene = *mTargetScene;
    auto& conn = mTargetScene.assign(scene);
    if(scene) {
        if(!oldScene && copySceneSettings) {
            const auto frameRange = scene->getFrameRange();
            mRenderSettings.fMinFrame = frameRange.fMin;
            mRenderSettings.fMaxFrame = frameRange.fMax;
            mRenderSettings.fBaseWidth = scene->canvasWidth();
            mRenderSettings.fBaseHeight = scene->canvasHeight();
            mRenderSettings.fVideoWidth = qRound(mRenderSettings.fBaseWidth*
                                                 mRenderSettings.fResolution);
            mRenderSettings.fVideoHeight = qRound(mRenderSettings.fBaseHeight*
                                                  mRenderSettings.fResolution);
            mRenderSettings.fBaseFps = scene->fps();
            mRenderSettings.fFps = mRenderSettings.fBaseFps;
        }
        conn << connect(scene, &Scene::dimensionsChanged,
                        this, [this](const int width, const int height) {
            mRenderSettings.fBaseWidth = width;
            mRenderSettings.fBaseHeight = height;
            mRenderSettings.fVideoWidth = qRound(mRenderSettings.fBaseWidth*
                                                 mRenderSettings.fResolution);
            mRenderSettings.fVideoHeight = qRound(mRenderSettings.fBaseHeight*
                                                  mRenderSettings.fResolution);
        });
        conn << connect(scene, &Scene::fpsChanged,
                        this, [this](const qreal fps) {
            mRenderSettings.fBaseFps = fps;
            mRenderSettings.fFps = mRenderSettings.fBaseFps;
        });
    }
}

Scene *RenderInstanceSettings::getTargetScene() const {
    return mTargetScene;
}

void RenderInstanceSettings::setCurrentRenderFrame(
        const int currentRenderFrame) {
    mCurrentRenderFrame = currentRenderFrame;
    emit renderFrameChanged(currentRenderFrame);
}

int RenderInstanceSettings::currentRenderFrame() {
    return mCurrentRenderFrame;
}

const OutputSettings &RenderInstanceSettings::getOutputRenderSettings() const {
    return mOutputSettings;
}

void RenderInstanceSettings::setOutputRenderSettings(
        const OutputSettings &settings) {
    mOutputSettings = settings;
}

const RenderSettings &RenderInstanceSettings::getRenderSettings() const {
    return mRenderSettings;
}

void RenderInstanceSettings::setRenderSettings(
        const RenderSettings &settings) {
    mRenderSettings = settings;
}

void RenderInstanceSettings::renderingAboutToStart() {
    mRenderError.clear();
    mRenderSettings.fTimeBase = { 1, qRound(mRenderSettings.fFps) };
    mRenderSettings.fFrameInc = mRenderSettings.fBaseFps/mRenderSettings.fFps;
}

#include <QSound>
void RenderInstanceSettings::setCurrentState(const RenderState &state,
                                             const QString &text) {
    mState = state;
    if(mState == RenderState::error) mRenderError = text;
    //if(mState == FINISHED) QSound::play(":/");
    emit stateChanged(mState);
}

const QString &RenderInstanceSettings::getRenderError() const {
    return mRenderError;
}

RenderState RenderInstanceSettings::getCurrentState() const {
    return mState;
}

void RenderInstanceSettings::setOutputSettingsProfile(
        OutputSettingsProfile *profile) {
    auto& conn = mOutputSettingsProfile.assign(profile);
    if(profile) {
        conn << connect(profile, &OutputSettingsProfile::changed,
                        this, [this, profile]() {
            mOutputSettings = profile->getSettings();
        });
        mOutputSettings = profile->getSettings();
    }
}

OutputSettingsProfile *RenderInstanceSettings::getOutputSettingsProfile() {
    return mOutputSettingsProfile;
}

void RenderInstanceSettings::write(eWriteStream &dst) const {
    dst.write(&mState, sizeof(RenderState));
    dst << mOutputDestination;
    dst << mRenderError;
    mRenderSettings.write(dst);
    mOutputSettings.write(dst);

    dst << (mOutputSettingsProfile ? mOutputSettingsProfile->getName() : "");

    int targetWriteId = -1;
    int targetDocumentId = -1;

    if(mTargetScene) {
        targetWriteId = mTargetScene->getWriteId();
        targetDocumentId = mTargetScene->getDocumentId();
    }
    dst << targetWriteId;
    dst << targetDocumentId;
}

void RenderInstanceSettings::read(eReadStream &src) {
    src.read(&mState, sizeof(RenderState));
    src >> mOutputDestination;
    src >> mRenderError;
    mRenderSettings.read(src);
    mOutputSettings.read(src);

    // always set render state to none on read
    mState = RenderState::none;

    QString outputProfile; src >> outputProfile;
    const auto profile = OutputSettingsProfile::sGetByName(outputProfile);
    setOutputSettingsProfile(profile);

    int targetReadId;
    src >> targetReadId;
    int targetDocumentId;
    src >> targetDocumentId;
    if(targetReadId != -1 && targetDocumentId != -1) {
        mTargetScene.assign(nullptr);
        src.addReadStreamDoneTask([this, targetReadId, targetDocumentId]
                                  (eReadStream& src) {
            BoundingBox* box = nullptr;
            if(targetReadId != -1)
                box = src.getBoxByReadId(targetReadId);
             if(!box && targetDocumentId != -1)
                 box = BoundingBox::sGetBoxByDocumentId(targetDocumentId);
             // TODO(kaixoo): cast to Scene
             if(const auto scene = enve_cast<Scene*>(box))
                 setTargetScene(scene, false);
        });
    }

    emit stateChanged(mState);
}
