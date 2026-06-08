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
#include <QComboBox>
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
    mFormat = new QComboBox(this);
    mFormat->addItem(tr("Lottie JSON"), QStringLiteral("json"));
    mFormat->addItem(tr("dotLottie"), QStringLiteral("lottie"));
    const QString format = AppSupport::getSettings("exportLottie",
                                                   "format",
                                                   "json").toString();
    mFormat->setCurrentIndex(qMax(0, mFormat->findData(format)));

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
    mEmbedImages->setEnabled(mFormat->currentData().toString() == QStringLiteral("json"));
    mPreviewBackground = new QComboBox(this);
    mPreviewBackground->addItem(tr("White"), QStringLiteral("white"));
    mPreviewBackground->addItem(tr("Black"), QStringLiteral("black"));
    mPreviewBackground->addItem(tr("Gray"), QStringLiteral("gray"));
    mPreviewBackground->addItem(tr("Transparent"), QStringLiteral("transparent"));
    const QString previewBackground = AppSupport::getSettings("exportLottie",
                                                              "previewBackground",
                                                              "white").toString();
    const int previewBackgroundIndex = mPreviewBackground->findData(previewBackground);
    mPreviewBackground->setCurrentIndex(qMax(0, previewBackgroundIndex));
    mNativeText = new QCheckBox(tr("Native text (experimental)"), this);
    mNativeText->setToolTip(tr("Disabled: exports text as vector outlines for best renderer compatibility. "
                               "\nEnabled: keeps simple text as native Lottie text, but it may not render "
                               "correctly in the canvas renderer."));
    mNativeText->setChecked(AppSupport::getSettings("exportLottie",
                                                    "nativeText",
                                                    false).toBool());
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
    connect(mFormat,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this] {
        const QString format = mFormat->currentData().toString();
        AppSupport::setSettings("exportLottie", "format", format);
        mEmbedImages->setEnabled(format == QStringLiteral("json"));
        emit formatChanged(format);
    });
    connect(mPreviewBackground,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this] {
        AppSupport::setSettings("exportLottie",
                                "previewBackground",
                                mPreviewBackground->currentData());
    });
    connect(mNativeText, &QCheckBox::stateChanged,
            this, [this] {
        AppSupport::setSettings("exportLottie",
                                "nativeText",
                                mNativeText->isChecked());
    });

    twoColLayout->addPair(new QLabel(tr("Scene")), sceneButton);
    twoColLayout->addPair(new QLabel(tr("First Frame")), mFirstFrame);
    twoColLayout->addPair(new QLabel(tr("Last Frame")), mLastFrame);
    twoColLayout->addPair(new QLabel(tr("Format")), mFormat);

    const auto sceneWidget = new QGroupBox(tr("Scene"), this);
    const auto optsWidget = new QGroupBox(tr("Options"), this);

    sceneWidget->setObjectName("BlueBox");
    optsWidget->setObjectName("BlueBox");

    const auto optsTwoCol = new TwoColumnLayout();
    optsTwoCol->addPair(mBackground, mEmbedImages);
    optsTwoCol->addPair(mNativeText, new QWidget(this));
    optsTwoCol->addPair(mNotify, new QWidget(this));
    optsTwoCol->addPair(new QLabel(tr("Preview background")), mPreviewBackground);
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
                                     mFormat->currentData().toString() == QStringLiteral("lottie") ?
                                         tr("Preview dotLottie") :
                                         tr("Preview Lottie"),
                                     this);
    mPreviewButton->setObjectName("LottiePreviewButton");

    connect(mPreviewButton, &QPushButton::released,
            this, [this] { showPreview(false); });
    connect(mFormat,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this] {
        mPreviewButton->setText(
                    mFormat->currentData().toString() == QStringLiteral("lottie") ?
                        tr("Preview dotLottie") :
                        tr("Preview Lottie"));
    });

    connect(buttonExport, &QPushButton::clicked, this, [this]() {
        const QString fileType = tr("Lottie Files %1", "ExportDialog_FileType");
        const QString extension = mFormat->currentData().toString();
        QString saveAs = eDialogs::saveFile(tr("Export Lottie"),
                                            AppSupport::getSettings("files",
                                                                    "recentExported",
                                                                    QDir::homePath()).toString(),
                                            fileType.arg(QStringLiteral("(*.%1)")
                                                         .arg(extension)));
        if (saveAs.isEmpty()) { return; }
        const QString suffix = QStringLiteral(".") + extension;
        if (!saveAs.endsWith(suffix, Qt::CaseInsensitive)) {
            saveAs.append(suffix);
        }
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

    const QString features = tr("Lottie export supports shapes, paths, groups, masks, "
                                "blend modes, images, text outlines/native text, "
                                "transform keyframes, animated paths, stroke drawing, "
                                "gradients, external or embedded assets, optimized "
                                "keyframes, JSON or dotLottie packaging, and HTML "
                                "preview with playback controls, "
                                "renderer selector, background selector, linked image "
                                "support, and SVG-renderer preview workaround.");
    Q_UNUSED(warnings)

    const auto warnWidget = new QPlainTextEdit(this);
    warnWidget->setSizePolicy(QSizePolicy::Expanding,
                              QSizePolicy::Expanding);
    warnWidget->setMinimumHeight(100);
    warnWidget->setReadOnly(true);
    warnWidget->setPlainText(features);
    settingsLayout->addWidget(warnWidget);

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
    const QString extension = mFormat->currentData().toString();
    if (mPreviewAnimationFile &&
            QFileInfo(mPreviewAnimationFile->fileName()).suffix() != extension) {
        mPreviewAnimationFile.reset();
    }
    if (!mPreviewAnimationFile) {
        const QString templ = QString::fromUtf8("%1/%2_lottie_preview_XXXXXX.%3")
                .arg(AppSupport::getAppTempPath(),
                     AppSupport::getAppName(),
                     extension);
        mPreviewAnimationFile = QSharedPointer<QTemporaryFile>::create(templ);
        mPreviewAnimationFile->setAutoRemove(false);
        mPreviewAnimationFile->open();
        mPreviewAnimationFile->close();
    }
    if (!mPreviewHtmlFile) {
        const QString templ = QString::fromUtf8("%1/%2_lottie_preview_XXXXXX.html").arg(AppSupport::getAppTempPath(),
                                                                                        AppSupport::getAppName());
        mPreviewHtmlFile = QSharedPointer<QTemporaryFile>::create(templ);
        mPreviewHtmlFile->setAutoRemove(false);
        mPreviewHtmlFile->open();
        mPreviewHtmlFile->close();
    }

    const QString animationFile = mPreviewAnimationFile->fileName();
    const QString htmlFile = mPreviewHtmlFile->fileName();
    if (!exportTo(animationFile) || !writePreviewHtml(animationFile, htmlFile)) {
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
                                             mEmbedImages->isChecked(),
                                             false,
                                             mNativeText->isChecked());
        const auto taskSPtr = qsptr<LottieExporter>(task, &QObject::deleteLater);
        task->nextStep();
        TaskScheduler::instance()->addComplexTask(taskSPtr);
        return true;
    } catch(const std::exception& e) {
        gPrintExceptionCritical(e);
        return false;
    }
}

