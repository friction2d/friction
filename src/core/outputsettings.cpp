/*
#
# enve2d - https://github.com/enve2d
#
# Copyright (c) enve2d developers
# Copyright (C) 2016-2020 Maurycy Liebner
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
*/

#include "outputsettings.h"
#include "ReadWrite/evformat.h"
#include "appsupport.h"

using namespace Friction::Core;

QList<qsptr<OutputSettingsProfile>> OutputSettingsProfile::sOutputProfiles;
bool OutputSettingsProfile::sOutputProfilesLoaded = false;

const std::map<int, QString> OutputSettings::sSampleFormatNames = {
    {AV_SAMPLE_FMT_U8, "8 bits unsigned"},
    {AV_SAMPLE_FMT_S16, "16 bits signed"},
    {AV_SAMPLE_FMT_S32, "32 bits signed"},
    {AV_SAMPLE_FMT_FLT, "32 bits float"},
    {AV_SAMPLE_FMT_DBL, "64 bits double"},
    {AV_SAMPLE_FMT_U8P, "8 bits unsigned, planar"},
    {AV_SAMPLE_FMT_S16P, "16 bits signed, planar"},
    {AV_SAMPLE_FMT_S32P, "32 bits signed, planar"},
    {AV_SAMPLE_FMT_FLTP, "32 bits float, planar"},
    {AV_SAMPLE_FMT_DBLP, "64 bits double, planar"},
    {AV_SAMPLE_FMT_S64, "64 bits signed"},
    {AV_SAMPLE_FMT_S64P, "64 bits signed, planar"}
};

const std::map<uint64_t, QString> gChannelLayouts = {
    {AV_CH_LAYOUT_MONO, "Mono"},
    {AV_CH_LAYOUT_STEREO, "Stereo"},
    {AV_CH_LAYOUT_2POINT1, "2.1"},
    {AV_CH_LAYOUT_SURROUND, "3.0"},
    {AV_CH_LAYOUT_2_1, "3.0(back)"},
    {AV_CH_LAYOUT_4POINT0, "4.0"},
    {AV_CH_LAYOUT_QUAD, "Quad"},
    {AV_CH_LAYOUT_2_2, "Quad(side)"},
    {AV_CH_LAYOUT_3POINT1, "3.1"},
    {AV_CH_LAYOUT_5POINT0_BACK, "5.0"},
    {AV_CH_LAYOUT_5POINT0, "5.0(side)"},
    {AV_CH_LAYOUT_4POINT1, "4.1"},
    {AV_CH_LAYOUT_5POINT1_BACK, "5.1"},
    {AV_CH_LAYOUT_5POINT1, "5.1(side)"},
    {AV_CH_LAYOUT_6POINT0, "6.0"},
    {AV_CH_LAYOUT_6POINT0_FRONT, "6.0(front)"},
    {AV_CH_LAYOUT_HEXAGONAL, "Hexagonal"},
    {AV_CH_LAYOUT_6POINT1, "6.1"},
    {AV_CH_LAYOUT_6POINT1_BACK, "6.1(back)"},
    {AV_CH_LAYOUT_6POINT1_FRONT, "6.1(front)"},
    {AV_CH_LAYOUT_7POINT0, "7.0"},
    {AV_CH_LAYOUT_7POINT0_FRONT, "7.0(front)"},
    {AV_CH_LAYOUT_7POINT1, "7.1"},
    {AV_CH_LAYOUT_7POINT1_WIDE_BACK, "7.1(wide)"},
    {AV_CH_LAYOUT_7POINT1_WIDE, "7.1(wide-side)"},
    {AV_CH_LAYOUT_OCTAGONAL, "Octagonal"},
    {AV_CH_LAYOUT_HEXADECAGONAL, "Hexadecagonal"},
    {AV_CH_LAYOUT_STEREO_DOWNMIX, "Downmix"},
    {AV_CH_FRONT_LEFT, "Front left"},
    {AV_CH_FRONT_RIGHT, "Front right"},
    {AV_CH_FRONT_CENTER, "Front center"},
    {AV_CH_LOW_FREQUENCY, "Low frequency"},
    {AV_CH_BACK_LEFT, "Back left"},
    {AV_CH_BACK_RIGHT, "Back right"},
    {AV_CH_TOP_CENTER, "Top center"},
    {AV_CH_TOP_FRONT_LEFT, "Top front left"},
    {AV_CH_TOP_FRONT_CENTER, "Top front center"},
    {AV_CH_TOP_FRONT_RIGHT, "Top front right"},
    {AV_CH_TOP_BACK_LEFT, "Top back left"},
    {AV_CH_TOP_BACK_CENTER, "Top back center"},
    {AV_CH_TOP_BACK_RIGHT, "Top back right"},
    {AV_CH_STEREO_LEFT, "Stereo left"},
    {AV_CH_STEREO_RIGHT, "Stereo right"},
    {AV_CH_WIDE_LEFT, "Wide left"},
    {AV_CH_WIDE_RIGHT, "Wide right"},
    {AV_CH_SURROUND_DIRECT_LEFT, "Surround direct left"},
    {AV_CH_SURROUND_DIRECT_RIGHT, "Surround direct right"},
    {AV_CH_LOW_FREQUENCY_2, "Low frequency 2"},
    {AV_CH_LAYOUT_NATIVE, "Native"}
};

