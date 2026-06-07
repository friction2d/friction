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

#ifndef AUDIOHANDLER_H
#define AUDIOHANDLER_H

#include "core_global.h"

#include <QIODevice>
#include <QDebug>
#include <QAudioFormat>

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
#include <QAudioSink>
#include <QAudioDevice>
#include <QMediaDevices>
using QtAudioDevice = QAudioDevice;
using QtAudioOutput = QAudioSink;
using QtAudioSampleFormat = QAudioFormat::SampleFormat;
#define AUDIO_SAMPLE_FORMAT QAudioFormat::Int32
#else
#include <QAudioOutput>
using QtAudioDevice = QAudioDeviceInfo;
using QtAudioOutput = QAudioOutput;
using QtAudioSampleFormat = QAudioFormat::SampleType;
#define AUDIO_SAMPLE_FORMAT QAudioFormat::SignedInt
#endif

struct CORE_EXPORT eSoundSettingsData;

class CORE_EXPORT AudioHandler : public QObject
{
    Q_OBJECT
public:
    AudioHandler();

    static AudioHandler* sInstance;

    struct DataRequest
    {
        char* fData;
        int fSize;

        operator bool() const {
            return fSize > 0;
        }
    };

    DataRequest dataRequest();

    void provideData(const DataRequest& request);

    void initializeAudio(eSoundSettingsData &soundSettings,
                         const QString &deviceName = QString());
    void initializeAudio(const QString &deviceName = QString(),
                         bool save = false);
    void startAudio();
    void pauseAudio();
    void resumeAudio();
    void stopAudio();
    void setVolume(const int value);
    qreal getVolume();
    const QString getDeviceName();
    QtAudioDevice findDevice(const QString &deviceName);
    const QStringList listDevices();

    QtAudioOutput* audioOutput() const { return mAudioOutput; }

signals:
    void deviceChanged();

private:
    QtAudioDevice mAudioDevice;
    QtAudioOutput *mAudioOutput = nullptr;
    QIODevice *mAudioIOOutput = nullptr; // not owned
    QAudioFormat mAudioFormat;

    QByteArray mAudioBuffer;
};

#endif // AUDIOHANDLER_H
