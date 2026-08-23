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

#include "audiohandler.h"
#include "soundcomposition.h"
#include "appsupport.h"

#include <iostream>

AudioHandler* AudioHandler::sInstance = nullptr;

AudioHandler::AudioHandler()
{
    Q_ASSERT(!sInstance);
    sInstance = this;
}

const int BufferSize = 32768;

QAudioFormat::SampleType toQtAudioFormat(const AVSampleFormat avFormat)
{
    if (avFormat == AV_SAMPLE_FMT_S32 || avFormat == AV_SAMPLE_FMT_S16) {
        return QAudioFormat::SignedInt;
    } else if (avFormat == AV_SAMPLE_FMT_FLT) {
        return QAudioFormat::Float;
    } else if (avFormat == AV_SAMPLE_FMT_U8) {
        return QAudioFormat::UnSignedInt;
    } else {
        RuntimeThrow("Unsupported sample format " + QString(av_get_sample_fmt_name(avFormat)));
        return QAudioFormat::Unknown;
    }
}

AVSampleFormat toAVAudioFormat(const QAudioFormat& qFormat)
{
    if (qFormat.sampleType() == QAudioFormat::SignedInt) {
        if (qFormat.sampleSize() == 16) { return AV_SAMPLE_FMT_S16; }
        if (qFormat.sampleSize() == 32) { return AV_SAMPLE_FMT_S32; }
    } else if (qFormat.sampleType() == QAudioFormat::Float) {
        return AV_SAMPLE_FMT_FLT;
    } else if (qFormat.sampleType() == QAudioFormat::UnSignedInt && qFormat.sampleSize() == 8) {
        return AV_SAMPLE_FMT_U8;
    }

    RuntimeThrow("Unsupported sample format: Type " +
                 QString::number(qFormat.sampleType()) +
                 " Size " + QString::number(qFormat.sampleSize()));

    return AV_SAMPLE_FMT_NONE;
}

void AudioHandler::initializeAudio(eSoundSettingsData& soundSettings,
                                   const QString &deviceName)
{
    if (mAudioOutput) {
        delete mAudioOutput;
        mAudioOutput = nullptr;
    }

    mAudioBuffer = QByteArray(BufferSize, 0);

    mAudioDevice = findDevice(deviceName);

    if (mAudioDevice.isNull()) {
        mAudioDevice = QAudioDeviceInfo::defaultOutputDevice();
    }

    qWarning() << "Audio Device:" << mAudioDevice.deviceName();

    mAudioFormat.setSampleRate(soundSettings.fSampleRate);
    mAudioFormat.setChannelCount(soundSettings.channelCount());
    mAudioFormat.setSampleSize(8 * soundSettings.bytesPerSample());
    mAudioFormat.setCodec("audio/pcm");
    mAudioFormat.setByteOrder(QAudioFormat::LittleEndian);
    mAudioFormat.setSampleType(toQtAudioFormat(soundSettings.fSampleFormat));

    QAudioDeviceInfo info(mAudioDevice);

    if (!info.isFormatSupported(mAudioFormat)) {
        qWarning() << "Requested audio format not supported, negotiating fallback...";
        QAudioFormat nearest = info.nearestFormat(mAudioFormat);

        try {
            soundSettings.fSampleFormat = toAVAudioFormat(nearest);
            mAudioFormat = nearest;
            soundSettings.fSampleRate = mAudioFormat.sampleRate();

            if (mAudioFormat.channelCount() == 1) {
                soundSettings.fChannelLayout = AV_CH_LAYOUT_MONO;
            } else if (mAudioFormat.channelCount() == 2) {
                soundSettings.fChannelLayout = AV_CH_LAYOUT_STEREO;
            }

            qWarning() << "Successfully negotiated format: Rate:" << mAudioFormat.sampleRate()
                       << "Size:" << mAudioFormat.sampleSize();

        } catch (const std::exception& e) {
            qWarning() << "Nearest format unusable, forcing standard 16-bit 44.1kHz stereo fallback";
            mAudioFormat.setSampleRate(44100);
            mAudioFormat.setChannelCount(2);
            mAudioFormat.setSampleSize(16);
            mAudioFormat.setSampleType(QAudioFormat::SignedInt);

            soundSettings.fSampleFormat = AV_SAMPLE_FMT_S16;
            soundSettings.fSampleRate = 44100;
            soundSettings.fChannelLayout = AV_CH_LAYOUT_STEREO;
        }
    }

    std::cout
        << "Audio Format:" << std::endl
        << "    Sample rate: " << mAudioFormat.sampleRate() << std::endl
        << "    Channel count: " << mAudioFormat.channelCount() << std::endl
        << "    Sample size: " << mAudioFormat.sampleSize() << std::endl
        << "    Codec: " << mAudioFormat.codec().toStdString() << std::endl
        << "    Sample Type: " << mAudioFormat.sampleType() << std::endl
        << "    Byte order: " << mAudioFormat.byteOrder() << std::endl;

    mAudioOutput = new QAudioOutput(mAudioDevice, mAudioFormat, this);
    mAudioOutput->setNotifyInterval(128);
    emit deviceChanged();
}