QString OutputSettings::sGetChannelsLayoutName(const uint64_t &layout)
{
    const auto it = gChannelLayouts.find(layout);
    if (it == gChannelLayouts.end()) { return "Unrecognized"; }
    return it->second;
}

uint64_t OutputSettings::sGetChannelsLayout(const QString &name)
{
    for (const auto& it : gChannelLayouts) {
        if (it.second == name) { return it.first; }
    }
    return AV_CH_LAYOUT_STEREO;
}

void OutputSettings::write(eWriteStream &dst) const
{
    dst << (fOutputFormat ? QString(fOutputFormat->name) : "");

    dst << fVideoEnabled;
    dst << (fVideoCodec ? fVideoCodec->id : -1);
    dst.write(&fVideoPixelFormat, sizeof(AVPixelFormat));
    dst << fVideoBitrate;
    dst << fVideoProfile;

    writeFormatOptions(dst);

    dst << fAudioEnabled;
    dst << (fAudioCodec ? fAudioCodec->id : -1);
    dst.write(&fAudioSampleFormat, sizeof(AVSampleFormat));
    dst << fAudioChannelsLayout;
    dst << fAudioSampleRate;
    dst << fAudioBitrate;
}

void OutputSettings::read(eReadStream &src)
{
    QString formatName; src >> formatName;
    fOutputFormat = av_guess_format(formatName.toUtf8().data(),
                                   nullptr, nullptr);

    src >> fVideoEnabled;
    int videoCodecId; src >> videoCodecId;
    const auto avVideoCodecId = static_cast<AVCodecID>(videoCodecId);
    fVideoCodec = avcodec_find_encoder(avVideoCodecId);
    src.read(&fVideoPixelFormat, sizeof(AVPixelFormat));
    src >> fVideoBitrate;
    if (src.evFileVersion() >= EvFormat::codecProfile) {
        src >> fVideoProfile;
    }

    readFormatOptions(src);

    src >> fAudioEnabled;
    int audioCodecId; src >> audioCodecId;
    const auto avAudioCodecId = static_cast<AVCodecID>(audioCodecId);
    fAudioCodec = avcodec_find_encoder(avAudioCodecId);
    src.read(&fAudioSampleFormat, sizeof(AVSampleFormat));
    src >> fAudioChannelsLayout;
    src >> fAudioSampleRate;
    src >> fAudioBitrate;
}

void OutputSettings::writeFormatOptions(eWriteStream &dst) const
{
    const auto list = OutputSettingsProfile::toFormatOptionsList(fVideoOptions);
    dst << list.fTypes.join("|").toUtf8();
    dst << list.fKeys.join("|").toUtf8();
    dst << list.fValues.join("|").toUtf8();
}

