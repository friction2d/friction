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

#include "renderinstancewidget.h"

#include "GUI/global.h"
#include "canvas.h"
#include "outputsettingsdialog.h"
#include "outputsettingsprofilesdialog.h"
#include "outputsettingsdisplaywidget.h"
#include "rendersettingsdisplaywidget.h"
#include "rendersettingsdialog.h"
#include "Private/document.h"
#include "appsupport.h"

#include <QMenu>
#include <QDesktopServices>
#include <QMessageBox>

RenderInstanceWidget::RenderInstanceWidget(Canvas *canvas,
                                           QWidget *parent)
    : ClosableContainer(parent)
    , mOutputSettings(nullptr)
    , mRenderSettingsDisplayWidget(nullptr)
    , mOutputSettingsDisplayWidget(nullptr)
    , mOutputDestinationLineEdit(nullptr)
    , mOutputSettingsProfilesButton(nullptr)
    , mOutputSettingsButton(nullptr)
    , mNameLabel(nullptr)
    , mSettings(canvas)
{
    iniGUI();
    connect(&mSettings, &RenderInstanceSettings::stateChanged,
            this, &RenderInstanceWidget::updateFromSettings);
    updateFromSettings();
}

RenderInstanceWidget::RenderInstanceWidget(const RenderInstanceSettings& sett,
                                           QWidget *parent)
    : ClosableContainer(parent)
    , mOutputSettings(nullptr)
    , mRenderSettingsDisplayWidget(nullptr)
    , mOutputSettingsDisplayWidget(nullptr)
    , mOutputDestinationLineEdit(nullptr)
    , mOutputSettingsProfilesButton(nullptr)
    , mOutputSettingsButton(nullptr)
    , mNameLabel(nullptr)
    , mSettings(sett)
{
    iniGUI();
    connect(&mSettings, &RenderInstanceSettings::stateChanged,
            this, &RenderInstanceWidget::updateFromSettings);
    updateFromSettings();
}

