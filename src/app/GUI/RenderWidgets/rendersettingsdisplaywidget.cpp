#include "rendersettingsdisplaywidget.h"
#include "Private/scene.h"
#include "GUI/global.h"

RenderSettingsDisplayWidget::RenderSettingsDisplayWidget(QWidget * const parent) :
    QWidget(parent) {
    mMainLayout = new QVBoxLayout(this);
    setLayout(mMainLayout);

    mSceneLabel = new QLabel("<b>Scene:</b><br>");
    mFrameRangeLabel = new QLabel("<b>Frame range:</b><br>");
    mResolutionLabel = new QLabel("<b>Resolution:</b><br>");
    mFpsLabel = new QLabel("<b>Fps:</b>");

    mSceneLabel->setWordWrap(true);
    mFrameRangeLabel->setWordWrap(true);
    mResolutionLabel->setWordWrap(true);
    mFpsLabel->setWordWrap(true);

    /*eSizesUI::widget.add(this, [this](const int size) {
        mSceneLabel->setFixedHeight(size);
        mFrameRangeLabel->setFixedHeight(size);
        mResolutionLabel->setFixedHeight(size);
        mFpsLabel->setFixedHeight(size);
    });*/

    mMainLayout->addWidget(mSceneLabel);
    mMainLayout->addWidget(mFrameRangeLabel);
    mMainLayout->addWidget(mResolutionLabel);
    mMainLayout->addWidget(mFpsLabel);

    //mMainLayout->setSpacing(0);
    //mMainLayout->setAlignment(Qt::AlignTop);
}

void RenderSettingsDisplayWidget::setRenderSettings(
        const Scene * const scene,
        const RenderSettings &settings) {
    mSceneLabel->setText("<b>Scene:</b><br>" +
                         (scene ? scene->name() : "-none-"));
    mFrameRangeLabel->setText(QString("<b>Frame range:</b><br>%1 - %2").
                              arg(settings.fMinFrame).arg(settings.fMaxFrame));
    mResolutionLabel->setText(QString("<b>Resolution:</b><br>%1% - %2 x %3").
                              arg(settings.fResolution*100, 0, 'f', 2).
                              arg(settings.fVideoWidth).
                              arg(settings.fVideoHeight));
    mFpsLabel->setText(QString("<b>Fps:</b> %1").arg(settings.fFps, 0, 'f', 2));
}
