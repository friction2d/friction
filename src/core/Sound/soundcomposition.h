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

#ifndef SOUNDCOMPOSITION_H
#define SOUNDCOMPOSITION_H

#include "Animators/dynamiccomplexanimator.h"
#include "CacheHandlers/hddcachablecachehandler.h"
#include "CacheHandlers/samples.h"
#include "esound.h"
#include "esoundsettings.h"
#include <math.h>

#include <QAudioOutput>
#include <QByteArray>
#include <QComboBox>
#include <QIODevice>
#include <QLabel>
#include <QMainWindow>
#include <QObject>
#include <QPushButton>
#include <QSlider>
#include <QTimer>
class Scene;
class SoundMerger;

class CORE_EXPORT SoundComposition : public QIODevice {
public:
    SoundComposition(Scene * const parent);

    void start(const int startFrame);
    void stop();

    qint64 readData(char *data, qint64 maxLen);
    qint64 writeData(const char *data, qint64 len);

    void addSound(const qsptr<eSound> &sound);
    void removeSound(const qsptr<eSound> &sound);

    void secondFinished(const int secondId,
                        const stdsptr<Samples>& samples);

    void setMinFrameUseRange(const int frame);
    void setMaxFrameUseRange(const int frame);

    void clearUseRange();

    void scheduleFrameRange(const FrameRange& range);
    SoundMerger * scheduleFrame(const int frameId);
    HddCachableCacheHandler& getCacheHandler() {
        return mSecondsCache;
    }

    bool hasAnySounds() const { return !mSounds.isEmpty(); }
private:
    SoundMerger * scheduleSecond(const int secondId);

    void frameRangeChanged(const FrameRange &range);

    void secondRangeChanged(const iValueRange &range) {
        mSecondsCache.remove(range);
    }

    eSoundSettingsData mSettings;
    QList<int> mProcessingSeconds;
    const Scene * const mParent;
    qint64 mPos;
    ConnContextObjList<qsptr<eSound>> mSounds;
    HddCachableCacheHandler mSecondsCache;
};

#endif // SOUNDCOMPOSITION_H