bool ExportLottieDialog::writePreviewHtml(const QString& animationFile,
                                          const QString& htmlFile)
{
    QFile animation(animationFile);
    if (!animation.open(QIODevice::ReadOnly)) { return false; }
    const QByteArray animationData = animation.readAll();
    animation.close();
    const QByteArray encodedAnimation = animationData.toBase64();
    const bool dotLottie = animationFile.endsWith(QStringLiteral(".lottie"),
                                                  Qt::CaseInsensitive);
    const QString assetsBase = QUrl::fromLocalFile(
                QFileInfo(animationFile).absolutePath() + QDir::separator()).toString();
    const QString previewBackground = mPreviewBackground->currentData().toString();
    QString projectName = QFileInfo(Document::sInstance->fEvFile).baseName();
    if (projectName.isEmpty()) { projectName = tr("Untitled"); }
    const QString previewName = dotLottie ?
                tr("Preview dotLottie - %1").arg(projectName) :
                tr("Preview Lottie - %1").arg(projectName);
    const qreal fileSizeKb = animationData.size() / 1024.0;
    const QString fileSize = fileSizeKb < 1024.0 ?
                tr("%1 KB").arg(QString::number(fileSizeKb, 'f', 1)) :
                tr("%1 MB").arg(QString::number(fileSizeKb / 1024.0, 'f', 2));

    QFile html(htmlFile);
    if (!html.open(QIODevice::WriteOnly | QIODevice::Truncate)) { return false; }

    QTextStream stream(&html);
    stream << "<!DOCTYPE html>\n";
    stream << "<html>\n";
    stream << "<head>\n";
    stream << "<meta charset=\"utf-8\" />\n";
    stream << "<title>" << previewName.toHtmlEscaped() << "</title>\n";
    stream << "<style>\n";
    stream << "html,body{width:100%;height:100%;margin:0;padding:0;overflow:hidden;}\n";
    stream << "html{background:#fff;}\n";
    stream << "body{font:13px -apple-system,BlinkMacSystemFont,\"Segoe UI\",sans-serif;color:#f1f3f4;}\n";
    stream << "#preview{width:100%;height:100%;cursor:pointer;}\n";
    stream << "#preview canvas{display:block;width:100%;height:100%;}\n";
    stream << "#preview.wireframe svg path{fill:rgba(239,100,116,.1)!important;stroke:rgba(239,100,116,.78)!important;stroke-width:2px!important;vector-effect:non-scaling-stroke!important;opacity:1!important;}\n";
    stream << "#preview.outlines svg path{fill:none!important;stroke:rgba(239,100,116,.9)!important;stroke-width:2px!important;vector-effect:non-scaling-stroke!important;opacity:1!important;}\n";
    stream << "#controls{position:fixed;left:0;right:0;top:0;z-index:2;box-sizing:border-box;min-height:44px;display:flex;align-items:center;justify-content:space-between;gap:10px;padding:7px 9px;background:#efefef;border-bottom:1px solid rgb(203,203,203);box-shadow:0 4px 12px rgba(0,0,0,.22);opacity:0;transition:opacity 140ms ease;}\n";
    stream << "#controls:hover,body:hover #controls,body:hover #progressWrap{opacity:1;}\n";
    stream << ".controlGroup{display:flex;align-items:center;gap:0;min-width:0;}\n";
    stream << "#previewInfo{position:absolute;left:50%;top:50%;transform:translate(-50%,-50%);display:flex;align-items:baseline;gap:7px;color:#323232;white-space:nowrap;pointer-events:none;}\n";
    stream << "#previewName{font-weight:600;}\n";
    stream << "#previewSize{font-size:11px;color:#888;}\n";
    stream << "#progressWrap{position:fixed;left:0;right:0;bottom:0;z-index:2;display:flex;align-items:center;gap:10px;box-sizing:border-box;padding:8px 10px;background:#efefef;border-top:1px solid rgb(203,203,203);box-shadow:0 4px 12px rgba(0,0,0,.22);opacity:0;transition:opacity 140ms ease;}\n";
    stream << "button{height:30px;border:1px solid #b0b0b0;background:#fff;color:#323232;font:inherit;min-width:34px;padding:0 9px;cursor:pointer;}\n";
    stream << "button:hover,button.active{color:#fff;background:#000;border-color:#000;}\n";
    stream << "button#renderer{border-radius:6px 0 0 6px;border-right-width:0;}\n";
    stream << "button#wireframe{border-radius:0;border-right-width:0;}\n";
    stream << "button#background{border-radius:0 6px 6px 0;}\n";
    stream << "button#mode{border-radius:6px 0 0 6px;border-right-width:0;}\n";
    stream << "button#speed{border-radius:0 6px 6px 0;}\n";
    stream << "body.dotlottie button#renderer,body.dotlottie button#wireframe{display:none;}\n";
    stream << "body.dotlottie button#background{border-radius:6px;}\n";
    stream << "button.active{border-color:#8ab4f8;color:#d2e3fc;}\n";
    stream << "#frame{min-width:92px;text-align:right;color:#c6bdc5;}\n";
    stream << "#scrub{flex:1;min-width:80px;accent-color:#000;}\n";
    stream << "#error{display:none;box-sizing:border-box;width:100%;height:100%;padding:24px;font:14px sans-serif;background:#202124;color:#f1f3f4;}\n";
    stream << "</style>\n";
    stream << "</head>\n";
    stream << "<body" << (dotLottie ? " class=\"dotlottie\"" : "") << ">\n";
    stream << "<div id=\"preview\"></div>\n";
    stream << "<div id=\"controls\">\n";
    stream << "<div class=\"controlGroup\">\n";
    stream << "<button id=\"renderer\" type=\"button\" value=\"canvas\">Canvas</button>\n";
    stream << "<button id=\"wireframe\" type=\"button\">Wireframe</button>\n";
    stream << "<button id=\"background\" type=\"button\" value=\""
           << previewBackground << "\">Background</button>\n";
    stream << "</div>\n";
    stream << "<div id=\"previewInfo\"><span id=\"previewName\">"
           << previewName.toHtmlEscaped()
           << "</span><span id=\"previewSize\">"
           << fileSize.toHtmlEscaped()
           << "</span></div>\n";
    stream << "<div class=\"controlGroup\">\n";
    stream << "<button id=\"mode\" type=\"button\" value=\"loop\">Loop</button>\n";
    stream << "<button id=\"speed\" type=\"button\" value=\"1\">1x</button>\n";
    stream << "</div>\n";
    stream << "</div>\n";
    stream << "<div id=\"progressWrap\">\n";
    stream << "<input id=\"scrub\" type=\"range\" min=\"0\" max=\"100\" value=\"0\" step=\"1\" />\n";
    stream << "<span id=\"frame\">0 / 0</span>\n";
    stream << "</div>\n";
    stream << "<div id=\"error\"></div>\n";
    if (dotLottie) {
    stream << "<script type=\"module\">\n";
    stream << "const encoded='" << QString::fromLatin1(encodedAnimation) << "';\n";
    stream << "const showError=(message)=>{const el=document.getElementById('error');el.textContent=message;el.style.display='block';document.getElementById('preview').style.display='none';document.getElementById('controls').style.display='none';document.getElementById('progressWrap').style.display='none';};\n";
    stream << "try{\n";
    stream << "const {DotLottie}=await import('https://cdn.jsdelivr.net/npm/@lottiefiles/dotlottie-web@0.74.0/+esm');\n";
    stream << "const raw=atob(encoded);const bytes=new Uint8Array(raw.length);for(let i=0;i<raw.length;i++){bytes[i]=raw.charCodeAt(i);}\n";
    stream << "const container=document.getElementById('preview');\n";
    stream << "const canvas=document.createElement('canvas');container.appendChild(canvas);\n";
    stream << "const scrub=document.getElementById('scrub');\n";
    stream << "const frame=document.getElementById('frame');\n";
    stream << "const mode=document.getElementById('mode');\n";
    stream << "const background=document.getElementById('background');\n";
    stream << "const speed=document.getElementById('speed');\n";
    stream << "let dragging=false;let onceCompleted=false;\n";
    stream << "const cycle=(button,options)=>{const index=options.findIndex((option)=>option[0]===button.value);const next=options[(index+1)%options.length];button.value=next[0];button.textContent=next[1];};\n";
    stream << "const backgroundOptions=[['white','White'],['black','Black'],['gray','Gray'],['transparent','Transparent']];\n";
    stream << "const modeOptions=[['loop','Loop'],['pingpong','Ping-pong'],['once','Once']];\n";
    stream << "const speedOptions=[['0.1','0.1x'],['0.25','0.25x'],['0.5','0.5x'],['1','1x'],['1.5','1.5x'],['2','2x'],['3','3x'],['4','4x']];\n";
    stream << "const anim=new DotLottie({canvas,data:bytes.buffer,autoplay:false,loop:true,renderConfig:{autoResize:true},layout:{fit:'contain',align:[0.5,0.5]}});\n";
    stream << "const total=()=>Math.max(1,Math.floor(anim.totalFrames||1));\n";
    stream << "const current=()=>Math.max(0,Math.min(total()-1,Math.floor(anim.currentFrame||0)));\n";
    stream << "const update=()=>{const t=total();const c=current();scrub.max=String(t-1);if(!dragging){scrub.value=String(c);}frame.textContent=c+' / '+(t-1);};\n";
    stream << "const applyMode=()=>{onceCompleted=false;const m=mode.value;anim.setMode(m==='pingpong'?'bounce':'forward');anim.setLoop(m!=='once');update();};\n";
    stream << "const applyBackground=()=>{const checker='repeating-conic-gradient(#b0b0b0 0% 25%,transparent 0% 50%) 50%/40px 40px';const colors={white:'#fff',black:'#000',gray:'#808080'};const value=background.value;if(value==='transparent'){document.documentElement.style.background=checker;document.body.style.background='transparent';return;}document.documentElement.style.background=colors[value]||colors.white;document.body.style.background=colors[value]||colors.white;};\n";
    stream << "const togglePlayback=()=>{if(mode.value==='once'&&onceCompleted){onceCompleted=false;anim.setFrame(0);anim.play();}else if(anim.isPlaying){anim.pause();}else{anim.play();}update();};\n";
    stream << "container.addEventListener('click',togglePlayback);\n";
    stream << "mode.addEventListener('click',()=>{cycle(mode,modeOptions);applyMode();});\n";
    stream << "background.addEventListener('click',()=>{cycle(background,backgroundOptions);background.textContent='Background';applyBackground();});\n";
    stream << "speed.addEventListener('click',()=>{cycle(speed,speedOptions);anim.setSpeed(parseFloat(speed.value)||1);});\n";
    stream << "scrub.addEventListener('input',()=>{dragging=true;onceCompleted=false;anim.setFrame(parseInt(scrub.value,10)||0);update();});\n";
    stream << "scrub.addEventListener('change',()=>{dragging=false;update();});\n";
    stream << "anim.addEventListener('load',()=>{applyMode();anim.setSpeed(parseFloat(speed.value)||1);anim.play();update();});\n";
    stream << "anim.addEventListener('frame',update);\n";
    stream << "anim.addEventListener('complete',()=>{if(mode.value==='once'){onceCompleted=true;}update();});\n";
    stream << "anim.addEventListener('loadError',(event)=>showError(event.error||'Could not load dotLottie animation.'));\n";
    stream << "anim.addEventListener('renderError',(event)=>showError(event.error||'Could not render dotLottie animation.'));\n";
    stream << "applyBackground();update();\n";
    stream << "}catch(error){showError(error.message || String(error));}\n";
    stream << "</script>\n";
    } else {
    stream << "<script src=\"https://cdnjs.cloudflare.com/ajax/libs/lottie-web/5.12.2/lottie.min.js\"></script>\n";
    stream << "<script>\n";
    stream << "const encoded='" << QString::fromLatin1(encodedAnimation) << "';\n";
    stream << "const assetsBase='" << assetsBase << "';\n";
    stream << "const showError=(message)=>{const el=document.getElementById('error');el.textContent=message;el.style.display='block';document.getElementById('preview').style.display='none';document.getElementById('controls').style.display='none';document.getElementById('progressWrap').style.display='none';};\n";
    stream << "try{if(!window.lottie){throw new Error('Could not load lottie-web. Check your network connection.');}\n";
    stream << "const baseAnimationData=JSON.parse(atob(encoded));\n";
    // TODO: Remove this preview-only workaround if lottie-web's SVG renderer
    // handles static layer transforms consistently. Some static exports render
    // correctly in canvas and other players, but lottie-web SVG can place layers
    // at the wrong origin unless the animation contains at least one tiny
    // animated transform. Keep the exported JSON clean and patch only the
    // temporary in-memory preview data when the user selects the SVG renderer.
    stream << "const appendSvgRendererFix=(data)=>{if(!Array.isArray(data.layers)||data.layers.some((layer)=>layer&&layer.nm==='SVG Renderer Fix')){return;}const nextId=data.layers.reduce((id,layer)=>Math.max(id,Number(layer&&layer.ind)||0),0)+1;const ip=Math.floor(data.ip||0);const last=Math.max(ip+1,Math.floor((data.op||ip+2)-1));data.layers.push({ddd:0,ind:nextId,ty:3,nm:'SVG Renderer Fix',sr:1,ip:ip,op:last+1,st:0,bm:0,ks:{o:{a:0,k:0},r:{a:0,k:0},a:{a:0,k:[0,0,0]},s:{a:0,k:[100,100,100]},p:{a:1,k:[{t:ip,s:[0,0,0],e:[0.001,0,0],i:{x:[0.833],y:[0.833]},o:{x:[0.167],y:[0.167]}},{t:last,s:[0.001,0,0]}]}}});};\n";
    stream << "const buildAnimationData=(rendererName)=>{const data=JSON.parse(JSON.stringify(baseAnimationData));if(Array.isArray(data.assets)){data.assets.forEach((asset)=>{if(asset&&asset.e===0&&asset.p){const joined=(asset.u||'')+asset.p;asset.p=new URL(joined,assetsBase).href;asset.u='';}});}if(rendererName==='svg'){appendSvgRendererFix(data);}return data;};\n";
    stream << "const container=document.getElementById('preview');\n";
    stream << "const scrub=document.getElementById('scrub');\n";
    stream << "const frame=document.getElementById('frame');\n";
    stream << "const mode=document.getElementById('mode');\n";
    stream << "const renderer=document.getElementById('renderer');\n";
    stream << "const wireframe=document.getElementById('wireframe');\n";
    stream << "const background=document.getElementById('background');\n";
    stream << "const speed=document.getElementById('speed');\n";
    stream << "let anim=null;\n";
    stream << "let dragging=false;\n";
    stream << "let direction=1;\n";
    stream << "let wireframeMode=0;\n";
    stream << "let onceCompleted=false;\n";
    stream << "let animationData=buildAnimationData(renderer.value);\n";
    stream << "const cycle=(button,options)=>{const index=options.findIndex((option)=>option[0]===button.value);const next=options[(index+1)%options.length];button.value=next[0];button.textContent=next[1];};\n";
    stream << "const rendererOptions=[['canvas','Canvas'],['svg','SVG']];\n";
    stream << "const backgroundOptions=[['white','White'],['black','Black'],['gray','Gray'],['transparent','Transparent']];\n";
    stream << "const modeOptions=[['loop','Loop'],['pingpong','Ping-pong'],['once','Once']];\n";
    stream << "const speedOptions=[['0.1','0.1x'],['0.25','0.25x'],['0.5','0.5x'],['1','1x'],['1.5','1.5x'],['2','2x'],['3','3x'],['4','4x']];\n";
    stream << "const total=()=>Math.max(1, Math.floor((anim && anim.totalFrames) || (animationData.op-animationData.ip) || 1));\n";
    stream << "const current=()=>Math.max(0, Math.min(total()-1, Math.floor((anim && anim.currentFrame) || 0)));\n";
    stream << "const update=()=>{const t=total();const c=current();scrub.max=String(t-1);if(!dragging){scrub.value=String(c);}frame.textContent=c+' / '+(t-1);};\n";
    stream << "const applyMode=()=>{if(!anim){return;}onceCompleted=false;const m=mode.value;anim.loop=m==='loop';if(m==='loop'||m==='once'){direction=1;anim.setDirection(1);}update();};\n";
    stream << "const applyBackground=()=>{const checker='repeating-conic-gradient(#b0b0b0 0% 25%,transparent 0% 50%) 50%/40px 40px';const colors={white:'#fff',black:'#000',gray:'#808080'};const value=background.value;if(value==='transparent'){document.documentElement.style.background=checker;document.body.style.background='transparent';return;}document.documentElement.style.background=colors[value]||colors.white;document.body.style.background=colors[value]||colors.white;};\n";
    stream << "const applyWireframe=()=>{const svgActive=renderer.value==='svg';container.classList.toggle('wireframe',svgActive&&wireframeMode===1);container.classList.toggle('outlines',svgActive&&wireframeMode===2);wireframe.classList.toggle('active',svgActive&&wireframeMode!==0);wireframe.textContent=wireframeMode===2?'Outlines':'Wireframe';wireframe.style.display=svgActive?'':'none';};\n";
    stream << "const onComplete=()=>{if(mode.value==='pingpong'){direction*=-1;anim.setDirection(direction);anim.goToAndPlay(direction>0?0:total()-1,true);}else if(mode.value==='once'){onceCompleted=true;}update();};\n";
    stream << "const createAnimation=(startFrame,autoplay)=>{if(anim){anim.destroy();}animationData=buildAnimationData(renderer.value);container.innerHTML='';anim=lottie.loadAnimation({container,renderer:renderer.value,loop:true,autoplay:false,animationData,rendererSettings:{imagePreserveAspectRatio:'xMidYMid meet'}});anim.setSpeed(parseFloat(speed.value)||1);const renderFrame=()=>{const rel=current();anim.goToAndStop(rel,true);if(!anim.isPaused){anim.play();}update();};anim.addEventListener('DOMLoaded',()=>{applyMode();anim.goToAndStop(startFrame,true);if(autoplay&&mode.value!=='once'){anim.play();}update();});anim.addEventListener('loaded_images',renderFrame);anim.addEventListener('enterFrame',update);anim.addEventListener('complete',onComplete);};\n";
    stream << "const togglePlayback=()=>{if(!anim){return;}if(anim.isPaused&&mode.value==='once'&&onceCompleted){onceCompleted=false;direction=1;anim.setDirection(1);anim.goToAndPlay(0,true);}else if(anim.isPaused){anim.play();}else{anim.pause();}update();};\n";
    stream << "container.addEventListener('click',togglePlayback);\n";
    stream << "mode.addEventListener('click',()=>{cycle(mode,modeOptions);applyMode();});\n";
    stream << "background.addEventListener('click',()=>{cycle(background,backgroundOptions);background.textContent='Background';applyBackground();});\n";
    stream << "wireframe.addEventListener('click',()=>{wireframeMode=(wireframeMode+1)%3;applyWireframe();});\n";
    stream << "renderer.addEventListener('click',()=>{const rel=current();const paused=!anim||anim.isPaused;cycle(renderer,rendererOptions);createAnimation(rel,!paused);applyWireframe();});\n";
    stream << "speed.addEventListener('click',()=>{cycle(speed,speedOptions);anim.setSpeed(parseFloat(speed.value)||1);});\n";
    stream << "scrub.addEventListener('input',()=>{dragging=true;onceCompleted=false;anim.goToAndStop(parseInt(scrub.value,10)||0,true);update();});\n";
    stream << "scrub.addEventListener('change',()=>{dragging=false;update();});\n";
    stream << "applyBackground();\n";
    stream << "applyWireframe();\n";
    stream << "createAnimation(0,true);\n";
    stream << "update();\n";
    stream << "}catch(error){showError(error.message || String(error));}\n";
    stream << "</script>\n";
    }
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
