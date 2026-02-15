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

#include "outputsettingsprofilesdialog.h"
#include "GUI/mainwindow.h"
#include "outputsettingsdialog.h"
#include "outputsettingsdisplaywidget.h"
#include "GUI/global.h"
#include "widgets/editablecombobox.h"

OutputProfilesDialog::OutputProfilesDialog(
        const OutputSettings &currentSettings,
        QWidget *parent) :
 QDialog(parent) {
    setWindowTitle("Output Settings Profile");
    setStyleSheet(MainWindow::sGetInstance()->styleSheet());
    mCurrentSettings = currentSettings;

    const auto mainLayout = new QVBoxLayout(this);
    mainLayout->setSizeConstraint(QLayout::SetFixedSize);
    setLayout(mainLayout);

    mInnerLayout = new QVBoxLayout();

    mProfileLayout = new QHBoxLayout();
    mProfileLabel = new QLabel("Profile:", this);
    mProfilesComboBox = new EditableComboBox(this);
    mProfilesComboBox->setMinimumContentsLength(20);
    mProfileLayout->addWidget(mProfileLabel);
    mProfileLayout->addWidget(mProfilesComboBox);
    mProfileLayout->setAlignment(Qt::AlignHCenter);

    mProfileButtonsLayout = new QHBoxLayout();
    mNewProfileButton = new QPushButton("New...", this);
    mNewProfileButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(mNewProfileButton, &QPushButton::released,
            this, &OutputProfilesDialog::createAndEditNewProfile);
    mDuplicateProfileButton = new QPushButton("Duplicate", this);
    mDuplicateProfileButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(mDuplicateProfileButton, &QPushButton::released,
            this, &OutputProfilesDialog::duplicateCurrentProfile);
    mEditProfileButton = new QPushButton("Edit...", this);
    mEditProfileButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(mEditProfileButton, &QPushButton::released,
            this, &OutputProfilesDialog::editCurrentProfile);
    mSaveProfileButton = new QPushButton("Save", this);
    mSaveProfileButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(mSaveProfileButton, &QPushButton::released,
            this, &OutputProfilesDialog::saveCurrentProfile);
    mDeleteProfileButton = new QPushButton("Delete", this);
    mDeleteProfileButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(mDeleteProfileButton, &QPushButton::released,
            this, &OutputProfilesDialog::deleteCurrentProfile);
    mProfileButtonsLayout->addWidget(mNewProfileButton);
    mProfileButtonsLayout->addWidget(mDuplicateProfileButton);
    mProfileButtonsLayout->addWidget(mEditProfileButton);
    mProfileButtonsLayout->addWidget(mSaveProfileButton);
    mProfileButtonsLayout->addWidget(mDeleteProfileButton);
    mNewProfileButton->setObjectName("dialogButton");
    mDuplicateProfileButton->setObjectName("dialogButton");
    mEditProfileButton->setObjectName("dialogButton");
    mSaveProfileButton->setObjectName("dialogButton");
    mDeleteProfileButton->setObjectName("dialogButton");
    mProfileButtonsLayout->setAlignment(Qt::AlignHCenter);

    mOutputSettingsDisplayWidget = new OutputSettingsDisplayWidget(this);
//    mOutputSettingsDisplayWidget->setMinimumSize(10*MIN_WIDGET_HEIGHT,
//                                                 7*MIN_WIDGET_HEIGHT);
    //mOutputSettingsDisplayWidget->setAlwaysShowAll(true);

    mButtonsLayout = new QHBoxLayout();
    mOkButton = new QPushButton("Ok", this);
    mCancelButton = new QPushButton("Cancel", this);
    mOkButton->setObjectName("dialogButton");
    mCancelButton->setObjectName("dialogButton");

    mOkButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    mCancelButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(mOkButton, &QPushButton::released,
            this, &QDialog::accept);
    connect(mCancelButton, &QPushButton::released,
            this, &QDialog::reject);

    mButtonsLayout->addWidget(mCancelButton);
    mButtonsLayout->addWidget(mOkButton);
    mButtonsLayout->setAlignment(Qt::AlignRight);
    mInnerLayout->setAlignment(Qt::AlignTop);
    mInnerLayout->addLayout(mProfileLayout);
    mInnerLayout->addLayout(mProfileButtonsLayout);
    mInnerLayout->addWidget(mOutputSettingsDisplayWidget);
    mInnerLayout->addStretch(1);
    mInnerLayout->addLayout(mButtonsLayout);
    mainLayout->addLayout(mInnerLayout);
    mStatusBar = new QStatusBar(this);
    mainLayout->addWidget(mStatusBar);

    for(const auto& profile : qAsConst(OutputSettingsProfile::sOutputProfiles)) {
        mProfilesComboBox->addItem(profile->getName());
    }

    updateButtonsEnabled();
    mProfilesComboBox->setInsertPolicy(QComboBox::NoInsert);
    if(mProfilesComboBox->count() == 0) {
        mProfilesComboBox->setDisabled(true);
    }
    connect(mProfilesComboBox, &QComboBox::editTextChanged,
            this, &OutputProfilesDialog::setCurrentProfileName);
    connect(mProfilesComboBox, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &OutputProfilesDialog::currentProfileChanged);

    currentProfileChanged();
}

