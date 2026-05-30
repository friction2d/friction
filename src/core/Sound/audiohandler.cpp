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

void AudioHandler::provideData(const DataRequest &request)
{
    if (!request || !mAudioIOOutput) { return; }
    mAudioIOOutput->write(request.fData, request.fSize);
}

AudioHandler::DataRequest AudioHandler::dataRequest()
{
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    if (mAudioOutput && mAudioOutput->state() != QAudio::StoppedState) {
        int chunkSize = mAudioFormat.bytesPerFrame() * 1024;

        if (chunkSize <= 0 || chunkSize > mAudioBuffer.size()) {
            chunkSize = 4096; // fallback (4 KB)
        }

        if (mAudioOutput->bytesFree() >= chunkSize) {
            return {mAudioBuffer.data(), chunkSize};
        }
    }
#else
    if (mAudioOutput && mAudioOutput->state() != QAudio::StoppedState) {
        if (mAudioOutput->bytesFree() >= mAudioOutput->periodSize()) {
            return {mAudioBuffer.data(), mAudioOutput->periodSize()};
        }
    }
#endif

    return {nullptr, 0};
}

const int BufferSize = 32768;

QtAudioSampleFormat toQtAudioFormat(const AVSampleFormat avFormat)
{
    if (avFormat == AV_SAMPLE_FMT_S32) {
        return AUDIO_SAMPLE_FORMAT;
    } else if (avFormat == AV_SAMPLE_FMT_FLT) {
        return QAudioFormat::Float;
    } else { RuntimeThrow("Unsupported sample format " +
                          av_get_sample_fmt_name(avFormat)); }
}

AVSampleFormat toAVAudioFormat(const QtAudioSampleFormat qFormat)
{
    if (qFormat == AUDIO_SAMPLE_FORMAT) {
        return AV_SAMPLE_FMT_S32;
    } else if (qFormat == QAudioFormat::Float) {
        return AV_SAMPLE_FMT_FLT;
    } else { RuntimeThrow("Unsupported sample format " +
                          QString::number(qFormat)); }
}

void AudioHandler::initializeAudio(eSoundSettingsData& soundSettings,
                                   const QString &deviceName)
{
    if (mAudioOutput) { delete mAudioOutput; }

    mAudioBuffer = QByteArray(BufferSize, 0);

    mAudioDevice = findDevice(deviceName);

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    qDebug() << "Using audio device" << mAudioDevice.description();
#else
    qDebug() << "Using audio device" << mAudioDevice.deviceName();
#endif

    mAudioFormat.setSampleRate(soundSettings.fSampleRate);
    mAudioFormat.setChannelCount(soundSettings.channelCount());

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    mAudioFormat.setSampleSize(8*soundSettings.bytesPerSample());
    mAudioFormat.setCodec("audio/pcm");
    mAudioFormat.setByteOrder(QAudioFormat::LittleEndian);
    mAudioFormat.setSampleType(toQtAudioFormat(soundSettings.fSampleFormat));

    QAudioDeviceInfo info(mAudioDevice);
    if (!info.isFormatSupported(mAudioFormat)) {
        const auto oldFormat = mAudioFormat;
        mAudioFormat = info.nearestFormat(mAudioFormat);
        /*std::cout << "Using:" << std::endl <<
                     "    Sample rate: " << mAudioFormat.sampleRate() << std::endl <<
                     "    Channel count: " << mAudioFormat.channelCount() << std::endl <<
                     "    Sample size: " << mAudioFormat.sampleSize() << std::endl <<
                     "    Codec: " << mAudioFormat.codec().toStdString() << std::endl <<
                     "    Sample Type: " << mAudioFormat.sampleType() << std::endl <<
                     "    Byte order: " << mAudioFormat.byteOrder() << std::endl;*/
        soundSettings.fSampleRate = mAudioFormat.sampleRate();
        soundSettings.fSampleFormat = toAVAudioFormat(mAudioFormat.sampleType());
    }

    mAudioOutput = new QAudioOutput(mAudioDevice, mAudioFormat, this);
    mAudioOutput->setNotifyInterval(128);
#else
    mAudioFormat.setSampleFormat(toQtAudioFormat(soundSettings.fSampleFormat));

    if (!mAudioDevice.isFormatSupported(mAudioFormat)) {
        mAudioFormat = mAudioDevice.preferredFormat();
        soundSettings.fSampleRate = mAudioFormat.sampleRate();
        soundSettings.fSampleFormat = toAVAudioFormat(mAudioFormat.sampleFormat());
    }

    qDebug() << mAudioFormat;

    mAudioOutput = new QAudioSink(mAudioDevice, mAudioFormat, this);
#endif

    emit deviceChanged();
}