void RenderInstanceWidget::iniGUI()
{
    if (!OutputSettingsProfile::sOutputProfilesLoaded) {
        OutputSettingsProfile::sOutputProfilesLoaded = true;
        QDir dirPath(AppSupport::getAppOutputProfilesPath());
        dirPath.setSorting(QDir::SortFlag::Name);
        for (const auto &fileInfo : dirPath.entryInfoList()) {
            if (!fileInfo.isFile()) { continue; }
            if (!fileInfo.completeSuffix().contains("conf")) { continue; }
            const auto profile = enve::make_shared<OutputSettingsProfile>();
            try {
                profile->load(fileInfo.absoluteFilePath());
            } catch(const std::exception& e) {
                gPrintExceptionCritical(e);
            }
            OutputSettingsProfile::sOutputProfiles << profile;
        }
    }

    setCheckable(true);
    setObjectName("darkWidget");
    mNameLabel = new QLineEdit(this);
    mNameLabel->setFocusPolicy(Qt::NoFocus);
    mNameLabel->setFixedHeight(eSizesUI::button);
    mNameLabel->setReadOnly(true);

    setLabelWidget(mNameLabel);

    QWidget *contWid = new QWidget(this);
    contWid->setContentsMargins(0, 0, 0, 0);

    const auto contLayout = new QVBoxLayout(contWid);
    contWid->setLayout(contLayout);
    contWid->setObjectName("RenderContentWidget");

    addContentWidget(contWid);

    mRenderSettingsDisplayWidget = new RenderSettingsDisplayWidget(this);

    QWidget *renderSettingsLabelWidget = new QWidget(this);
    renderSettingsLabelWidget->setContentsMargins(0, 0, 0, 0);

    QVBoxLayout *renderSettingsLayout = new QVBoxLayout(renderSettingsLabelWidget);

    const auto renderSettingsButton = new QPushButton(QIcon::fromTheme("sequence"),
                                            tr("Scene Properties"));
    renderSettingsButton->setFocusPolicy(Qt::NoFocus);
    renderSettingsButton->setObjectName("renderSettings");
    renderSettingsButton->setSizePolicy(QSizePolicy::Preferred,
                                        QSizePolicy::Preferred);

    connect(renderSettingsButton, &QPushButton::pressed,
            this, &RenderInstanceWidget::openRenderSettingsDialog);

    renderSettingsLayout->addWidget(renderSettingsButton);
    renderSettingsLayout->addWidget(mRenderSettingsDisplayWidget);

    contLayout->addWidget(renderSettingsLabelWidget);

    mOutputSettingsDisplayWidget = new OutputSettingsDisplayWidget(this);

    QWidget *outputSettingsLabelWidget = new QWidget(this);
    outputSettingsLabelWidget->setContentsMargins(0, 0, 0, 0);
    QVBoxLayout *outputSettingsLayout = new QVBoxLayout(outputSettingsLabelWidget);

    mOutputSettingsProfilesButton = new OutputProfilesListButton(this);
    mOutputSettingsProfilesButton->setFocusPolicy(Qt::NoFocus);
    mOutputSettingsProfilesButton->setSizePolicy(QSizePolicy::Expanding,
                                                 QSizePolicy::Preferred);

    connect(mOutputSettingsProfilesButton, &OutputProfilesListButton::profileSelected,
            this, &RenderInstanceWidget::outputSettingsProfileSelected);

    mOutputSettingsButton = new QPushButton(QIcon::fromTheme("file_movie"),
                                            tr("Format"));
    mOutputSettingsButton->setFocusPolicy(Qt::NoFocus);
    mOutputSettingsButton->setSizePolicy(QSizePolicy::Expanding,
                                         QSizePolicy::Preferred);

    connect(mOutputSettingsButton, &QPushButton::pressed,
            this, &RenderInstanceWidget::openOutputSettingsDialog);

    QWidget *outputSettingsOptWidget = new QWidget(this);
    outputSettingsOptWidget->setContentsMargins(0, 0, 0, 0);
    const auto outputSettingsOptLayout = new QHBoxLayout(outputSettingsOptWidget);

    outputSettingsOptLayout->setMargin(0);
    outputSettingsOptLayout->addWidget(mOutputSettingsProfilesButton);
    outputSettingsOptLayout->addWidget(mOutputSettingsButton);

    outputSettingsLayout->addWidget(outputSettingsOptWidget);
    outputSettingsLayout->addWidget(mOutputSettingsDisplayWidget);

    const auto outputDestinationButton = new QPushButton(QIcon::fromTheme("disk_drive"),
                                                         QString(),
                                                         this);
    outputDestinationButton->setFocusPolicy(Qt::NoFocus);
    outputDestinationButton->setToolTip(tr("Select output file"));

    connect(outputDestinationButton, &QPushButton::pressed,
            this, &RenderInstanceWidget::openOutputDestinationDialog);

    const auto playButton = new QPushButton(QIcon::fromTheme("play"),
                                            QString(),
                                            this);
    playButton->setFocusPolicy(Qt::NoFocus);
    playButton->setToolTip(tr("Open in default application"));

    connect(playButton, &QPushButton::pressed,
            this, [this]() {
        const QString dst = mOutputDestinationLineEdit->text();
        if (dst.trimmed().isEmpty()) { return; }
        QDesktopServices::openUrl(dst.contains("%0") ?
                                      QUrl::fromLocalFile(QFileInfo(dst).absolutePath()) :
                                      QUrl::fromLocalFile(dst));
    });

    mOutputDestinationLineEdit = new QLineEdit(this);
    mOutputDestinationLineEdit->setFocusPolicy(Qt::NoFocus);
    mOutputDestinationLineEdit->setSizePolicy(QSizePolicy::Expanding,
                                              QSizePolicy::Preferred);
    mOutputDestinationLineEdit->setReadOnly(true);
    mOutputDestinationLineEdit->setPlaceholderText(tr("Destination ..."));

    eSizesUI::widget.add(mOutputSettingsProfilesButton,
                         [renderSettingsButton,
                          playButton,
                          outputDestinationButton,
                          this](const int size) {
        Q_UNUSED(size)
        renderSettingsButton->setFixedHeight(eSizesUI::button);
        mOutputSettingsButton->setFixedHeight(eSizesUI::button);
        mOutputSettingsProfilesButton->setFixedHeight(eSizesUI::button);
        outputDestinationButton->setFixedHeight(eSizesUI::button);
        playButton->setFixedSize(QSize(eSizesUI::button, eSizesUI::button));
        mOutputDestinationLineEdit->setFixedHeight(eSizesUI::button);
    });

    QWidget *outputDestinationWidget = new QWidget(this);
    outputDestinationWidget->setContentsMargins(0, 0, 0, 0);
    const auto outputDestinationLayout = new QHBoxLayout(outputDestinationWidget);
    outputDestinationLayout->setMargin(0);

    outputDestinationLayout->addWidget(outputDestinationButton);
    outputDestinationLayout->addWidget(mOutputDestinationLineEdit);
    outputDestinationLayout->addWidget(playButton);

    outputSettingsLayout->addWidget(outputDestinationWidget);

    contLayout->addWidget(outputSettingsLabelWidget);

    contLayout->setSpacing(0);
    contLayout->setContentsMargins(0, 0, 0, 0);
}