OutputSettingsProfile *OutputProfilesDialog::getCurrentProfile() {
    if(OutputSettingsProfile::sOutputProfiles.isEmpty()) return nullptr;
    const int index = mProfilesComboBox->currentIndex();
    if(index < 0) return nullptr;
    if(index >= OutputSettingsProfile::sOutputProfiles.count()) return nullptr;
    return OutputSettingsProfile::sOutputProfiles.at(index).get();
}

void OutputProfilesDialog::updateButtonsEnabled() {
    if(OutputSettingsProfile::sOutputProfiles.isEmpty()) {
        mDuplicateProfileButton->setDisabled(true);
        mEditProfileButton->setDisabled(true);
        mDeleteProfileButton->setDisabled(true);
        mSaveProfileButton->setDisabled(true);
        mProfilesComboBox->setDisabled(true);
    } else {
        mDuplicateProfileButton->setEnabled(true);
        mEditProfileButton->setEnabled(true);
        mDeleteProfileButton->setEnabled(true);
        mSaveProfileButton->setEnabled(true);
        mProfilesComboBox->setEnabled(true);
    }
}

void OutputProfilesDialog::currentProfileChanged() {
    OutputSettingsProfile *currentProfile = getCurrentProfile();
    if(!currentProfile) {
        mOutputSettingsDisplayWidget->hide();
        return;
    }
    mOutputSettingsDisplayWidget->show();
    mOutputSettingsDisplayWidget->setOutputSettings(currentProfile->getSettings());
}

void OutputProfilesDialog::setCurrentProfileName(const QString &name) {
    OutputSettingsProfile *currentProfile = getCurrentProfile();
    if(!currentProfile) return;
    currentProfile->setName(name);
    mProfilesComboBox->setItemText(mProfilesComboBox->currentIndex(), name);
}

void OutputProfilesDialog::deleteCurrentProfile() {
    OutputSettingsProfile *currentProfile = getCurrentProfile();
    if(!currentProfile) return;
    const int currentId = mProfilesComboBox->currentIndex();
    mProfilesComboBox->removeItem(currentId);
    OutputSettingsProfile::sOutputProfiles.takeAt(currentId)->removeFile();
    updateButtonsEnabled();
    currentProfileChanged();
}

void OutputProfilesDialog::duplicateCurrentProfile() {
    OutputSettingsProfile *currentProfile = getCurrentProfile();
    if(!currentProfile) return;
    auto newProfile = enve::make_shared<OutputSettingsProfile>();
    newProfile->setSettings(currentProfile->getSettings());
    newProfile->setName(currentProfile->getName() + " copy");
    OutputSettingsProfile::sOutputProfiles.append(newProfile);
    mProfilesComboBox->addItem(newProfile->getName());
    mProfilesComboBox->setCurrentIndex(mProfilesComboBox->count() - 1);
}

void OutputProfilesDialog::createAndEditNewProfile() {
    OutputSettingsDialog *dialog = new OutputSettingsDialog(OutputSettings(), this);
    if(dialog->exec()) {
        auto newProfile = enve::make_shared<OutputSettingsProfile>();
        OutputSettingsProfile::sOutputProfiles.append(newProfile);
        mProfilesComboBox->addItem(newProfile->getName());
        mProfilesComboBox->setCurrentIndex(mProfilesComboBox->count() - 1);
        updateButtonsEnabled();
        newProfile->setSettings(dialog->getSettings());
        currentProfileChanged();
    }
    delete dialog;
}

void OutputProfilesDialog::editCurrentProfile() {
    const auto currentProfile = getCurrentProfile();
    if(!currentProfile) return;
    const OutputSettings &outputSettings = currentProfile->getSettings();
    const auto dialog = new OutputSettingsDialog(outputSettings, this);
    if(dialog->exec()) {
        currentProfile->setSettings(dialog->getSettings());
        currentProfileChanged();
    }
    delete dialog;
}

void OutputProfilesDialog::saveCurrentProfile() {
    const auto profile = getCurrentProfile();
    if(!profile) return;
    try {
        profile->save();
        mStatusBar->showMessage("Saved to '" + profile->path() + "'", 2000);
    } catch(const std::exception& e) {
        gPrintExceptionCritical(e);
    }
}