void OutputSettings::readFormatOptions(eReadStream &src)
{
    if (src.evFileVersion() < EvFormat::formatOptions) { return; }
    if (src.evFileVersion() == EvFormat::formatOptions) {
        src.read(nullptr, sizeof(FormatOptions));
        return;
    }

    QByteArray dTypes, dKeys, dValues;
    src >> dTypes;
    src >> dKeys;
    src >> dValues;

    FormatOptionsList list;
    list.fTypes = QString::fromUtf8(dTypes).split("|");
    list.fKeys = QString::fromUtf8(dKeys).split("|");
    list.fValues = QString::fromUtf8(dValues).split("|");
    if (OutputSettingsProfile::isValidFormatOptionsList(list)) {
        fVideoOptions = OutputSettingsProfile::toFormatOptions(list);
    }
}

OutputSettingsProfile::OutputSettingsProfile() {}

void OutputSettingsProfile::setSettings(const OutputSettings &settings)
{
    mSettings = settings;
    emit changed();
}

void OutputSettingsProfile::save()
{
    const QString path = QString::fromUtf8("%1/%2.conf")
                         .arg(AppSupport::getAppOutputProfilesPath(),
                              mName);
    qDebug() << "save output profile" << path;

    QSettings profile(path, QSettings::IniFormat);
    profile.beginGroup(QString::fromUtf8("output_profile_v1"));
    profile.setValue(QString::fromUtf8("name"),
                     mName);
    profile.setValue(QString::fromUtf8("format"),
                     mSettings.fOutputFormat ? mSettings.fOutputFormat->name : QString::fromUtf8("null"));
    profile.setValue(QString::fromUtf8("video_enabled"),
                     mSettings.fVideoEnabled);
    if (mSettings.fVideoEnabled) {
        profile.setValue(QString::fromUtf8("video_codec"),
                         mSettings.fVideoCodec ? mSettings.fVideoCodec->name : QString::fromUtf8("null"));
        profile.setValue(QString::fromUtf8("video_pixel_format"),
                         av_get_pix_fmt_name(mSettings.fVideoPixelFormat));
        profile.setValue(QString::fromUtf8("video_bitrate"),
                         mSettings.fVideoBitrate);
        profile.setValue(QString::fromUtf8("video_profile"),
                         mSettings.fVideoProfile);

        const auto options = toFormatOptionsList(mSettings.fVideoOptions);
        if (isValidFormatOptionsList(options)) {
            profile.setValue(QString::fromUtf8("video_options_types"), options.fTypes);
            profile.setValue(QString::fromUtf8("video_options_keys"), options.fKeys);
            profile.setValue(QString::fromUtf8("video_options_values"), options.fValues);
        }
    }
    profile.setValue(QString::fromUtf8("audio_enabled"),
                     mSettings.fAudioEnabled);
    if (mSettings.fAudioEnabled) {
        profile.setValue(QString::fromUtf8("audio_codec"),
                         mSettings.fAudioCodec ?  mSettings.fAudioCodec->name : QString::fromUtf8("null"));
        profile.setValue(QString::fromUtf8("audio_sample_format"),
                         av_get_sample_fmt_name(mSettings.fAudioSampleFormat));
        profile.setValue(QString::fromUtf8("audio_channel_layout"),
                         OutputSettings::sGetChannelsLayoutName(mSettings.fAudioChannelsLayout));
        profile.setValue(QString::fromUtf8("audio_sample_rate"),
                         mSettings.fAudioSampleRate);
        profile.setValue(QString::fromUtf8("audio_bitrate"),
                         mSettings.fAudioBitrate);
    }
    profile.endGroup();

    mPath = path;
}

