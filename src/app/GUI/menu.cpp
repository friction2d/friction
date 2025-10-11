/*
#
# Friction - https://friction.graphics
#
# Copyright (c) Ole-André Rodlie and contributors
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3.
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

#include "mainwindow.h"

#include "GUI/Settings/settingsdialog.h"
#include "GUI/timelinedockwidget.h"
#include "dialogs/commandpalette.h"
#include "memoryhandler.h"
#include "misc/noshortcutaction.h"
#include "dialogs/scenesettingsdialog.h"

#include <QDesktopServices>
#include <QClipboard>
#include <QStatusBar>

using namespace Friction;

void MainWindow::setupMenuBar()
{
    mMenuBar = new QMenuBar(nullptr);
    connectAppFont(mMenuBar);

    mFileMenu = mMenuBar->addMenu(tr("File", "MenuBar"));

    const auto newAct = mFileMenu->addAction(QIcon::fromTheme("file_blank"),
                                             tr("New", "MenuBar_File"),
                                             this, &MainWindow::newFile,
                                             Qt::CTRL + Qt::Key_N);
    newAct->setData(tr("New Project"));
    newAct->setObjectName("NewProjectAct");

    cmdAddAction(newAct);

    const auto openAct = mFileMenu->addAction(QIcon::fromTheme("file_folder"),
                                              tr("Open", "MenuBar_File"),
                                              this, qOverload<>(&MainWindow::openFile),
                                              Qt::CTRL + Qt::Key_O);
    openAct->setData(tr("Open Project"));
    openAct->setObjectName("OpenProjectAct");

    cmdAddAction(openAct);
    mRecentMenu = mFileMenu->addMenu(QIcon::fromTheme("file_folder"),
                                     tr("Open Recent", "MenuBar_File"));

    mLinkedAct = mFileMenu->addAction(QIcon::fromTheme("linked"),
                                      tr("Link"),
                                      this, &MainWindow::linkFile,
                                      Qt::CTRL + Qt::Key_L);
    mLinkedAct->setEnabled(false);
    mLinkedAct->setData(tr("Link File"));
    mLinkedAct->setObjectName("LinkFileAct");

    cmdAddAction(mLinkedAct);

    mImportAct = mFileMenu->addAction(QIcon::fromTheme("file_import"),
                                      tr("Import", "MenuBar_File"),
                                      this, qOverload<>(&MainWindow::importFile),
                                      Qt::CTRL + Qt::Key_I);
    mImportAct->setEnabled(false);
    mImportAct->setObjectName("ImportFileAct");
    cmdAddAction(mImportAct);

    mImportSeqAct = mFileMenu->addAction(QIcon::fromTheme("renderlayers"),
                                         tr("Import Image Sequence", "MenuBar_File"),
                                         this, &MainWindow::importImageSequence);
    mImportSeqAct->setEnabled(false);
    cmdAddAction(mImportSeqAct);

    mRevertAct = mFileMenu->addAction(QIcon::fromTheme("loop_back"),
                                      tr("Revert", "MenuBar_File"),
                                      this, &MainWindow::revert);
    mRevertAct->setEnabled(false);
    mRevertAct->setData(tr("Revert Project"));
    cmdAddAction(mRevertAct);

    mFileMenu->addSeparator();

    mSaveAct = mFileMenu->addAction(QIcon::fromTheme("disk_drive"),
                                    tr("Save", "MenuBar_File"),
                                    this, qOverload<>(&MainWindow::saveFile),
                                    Qt::CTRL + Qt::Key_S);
    mSaveAct->setEnabled(false);
    mSaveAct->setData(tr("Save Project"));
    mSaveAct->setObjectName("SaveProjectAct");
    cmdAddAction(mSaveAct);

    mSaveAsAct = mFileMenu->addAction(QIcon::fromTheme("disk_drive"),
                                      tr("Save As", "MenuBar_File"),
                                      this, [this]() { saveFileAs(); },
                                      Qt::CTRL + Qt::SHIFT + Qt::Key_S);
    mSaveAsAct->setEnabled(false);
    mSaveAsAct->setData(tr("Save Project As ..."));
    cmdAddAction(mSaveAsAct);

    mSaveBackAct = mFileMenu->addAction(QIcon::fromTheme("disk_drive"),
                                        tr("Save Backup", "MenuBar_File"),
                                        this, &MainWindow::saveBackup);
    mSaveBackAct->setEnabled(false);
    mSaveBackAct->setData(tr("Save Project Backup"));
    cmdAddAction(mSaveBackAct);

    mPreviewSVGAct = mFileMenu->addAction(QIcon::fromTheme("seq_preview"),
                                          tr("Preview SVG", "MenuBar_File"),
                                          this,[this]{ exportSVG(true); },
                                          QKeySequence(AppSupport::getSettings("shortcuts",
                                                                               "previewSVG",
                                                                               "Ctrl+F12").toString()));
    mPreviewSVGAct->setEnabled(false);
    mPreviewSVGAct->setToolTip(tr("Preview SVG Animation in Web Browser"));
    mPreviewSVGAct->setData(mPreviewSVGAct->toolTip());
    mPreviewSVGAct->setObjectName("PreviewSVGAct");
    cmdAddAction(mPreviewSVGAct);

    mExportSVGAct = mFileMenu->addAction(QIcon::fromTheme("output"),
                                         tr("Export SVG", "MenuBar_File"),
                                         this, &MainWindow::exportSVG,
                                         QKeySequence(AppSupport::getSettings("shortcuts",
                                                                              "exportSVG",
                                                                              "Shift+F12").toString()));
    mExportSVGAct->setEnabled(false);
    mExportSVGAct->setToolTip(tr("Export SVG Animation for the Web"));
    mExportSVGAct->setData(mExportSVGAct->toolTip());
    mExportSVGAct->setObjectName("ExportSVGAct");
    cmdAddAction(mExportSVGAct);

    mFileMenu->addSeparator();
    mCloseProjectAct = mFileMenu->addAction(QIcon::fromTheme("dialog-cancel"),
                                            tr("Close", "MenuBar_File"),
                                            this, &MainWindow::closeProject,
                                            QKeySequence(tr("Ctrl+W")));
    mCloseProjectAct->setEnabled(false);
    mCloseProjectAct->setData(tr("Close Project"));
    cmdAddAction(mCloseProjectAct);

    const auto prefsAct = mFileMenu->addAction(QIcon::fromTheme("preferences"),
                                               tr("Preferences", "MenuBar_Edit"), [this]() {
                                                   const auto settDial = new SettingsDialog(this);
                                                   settDial->setAttribute(Qt::WA_DeleteOnClose);
                                                   settDial->show();
                                               }, QKeySequence(tr("Ctrl+P")));
    cmdAddAction(prefsAct);

    const auto quitAppAct = mFileMenu->addAction(QIcon::fromTheme("quit"),
                                                 tr("Exit", "MenuBar_File"),
                                                 this, &MainWindow::close,
                                                 QKeySequence(tr("Ctrl+Q")));
    quitAppAct->setData(tr("Quit Friction"));
    cmdAddAction(quitAppAct);

    mEditMenu = mMenuBar->addMenu(tr("Edit", "MenuBar"));

    const auto undoQAct = mEditMenu->addAction(QIcon::fromTheme("loop_back"),
                                               tr("Undo", "MenuBar_Edit"));
    undoQAct->setShortcut(Qt::CTRL + Qt::Key_Z);
    mActions.undoAction->connect(undoQAct);
    cmdAddAction(undoQAct);

    const auto redoQAct = mEditMenu->addAction(QIcon::fromTheme("loop_forwards"),
                                               tr("Redo", "MenuBar_Edit"));
    redoQAct->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_Z);
    mActions.redoAction->connect(redoQAct);
    cmdAddAction(redoQAct);

    {   // add undo/redo to tool controls
        const auto toolbar = mToolBox->getToolBar(Ui::ToolBox::Controls);
        if (toolbar) {
            toolbar->insertAction(toolbar->actions().at(0), redoQAct);
            toolbar->insertAction(toolbar->actions().at(0), undoQAct);
            Friction::Core::Theme::setToolbarButtonStyle("ToolBoxButton", toolbar, redoQAct);
            Friction::Core::Theme::setToolbarButtonStyle("ToolBoxButton", toolbar, undoQAct);
        }
    }

    mEditMenu->addSeparator();

    {
        const auto qAct = new NoShortcutAction(tr("Copy", "MenuBar_Edit"));
        qAct->setIcon(QIcon::fromTheme("copy"));
        mEditMenu->addAction(qAct);
#ifndef Q_OS_MAC
        qAct->setShortcut(Qt::CTRL + Qt::Key_C);
#endif
        mActions.copyAction->connect(qAct);
        cmdAddAction(qAct);
    }

    {
        const auto qAct = new NoShortcutAction(tr("Cut", "MenuBar_Edit"));
        qAct->setIcon(QIcon::fromTheme("cut"));
        mEditMenu->addAction(qAct);
#ifndef Q_OS_MAC
        qAct->setShortcut(Qt::CTRL + Qt::Key_X);
#endif
        mActions.cutAction->connect(qAct);
        cmdAddAction(qAct);
    }

    {
        const auto qAct = new NoShortcutAction(tr("Paste", "MenuBar_Edit"));
        qAct->setIcon(QIcon::fromTheme("paste"));
        mEditMenu->addAction(qAct);
#ifndef Q_OS_MAC
        qAct->setShortcut(Qt::CTRL + Qt::Key_V);
#endif
        mActions.pasteAction->connect(qAct);
        cmdAddAction(qAct);
    }

    { // import (paste) SVG from clipboard
        mEditMenu->addAction(QIcon::fromTheme("paste"),
                             tr("Paste from Clipboard"),
                             [this]() {
                                 const auto clipboard = QGuiApplication::clipboard();
                                 if (clipboard) {
                                     const auto mime = clipboard->mimeData();
                                     qDebug() << mime->formats() << mime->text();
                                     if (mime->hasText() && mime->text().contains("<svg")) {
                                         const QString svg = mime->text();
                                         try { mActions.importClipboard(svg); }
                                         catch (const std::exception& e) { gPrintExceptionCritical(e); }
                                     }
                                 }
                             }, QKeySequence(tr("Ctrl+Shift+V")));
    }

    {
        const auto qAct = new NoShortcutAction(tr("Duplicate", "MenuBar_Edit"));
        mEditMenu->addAction(qAct);
        qAct->setIcon(QIcon::fromTheme("duplicate"));
#ifndef Q_OS_MAC
        qAct->setShortcut(Qt::CTRL + Qt::Key_D);
#endif
        mActions.duplicateAction->connect(qAct);
        cmdAddAction(qAct);
    }

    {
        const auto qAct = new NoShortcutAction(tr("Delete", "MenuBar_Edit"));
        qAct->setIcon(QIcon::fromTheme("trash"));
        mEditMenu->addAction(qAct);
        qAct->setShortcut(Qt::Key_Delete);
        mActions.deleteAction->connect(qAct);
        cmdAddAction(qAct);
    }

    mEditMenu->addSeparator();

    {
        mSelectAllAct = new NoShortcutAction(tr("Select All",
                                                "MenuBar_Edit"),
                                             &mActions,
                                             &Actions::selectAllAction,
                                             Qt::Key_A,
                                             mEditMenu);
        mSelectAllAct->setIcon(QIcon::fromTheme("select"));
        mSelectAllAct->setEnabled(false);
        mEditMenu->addAction(mSelectAllAct);
        cmdAddAction(mSelectAllAct);
    }

    {
        mInvertSelAct = new NoShortcutAction(tr("Invert Selection",
                                                "MenuBar_Edit"),
                                             &mActions,
                                             &Actions::invertSelectionAction,
                                             Qt::SHIFT + Qt::Key_A,
                                             mEditMenu);
        mInvertSelAct->setIcon(QIcon::fromTheme("select"));
        mInvertSelAct->setEnabled(false);
        mEditMenu->addAction(mInvertSelAct);
        cmdAddAction(mInvertSelAct);
    }

    {
        mClearSelAct = new NoShortcutAction(tr("Clear Selection",
                                               "MenuBar_Edit"),
                                            &mActions,
                                            &Actions::clearSelectionAction,
                                            Qt::ALT + Qt::Key_A,
                                            mEditMenu);
        mClearSelAct->setIcon(QIcon::fromTheme("select"));
        mClearSelAct->setEnabled(false);
        mEditMenu->addAction(mClearSelAct);
        cmdAddAction(mClearSelAct);
    }

    mEditMenu->addSeparator();

    mAddKeyAct = mEditMenu->addAction(QIcon::fromTheme("plus"),
                                      tr("Add Key(s)"), [this]() {
                                          const auto scene = *mDocument.fActiveScene;
                                          if (!scene) { return; }
                                          scene->addKeySelectedProperties();
                                      }, QKeySequence(tr("Insert")));
    mAddKeyAct->setEnabled(false);
    cmdAddAction(mAddKeyAct);

    mEditMenu->addSeparator();

    const auto clearCacheAct = mEditMenu->addAction(QIcon::fromTheme("trash"),
                                                    tr("Clear Cache", "MenuBar_Edit"), [this]() {
                                                        const auto m = MemoryHandler::sInstance;
                                                        m->clearMemory();
                                                        mTimeline->update();
                                                    }, QKeySequence(tr("Ctrl+R")));
    cmdAddAction(clearCacheAct);

    const auto clearRecentAct = mEditMenu->addAction(QIcon::fromTheme("trash"),
                                                     tr("Clear Recent Files"), [this]() {
                                                         mRecentFiles.clear();
                                                         writeRecentFiles();
                                                         updateRecentMenu();
                                                     });
    cmdAddAction(clearRecentAct);

    mViewMenu = mMenuBar->addMenu(tr("View", "MenuBar"));

    mObjectMenu = mMenuBar->addMenu(tr("Object", "MenuBar"));

    mObjectMenu->addSeparator();

    const auto raiseQAct = mObjectMenu->addAction(
        tr("Raise", "MenuBar_Object"));
    raiseQAct->setIcon(QIcon::fromTheme("go-up"));
    raiseQAct->setShortcut(Qt::Key_PageUp);
    mActions.raiseAction->connect(raiseQAct);
    raiseQAct->setData(tr("Raise Object"));
    cmdAddAction(raiseQAct);

    const auto lowerQAct = mObjectMenu->addAction(
        tr("Lower", "MenuBar_Object"));
    lowerQAct->setIcon(QIcon::fromTheme("go-down"));
    lowerQAct->setShortcut(Qt::Key_PageDown);
    mActions.lowerAction->connect(lowerQAct);
    lowerQAct->setData(tr("Lower Object"));
    cmdAddAction(lowerQAct);

    const auto rttQAct = mObjectMenu->addAction(
        tr("Raise to Top", "MenuBar_Object"));
    rttQAct->setIcon(QIcon::fromTheme("raise-top"));
    rttQAct->setShortcut(Qt::Key_Home);
    mActions.raiseToTopAction->connect(rttQAct);
    rttQAct->setData(tr("Raise Object to Top"));
    cmdAddAction(rttQAct);

    const auto ltbQAct = mObjectMenu->addAction(
        tr("Lower to Bottom", "MenuBar_Object"));
    ltbQAct->setIcon(QIcon::fromTheme("raise-bottom"));
    ltbQAct->setShortcut(Qt::Key_End);
    mActions.lowerToBottomAction->connect(ltbQAct);
    ltbQAct->setData(tr("Lower Object to Bottom"));
    cmdAddAction(ltbQAct);

    mObjectMenu->addSeparator();

    {
        const auto qAct = mObjectMenu->addAction(
            tr("Rotate 90° CW", "MenuBar_Object"));
        qAct->setIcon(QIcon::fromTheme("loop_forwards"));
        mActions.rotate90CWAction->connect(qAct);
    }

    {
        const auto qAct = mObjectMenu->addAction(
            tr("Rotate 90° CCW", "MenuBar_Object"));
        qAct->setIcon(QIcon::fromTheme("loop_back"));
        mActions.rotate90CCWAction->connect(qAct);
    }

    {
        const auto qAct = mObjectMenu->addAction(tr("Flip Horizontal", "MenuBar_Object"));
        qAct->setIcon(QIcon::fromTheme("width"));
        qAct->setShortcut(Qt::Key_H);
        mActions.flipHorizontalAction->connect(qAct);
        cmdAddAction(qAct);
    }

    {
        const auto qAct = mObjectMenu->addAction(tr("Flip Vertical", "MenuBar_Object"));
        qAct->setIcon(QIcon::fromTheme("height"));
        qAct->setShortcut(Qt::Key_V);
        mActions.flipVerticalAction->connect(qAct);
        cmdAddAction(qAct);
    }

    mObjectMenu->addSeparator();

    const auto groupQAct = mObjectMenu->addAction(
        tr("Group", "MenuBar_Object"));
    groupQAct->setIcon(QIcon::fromTheme("group"));
    groupQAct->setShortcut(Qt::CTRL + Qt::Key_G);
    mActions.groupAction->connect(groupQAct);
    groupQAct->setData(tr("Group Selected"));
    cmdAddAction(groupQAct);

    const auto ungroupQAct = mObjectMenu->addAction(
        tr("Ungroup", "MenuBar_Object"));
    ungroupQAct->setIcon(QIcon::fromTheme("group"));
    ungroupQAct->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_G);
    mActions.ungroupAction->connect(ungroupQAct);
    cmdAddAction(ungroupQAct);

    mPathMenu = mMenuBar->addMenu(tr("Path", "MenuBar"));

    const auto otpQAct = mPathMenu->addAction(
        tr("Object to Path", "MenuBar_Path"));
    mActions.objectsToPathAction->connect(otpQAct);
    cmdAddAction(otpQAct);

    const auto stpQAct = mPathMenu->addAction(
        tr("Stroke to Path", "MenuBar_Path"));
    mActions.strokeToPathAction->connect(stpQAct);
    cmdAddAction(stpQAct);

    mPathMenu->addSeparator();

    {
        const auto qAct = mPathMenu->addAction(
            tr("Union", "MenuBar_Path"));
        qAct->setIcon(QIcon::fromTheme("booleans_union"));
        qAct->setShortcut(Qt::CTRL + Qt::Key_Plus);
        mActions.pathsUnionAction->connect(qAct);
        cmdAddAction(qAct);
    }

    {
        const auto qAct = mPathMenu->addAction(
            tr("Difference", "MenuBar_Path"));
        qAct->setIcon(QIcon::fromTheme("booleans_difference"));
        qAct->setShortcut(Qt::CTRL + Qt::Key_Minus);
        mActions.pathsDifferenceAction->connect(qAct);
        cmdAddAction(qAct);
    }

    {
        const auto qAct = mPathMenu->addAction(
            tr("Intersection", "MenuBar_Path"));
        qAct->setIcon(QIcon::fromTheme("booleans_intersection"));
        qAct->setShortcut(Qt::CTRL + Qt::Key_Asterisk);
        mActions.pathsIntersectionAction->connect(qAct);
        cmdAddAction(qAct);
    }

    {
        const auto qAct = mPathMenu->addAction(
            tr("Exclusion", "MenuBar_Path"));
        qAct->setIcon(QIcon::fromTheme("booleans_exclusion"));
        qAct->setShortcut(Qt::CTRL + Qt::Key_AsciiCircum);
        mActions.pathsExclusionAction->connect(qAct);
        cmdAddAction(qAct);
    }

    {
        const auto qAct = mPathMenu->addAction(
            tr("Division", "MenuBar_Path"));
        qAct->setIcon(QIcon::fromTheme("booleans_division"));
        qAct->setShortcut(Qt::CTRL + Qt::Key_Slash);
        mActions.pathsDivisionAction->connect(qAct);
        cmdAddAction(qAct);
    }

    mPathMenu->addSeparator();

    {
        const auto qAct = mPathMenu->addAction(
            tr("Combine", "MenuBar_Path"));
        qAct->setIcon(QIcon::fromTheme("booleans_combine"));
        qAct->setShortcut(Qt::CTRL + Qt::Key_K);
        mActions.pathsCombineAction->connect(qAct);
        cmdAddAction(qAct);
    }

    {
        const auto qAct = mPathMenu->addAction(
            tr("Break Apart", "MenuBar_Path"));
        qAct->setIcon(QIcon::fromTheme("booleans_break_apart"));
        qAct->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_K);
        mActions.pathsBreakApartAction->connect(qAct);
        cmdAddAction(qAct);
    }

    setupMenuScene();

    mEffectsMenu = mMenuBar->addMenu(tr("Effects"));
    mEffectsMenu->setEnabled(false);
    setupMenuEffects();

    const auto zoomMenu = mViewMenu->addMenu(QIcon::fromTheme("zoom"), tr("Zoom","MenuBar_View"));

    mZoomInAction = zoomMenu->addAction(tr("Zoom In", "MenuBar_View_Zoom"));
    mZoomInAction->setIcon(QIcon::fromTheme("zoom_in"));
    mZoomInAction->setShortcut(QKeySequence("Ctrl+Shift++"));
    cmdAddAction(mZoomInAction);
    connect(mZoomInAction, &QAction::triggered,
            this, [](){
                const auto target = KeyFocusTarget::KFT_getCurrentTarget();
                const auto cwTarget = dynamic_cast<CanvasWindow*>(target);
                if (!cwTarget) { return; }
                cwTarget->zoomInView();
            });

    mZoomOutAction = zoomMenu->addAction(tr("Zoom Out", "MenuBar_View_Zoom"));
    mZoomOutAction->setIcon(QIcon::fromTheme("zoom_out"));
    mZoomOutAction->setShortcut(QKeySequence("Ctrl+Shift+-"));
    cmdAddAction(mZoomOutAction);
    connect(mZoomOutAction, &QAction::triggered,
            this, [](){
                const auto target = KeyFocusTarget::KFT_getCurrentTarget();
                const auto cwTarget = dynamic_cast<CanvasWindow*>(target);
                if (!cwTarget) { return; }
                cwTarget->zoomOutView();
            });

    mFitViewAction = zoomMenu->addAction(tr("Fit to Canvas", "MenuBar_View_Zoom"));
    mFitViewAction->setIcon(QIcon::fromTheme("zoom_all"));
    mFitViewAction->setShortcut(QKeySequence("Ctrl+0"));
    connect(mFitViewAction, &QAction::triggered,
            this, [](){
                const auto target = KeyFocusTarget::KFT_getCurrentTarget();
                const auto cwTarget = dynamic_cast<CanvasWindow*>(target);
                if (!cwTarget) { return; }
                cwTarget->fitCanvasToSize();
            });
    cmdAddAction(mFitViewAction);

    const auto fitViewWidth = zoomMenu->addAction(QIcon::fromTheme("zoom_all"),
                                                  tr("Fit to Canvas Width"));
    fitViewWidth->setShortcut(QKeySequence("Ctrl+9"));
    connect(fitViewWidth, &QAction::triggered,
            this, []() {
                const auto target = KeyFocusTarget::KFT_getCurrentTarget();
                const auto cwTarget = dynamic_cast<CanvasWindow*>(target);
                if (!cwTarget) { return; }
                cwTarget->fitCanvasToSize(true);
            });
    cmdAddAction(fitViewWidth);

    mResetZoomAction = zoomMenu->addAction(tr("Reset Zoom", "MenuBar_View_Zoom"));
    mResetZoomAction->setShortcut(QKeySequence("Ctrl+1"));
    connect(mResetZoomAction, &QAction::triggered,
            this, [](){
                const auto target = KeyFocusTarget::KFT_getCurrentTarget();
                const auto cwTarget = dynamic_cast<CanvasWindow*>(target);
                if (!cwTarget) { return; }
                cwTarget->resetTransformation();
            });
    cmdAddAction(mResetZoomAction);

    const auto filteringMenu = mViewMenu->addMenu(QIcon::fromTheme("user-desktop"),
                                                  tr("Filtering", "MenuBar_View"));

    mNoneQuality = filteringMenu->addAction(
        tr("None", "MenuBar_View_Filtering"), [this]() {
            eFilterSettings::sSetDisplayFilter(kNone_SkFilterQuality);
            mStackWidget->widget(mStackIndexScene)->update();

            mLowQuality->setChecked(false);
            mMediumQuality->setChecked(false);
            mHighQuality->setChecked(false);
            mDynamicQuality->setChecked(false);
        });
    mNoneQuality->setCheckable(true);
    mNoneQuality->setChecked(eFilterSettings::sDisplay() == kNone_SkFilterQuality &&
                             !eFilterSettings::sSmartDisplat());

    mLowQuality = filteringMenu->addAction(
        tr("Low", "MenuBar_View_Filtering"), [this]() {
            eFilterSettings::sSetDisplayFilter(kLow_SkFilterQuality);
            centralWidget()->update();

            mNoneQuality->setChecked(false);
            mMediumQuality->setChecked(false);
            mHighQuality->setChecked(false);
            mDynamicQuality->setChecked(false);
        });
    mLowQuality->setCheckable(true);
    mLowQuality->setChecked(eFilterSettings::sDisplay() == kLow_SkFilterQuality &&
                            !eFilterSettings::sSmartDisplat());

    mMediumQuality = filteringMenu->addAction(
        tr("Medium", "MenuBar_View_Filtering"), [this]() {
            eFilterSettings::sSetDisplayFilter(kMedium_SkFilterQuality);
            centralWidget()->update();

            mNoneQuality->setChecked(false);
            mLowQuality->setChecked(false);
            mHighQuality->setChecked(false);
            mDynamicQuality->setChecked(false);
        });
    mMediumQuality->setCheckable(true);
    mMediumQuality->setChecked(eFilterSettings::sDisplay() == kMedium_SkFilterQuality &&
                               !eFilterSettings::sSmartDisplat());

    mHighQuality = filteringMenu->addAction(
        tr("High", "MenuBar_View_Filtering"), [this]() {
            eFilterSettings::sSetDisplayFilter(kHigh_SkFilterQuality);
            centralWidget()->update();

            mNoneQuality->setChecked(false);
            mLowQuality->setChecked(false);
            mMediumQuality->setChecked(false);
            mDynamicQuality->setChecked(false);
        });
    mHighQuality->setCheckable(true);
    mHighQuality->setChecked(eFilterSettings::sDisplay() == kHigh_SkFilterQuality &&
                             !eFilterSettings::sSmartDisplat());

    mDynamicQuality = filteringMenu->addAction(
        tr("Dynamic", "MenuBar_View_Filtering"), [this]() {
            eFilterSettings::sSetSmartDisplay(true);
            centralWidget()->update();

            mLowQuality->setChecked(false);
            mMediumQuality->setChecked(false);
            mHighQuality->setChecked(false);
            mNoneQuality->setChecked(false);
        });
    mDynamicQuality->setCheckable(true);
    mDynamicQuality->setChecked(eFilterSettings::sSmartDisplat());

    const auto menuGizmo = mViewMenu->addMenu(QIcon::fromTheme("gizmos"),
                                              tr("Transform Interacts"));
    setupMenuGizmo(menuGizmo, Core::Gizmos::Interact::Position);
    setupMenuGizmo(menuGizmo, Core::Gizmos::Interact::Rotate);
    setupMenuGizmo(menuGizmo, Core::Gizmos::Interact::Scale);
    setupMenuGizmo(menuGizmo, Core::Gizmos::Interact::Shear);

    mClipViewToCanvas = mViewMenu->addAction(
        tr("Clip to Scene", "MenuBar_View"));
    mClipViewToCanvas->setCheckable(true);
    //mClipViewToCanvas->setChecked(true);
    mClipViewToCanvas->setShortcut(QKeySequence(Qt::Key_C));
    cmdAddAction(mClipViewToCanvas);
    connect(mClipViewToCanvas, &QAction::triggered,
            &mActions, &Actions::setClipToCanvas);

    mViewMenu->addSeparator();

    const auto previewCacheAct = mViewMenu->addAction(tr("Preview Cache"));
    previewCacheAct->setCheckable(true);
    previewCacheAct->setChecked(eSettings::instance().fPreviewCache);
    connect(previewCacheAct, &QAction::triggered,
            this, [this, previewCacheAct]() {
                const bool checked = previewCacheAct->isChecked();
                eSettings::sInstance->fPreviewCache = checked;
                eSettings::sInstance->saveKeyToFile("PreviewCache");
                statusBar()->showMessage(tr("%1 Preview Cache").arg(checked ?
                                                                        tr("Enabled") :
                                                                        tr("Disabled")),
                                         5000);
            });
    cmdAddAction(previewCacheAct);

    mViewMenu->addSeparator();

    mRasterEffectsVisible = mViewMenu->addAction(
        tr("Raster Effects", "MenuBar_View"));
    mRasterEffectsVisible->setCheckable(true);
    mRasterEffectsVisible->setChecked(true);
    connect(mRasterEffectsVisible, &QAction::triggered,
            &mActions, &Actions::setRasterEffectsVisible);

    mPathEffectsVisible = mViewMenu->addAction(
        tr("Path Effects", "MenuBar_View"));
    mPathEffectsVisible->setCheckable(true);
    mPathEffectsVisible->setChecked(true);
    connect(mPathEffectsVisible, &QAction::triggered,
            &mActions, &Actions::setPathEffectsVisible);

    mViewMenu->addSeparator();

    mViewFullScreenAct = mViewMenu->addAction(tr("Full Screen"));
    mViewFullScreenAct->setCheckable(true);
    mViewFullScreenAct->setShortcut(QKeySequence(AppSupport::getSettings("shortcuts",
                                                                         "fullScreen",
                                                                         "F11").toString()));
    cmdAddAction(mViewFullScreenAct);
    connect(mViewFullScreenAct, &QAction::triggered,
            this, [this](const bool checked) {
                if (checked) { showFullScreen(); }
                else { showNormal(); }
            });

    mViewMenu->addSeparator();

    mViewTimelineAct = mViewMenu->addAction(tr("View Timeline"));
    mViewTimelineAct->setCheckable(true);
    mViewTimelineAct->setChecked(true);
    mViewTimelineAct->setShortcut(QKeySequence(Qt::Key_T));
    connect(mViewTimelineAct, &QAction::triggered,
            this, [this](bool triggered) {
                if (mTimelineWindowAct->isChecked()) {
                    mViewTimelineAct->setChecked(true); // ignore if window
                } else {
                    mUI->setDockVisible(tr("Timeline"), triggered);
                }
            });

    mViewFillStrokeAct = mViewMenu->addAction(tr("View Fill and Stroke"));
    mViewFillStrokeAct->setCheckable(true);
    mViewFillStrokeAct->setChecked(true);
    mViewFillStrokeAct->setShortcut(QKeySequence(Qt::Key_F));
    connect(mViewFillStrokeAct, &QAction::triggered,
            this, [this](bool triggered) {
                mUI->setDockVisible("Fill and Stroke", triggered);
                AppSupport::setSettings("ui", "FillStrokeVisible", triggered);
            });

    mViewMenu->addSeparator();

    mTimelineWindowAct = mViewMenu->addAction(tr("Timeline in Window"));
    mTimelineWindowAct->setCheckable(true);
    connect(mTimelineWindowAct, &QAction::triggered,
            this, [this](bool triggered) {
                if (mShutdown) { return; }
                if (!triggered) { mTimelineWindow->close(); }
                else { openTimelineWindow(); }
            });

    mRenderWindowAct = mViewMenu->addAction(tr("Queue in Window"));
    mRenderWindowAct->setCheckable(true);
    connect(mRenderWindowAct, &QAction::triggered,
            this, [this](bool triggered) {
                if (mShutdown) { return; }
                if (!triggered) { mRenderWindow->close(); }
                else { openRenderQueueWindow(); }
            });

    mViewMenu->addSeparator();

    mToolBarMainAct = mViewMenu->addAction(tr("Main Toolbar"));
    mToolBarMainAct->setCheckable(true);
    connect(mToolBarMainAct, &QAction::triggered,
            this, [this](bool triggered) {
                if (!mToolbar) { return; }
                mToolbar->setVisible(triggered);
                AppSupport::setSettings("ui",
                                        "ToolBarMainVisible",
                                        triggered);
            });
    mToolBarColorAct = mViewMenu->addAction(tr("Color Toolbar"));
    mToolBarColorAct->setCheckable(true);
    connect(mToolBarColorAct, &QAction::triggered,
            this, [this](bool triggered) {
                if (!mColorToolBar) { return; }
                mColorToolBar->setVisible(triggered);
                AppSupport::setSettings("ui",
                                        "ToolBarColorVisible",
                                        triggered);
            });

    mViewMenu->addSeparator();

    setupMenuExtras();

    const auto help = mMenuBar->addMenu(tr("Help", "MenuBar"));

    const auto aboutAct = help->addAction(QIcon::fromTheme(AppSupport::getAppID()),
                                          tr("About", "MenuBar_Help"),
                                          this,
                                          &MainWindow::openAboutWindow);
    cmdAddAction(aboutAct);

    QString cmdDefKey = "Ctrl+Space";
#ifdef Q_OS_MAC
    cmdDefKey = "Alt+Space";
#endif

    help->addAction(QIcon::fromTheme("cmd"),
                    tr("Command Palette"), this, [this]() {
                        CommandPalette dialog(mDocument, this);
                        dialog.exec();
                    }, QKeySequence(AppSupport::getSettings("shortcuts",
                                                         "cmdPalette",
                                                         cmdDefKey).toString()));

    help->addSeparator();
    help->addAction(QIcon::fromTheme("user-home"),
                    tr("Website"), this, []() {
                        QDesktopServices::openUrl(QUrl(AppSupport::getAppUrl()));
                    });

    help->addAction(QIcon::fromTheme("dialog-information"),
                    tr("Documentation"), this, []() {
                        const QString docUrl = AppSupport::hasOfflineDocs() ?
                                                   AppSupport::getOfflineDocs() :
                                                   AppSupport::getOnlineDocs();
                        QDesktopServices::openUrl(QUrl(docUrl));
                    });

    help->addSeparator();
    help->addAction(QIcon::fromTheme("renderlayers"),
                    tr("Reinstall default render profiles"),
                    this, &MainWindow::askInstallRenderPresets);
    help->addAction(QIcon::fromTheme("renderlayers"),
                    tr("Reinstall default expressions presets"),
                    this, &MainWindow::askInstallExpressionsPresets);
    help->addAction(QIcon::fromTheme("color"),
                    tr("Restore default fill and stroke"),
                    this, &MainWindow::askRestoreFillStrokeDefault);
    help->addAction(QIcon::fromTheme("workspace"),
                    tr("Restore default user interface"),
                    this, &MainWindow::askRestoreDefaultUi);

    // toolbar actions
    mToolbar->addAction(newAct);
    mToolbar->addAction(openAct);
    mToolbar->addAction(mSaveAct);
    mToolbar->addAction(mImportAct);
    mToolbar->addAction(mLinkedAct);

    mRenderVideoAct = mToolbar->addAction(QIcon::fromTheme("render_animation"),
                                          tr("Render"),
                                          this, &MainWindow::openRendererWindow);
    mRenderVideoAct->setEnabled(false);
    mRenderVideoAct->setObjectName("RenderVideoAct");

    mToolbar->addAction(mPreviewSVGAct);
    mToolbar->addAction(mExportSVGAct);
    mToolbar->updateActions();

    setMenuBar(mMenuBar);
    setupPropertiesActions();

    mViewMenu->addSeparator();
    {
        const auto act = new QAction(QIcon::fromTheme("unlocked"),
                                     tr("Unlock all toolbars"), this);
        connect(act, &QAction::triggered, this, [this]() {
            mToolbar->setMovable(true);
            mToolBox->setMovable(true);
            mColorToolBar->setMovable(true);
        });
        mViewMenu->addAction(act);
    }
    {
        const auto act = new QAction(QIcon::fromTheme("locked"),
                                     tr("Lock all toolbars"), this);
        connect(act, &QAction::triggered, this, [this]() {
            mToolbar->setMovable(false);
            mToolBox->setMovable(false);
            mColorToolBar->setMovable(false);
        });
        mViewMenu->addAction(act);
    }

#ifndef Q_OS_MAC
    const auto frictionButton = new QPushButton(this);
    frictionButton->setFlat(true);
    frictionButton->setIcon(QIcon::fromTheme(AppSupport::getAppID()));
    frictionButton->setObjectName("AboutButton");
    frictionButton->setFocusPolicy(Qt::NoFocus);

    connect(frictionButton, &QPushButton::released,
            this, &MainWindow::openAboutWindow);

    mMenuBar->setCornerWidget(frictionButton,
                              Qt::TopRightCorner);
#endif
}

void MainWindow::setupMenuScene()
{
    mSceneMenu = mMenuBar->addMenu(tr("Scene", "MenuBar"));

    const auto newSceneAct = mSceneMenu->addAction(QIcon::fromTheme("file_new"),
                                                   tr("New Scene", "MenuBar_Scene"),
                                                   this, [this]() {
                                                       SceneSettingsDialog::sNewSceneDialog(mDocument, this);
                                                   });
    cmdAddAction(newSceneAct);

    const auto deleteSceneAct = mSceneMenu->addAction(QIcon::fromTheme("cancel"),
                                                      tr("Delete Scene", "MenuBar_Scene"));
    mActions.deleteSceneAction->connect(deleteSceneAct);
    cmdAddAction(deleteSceneAct);

    const auto scenePropAct = mSceneMenu->addAction(QIcon::fromTheme("sequence"),
                                                    tr("Scene Properties", "MenuBar_Scene"));
    mActions.sceneSettingsAction->connect(scenePropAct);
    cmdAddAction(scenePropAct);

    mSceneMenu->addSeparator();

    mAddToQueAct = mSceneMenu->addAction(QIcon::fromTheme("render_animation"),
                                         tr("Add to Render Queue", "MenuBar_Scene"),
                                         this, &MainWindow::addCanvasToRenderQue,
                                         QKeySequence(AppSupport::getSettings("shortcuts",
                                                                              "addToQue",
                                                                              "F12").toString()));
    mAddToQueAct->setEnabled(false);
    cmdAddAction(mAddToQueAct);

    mSceneMenu->addSeparator();
    mSceneMenu->addAction(QIcon::fromTheme("range-in"),
                          tr("Set In"), this, [this]() {
                              const auto scene = *mDocument.fActiveScene;
                              if (!scene) { return; }
                              scene->setFrameIn(true, scene->getCurrentFrame());
                          });
    mSceneMenu->addAction(QIcon::fromTheme("range-out"),
                          tr("Set Out"), this, [this]() {
                              const auto scene = *mDocument.fActiveScene;
                              if (!scene) { return; }
                              scene->setFrameOut(true, scene->getCurrentFrame());
                          });
    mSceneMenu->addAction(QIcon::fromTheme("range-clear"),
                          tr("Clear In/Out"), this, [this]() {
                              const auto scene = *mDocument.fActiveScene;
                              if (!scene) { return; }
                              scene->clearFrameInOut();
                          });
    mSceneMenu->addSeparator();
    mSceneMenu->addAction(QIcon::fromTheme("markers-add"),
                          tr("Add Marker"), this, [this]() {
                              const auto scene = *mDocument.fActiveScene;
                              if (!scene) { return; }
                              scene->setMarker(scene->getCurrentFrame());
                          });
    mSceneMenu->addAction(QIcon::fromTheme("trash"),
                          tr("Clear Markers"), this, [this]() {
                              const auto scene = *mDocument.fActiveScene;
                              if (!scene) { return; }
                              scene->clearMarkers();
                          });
    mSceneMenu->addAction(QIcon::fromTheme("markers-edit"),
                          tr("Edit Markers"), this, [this]() {
                              openMarkerEditor();
                          });
}
