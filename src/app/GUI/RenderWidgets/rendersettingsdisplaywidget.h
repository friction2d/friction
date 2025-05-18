#ifndef RENDERSETTINGSDISPLAYWIDGET_H
#define RENDERSETTINGSDISPLAYWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QGroupBox>
#include "renderinstancesettings.h"

class RenderSettingsDisplayWidget : public QWidget {
public:
    RenderSettingsDisplayWidget(QWidget* const parent);

    void setRenderSettings(const Scene* const scene,
                           const RenderSettings &settings);
private:
    QVBoxLayout *mMainLayout;
    QLabel *mSceneLabel;
    QLabel *mFrameRangeLabel;
    QLabel *mResolutionLabel;
    QLabel *mFpsLabel;
};

#endif // RENDERSETTINGSDISPLAYWIDGET_H