void AudioHandler::initializeAudio(const QString &deviceName,
                                   bool save)
{
    if (mAudioOutput) { delete mAudioOutput; }

    mAudioBuffer = QByteArray(BufferSize, 0);

    mAudioDevice = findDevice(deviceName);
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    qDebug() << "Using audio device" << mAudioDevice.description();
#else
    qDebug() << "Using audio device" << mAudioDevice.deviceName();
#endif
    if (save) {
        AppSupport::setSettings(QString::fromUtf8("audio"),
                                QString::fromUtf8("output"),
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
                                mAudioDevice.id());
#else
                                mAudioDevice..deviceName());
#endif
    }

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QAudioDeviceInfo info(mAudioDevice);
    if (!info.isFormatSupported(mAudioFormat)) {
        mAudioFormat = info.nearestFormat(mAudioFormat);
    }
#else
    if (!mAudioDevice.isFormatSupported(mAudioFormat)) {
        mAudioFormat = mAudioDevice.preferredFormat();
    }
#endif

    mAudioOutput = new QtAudioOutput(mAudioDevice, mAudioFormat, this);
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    mAudioOutput->setNotifyInterval(128);
#endif

    emit deviceChanged();
}

void AudioHandler::startAudio() {
    if (!mAudioOutput) { return; }
    mAudioIOOutput = mAudioOutput->start();
}

void AudioHandler::pauseAudio()
{
    if (mAudioOutput) { mAudioOutput->suspend(); }
}

void AudioHandler::resumeAudio()
{
    if (mAudioOutput) { mAudioOutput->resume(); }
}

void AudioHandler::stopAudio()
{
    if (!mAudioOutput) { return; }
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
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    return mAudioDevice.deviceName();
#else
    return mAudioDevice.description();
#endif
}

QtAudioDevice AudioHandler::findDevice(const QString &deviceName)
{
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    if (deviceName.isEmpty()) { return QAudioDeviceInfo::defaultOutputDevice(); }
    const auto deviceInfos = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
    for (const QAudioDeviceInfo &deviceInfo : deviceInfos) {
        if (deviceInfo.deviceName() == deviceName) { return deviceInfo; }
    }
    return QAudioDeviceInfo::defaultOutputDevice();
#else
    if (deviceName.isEmpty()) { return QMediaDevices::defaultAudioOutput(); }
    const auto deviceInfos = QMediaDevices::audioOutputs();
    for (const QAudioDevice &deviceInfo : deviceInfos) {
        if (deviceInfo.id() == deviceName ||
            deviceInfo.description() == deviceName) { return deviceInfo; }
    }
    return QMediaDevices::defaultAudioOutput();
#endif
}

const QStringList AudioHandler::listDevices()
{
    QStringList devices;
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    const auto deviceInfos = QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
    for (const QAudioDeviceInfo &deviceInfo : deviceInfos) {
        devices << deviceInfo.deviceName();
    }
#else
    const auto deviceInfos = QMediaDevices::audioOutputs();
    for (const QAudioDevice &deviceInfo : deviceInfos) {
        devices << deviceInfo.description();
    }
#endif
    return devices;
}
