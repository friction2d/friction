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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsView>
#include <QComboBox>
#include <QPushButton>
#include <QActionGroup>
#include <QStackedWidget>
#include <QToolBox>
#include <QToolButton>
#include <QLabel>
#include <QToolBar>
#include <QComboBox>
#include <QTimer>

#include "widgets/fontswidget.h"
#include "Private/Tasks/taskscheduler.h"
#include "Private/document.h"
#include "Sound/audiohandler.h"
#include "actions.h"
#include "layouthandler.h"
#include "renderhandler.h"
#include "fileshandler.h"
#include "ekeyfilter.h"
#include "GUI/RenderWidgets/renderwidget.h"
#include "widgets/colortoolbar.h"
#include "widgets/qdoubleslider.h"
#include "widgets/canvastoolbar.h"
#include "window.h"
#include "widgets/aboutwidget.h"
#include "widgets/uilayout.h"
#include "widgets/toolbar.h"

#ifdef PROJECT_OFFICIAL
#include "updater/updatechecker.h"
#endif

class VideoEncoder;
class RenderWidget;
class ActionButton;
class BoxesList;
class UpdateScheduler;
class AnimationDockWidget;
class ColorSettingsWidget;
class FillStrokeSettingsWidget;
class TimelineDockWidget;
class CanvasWindow;
class MemoryHandler;

class ObjectSettingsWidget;
class BoxScrollWidget;
class ScrollArea;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(Document &document,
               Actions &actions,
               AudioHandler &audioHandler,
               RenderHandler &renderHandler,
               const QString &openProject = QString(),
               QWidget * const parent = nullptr);
    ~MainWindow();

    static MainWindow *sGetInstance();

    AnimationDockWidget *getAnimationDockWidget();
    BoxScrollWidget *getObjectSettingsList();

    FillStrokeSettingsWidget *getFillStrokeSettings();
    void saveToFile(const QString &path,
                    const bool addRecent = true);
    void saveToFileXEV(const QString& path);
    void loadEVFile(const QString &path);
    void loadXevFile(const QString &path);
    void clearAll();
    void updateTitle();
    void setFileChangedSinceSaving(const bool changed);
    void disableEventFilter();
    void enableEventFilter();

    void scheduleBoxesListRepaint();
    void disable();
    void enable();

    void updateCanvasModeButtonsChecked();

    void setCurrentBox(BoundingBox *box);
    void setCurrentBoxFocus(BoundingBox *box);

    void setResolution(qreal percent);

    void updateSettingsForCurrentCanvas(Canvas * const scene);

    void addCanvas(Canvas * const newCanvas);

    SimpleBrushWrapper *getCurrentBrush() const;

    void setResolutionValue(const qreal value);

    void addCanvasToRenderQue();

    const QStringList& getRecentFiles() const {
        return mRecentFiles;
    }
    stdsptr<void> lock();

    bool processKeyEvent(QKeyEvent *event);

#ifdef Q_OS_MAC
    bool processBoxesListKeyEvent(QKeyEvent *event);
#endif

    void installNumericFilter(QObject* const object) {
        object->installEventFilter(mNumericFilter);
    }

    void installLineFilter(QObject* const object) {
        object->installEventFilter(mLineFilter);
    }

    void readSettings(const QString &openProject = QString());
    void writeSettings();

public:
    void newFile();
    bool askForSaving();
    void openFile();
    void openFile(const QString& openPath);
    void saveFile();
    void saveFile(const QString& path,
                  const bool setPath = true);
    void saveFileAs(const bool setPath = true);
    void saveBackup();
    const QString checkBeforeExportSVG();
    void exportSVG(const bool &preview = false);
    void updateLastOpenDir(const QString &path);
    void updateLastSaveDir(const QString &path);
    const QString getLastOpenDir();
    const QString getLastSaveDir();
    bool closeProject();
    void linkFile();
    void importImageSequence();
    void importFile();
    void revert();
    void updateAutoSaveBackupState();
    void openRendererWindow();
    void cmdAddAction(QAction *act);
    LayoutHandler* getLayoutHandler();
    TimelineDockWidget* getTimeLineWidget();
    void focusFontWidget(const bool focus = true);
    void focusColorWidget();
    void openMarkerEditor();
    void openExpressionDialog(QrealAnimator* const target);
    void openApplyExpressionDialog(QrealAnimator* const target);