void RenderInstanceWidget::updateFromSettings()
{
    const auto renderState = mSettings.getCurrentState();
    bool enabled = renderState != RenderState::paused &&
                   renderState != RenderState::rendering;
    setEnabled(enabled);

    QString nameLabelTxt = QString(mSettings.getName());
    const OutputSettings &outputSettings = mSettings.getOutputRenderSettings();

    const auto format = outputSettings.fOutputFormat;
    if (format) {
        QString formatString(format->name);
        if (formatString == "image2") {
            const auto codec = outputSettings.fVideoCodec;
            if (codec) { formatString = QString(codec->name); }
        }
        nameLabelTxt.append(QString(" [%1]").arg(QString(formatString).toUpper()));
    }

    switch(renderState) {
    case RenderState::error:
        nameLabelTxt.append(QString(" : %1").arg(tr("Error")));
        break;
    case RenderState::finished:
        nameLabelTxt.append(QString(" : %1").arg(tr("Finished")));
        mCheckBox->setChecked(false);
        break;
    case RenderState::rendering:
        nameLabelTxt.append(QString(" : %1").arg(tr("Rendering ...")));
        break;
    case RenderState::waiting:
        nameLabelTxt.append(QString(" : %1").arg(tr("Waiting ...")));
        break;
    case RenderState::paused:
        nameLabelTxt.append(QString(" : %1").arg(tr("Paused")));
        break;
    default:;
    }

    mNameLabel->setText(nameLabelTxt);

    QString destinationTxt = mSettings.getOutputDestination();
    mOutputDestinationLineEdit->setText(destinationTxt);

    mOutputSettingsDisplayWidget->setOutputSettings(outputSettings);

    const RenderSettings &renderSettings = mSettings.getRenderSettings();
    mRenderSettingsDisplayWidget->setRenderSettings(mSettings.getTargetCanvas(),
                                                    renderSettings);
}

RenderInstanceSettings &RenderInstanceWidget::getSettings()
{
    return mSettings;
}

void RenderInstanceWidget::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::RightButton) {
        QMenu menu(this);

        const auto state = mSettings.getCurrentState();
        const bool deletable = (state != RenderState::rendering && state != RenderState::paused);

        const auto dupAct = menu.addAction(QIcon::fromTheme("duplicate"), tr("Duplicate"));
        dupAct->setData(0);

        const auto delAct = menu.addAction(QIcon::fromTheme("minus"), tr("Remove"));
        delAct->setData(1);
        delAct->setEnabled(deletable);

        const auto act = menu.exec(e->globalPos());
        if (act) {
            switch (act->data().toInt()) {
            case 0:
                emit duplicate(mSettings);
                break;
            case 1:
                deleteLater();
                break;
            default:;
            }
        }
    } else { return ClosableContainer::mousePressEvent(e); }
}

void RenderInstanceWidget::openOutputSettingsDialog()
{
    const OutputSettings &outputSettings = mSettings.getOutputRenderSettings();
    OutputSettingsDialog dialog(outputSettings, this);

    if (dialog.exec()) {
        const auto origSettings = mSettings.getOutputRenderSettings();
        mSettings.setOutputSettingsProfile(nullptr);
        OutputSettings outputSettings = dialog.getSettings();
        mSettings.setOutputRenderSettings(outputSettings);
        mOutputSettingsDisplayWidget->setOutputSettings(outputSettings);

        if (outputSettings.fOutputFormat != origSettings.fOutputFormat) {
            clearOutputDestination();
        }
    }

}

void RenderInstanceWidget::outputSettingsProfileSelected(OutputSettingsProfile *profile)
{
    const auto origSettings = mSettings.getOutputRenderSettings();
    mSettings.setOutputSettingsProfile(profile);
    updateFromSettings();

    if (origSettings.fOutputFormat !=
        mSettings.getOutputRenderSettings().fOutputFormat) {
        clearOutputDestination();
    }
}

void RenderInstanceWidget::clearOutputDestination()
{
    QString dst;
    mSettings.setOutputDestination(dst);
    mOutputDestinationLineEdit->setText(dst);
}

