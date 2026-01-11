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

#ifndef ESOUNDSETTINGS_H
#define ESOUNDSETTINGS_H

#include <QObject>
extern "C" {
    #include <libavutil/samplefmt.h>
    #include <libavutil/channel_layout.h>
}

#include "../core_global.h"

#include "utils/ffmpeghelper.h"

struct CORE_EXPORT eSoundSettingsData {
#ifdef Q_OS_MAC
    int fSampleRate = 22050;
#else
    int fSampleRate = 44100;
#endif
    AVSampleFormat fSampleFormat = AV_SAMPLE_FMT_FLT;
    uint64_t fChannelLayout = AV_CH_LAYOUT_STEREO;

    bool planarFormat() const {
        return av_sample_fmt_is_planar(fSampleFormat);
    }

    int channelCount() const {
        return Friction::Utils::FFmpegHelper::getNbChannels(fChannelLayout);
    }

    int bytesPerSample() const {
        return av_get_bytes_per_sample(fSampleFormat);
    }

    bool operator==(const eSoundSettingsData &other) {
        return fSampleRate == other.fSampleRate &&
               fSampleFormat == other.fSampleFormat &&
               fChannelLayout == other.fChannelLayout;
    }
};

class CORE_EXPORT eSoundSettings : public QObject, private eSoundSettingsData {
    Q_OBJECT
public:
    eSoundSettings();

    static eSoundSettings* sInstance;

    static int sSampleRate();
    static AVSampleFormat sSampleFormat();
    static uint64_t sChannelLayout();
    static bool sPlanarFormat();
    static int sChannelCount();
    static int sBytesPerSample();
    static eSoundSettingsData &sData();

    static void sSetSampleRate(const int sampleRate);
    static void sSetSampleFormat(const AVSampleFormat format);
    static void sSetChannelLayout(const uint64_t layout);

    static void sSave();
    static void sRestore();

    void save();
    void restore();

    void setAll(const eSoundSettingsData& data);
    void setSampleRate(const int sampleRate);
    void setSampleFormat(const AVSampleFormat format);
    void setChannelLayout(const uint64_t layout);
private:
    using eSoundSettingsData::operator=;
    eSoundSettingsData mSaved;
signals:
    void settingsChanged();
};

#endif // ESOUNDSETTINGS_H