protected:
    void lockFinished();
    void resizeEvent(QResizeEvent *e);
    void showEvent(QShowEvent *e);

private:
    bool mShutdown;
    QWidget *mWelcomeDialog;
    QStackedWidget *mStackWidget;

    QTabWidget *mTabProperties;

    void openWelcomeDialog();
    void closeWelcomeDialog();

    eKeyFilter* mNumericFilter = eKeyFilter::sCreateNumberFilter(this);
    eKeyFilter* mLineFilter = eKeyFilter::sCreateLineFilter(this);

    friend class Lock;
    class Lock : public StdSelfRef {
        e_OBJECT
    protected:
        Lock(MainWindow * const window) : mWindow(window) {}
    public:
        ~Lock() { mWindow->lockFinished(); }
    private:
        MainWindow * const mWindow;
    };
    stdptr<Lock> mLock;
    static MainWindow *sInstance;

    void updateRecentMenu();

    void addRecentFile(const QString &recent);
    void readRecentFiles();
    void writeRecentFiles();

    void updateAudioDevices();

    QStringList mRecentFiles;

    void handleNewVideoClip(const VideoBox::VideoSpecs &specs);

    void handleCurrentPixelColor(const QColor &color);

    TimelineDockWidget *mTimeline;
    RenderWidget *mRenderWidget;

    Friction::Ui::ToolBar *mToolbar;

    QActionGroup *mToolBoxGroupMain;
    QActionGroup *mToolBoxGroupNodes;

    Friction::Ui::ToolBar *mToolBoxMain;

    UILayout *mUI;

    QAction *mSaveAct;
    QAction *mSaveAsAct;
    QAction *mSaveBackAct;
    QAction *mPreviewSVGAct;
    QAction *mExportSVGAct;
    QAction *mRenderVideoAct;
    QAction *mCloseProjectAct;
    QAction *mLinkedAct;
    QAction *mImportAct;
    QAction *mImportSeqAct;
    QAction *mRevertAct;
    QAction *mSelectAllAct;
    QAction *mInvertSelAct;
    QAction *mClearSelAct;
    QAction *mAddKeyAct;

    QAction *mActionConnectPointsAct;
    QAction *mActionDisconnectPointsAct;
    QAction *mActionMergePointsAct;
    QAction* mActionNewNodeAct;

    QAction *mActionSymmetricPointCtrlsAct;
    QAction *mActionSmoothPointCtrlsAct;
    QAction *mActionCornerPointCtrlsAct;

    QAction *mActionLineAct;
    QAction *mActionCurveAct;

    QAction *mResetZoomAction;
    QAction *mZoomInAction;
    QAction *mZoomOutAction;
    QAction *mFitViewAction;

    QAction *mNoneQuality;
    QAction *mLowQuality;
    QAction *mMediumQuality;
    QAction *mHighQuality;
    QAction *mDynamicQuality;

    QAction *mClipViewToCanvas;
    QAction *mRasterEffectsVisible;
    QAction *mPathEffectsVisible;

    QAction *mAddToQueAct;
    QAction *mViewFullScreenAct;

    QAction *mLocalPivotAct;

    QToolButton *mNodeVisibility;
    QAction *mNodeVisibilityAct;

    Friction::Ui::FontsWidget *mFontWidget;
    QAction* mFontWidgetAct;

    QAction *mDrawPathAuto;
    QDoubleSlider *mDrawPathSmooth;
    QDoubleSlider *mDrawPathMaxError;

    QAction *mToolBoxDrawActLabel1;
    QAction *mToolBoxDrawActLabel2;
    QAction *mToolBoxDrawActIcon1;
    QAction *mToolBoxDrawActIcon2;
    QAction *mToolBoxDrawActMaxError;
    QAction *mToolBoxDrawActSmooth;
    QAction *mToolBoxDrawActSep;

    QMenuBar *mMenuBar;
    QMenu *mFileMenu;
    QMenu *mRecentMenu;
    QMenu *mSelectSameMenu;
    QMenu *mEditMenu;
    QMenu *mObjectMenu;
    QMenu *mPathMenu;
    QMenu *mEffectsMenu;
    QMenu *mSceneMenu;
    QMenu *mViewMenu;
    QMenu *mPanelsMenu;
    QMenu *mRenderMenu;

    FilesHandler mFilesHandler;
    Document& mDocument;
    Actions& mActions;
    AudioHandler& mAudioHandler;
    RenderHandler& mRenderHandler;

    LayoutHandler *mLayoutHandler = nullptr;

    FillStrokeSettingsWidget *mFillStrokeSettings;

    bool mChangedSinceSaving = false;
    bool mEventFilterDisabled = true;
    bool isEnabled();
    QWidget *mGrayOutWidget = nullptr;
    bool mDisplayedFillStrokeSettingsUpdateNeeded = false;

    BoxScrollWidget *mObjectSettingsWidget = nullptr;
    ScrollArea *mObjectSettingsScrollArea;

    void setupToolBar();
    void setupMenuBar();
    void setupMenuScene();
    void setupMenuEffects();
    void setupMenuExtras();
    void setupPropertiesActions();

    BoundingBox* getCurrentBox();

    void addRasterEffect(const qsptr<RasterEffect> &effect);
    void addBlendEffect(const qsptr<BlendEffect> &effect);
    void addTransformEffect(const qsptr<TransformEffect> &effect);
    void addPathEffect(const qsptr<PathEffect> &effect);
    void addFillPathEffect(const qsptr<PathEffect> &effect);
    void addOutlineBasePathEffect(const qsptr<PathEffect> &effect);
    void addOutlinePathEffect(const qsptr<PathEffect> &effect);

    QList<SceneBoundGradient*> mLoadedGradientsList;

    int mStackIndexScene;
    int mStackIndexWelcome;

    int mTabColorIndex;
    int mTabTextIndex;
    int mTabPropertiesIndex;
    int mTabAssetsIndex;
    int mTabQueueIndex;

    Friction::Ui::ColorToolBar *mColorToolBar;
    Friction::Ui::CanvasToolBar *mCanvasToolBar;

    void setupToolBox();
    void setupToolBoxMain();
    void setupToolBoxNodes();
    void setupToolBoxDraw();

    void setEnableToolBoxNodes(const bool &enable);
    void setEnableToolBoxDraw(const bool &enable);

    bool mBackupOnSave;
    bool mAutoSave;
    int mAutoSaveTimeout;

    QTimer *mAutoSaveTimer;
    void checkAutoSaveTimer();

    AboutWidget *mAboutWidget;
    Window *mAboutWindow;
    void openAboutWindow();

    QAction *mViewTimelineAct;
    Window *mTimelineWindow;
    QAction *mTimelineWindowAct;
    void openTimelineWindow();
    void closedTimelineWindow();

    QAction *mViewFillStrokeAct;

    Window *mRenderWindow;
    QAction *mRenderWindowAct;
    void openRenderQueueWindow();
    void closedRenderQueueWindow();

    void initRenderPresets(const bool reinstall = false);
    void askInstallRenderPresets();

    QLabel *mColorPickLabel;

    QAction *mToolBarMainAct;
    QAction *mToolBarColorAct;

#ifdef PROJECT_OFFICIAL
    Friction::Ui::UpdateChecker *mUpdate;
#endif

protected:
    void keyPressEvent(QKeyEvent *event);
    bool eventFilter(QObject *obj, QEvent *e);
    void closeEvent(QCloseEvent *e);
};

#endif // MAINWINDOW_H