void RenderInstanceWidget::openOutputDestinationDialog()
{
    QString supportedExts;
    QString selectedExt;

    const OutputSettings &outputSettings = mSettings.getOutputRenderSettings();
    const auto format = outputSettings.fOutputFormat;
    bool isSeq = false;

    if (format) {
        isSeq = !std::strcmp(format->name, "image2");
        QString tmpStr = QString(format->extensions);
        const auto exts = getExportImageExtensions(outputSettings);
        if (!exts.first.isEmpty() && !exts.second.isEmpty()) {
            selectedExt = exts.first;
            supportedExts = tr("Output File (%1)").arg(exts.second.join(" "));
        } else {
            const QStringList supportedExt = tmpStr.split(",");
            selectedExt = "." + supportedExt.first();
            tmpStr.replace(",", " *.");
            supportedExts = tr("Output File (*.%1)").arg(tmpStr);
        }
    } else { return; }

    if (AppSupport::isFlatpak()) {
        qWarning() << "supported extensions" << supportedExts;
        qWarning() << "selected extension" << selectedExt;
    }

    QString iniText = mSettings.getOutputDestination();
    if (iniText.isEmpty()) {
        iniText = QString("%1/%2%3").arg(Document::sInstance->projectDirectory(),
                                         tr("untitled"),
                                         selectedExt);
    }

    if (AppSupport::isFlatpak()) {
        qWarning() << "initial output destination" << iniText;
    }

    QString saveAs;

    if (isSeq) {
        saveAs = AppSupport::getSaveSequence(this,
                                             tr("Output Destination"),
                                             iniText);
    } else {
        saveAs = AppSupport::getSaveFile(this,
                                         tr("Output Destination"),
                                         iniText,
                                         supportedExts);
    }

    if (AppSupport::isFlatpak()) {
        qWarning() << "output destination is now" << saveAs;
    }

    if (saveAs.isEmpty()) { return; }

    mSettings.setOutputDestination(saveAs);
    mOutputDestinationLineEdit->setText(saveAs);
}

void RenderInstanceWidget::openRenderSettingsDialog()
{
    RenderSettingsDialog dialog(mSettings, this);

    if (dialog.exec()) {
        const RenderSettings sett = dialog.getSettings();
        mSettings.setRenderSettings(sett);
        mSettings.setTargetCanvas(dialog.getCurrentScene());
        updateFromSettings();
    }
}

const QPair<QString, QStringList>
RenderInstanceWidget::getExportImageExtensions(const OutputSettings &settings)
{
    const auto format = settings.fOutputFormat;
    const auto codec = settings.fVideoCodec;
    QPair<QString,QStringList> ext;

    if (!format) { return ext; }

    if (QString(format->long_name).startsWith("image2") && codec) {
        const auto codecName = QString(codec->name);
        if (codecName == "png") {
            ext.first = ".png";
            ext.second << "*.png";
        } else if (codecName == "tiff") {
            ext.first = ".tif";
            ext.second << "*.tif" << "*.tiff";
        } else if (codecName.endsWith("jpeg")) {
            ext.first = ".jpg";
            ext.second << "*.jpg" << "*.jpeg";
        }
    }

    return ext;
}

void RenderInstanceWidget::write(eWriteStream &dst) const
{
    mSettings.write(dst);
    dst << isChecked();
}

void RenderInstanceWidget::read(eReadStream &src)
{
    mSettings.read(src);
    bool checked; src >> checked;
    setChecked(checked);
}

// TODO: xev/xml read/write are missing!

void RenderInstanceWidget::updateRenderSettings()
{
    const RenderSettings &renderSettings = mSettings.getRenderSettings();
    mRenderSettingsDisplayWidget->setRenderSettings(mSettings.getTargetCanvas(),
                                                    renderSettings);

    if (mSettings.getTargetCanvas()) {
        const auto label = mSettings.getTargetCanvas()->prp_getName();
        if (!label.isEmpty()) { mNameLabel->setText(label); }
    }

    updateFromSettings();
}

OutputProfilesListButton::OutputProfilesListButton(RenderInstanceWidget *parent)
    : QPushButton(parent)
{
    mParentWidget = parent;
    setText(tr("Profiles"));
    setIcon(QIcon::fromTheme("renderlayers"));
    setToolTip(tr("Select output profile"));
}

void OutputProfilesListButton::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        QMenu menu;
        int i = 0;

        for (const auto& profile : OutputSettingsProfile::sOutputProfiles) {
            QAction *actionT = new QAction(profile->getName());
            actionT->setData(QVariant(i));
            menu.addAction(actionT);
            i++;
        }

        if (OutputSettingsProfile::sOutputProfiles.isEmpty()) {
            menu.addAction(tr("No profiles"))->setEnabled(false);
        }

        menu.addSeparator();

        QAction *actionT = new QAction(tr("Edit..."));
        actionT->setData(QVariant(-1));
        menu.addAction(actionT);

        QAction *selectedAction = menu.exec(mapToGlobal(QPoint(0, height())));
        if (selectedAction) {
            int profileId = selectedAction->data().toInt();
            if (profileId == -1) {
                const OutputSettings &outputSettings = mParentWidget->getSettings().getOutputRenderSettings();
                OutputProfilesDialog dialog(outputSettings, this);
                if (dialog.exec()) {
                    const auto profile = dialog.getCurrentProfile();
                    emit profileSelected(profile);
                }
            } else {
                const auto profile = OutputSettingsProfile::sOutputProfiles.at(profileId).get();
                emit profileSelected(profile);
            }
        }
    }
}
