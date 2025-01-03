/*
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

#include "exportsvgdialog.h"

#include "Private/document.h"
#include "widgets/scenechooser.h"
#include "GUI/global.h"
#include "canvas.h"
#include "svgexporter.h"
#include "GUI/edialogs.h"
#include "widgets/twocolumnlayout.h"
#include "appsupport.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QMenuBar>
#include <QDesktopServices>
#include <QPlainTextEdit>
#include <QProcess>
#include <QMessageBox>
#include <QGroupBox>

ExportSvgDialog::ExportSvgDialog(QWidget* const parent,
                                 const QString &warnings)
    : Friction::Ui::Dialog(parent)
{
    setWindowTitle(tr("Export SVG"));

    const auto settingsLayout = new QVBoxLayout();

    const auto twoColLayout = new TwoColumnLayout();

    const auto document = Document::sInstance;
    mScene = new SceneChooser(*document, false, this);
    auto scene = *document->fActiveScene;
    if (!scene) {
        const auto& visScenes = document->fVisibleScenes;
        if (!visScenes.empty()) { scene = visScenes.begin()->first; }
        else {
            const auto& scenes = document->fScenes;
            if (!scenes.isEmpty()) { scene = scenes.first().get(); }
        }
    }
    mScene->setCurrentScene(scene);

    const auto sceneButton = new QPushButton(mScene->title(), this);
    sceneButton->setMenu(mScene);

    mFirstFrame = new QSpinBox(this);
    mLastFrame = new QSpinBox(this);

    const int minFrame = scene ? scene->getMinFrame() : 0;
    const int maxFrame = scene ? scene->getMaxFrame() : 0;

    mFirstFrame->setRange(-INT_MAX, maxFrame);
    mFirstFrame->setValue(minFrame);
    mLastFrame->setRange(minFrame, INT_MAX);
    mLastFrame->setValue(maxFrame);

    mBackground = new QCheckBox(tr("Background"), this);
    mBackground->setChecked(AppSupport::getSettings("exportSVG",
                                                    "background",
                                                    true).toBool());
    mFixedSize = new QCheckBox(tr("Fixed Size"), this);
    mFixedSize->setChecked(AppSupport::getSettings("exportSVG",
                                                   "fixed",
                                                   false).toBool());
    mLoop = new QCheckBox(tr("Loop"), this);
    mLoop->setChecked(AppSupport::getSettings("exportSVG",
                                              "loop",
                                              true).toBool());

    mOptimize = new QCheckBox(tr("Optimize for Web"), this);
    mOptimize->setChecked(AppSupport::getSettings("exportSVG",
                                                  "optimize",
                                                  false).toBool());
    bool hasSVGO = !AppSupport::getSVGO().isEmpty();
    mOptimize->setEnabled(hasSVGO);
    if (!hasSVGO) {
        mOptimize->setChecked(false);
        mOptimize->setToolTip(tr("SVG Optimizer is missing."));
    }

    mNotify = new QCheckBox(tr("Notify when done"), this);
    mNotify->setChecked(AppSupport::getSettings("exportSVG",
                                                "notify",
                                                true).toBool());

    connect(mBackground, &QCheckBox::stateChanged,
            this, [this] {
        AppSupport::setSettings("exportSVG",
                                "background",
                                mBackground->isChecked());
    });
    connect(mFixedSize, &QCheckBox::stateChanged,
            this, [this] {
        AppSupport::setSettings("exportSVG",
                                "fixed",
                                mFixedSize->isChecked());
    });
    connect(mLoop, &QCheckBox::stateChanged,
            this, [this] {
        AppSupport::setSettings("exportSVG",
                                "loop",
                                mLoop->isChecked());
    });
    connect(mOptimize, &QCheckBox::stateChanged,
            this, [this] {
        AppSupport::setSettings("exportSVG",
                                "optimize",
                                mOptimize->isChecked());
    });
    connect(mNotify, &QCheckBox::stateChanged,
            this, [this] {
        AppSupport::setSettings("exportSVG",
                                "notify",
                                mNotify->isChecked());
    });

    twoColLayout->addPair(new QLabel(tr("Scene")), sceneButton);
    twoColLayout->addPair(new QLabel(tr("First Frame")), mFirstFrame);
    twoColLayout->addPair(new QLabel(tr("Last Frame")), mLastFrame);

    // image options
    mImageFormat = new QComboBox(this);
    mImageFormat->setToolTip(tr("Image format used for raster elements (images/videos) contained in the scene"));
    mImageFormat->addItem(tr("PNG Image Format"));
    mImageFormat->addItem(tr("JPEG Image Format"));
    mImageFormat->addItem(tr("WEBP Image Format"));
    mImageFormat->setMinimumWidth(50);
    mImageFormat->setSizeIncrement(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mImageFormat->setCurrentIndex(AppSupport::getSettings("exportSVG",
                                                          "imageFormat",
                                                          0).toInt());
    connect(mImageFormat,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            [=](int i) {
        AppSupport::setSettings("exportSVG",
                                "imageFormat",
                                i);
    });

    mImageQuality = new QSpinBox(this);
    mImageQuality->setToolTip(tr("Image format quality"));
    mImageQuality->setRange(1, 100);
    mImageFormat->setMinimumWidth(50);
    mImageQuality->setSizeIncrement(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mImageQuality->setValue(AppSupport::getSettings("exportSVG",
                                                    "imageQuality",
                                                    100).toInt());
    mImageQuality->setSuffix(tr("%"));

    connect(mImageQuality,
            QOverload<int>::of(&QSpinBox::valueChanged),
            [=](int i) {
        AppSupport::setSettings("exportSVG",
                                "imageQuality",
                                i);
    });

    // layout
    const auto sceneWidget = new QGroupBox(tr("Scene"), this);
    const auto optsWidget = new QGroupBox(tr("Options"), this);

    sceneWidget->setObjectName("BlueBox");
    optsWidget->setObjectName("BlueBox");

    const auto optsTwoCol = new TwoColumnLayout();
    optsTwoCol->addPair(mImageFormat, mImageQuality);
    optsTwoCol->addPair(mBackground, mFixedSize);
    optsTwoCol->addPair(mLoop, mOptimize);
    optsTwoCol->addPair(mNotify, new QWidget());
    optsTwoCol->addSpacing(4);

    sceneWidget->setLayout(twoColLayout);
    optsWidget->setLayout(optsTwoCol);

    const auto container = new QWidget(this);
    const auto wrapper = new QHBoxLayout(container);

    container->setContentsMargins(0, 0, 0, 0);
    wrapper->setContentsMargins(0, 0, 0, 0);
    wrapper->addWidget(sceneWidget);
    wrapper->addWidget(optsWidget);

    settingsLayout->addWidget(container);

    connect(mFirstFrame, qOverload<int>(&QSpinBox::valueChanged),
            mLastFrame, &QSpinBox::setMinimum);
    connect(mLastFrame, qOverload<int>(&QSpinBox::valueChanged),
            mFirstFrame, &QSpinBox::setMaximum);

    const auto buttons = new QWidget(this);
    buttons->setContentsMargins(0, 0, 0, 0);
    const auto buttonsLayout = new QHBoxLayout(buttons);
    buttonsLayout->setContentsMargins(0, 0, 0, 0);

    const auto buttonExport = new QPushButton(QIcon::fromTheme("dialog-ok"),
                                              tr("Export"),
                                              this);
    const auto buttonCancel = new QPushButton(QIcon::fromTheme("dialog-cancel"),
                                              tr("Close"),
                                              this);

    connect(buttonExport, &QPushButton::clicked, this, [this]() {
        const QString fileType = tr("SVG Files %1", "ExportDialog_FileType");
        QString saveAs = eDialogs::saveFile(tr("Export SVG"),
                                            AppSupport::getSettings("files",
                                                                    "recentExported",
                                                                    QDir::homePath()).toString(),
                                            fileType.arg("(*.svg)"));
        if (saveAs.isEmpty()) { return; }
        if (!saveAs.endsWith(".svg")) { saveAs.append(".svg"); }
        QFileInfo saveInfo(saveAs);
        AppSupport::setSettings("files",
                                "recentExported",
                                saveInfo.absoluteDir().absolutePath());
        const bool success = exportTo(saveAs);
        if (success) {
            if (mOptimize->isEnabled() &&
                mOptimize->isChecked()) {
                if (!QProcess::startDetached(AppSupport::getSVGO(),
                                             QStringList() << "--config"
                                                           << AppSupport::getSVGOConfig()
                                                           << saveAs)) {
                    QMessageBox::warning(this,
                                         tr("Optimizer Failed"),
                                         tr("Failed to launch SVG optimizer."));
                }
            }
            if (mNotify->isChecked()) { finishedDialog(saveAs); }
            accept();
        }
    });

    connect(buttonCancel, &QPushButton::clicked, this, &QDialog::reject);

    if (!warnings.isEmpty()) {
        const auto warnWidget = new QPlainTextEdit(this);
        warnWidget->setSizePolicy(QSizePolicy::Expanding,
                                  QSizePolicy::Expanding);
        warnWidget->setMinimumHeight(100);
        warnWidget->setReadOnly(true);
        warnWidget->setPlainText(warnings);
        settingsLayout->addWidget(warnWidget);
    } else {
        settingsLayout->addStretch();
    }

    settingsLayout->addWidget(buttons);

    mPreviewButton = new QPushButton(tr("Preview"), this);
    mPreviewButton->setIcon(QIcon::fromTheme("seq_preview"));
    mPreviewButton->setObjectName("SVGPreviewButton");

    connect(mPreviewButton, &QPushButton::released,
            this, [this] { showPreview(false); });

    mPreviewButton->setEnabled(scene);
    buttonExport->setEnabled(scene);
    connect(mScene, &SceneChooser::currentChanged,
            this, [this, sceneButton, buttonExport](Canvas* const scene) {
        buttonExport->setEnabled(scene);
        mPreviewButton->setEnabled(scene);
        sceneButton->setText(mScene->title());
    });

    buttonsLayout->addWidget(mPreviewButton);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(buttonExport);
    buttonsLayout->addWidget(buttonCancel);

    setLayout(settingsLayout);
}

void ExportSvgDialog::showPreview(const bool &closeWhenDone)
{
    if (!mPreviewFile) {
        const QString templ = QString::fromUtf8("%1/%2_svg_preview_XXXXXX.html").arg(AppSupport::getAppTempPath(),
                                                                                     AppSupport::getAppName());
        mPreviewFile = qsptr<QTemporaryFile>::create(templ);
        mPreviewFile->setAutoRemove(false);
        mPreviewFile->open();
        mPreviewFile->close();
    }
    const auto fileName = mPreviewFile->fileName();
    const auto task = exportTo(fileName, true);
    if (!task) {
        if (closeWhenDone) { close(); }
        return;
    }
    task->addDependent({[fileName, this, closeWhenDone]() {
                            QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
                            if (closeWhenDone) { close(); }
                        }, nullptr});
}

ComplexTask* ExportSvgDialog::exportTo(const QString& file,
                                       bool preview)
{
    try {
        const auto scene = mScene->getCurrentScene();
        if (!scene) { RuntimeThrow(tr("No scene selected")); }
        const int firstFrame = mFirstFrame->value();
        const int lastFrame = mLastFrame->value();

        const bool background = mBackground->isChecked();
        const bool fixedSize = mFixedSize->isChecked();
        const bool loop = mLoop->isChecked();

        int imageQuality = mImageQuality->value();
        SkEncodedImageFormat imageFormat;
        switch(mImageFormat->currentIndex()) {
        case 1:
            imageFormat = SkEncodedImageFormat::kJPEG;
            break;
        case 2:
            imageFormat = SkEncodedImageFormat::kWEBP;
            break;
        default:
            imageFormat = SkEncodedImageFormat::kPNG;
        }

        const FrameRange frameRange{firstFrame, lastFrame};
        const qreal fps = scene->getFps();

        const auto task = new SvgExporter(file, scene, frameRange, fps,
                                          background, fixedSize, loop,
                                          imageFormat, imageQuality, preview);
        const auto taskSPtr = qsptr<SvgExporter>(task, &QObject::deleteLater);
        task->nextStep();
        TaskScheduler::instance()->addComplexTask(taskSPtr);
        return task;
    } catch(const std::exception& e) {
        gPrintExceptionCritical(e);
        return nullptr;
    }
}

void ExportSvgDialog::finishedDialog(const QString &fileName)
{
    const QString askOpenFile = tr("Open File");
    const QString askOpenFolder = tr("Open Folder");
    const QString askClose = tr("Close");
    const int ask = QMessageBox::information(this,
                                             tr("SVG export finished"),
                                             tr("Project exported to <code>%1</code>.").arg(fileName),
                                             askOpenFile,
                                             askOpenFolder,
                                             askClose,
                                             2,
                                             2);
    QUrl url;
    switch (ask) {
    case 0:
        url = QUrl::fromLocalFile(fileName);
        break;
    case 1:
        url = QUrl::fromLocalFile(QFileInfo(fileName).absolutePath());
        break;
    default:;
    }
    if (!url.isEmpty()) { QDesktopServices::openUrl(url); }
}
