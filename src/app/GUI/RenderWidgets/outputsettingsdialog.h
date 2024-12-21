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

#ifndef OUTPUTSETTINGSDIALOG_H
#define OUTPUTSETTINGSDIALOG_H

#include <QDialog>
#include <QHBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QCheckBox>
#include <QTreeWidget>
#include <QTreeView>

#include "renderinstancesettings.h"
#include "widgets/twocolumnlayout.h"

#define COMPLIANCE FF_COMPLIANCE_NORMAL

class OutputSettingsDialog : public QDialog {
    Q_OBJECT
public:
    struct FormatCodecs {
        FormatCodecs(const QList<AVCodecID> &vidCodecs,
                     const QList<AVCodecID> &audioCodec,
                     const char *name);

        const AVOutputFormat *mFormat = nullptr;
        QList<AVCodecID> mVidCodecs;
        QList<AVCodecID> mAudioCodecs;
    };

    OutputSettingsDialog(const OutputSettings &settings,
                         QWidget *parent = nullptr);

    OutputSettings getSettings();
protected:
    void updateAvailableOutputFormats();

    void updateAvailablePixelFormats();
    void updateAvailableVideoCodecs();

    void updateAvailableAudioCodecs();
    void updateAvailableSampleRates();
    void updateAvailableSampleFormats();
    void updateAvailableVideoProfiles();
    void updateAvailableAudioBitrates();
    void updateAvailableAudioChannelLayouts();

    void restoreInitialSettings();
    void restoreVideoProfileSettings();

    void setShowAllFormatsAndCodecs(const bool bT) {
        if(mShowAllFormatsAndCodecs == bT) return;
        mShowAllFormatsAndCodecs = bT;
        updateAvailableOutputFormats();
    }
    void updateAvailableCodecs();

    QList<FormatCodecs> mSupportedFormats;
    bool mShowAllFormatsAndCodecs = false;
    QList<AVPixelFormat> mPixelFormatsList;
    QList<const AVCodec*> mVideoCodecsList;
    QList<const AVOutputFormat*> mOutputFormatsList;
    QList<const AVCodec*> mAudioCodecsList;
    QList<AVSampleFormat> mSampleFormatsList;
    QList<uint64_t> mAudioChannelLayoutsList;

    const OutputSettings mInitialSettings;
    QVBoxLayout *mMainLayout = nullptr;

    QHBoxLayout *mOutputFormatsLayout = nullptr;
    QLabel *mOutputFormatsLabel = nullptr;
    QComboBox *mOutputFormatsComboBox = nullptr;

    QGroupBox *mVideoGroupBox = nullptr;
    TwoColumnLayout *mVideoSettingsLayout = nullptr;
    QLabel *mVideoCodecsLabel = nullptr;
    QComboBox *mVideoCodecsComboBox = nullptr;
    QLabel *mPixelFormatsLabel = nullptr;
    QComboBox *mPixelFormatsComboBox = nullptr;
    QLabel *mBitrateLabel = nullptr;
    QDoubleSpinBox *mBitrateSpinBox = nullptr;
    QLabel *mVideoProfileLabel = nullptr;
    QComboBox *mVideoProfileComboBox = nullptr;

    QGroupBox *mAudioGroupBox = nullptr;
    TwoColumnLayout *mAudioSettingsLayout = nullptr;
    QLabel *mAudioCodecsLabel = nullptr;
    QComboBox *mAudioCodecsComboBox = nullptr;
    QLabel *mSampleFormatsLabel = nullptr;
    QComboBox *mSampleFormatsComboBox = nullptr;
    QLabel *mSampleRateLabel = nullptr;
    QComboBox *mSampleRateComboBox = nullptr;
    QLabel *mAudioBitrateLabel = nullptr;
    QComboBox *mAudioBitrateComboBox = nullptr;
    QLabel *mAudioChannelLayoutLabel = nullptr;
    QComboBox *mAudioChannelLayoutsComboBox = nullptr;

    QHBoxLayout *mShowLayout = nullptr;
    QLabel *mShowLabel = nullptr;
    QCheckBox *mShowAllFormatsAndCodecsCheckBox = nullptr;

    QHBoxLayout *mButtonsLayout = nullptr;
    QPushButton *mOkButton = nullptr;
    QPushButton *mCancelButton = nullptr;
    QPushButton *mResetButton = nullptr;

    QTreeWidget *mFormatOptionsTree;
    void setupFormatOptionsTree();
    void populateFormatOptionsTree(const Friction::Core::FormatOptions &options);
    QComboBox *createComboBoxFormatOptType(int selected = Friction::Core::FormatType::fTypeMeta);
    Friction::Core::FormatOptions getFormatOptions();

    void addVideoCodec(const AVCodec * const codec,
                       const AVOutputFormat *outputFormat,
                       const QString &currentCodecName);
    void addAudioCodec(const AVCodecID &codecId,
                       const AVOutputFormat *outputFormat,
                       const QString &currentCodecName);
};

#endif // OUTPUTSETTINGSDIALOG_H
