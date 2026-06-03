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
*/

#include "exportlottiedialog.h"

#include "Private/Tasks/taskscheduler.h"
#include "Private/document.h"
#include "GUI/edialogs.h"
#include "appsupport.h"
#include "canvas.h"
#include "lottie/lottieexporter.h"
#include "widgets/scenechooser.h"
#include "widgets/twocolumnlayout.h"

#include <QCheckBox>
#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTemporaryFile>
#include <QTextStream>
#include <QUrl>
#include <QVBoxLayout>

ExportLottieDialog::ExportLottieDialog(QWidget* const parent,
                                       const QString& warnings)
    : Friction::Ui::Dialog(parent)
{
    setWindowTitle(tr("Export Lottie"));

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
    mBackground->setChecked(AppSupport::getSettings("exportLottie",
                                                    "background",
                                                    true).toBool());
    mEmbedImages = new QCheckBox(tr("Embed images"), this);
    mEmbedImages->setChecked(AppSupport::getSettings("exportLottie",
                                                     "embedImages",
                                                     true).toBool());
    mNotify = new QCheckBox(tr("Notify when done"), this);
    mNotify->setChecked(AppSupport::getSettings("exportLottie",
                                                "notify",
                                                true).toBool());

    connect(mBackground, &QCheckBox::stateChanged,
            this, [this] {
        AppSupport::setSettings("exportLottie",
                                "background",
                                mBackground->isChecked());
    });
    connect(mNotify, &QCheckBox::stateChanged,
            this, [this] {
        AppSupport::setSettings("exportLottie",
                                "notify",
                                mNotify->isChecked());
    });
    connect(mEmbedImages, &QCheckBox::stateChanged,
            this, [this] {
        AppSupport::setSettings("exportLottie",
                                "embedImages",
                                mEmbedImages->isChecked());
    });

    twoColLayout->addPair(new QLabel(tr("Scene")), sceneButton);
    twoColLayout->addPair(new QLabel(tr("First Frame")), mFirstFrame);
    twoColLayout->addPair(new QLabel(tr("Last Frame")), mLastFrame);

    const auto sceneWidget = new QGroupBox(tr("Scene"), this);
    const auto optsWidget = new QGroupBox(tr("Options"), this);

    sceneWidget->setObjectName("BlueBox");
    optsWidget->setObjectName("BlueBox");

    const auto optsTwoCol = new TwoColumnLayout();
    optsTwoCol->addPair(mBackground, mEmbedImages);
    optsTwoCol->addPair(mNotify, new QWidget(this));
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
    mPreviewButton = new QPushButton(QIcon::fromTheme("seq_preview"),
                                     tr("Preview"),
                                     this);
    mPreviewButton->setObjectName("LottiePreviewButton");

    connect(mPreviewButton, &QPushButton::released,
            this, [this] { showPreview(false); });

    connect(buttonExport, &QPushButton::clicked, this, [this]() {
        const QString fileType = tr("Lottie Files %1", "ExportDialog_FileType");
        QString saveAs = eDialogs::saveFile(tr("Export Lottie"),
                                            AppSupport::getSettings("files",
                                                                    "recentExported",
                                                                    QDir::homePath()).toString(),
                                            fileType.arg("(*.json)"));
        if (saveAs.isEmpty()) { return; }
        if (!saveAs.endsWith(".json")) { saveAs.append(".json"); }
        QFileInfo saveInfo(saveAs);
        AppSupport::setSettings("files",
                                "recentExported",
                                saveInfo.absoluteDir().absolutePath());
        const bool success = exportTo(saveAs);
        if (success) {
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

    buttonsLayout->addWidget(mPreviewButton);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(buttonExport);
    buttonsLayout->addWidget(buttonCancel);
    settingsLayout->addWidget(buttons);

    mPreviewButton->setEnabled(scene);
    buttonExport->setEnabled(scene);
    connect(mScene, &SceneChooser::currentChanged,
            this, [this, sceneButton, buttonExport](Canvas* const scene) {
        buttonExport->setEnabled(scene);
        mPreviewButton->setEnabled(scene);
        sceneButton->setText(mScene->title());
    });

    setLayout(settingsLayout);
}

void ExportLottieDialog::showPreview(const bool& closeWhenDone)
{
    if (!mPreviewJsonFile) {
        const QString templ = QString::fromUtf8("%1/%2_lottie_preview_XXXXXX.json").arg(AppSupport::getAppTempPath(),
                                                                                        AppSupport::getAppName());
        mPreviewJsonFile = QSharedPointer<QTemporaryFile>::create(templ);
        mPreviewJsonFile->setAutoRemove(false);
        mPreviewJsonFile->open();
        mPreviewJsonFile->close();
    }
    if (!mPreviewHtmlFile) {
        const QString templ = QString::fromUtf8("%1/%2_lottie_preview_XXXXXX.html").arg(AppSupport::getAppTempPath(),
                                                                                        AppSupport::getAppName());
        mPreviewHtmlFile = QSharedPointer<QTemporaryFile>::create(templ);
        mPreviewHtmlFile->setAutoRemove(false);
        mPreviewHtmlFile->open();
        mPreviewHtmlFile->close();
    }

    const QString jsonFile = mPreviewJsonFile->fileName();
    const QString htmlFile = mPreviewHtmlFile->fileName();
    if (!exportTo(jsonFile) || !writePreviewHtml(jsonFile, htmlFile)) {
        if (closeWhenDone) { close(); }
        return;
    }

    QDesktopServices::openUrl(QUrl::fromLocalFile(htmlFile));
    if (closeWhenDone) { close(); }
}

bool ExportLottieDialog::exportTo(const QString& file)
{
    try {
        const auto scene = mScene->getCurrentScene();
        if (!scene) { RuntimeThrow(tr("No scene selected")); }

        const FrameRange frameRange{mFirstFrame->value(), mLastFrame->value()};
        const auto task = new LottieExporter(file,
                                             scene,
                                             frameRange,
                                             scene->getFps(),
                                             mBackground->isChecked(),
                                             mEmbedImages->isChecked());
        const auto taskSPtr = qsptr<LottieExporter>(task, &QObject::deleteLater);
        task->nextStep();
        TaskScheduler::instance()->addComplexTask(taskSPtr);
        return true;
    } catch(const std::exception& e) {
        gPrintExceptionCritical(e);
        return false;
    }
}

bool ExportLottieDialog::writePreviewHtml(const QString& jsonFile,
                                          const QString& htmlFile)
{
    QFile json(jsonFile);
    if (!json.open(QIODevice::ReadOnly)) { return false; }
    const QByteArray encodedJson = json.readAll().toBase64();
    json.close();

    QFile html(htmlFile);
    if (!html.open(QIODevice::WriteOnly | QIODevice::Truncate)) { return false; }

    QTextStream stream(&html);
    stream << "<!DOCTYPE html>\n";
    stream << "<html>\n";
    stream << "<head>\n";
    stream << "<meta charset=\"utf-8\" />\n";
    stream << "<title>" << tr("Lottie Preview") << "</title>\n";
    stream << "<style>\n";
    stream << "html,body{width:100%;height:100%;margin:0;padding:0;overflow:hidden;}\n";
    stream << "html{background:repeating-conic-gradient(#b0b0b0 0% 25%,transparent 0% 50%) 50%/40px 40px;}\n";
    stream << "body{font:13px -apple-system,BlinkMacSystemFont,\"Segoe UI\",sans-serif;color:#f1f3f4;}\n";
    stream << "#preview{width:100%;height:calc(100% - 56px);}\n";
    stream << "#controls{position:fixed;left:0;right:0;bottom:0;box-sizing:border-box;height:56px;display:flex;align-items:center;gap:10px;padding:10px 14px;background:rgba(32,33,36,.92);box-shadow:0 -1px 8px rgba(0,0,0,.25);}\n";
    stream << "button,select{height:32px;border:1px solid #5f6368;border-radius:4px;background:#303134;color:#f1f3f4;font:inherit;}\n";
    stream << "button{min-width:38px;padding:0 10px;cursor:pointer;}\n";
    stream << "button:hover,select:hover{background:#3c4043;}\n";
    stream << "label{display:flex;align-items:center;gap:5px;white-space:nowrap;}\n";
    stream << "#frame{min-width:92px;text-align:right;color:#bdc1c6;}\n";
    stream << "#scrub{flex:1;min-width:80px;}\n";
    stream << "#error{display:none;box-sizing:border-box;width:100%;height:100%;padding:24px;font:14px sans-serif;background:#202124;color:#f1f3f4;}\n";
    stream << "</style>\n";
    stream << "</head>\n";
    stream << "<body>\n";
    stream << "<div id=\"preview\"></div>\n";
    stream << "<div id=\"controls\">\n";
    stream << "<button id=\"play\" type=\"button\">Pause</button>\n";
    stream << "<button id=\"restart\" type=\"button\">Restart</button>\n";
    stream << "<input id=\"scrub\" type=\"range\" min=\"0\" max=\"100\" value=\"0\" step=\"1\" />\n";
    stream << "<span id=\"frame\">0 / 0</span>\n";
    stream << "<select id=\"mode\">\n";
    stream << "<option value=\"loop\" selected>Loop</option>\n";
    stream << "<option value=\"pingpong\">Ping-pong</option>\n";
    stream << "<option value=\"once\">Once</option>\n";
    stream << "</select>\n";
    stream << "<select id=\"speed\">\n";
    stream << "<option value=\"0.25\">0.25x</option>\n";
    stream << "<option value=\"0.5\">0.5x</option>\n";
    stream << "<option value=\"1\" selected>1x</option>\n";
    stream << "<option value=\"1.5\">1.5x</option>\n";
    stream << "<option value=\"2\">2x</option>\n";
    stream << "</select>\n";
    stream << "</div>\n";
    stream << "<div id=\"error\"></div>\n";
    stream << "<script src=\"https://cdnjs.cloudflare.com/ajax/libs/lottie-web/5.12.2/lottie.min.js\"></script>\n";
    stream << "<script>\n";
    stream << "const encoded='" << QString::fromLatin1(encodedJson) << "';\n";
    stream << "const showError=(message)=>{const el=document.getElementById('error');el.textContent=message;el.style.display='block';document.getElementById('preview').style.display='none';document.getElementById('controls').style.display='none';};\n";
    stream << "try{if(!window.lottie){throw new Error('Could not load lottie-web. Check your network connection.');}\n";
    stream << "const animationData=JSON.parse(atob(encoded));\n";
    stream << "const anim=lottie.loadAnimation({container:document.getElementById('preview'),renderer:'svg',loop:true,autoplay:true,animationData});\n";
    stream << "const play=document.getElementById('play');\n";
    stream << "const restart=document.getElementById('restart');\n";
    stream << "const scrub=document.getElementById('scrub');\n";
    stream << "const frame=document.getElementById('frame');\n";
    stream << "const mode=document.getElementById('mode');\n";
    stream << "const speed=document.getElementById('speed');\n";
    stream << "let dragging=false;\n";
    stream << "let direction=1;\n";
    stream << "const total=()=>Math.max(1, Math.floor(anim.totalFrames || (animationData.op-animationData.ip) || 1));\n";
    stream << "const current=()=>Math.max(0, Math.min(total()-1, Math.floor(anim.currentFrame || 0)));\n";
    stream << "const update=()=>{const t=total();const c=current();scrub.max=String(t-1);if(!dragging){scrub.value=String(c);}frame.textContent=(c+1)+' / '+t;play.textContent=anim.isPaused?'Play':'Pause';};\n";
    stream << "const applyMode=()=>{const m=mode.value;anim.loop=m==='loop';if(m==='loop'||m==='once'){direction=1;anim.setDirection(1);}update();};\n";
    stream << "play.addEventListener('click',()=>{if(anim.isPaused){anim.play();}else{anim.pause();}update();});\n";
    stream << "restart.addEventListener('click',()=>{direction=1;anim.setDirection(1);anim.goToAndPlay(0,true);update();});\n";
    stream << "mode.addEventListener('change',applyMode);\n";
    stream << "speed.addEventListener('change',()=>{anim.setSpeed(parseFloat(speed.value)||1);});\n";
    stream << "scrub.addEventListener('input',()=>{dragging=true;anim.goToAndStop(parseInt(scrub.value,10)||0,true);update();});\n";
    stream << "scrub.addEventListener('change',()=>{dragging=false;update();});\n";
    stream << "anim.addEventListener('DOMLoaded',update);\n";
    stream << "anim.addEventListener('enterFrame',update);\n";
    stream << "anim.addEventListener('complete',()=>{if(mode.value==='pingpong'){direction*=-1;anim.setDirection(direction);anim.goToAndPlay(direction>0?0:total()-1,true);}update();});\n";
    stream << "applyMode();\n";
    stream << "update();\n";
    stream << "}catch(error){showError(error.message || String(error));}\n";
    stream << "</script>\n";
    stream << "</body>\n";
    stream << "</html>\n";
    stream.flush();
    html.close();
    return true;
}

void ExportLottieDialog::finishedDialog(const QString& fileName)
{
    const QString askOpenFile = tr("Open File");
    const QString askOpenFolder = tr("Open Folder");
    const QString askClose = tr("Close");
    const int ask = QMessageBox::information(this,
                                             tr("Lottie export finished"),
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