void AudioHandler::initializeAudio(const QString &deviceName,
                                   bool save)
{
    if (mAudioOutput) {
        delete mAudioOutput;
        mAudioOutput = nullptr;
    }

    mAudioBuffer = QByteArray(BufferSize, 0);

    mAudioDevice = findDevice(deviceName);

    if (mAudioDevice.isNull()) {
        mAudioDevice = QAudioDeviceInfo::defaultOutputDevice();
    }

    qWarning() << "Audio Device:" << mAudioDevice.deviceName();

    if (save) {
        AppSupport::setSettings(QString::fromUtf8("audio"),
                                QString::fromUtf8("output"),
                                mAudioDevice.deviceName());
    }

    QAudioDeviceInfo info(mAudioDevice);
    if (!info.isFormatSupported(mAudioFormat)) {
        qWarning() << "Requested audio format not supported, negotiating fallback...";
        QAudioFormat nearest = info.nearestFormat(mAudioFormat);
        eSoundSettingsData& soundSettings = eSoundSettings::sData();

        try {
            soundSettings.fSampleFormat = toAVAudioFormat(nearest);
            mAudioFormat = nearest;
            soundSettings.fSampleRate = mAudioFormat.sampleRate();

            if (mAudioFormat.channelCount() == 1) {
                soundSettings.fChannelLayout = AV_CH_LAYOUT_MONO;
            } else if (mAudioFormat.channelCount() == 2) {
                soundSettings.fChannelLayout = AV_CH_LAYOUT_STEREO;
            }

        } catch (const std::exception& e) {
            qWarning() << "Nearest format unusable, forcing standard 16-bit 44.1kHz stereo fallback";
            mAudioFormat.setSampleRate(44100);
            mAudioFormat.setChannelCount(2);
            mAudioFormat.setSampleSize(16);
            mAudioFormat.setSampleType(QAudioFormat::SignedInt);

            soundSettings.fSampleFormat = AV_SAMPLE_FMT_S16;
            soundSettings.fSampleRate = 44100;
            soundSettings.fChannelLayout = AV_CH_LAYOUT_STEREO;
        }

        if (eSoundSettings::sInstance) {
            eSoundSettings::sInstance->setAll(soundSettings);
        }
    }

    std::cout
        << "Audio Format:" << std::endl
        << "    Sample rate: " << mAudioFormat.sampleRate() << std::endl
        << "    Channel count: " << mAudioFormat.channelCount() << std::endl
        << "    Sample size: " << mAudioFormat.sampleSize() << std::endl
        << "    Codec: " << mAudioFormat.codec().toStdString() << std::endl
        << "    Sample Type: " << mAudioFormat.sampleType() << std::endl
        << "    Byte order: " << mAudioFormat.byteOrder() << std::endl;

    mAudioOutput = new QAudioOutput(mAudioDevice, mAudioFormat, this);
    mAudioOutput->setNotifyInterval(128);
    emit deviceChanged();
}

void AudioHandler::startAudio() {
    //if (!QAudioDeviceInfo::availableDevices(QAudio::AudioOutput)
        //.contains(mAudioDevice)) { initializeAudio(); }
    mAudioIOOutput = mAudioOutput->start();
}

void AudioHandler::pauseAudio()
{
    mAudioOutput->suspend();
}

void AudioHandler::resumeAudio()
{
    mAudioOutput->resume();
}

void AudioHandler::stopAudio()
{
    mAudioIOOutput = nullptr;
    mAudioOutput->stop();
    mAudioOutput->reset();
}

void AudioHandler::setVolume(const int value)
{
    if (!mAudioOutput) { return; }
    mAudioOutput->setVolume(qreal(value) / 100);
}

qreal AudioHandler::getVolume()
{
    if (mAudioOutput) { return mAudioOutput->volume(); }
    return 0;
}

const QString AudioHandler::getDeviceName()
{
    return mAudioDevice.deviceName();
}

QAudioDeviceInfo AudioHandler::findDevice(const QString &deviceName)
{
    if (deviceName.isEmpty()) { return QAudioDeviceInfo::defaultOutputDevice(); }
    const auto deviceInfos = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
    for (const QAudioDeviceInfo &deviceInfo : deviceInfos) {
        if (deviceInfo.deviceName() == deviceName) { return deviceInfo; }
    }
    return QAudioDeviceInfo::defaultOutputDevice();
}

const QStringList AudioHandler::listDevices()
{
    QStringList devices;
    const auto deviceInfos = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
    for (const QAudioDeviceInfo &deviceInfo : deviceInfos) {
        devices << deviceInfo.deviceName();
    }
    return devices;
}