void OutputSettingsProfile::load(const QString &path)
{
    qDebug() << "load output profile" << path;
    if (!QFile::exists(path)) { RuntimeThrow(tr("Failed to load profile: %1").arg(path)); }

    bool validProfile = false;

    QString profileName;
    QString profileFormat;

    bool profileVideoEnabled = false;
    bool profileAudioEnabled = false;

    QSettings profile(path, QSettings::IniFormat);
    profile.beginGroup(QString::fromUtf8("output_profile_v1"));

    profileName = profile.value(QString::fromUtf8("name")).toString();
    profileFormat = profile.value(QString::fromUtf8("format")).toString();
    profileVideoEnabled = profile.value(QString::fromUtf8("video_enabled"), false).toBool();
    profileAudioEnabled = profile.value(QString::fromUtf8("audio_enabled"), false).toBool();

    validProfile = ((!profileName.isEmpty() && !profileFormat.isEmpty()) &&
                    (profileVideoEnabled || profileAudioEnabled));

    if (!validProfile) {
        profile.endGroup();
        RuntimeThrow(tr("Failed to load profile: %1").arg(path));
    }

    if (validProfile) {
        mPath = path;
        mName = profileName;
        mSettings.fOutputFormat = av_guess_format(profileFormat.toUtf8().data(),
                                                  nullptr,
                                                  nullptr);
        mSettings.fVideoEnabled = profileVideoEnabled;
        if (mSettings.fVideoEnabled) {
            mSettings.fVideoCodec = avcodec_find_encoder_by_name(profile
                                                                 .value(QString::fromUtf8("video_codec"))
                                                                 .toString()
                                                                 .toUtf8()
                                                                 .data());
            mSettings.fVideoPixelFormat = av_get_pix_fmt(profile
                                                         .value(QString::fromUtf8("video_pixel_format"))
                                                         .toString()
                                                         .toUtf8()
                                                         .data());
            mSettings.fVideoBitrate = profile.value(QString::fromUtf8("video_bitrate")).toInt();
            mSettings.fVideoProfile = profile.value(QString::fromUtf8("video_profile"),
                                                    FF_PROFILE_UNKNOWN).toInt();

            const auto options = FormatOptionsList{profile.value("video_options_types").toStringList(),
                                                   profile.value("video_options_keys").toStringList(),
                                                   profile.value("video_options_values").toStringList()};
            if (isValidFormatOptionsList(options)) {
                mSettings.fVideoOptions = toFormatOptions(options);
            }
        }
        mSettings.fAudioEnabled = profileAudioEnabled;
        if (mSettings.fAudioEnabled) {
            mSettings.fAudioCodec = avcodec_find_encoder_by_name(profile
                                                                 .value(QString::fromUtf8("audio_codec"))
                                                                 .toString()
                                                                 .toUtf8()
                                                                 .data());
            mSettings.fAudioSampleFormat = av_get_sample_fmt(profile
                                                             .value(QString::fromUtf8("audio_sample_format"))
                                                             .toString()
                                                             .toUtf8()
                                                             .data());
            mSettings.fAudioChannelsLayout = OutputSettings::sGetChannelsLayout(profile
                                                                                .value(QString::fromUtf8("audio_channel_layout"))
                                                                                .toString());
            mSettings.fAudioSampleRate = profile.value(QString::fromUtf8("audio_sample_rate")).toInt();
            mSettings.fAudioBitrate = profile.value(QString::fromUtf8("audio_bitrate")).toInt();
        }
    } else {
        profile.endGroup();
        RuntimeThrow(tr("Failed to load profile: %1").arg(path));
    }
    profile.endGroup();
    emit changed();
}

void OutputSettingsProfile::removeFile()
{
    if (!wasSaved()) { return; }
    QFile(mPath).remove();
}

OutputSettingsProfile *OutputSettingsProfile::sGetByName(const QString &name)
{
    for (const auto& profile : qAsConst(sOutputProfiles)) {
        if (profile->getName() == name) { return profile.get(); }
    }
    return nullptr;
}

FormatOptions OutputSettingsProfile::toFormatOptions(const FormatOptionsList &list)
{
    FormatOptions options;
    if (!isValidFormatOptionsList(list)) { return options; }
    for (int i = 0; i < list.fKeys.count(); i++) {
        if (list.fKeys.at(i).isEmpty()) { continue; }
        options.fValues.push_back({list.fKeys.at(i),
                                   list.fValues.at(i),
                                   list.fTypes.at(i).toInt()});
    }
    return options;
}

FormatOptionsList OutputSettingsProfile::toFormatOptionsList(const FormatOptions &options)
{
    FormatOptionsList list;
    for (const auto &option : options.fValues) {
        if (option.fKey.isEmpty()) { continue; }
        list.fKeys << option.fKey;
        list.fValues << option.fValue;
        list.fTypes << QString::number(option.fType);
    }
    return list;
}

bool OutputSettingsProfile::isValidFormatOptionsList(const Friction::Core::FormatOptionsList &list)
{
    if (list.fTypes.count() > 0 &&
        list.fTypes.count() == list.fKeys.count() &&
        list.fTypes.count() == list.fValues.count()) {
        return true;
    }
    return false;
}
