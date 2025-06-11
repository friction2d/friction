#ifndef RENDERSETTINGSDIALOG_H
#define RENDERSETTINGSDIALOG_H
#include <QDialog>

#include "renderinstancesettings.h"

#include <QLabel>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QComboBox>
#include <QPushButton>

class RenderSettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit RenderSettingsDialog(const RenderInstanceSettings &settings,
                                  QWidget *parent = nullptr);

    RenderSettings getSettings() const;
    Scene* getCurrentScene() const
    { return const_cast<Scene*>(mCurrentScene); }
private:
    void addSeparator();

    void connectDims();
    void disconnectDims();

    void updateValuesFromRes();
    void updateValuesFromWidth();
    void updateValuesFromHeight();

    void restoreInitialSettings();

    const Scene* const mInitialScene;
    const Scene* mCurrentScene = nullptr;
    const RenderSettings mInitialSettings;

    QLabel* mSceneLabel = nullptr;
    QComboBox* mSceneCombo = nullptr;

    QLabel *mResolutionLabel = nullptr;
    QDoubleSpinBox *mResolutionSpin = nullptr;

    QLabel *mWidthLabel = nullptr;
    QSpinBox *mWidthSpin = nullptr;

    QLabel *mHeightLabel = nullptr;
    QSpinBox *mHeightSpin = nullptr;

    QLabel *mFrameRangeLabel = nullptr;
    QSpinBox *mMinFrameSpin = nullptr;
    QSpinBox *mMaxFrameSpin = nullptr;
    QPushButton *mFrameRangeButton;

//    QLabel *mFpsLabel = nullptr;
//    QDoubleSpinBox *mFpsSpin = nullptr;

    QHBoxLayout *mButtonsLayout = nullptr;
    QPushButton *mOkButton = nullptr;
    QPushButton *mCancelButton = nullptr;
    QPushButton *mResetButton = nullptr;
signals:

public slots:
};

#endif // RENDERSETTINGSDIALOG_H
