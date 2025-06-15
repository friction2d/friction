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

#include <QToolButton>
#include <QStackedLayout>
#include <QDesktopWidget>
#include <QStatusBar>

#include "timelinewidget.h"
#include "widgets/framescrollbar.h"
#include "mainwindow.h"
#include "timelinedockwidget.h"
#include "GUI/BoxesList/boxsinglewidget.h"
#include "swt_abstraction.h"
#include "GUI/BoxesList/boxscrollwidget.h"
#include "keysview.h"
#include "GUI/BoxesList/boxscrollarea.h"
#include "GUI/BoxesList/boxscroller.h"
#include "animationdockwidget.h"
#include "GUI/global.h"
#include "Private/scene.h"
#include "widgets/scenechooser.h"
#include "widgets/changewidthwidget.h"
#include "timelinehighlightwidget.h"
#include "themesupport.h"

TimelineWidget::TimelineWidget(Document &document,
                               QWidget * const menu,
                               QWidget *parent)
    : QWidget(parent)
    , mDocument(document)
{
    setPalette(ThemeSupport::getDarkerPalette());
    setAutoFillBackground(true);

    mMainLayout = new QGridLayout(this);
    mMainLayout->setSpacing(0);
    mMainLayout->setMargin(0);

    mMenuLayout = new QHBoxLayout();
    //mMenuLayout->setSpacing(0);
    mMenuLayout->setMargin(0);

    mBoxesListMenuBar = new FakeMenuBar(this);
    mBoxesListMenuBar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);

    mSceneChooser = new SceneChooser(mDocument, true,
                                     mBoxesListMenuBar);
    mBoxesListMenuBar->addMenu(mSceneChooser);

    mCornerMenuBar = new FakeMenuBar(this);

    mCornerMenuBar->setObjectName("TimelineMenu");
    menu->setObjectName("TimelineMenu");

    const auto iconsDir = eSettings::sIconsDir();

    const QIcon filterIcon = QIcon::fromTheme("filter");

    QMenu * const settingsMenu = mCornerMenuBar->addMenu(filterIcon, tr("Filters"));
    QMenu * const objectsMenu = settingsMenu->addMenu(filterIcon, tr("State"));

    const auto ruleActionAdder = [this, objectsMenu](
            const SWT_BoxRule rule, const QString& text) {
        const auto slot = [this, rule]() { setBoxRule(rule); };
        const auto action = objectsMenu->addAction(text, this, slot);
        action->setCheckable(true);
        connect(this, &TimelineWidget::boxRuleChanged,
                action, [action, rule](const SWT_BoxRule setRule) {
            action->setChecked(rule == setRule);
        });
        return action;
    };

    ruleActionAdder(SWT_BoxRule::all, "All")->setChecked(true);
    ruleActionAdder(SWT_BoxRule::selected, "Selected");
    ruleActionAdder(SWT_BoxRule::animated, "Animated");
    ruleActionAdder(SWT_BoxRule::notAnimated, "Not Animated");
    ruleActionAdder(SWT_BoxRule::visible, "Visible");
    ruleActionAdder(SWT_BoxRule::hidden, "Hidden");
    ruleActionAdder(SWT_BoxRule::unlocked, "Unlocked");
    ruleActionAdder(SWT_BoxRule::locked, "Locked");

    QMenu * const targetMenu = settingsMenu->addMenu(filterIcon, "Target");

    const auto targetActionAdder = [this, targetMenu](
            const SWT_Target target, const QString& text) {
        const auto slot = [this, target]() { setTarget(target); };
        const auto action = targetMenu->addAction(text, this, slot);
        action->setCheckable(true);
        connect(this, &TimelineWidget::targetChanged,
                action, [action, target](const SWT_Target setTarget) {
            action->setChecked(target == setTarget);
        });
        return action;
    };

    //targetActionAdder(SWT_Target::all, "All");
    targetActionAdder(SWT_Target::canvas, "Current Scene")->setChecked(true);
    targetActionAdder(SWT_Target::group, "Current Group");

    QMenu * const typeMenu = settingsMenu->addMenu(filterIcon, "Type");

    const auto typeActionAdder = [this, typeMenu](
            const SWT_Type type, const QString& text) {
        const auto slot = [this, type]() { setType(type); };
        const auto action = typeMenu->addAction(text, this, slot);
        action->setCheckable(true);
        connect(this, &TimelineWidget::typeChanged,
                action, [action, type](const SWT_Type setType) {
            action->setChecked(type == setType);
        });
        return action;
    };

    typeActionAdder(SWT_Type::all, "All")->setChecked(true);
    typeActionAdder(SWT_Type::sound, "Sound");
    typeActionAdder(SWT_Type::graphics, "Graphics");

    settingsMenu->addSeparator();

    {
        const auto op = [this]() {
            setBoxRule(SWT_BoxRule::all);
            setTarget(SWT_Target::canvas);
            setType(SWT_Type::all);
        };
        const auto act = settingsMenu->addAction("Reset", this, op);
        const auto can = [this]() {
            const auto rules = mBoxesListWidget->getRulesCollection();
            return rules.fRule != SWT_BoxRule::all ||
                   rules.fTarget != SWT_Target::canvas ||
                   rules.fType != SWT_Type::all;
        };
        const auto setEnabled = [act, can]() { act->setEnabled(can()); };
        connect(this, &TimelineWidget::typeChanged, act, setEnabled);
        connect(this, &TimelineWidget::targetChanged, act, setEnabled);
        connect(this, &TimelineWidget::boxRuleChanged, act, setEnabled);
    }

    //QMenu *viewMenu = mBoxesListMenuBar->addMenu("View");
    mGraphAct = mCornerMenuBar->addAction(QIcon::fromTheme("graph"),
                                          "Graph");
    mGraphAct->setCheckable(true);
    connect(mGraphAct, &QAction::toggled,
            this, &TimelineWidget::setGraphEnabled);

    //mCornerMenuBar->setContentsMargins(0, 0, 1, 0);

    mSearchLine = new QLineEdit("", mBoxesListMenuBar);
    mSearchLine->setObjectName("SearchLine");
    mSearchLine->setMinimumHeight(0);
    mSearchLine->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    MainWindow::sGetInstance()->installLineFilter(mSearchLine);
    mSearchLine->setPlaceholderText(tr("Search ..."));
    connect(mSearchLine, &QLineEdit::textChanged,
            this, &TimelineWidget::setSearchText);
    mSearchLine->setFocusPolicy(Qt::ClickFocus);

    mMenuLayout->addWidget(mBoxesListMenuBar);
    mMenuLayout->addWidget(mSearchLine);
    mMenuLayout->addWidget(mCornerMenuBar);
    mMenuLayout->addWidget(menu);

    mMenuWidget = new QWidget(this);
    mMenuWidget->setLayout(mMenuLayout);

    mBoxesListScrollArea = new ScrollArea(this);

    mBoxesListWidget = new BoxScrollWidget(mDocument, mBoxesListScrollArea);
    mBoxesListWidget->setCurrentRule(SWT_BoxRule::all);
    mBoxesListWidget->setCurrentTarget(nullptr, SWT_Target::canvas);

    mBoxesListScrollArea->setWidget(mBoxesListWidget);
    mMainLayout->addWidget(mMenuWidget, 0, 0, Qt::AlignTop);
    mMainLayout->addWidget(mBoxesListScrollArea, 1, 0);

    mKeysViewLayout = new QVBoxLayout();

    mKeysView = new KeysView(mBoxesListWidget, this);
    mKeysViewLayout->addWidget(mKeysView);

    connect(mKeysView, &KeysView::statusMessage,
            this, [](const QString &message) {
        if (MainWindow::sGetInstance()->statusBar()) {
            MainWindow::sGetInstance()->statusBar()->showMessage(message, 5000);
        }
    });

    const auto high1 = mBoxesListWidget->requestHighlighter();
    const auto high2 = mKeysView->requestHighlighter();
    high1->setOther(high2);
    high2->setOther(high1);

    mAnimationDockWidget = new AnimationDockWidget(this, mKeysView);
    mAnimationDockWidget->showGraph(false);
    mMainLayout->addLayout(mKeysViewLayout, 1, 1);

    const auto keysViewScrollbarLayout = new QHBoxLayout();
    const auto layoutT = new QVBoxLayout();
    layoutT->setAlignment(Qt::AlignBottom);
    layoutT->addWidget(mAnimationDockWidget);
    keysViewScrollbarLayout->addLayout(layoutT);
    mKeysView->setLayout(keysViewScrollbarLayout);
    keysViewScrollbarLayout->setAlignment(Qt::AlignRight);
    /*keysViewScrollbarLayout->addWidget(
                mBoxesListScrollArea->verticalScrollBar());*/
    mBoxesListScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    keysViewScrollbarLayout->setContentsMargins(0, 0, 0, 0);

    connect(mBoxesListScrollArea->verticalScrollBar(),
            &QScrollBar::valueChanged,
            mBoxesListWidget, &BoxScrollWidget::changeVisibleTop);
    connect(mBoxesListScrollArea, &ScrollArea::heightChanged,
            mBoxesListWidget, &BoxScrollWidget::changeVisibleHeight);
    connect(mBoxesListScrollArea, &ScrollArea::widthChanged,
            mBoxesListWidget, &BoxScrollWidget::setWidth);

    connect(mBoxesListScrollArea->verticalScrollBar(),
            &QScrollBar::valueChanged,
            this, &TimelineWidget::moveSlider);
    connect(mKeysView, &KeysView::wheelEventSignal,
            mBoxesListScrollArea, &ScrollArea::callWheelEvent);
    connect(mKeysView, &KeysView::changedViewedFrames,
             this, &TimelineWidget::setViewedFrameRange);

    connect(mSceneChooser, &SceneChooser::currentChanged,
            this, &TimelineWidget::setCurrentScene);

    eSizesUI::widget.add(mBoxesListScrollArea, [this](const int size) {
        mBoxesListScrollArea->setFixedWidth(20*size);
    });

    setLayout(mMainLayout);

    mFrameScrollBar = new FrameScrollBar(1, 1, false, false, false, this);
    mFrameScrollBar->setSizePolicy(QSizePolicy::Minimum,
                                   QSizePolicy::Preferred);

    const qreal dpi = QApplication::desktop()->logicalDpiX() / 96.0;
    mFrameScrollBar->setFixedHeight(40 * dpi);

//    connect(MemoryHandler::sGetInstance(), &MemoryHandler::memoryFreed,
//            frameScrollBar,
//            qOverload<>(&FrameScrollBar::update));
    connect(mFrameScrollBar, &FrameScrollBar::triggeredFrameRangeChange,
            this, [this](const FrameRange& range){
        const auto scene = mSceneChooser->getCurrentScene();
        if(scene) scene->anim_setAbsFrame(range.fMin);
        Document::sInstance->actionFinished();
    });
    mMainLayout->addWidget(mFrameScrollBar, 0, 1);

    mFrameRangeScrollBar = new FrameScrollBar(20, 200, true, true, true, this);
    mFrameRangeScrollBar->setFixedHeight(10);
    //eSizesUI::widget.add(mFrameRangeScrollBar, [this](const int size) {
    //    mFrameRangeScrollBar->setMinimumHeight(size+5/**2/3*/);
    //});

    connect(mFrameRangeScrollBar, &FrameScrollBar::triggeredFrameRangeChange,
            this, &TimelineWidget::setViewedFrameRange);
    connect(mKeysView, &KeysView::wheelEventSignal,
            mFrameRangeScrollBar, &FrameScrollBar::callWheelEvent);

#ifdef Q_OS_MAC
    connect(mKeysView, &KeysView::panEventSignal,
            mFrameRangeScrollBar, &FrameScrollBar::callPanEvent);
    connect(mKeysView, &KeysView::nativeEventSignal,
            mFrameRangeScrollBar, &FrameScrollBar::callNativeGestures);
#endif

    mKeysViewLayout->addWidget(mFrameRangeScrollBar);
    //mSceneChooser->setCurrentScene(mDocument.fActiveScene); // why?


    const auto chww = new ChangeWidthWidget(this);
    chww->show();
    chww->updatePos();
    chww->raise();
    connect(chww, &ChangeWidthWidget::widthSet,
            this, &TimelineWidget::setBoxesListWidth);

    readSettings(chww);
}

TimelineWidget::~TimelineWidget()
{
    writeSettings();
}

void TimelineWidget::setCurrentScene(Scene * const scene) {
    if(scene == mCurrentScene) return;
    if(mCurrentScene) {
        disconnect(mCurrentScene, nullptr, mFrameScrollBar, nullptr);
        disconnect(mCurrentScene, nullptr, this, nullptr);
    }

    mCurrentScene = scene;
    mSceneChooser->setCurrentScene(scene);
    mFrameScrollBar->setCurrentCanvas(scene);
    mFrameRangeScrollBar->setCurrentCanvas(scene);
    mBoxesListWidget->setCurrentScene(scene);
    mKeysView->setCurrentScene(scene);
    if(scene) {
        setCanvasFrameRange(scene->getFrameRange());
        mFrameScrollBar->setFirstViewedFrame(scene->getCurrentFrame());
        mFrameRangeScrollBar->setFirstViewedFrame(scene->getCurrentFrame());
        setViewedFrameRange(mFrameRangeScrollBar->getViewedRange());

        connect(scene, &Scene::currentFrameChanged,
                mFrameScrollBar, &FrameScrollBar::setFirstViewedFrame);
        connect(scene, &Scene::newFrameRange,
                this, &TimelineWidget::setCanvasFrameRange);


        const auto rules = mBoxesListWidget->getRulesCollection();
        if(rules.fTarget == SWT_Target::canvas) {
            mBoxesListWidget->scheduleContentUpdateIfIsCurrentTarget(
                        scene, SWT_Target::canvas);
        } else if(rules.fTarget == SWT_Target::group) {
            mBoxesListWidget->scheduleContentUpdateIfIsCurrentTarget(
                        scene->getCurrentGroup(), SWT_Target::group);
        }
        connect(scene, &Scene::currentContainerSet, this,
                [this](ContainerBox* const container) {
            mBoxesListWidget->scheduleContentUpdateIfIsCurrentTarget(
                        container, SWT_Target::group);
        });
        connect(scene, &Scene::requestUpdate, this, [this]() {
            mFrameScrollBar->update();
            mBoxesListWidget->updateVisible();
            update();
        });
    }
}

void TimelineWidget::setGraphEnabled(const bool enabled) {
    mKeysView->setGraphViewed(enabled);
    mAnimationDockWidget->showGraph(enabled);
    //const auto iconsDir = eSettings::sIconsDir();
    //if(enabled) mGraphAct->setIcon(QIcon(iconsDir + "/graphEnabled.png"));
    //else mGraphAct->setIcon(QIcon(iconsDir + "/graphDisabled.png"));
}

void TimelineWidget::writeState(eWriteStream &dst) const {
    const int id = mBoxesListWidget->getId();
    dst.objListIdConv().assign(id);

    if(mCurrentScene) {
        dst << mCurrentScene->getWriteId();
        dst << mCurrentScene->getDocumentId();
    } else {
        dst << -1;
        dst << -1;
    }

    dst << mSearchLine->text();
    dst << mBoxesListScrollArea->verticalScrollBar()->sliderPosition();

    dst << mFrameScrollBar->getFirstViewedFrame();
    dst << mFrameRangeScrollBar->getFirstViewedFrame();
    dst << mFrameRangeScrollBar->getLastViewedFrame();

    const auto rules = mBoxesListWidget->getRulesCollection();
    dst.write(&rules.fRule, sizeof(SWT_BoxRule));
    dst.write(&rules.fType, sizeof(SWT_Type));
    dst.write(&rules.fTarget, sizeof(SWT_Target));
}

void TimelineWidget::readState(eReadStream &src) {
    const int id = mBoxesListWidget->getId();
    src.objListIdConv().assign(id);

    int sceneReadId; src >> sceneReadId;
    int sceneDocumentId; src >> sceneDocumentId;

    QString search; src >> search;
    int sliderPos; src >> sliderPos;

    int frame; src >> frame;
    int minViewedFrame; src >> minViewedFrame;
    int maxViewedFrame; src >> maxViewedFrame;

    if(src.evFileVersion() > 6) {
        SWT_BoxRule boxRule;
        SWT_Type type;
        SWT_Target target;
        src.read(&boxRule, sizeof(SWT_BoxRule));
        src.read(&type, sizeof(SWT_Type));
        src.read(&target, sizeof(SWT_Target));
        setBoxRule(boxRule);
        setType(type);
        setTarget(target);
    }

    src.addReadStreamDoneTask([this, sceneReadId, sceneDocumentId]
                              (eReadStream& src) {
        BoundingBox* sceneBox = nullptr;
        if(sceneReadId != -1)
            sceneBox = src.getBoxByReadId(sceneReadId);
        if(!sceneBox && sceneDocumentId != -1)
            sceneBox = BoundingBox::sGetBoxByDocumentId(sceneDocumentId);

        setCurrentScene(enve_cast<Scene*>(sceneBox));
    });

    mSearchLine->setText(search);

    //mBoxesListScrollArea->verticalScrollBar()->setSliderPosition(sliderPos);
    //mKeysView->setViewedVerticalRange(sliderPos, sliderPos + mBoxesListScrollArea->height());

    mFrameScrollBar->setFirstViewedFrame(frame);
    setViewedFrameRange({minViewedFrame, maxViewedFrame});
}

void TimelineWidget::readStateXEV(XevReadBoxesHandler& boxReadHandler,
                                  const QDomElement& ele,
                                  RuntimeIdToWriteId& objListIdConv) {
    objListIdConv.assign(mBoxesListWidget->getId());

    const auto frameRangeStr = ele.attribute("frameRange");
    const auto frameStr = ele.attribute("frame");

    const auto frameRangeValStrs = frameRangeStr.splitRef(' ');
    if(frameRangeValStrs.count() != 2)
        RuntimeThrow("Invalid frame range value " + frameRangeStr);

    const auto minViewedFrameStr = frameRangeValStrs.first();
    const int minViewedFrame = XmlExportHelpers::stringToInt(minViewedFrameStr);

    const auto maxViewedFrameStr = frameRangeValStrs.last();
    const int maxViewedFrame = XmlExportHelpers::stringToInt(maxViewedFrameStr);

    const int frame = XmlExportHelpers::stringToInt(frameStr);

    const auto search = ele.attribute("search");

    const auto sceneIdStr = ele.attribute("sceneId");
    const int sceneId = XmlExportHelpers::stringToInt(sceneIdStr);

    boxReadHandler.addXevImporterDoneTask(
                [this, sceneId](const XevReadBoxesHandler& imp) {
        const auto sceneBox = imp.getBoxByReadId(sceneId);
        const auto scene = enve_cast<Scene*>(sceneBox);
        setCurrentScene(scene);
    });

    const auto boxRuleStr = ele.attribute("objRule");
    const auto boxRule = XmlExportHelpers::stringToEnum<SWT_BoxRule>(boxRuleStr);
    setBoxRule(boxRule);

    const auto typeStr = ele.attribute("objType");
    const auto type = XmlExportHelpers::stringToEnum<SWT_Type>(typeStr);
    setType(type);

    const auto targetStr = ele.attribute("objTarget");
    const auto target = XmlExportHelpers::stringToEnum<SWT_Target>(targetStr);
    setTarget(target);

    mSearchLine->setText(search);

    mFrameScrollBar->setFirstViewedFrame(frame);
    setViewedFrameRange({minViewedFrame, maxViewedFrame});
}

void TimelineWidget::writeStateXEV(QDomElement& ele, QDomDocument& doc,
                                   RuntimeIdToWriteId& objListIdConv) const {
    Q_UNUSED(doc)

    objListIdConv.assign(mBoxesListWidget->getId());

    const int frame = mFrameScrollBar->getFirstViewedFrame();
    const int minViewedFrame = mFrameRangeScrollBar->getFirstViewedFrame();
    const int maxViewedFrame = mFrameRangeScrollBar->getLastViewedFrame();
    const QString frameRange = QString("%1 %2").arg(minViewedFrame).
                                                arg(maxViewedFrame);

    ele.setAttribute("frameRange", frameRange);
    ele.setAttribute("frame", frame);

    const int sceneId = mCurrentScene ? mCurrentScene->getWriteId() : -1;
    ele.setAttribute("sceneId", sceneId);

    const auto rules = mBoxesListWidget->getRulesCollection();
    ele.setAttribute("objRule", static_cast<int>(rules.fRule));
    ele.setAttribute("objType", static_cast<int>(rules.fType));
    ele.setAttribute("objTarget", static_cast<int>(rules.fTarget));

    ele.setAttribute("search", mSearchLine->text());
}

void TimelineWidget::readSettings(ChangeWidthWidget *chww)
{
    const auto tWidth = AppSupport::getSettings("ui",
                                                "TimeLineMenuWidth");
    setBoxesListWidth(tWidth.isValid() ? tWidth.toInt() : chww->getCurrentWidth());
    if (tWidth.isValid()) { chww->setWidth(tWidth.toInt()); }
}

void TimelineWidget::writeSettings()
{
    AppSupport::setSettings("ui", "TimeLineMenuWidth",
                            mBoxesListScrollArea->width());
}

void TimelineWidget::moveSlider(int val) {
    int diff = val%eSizesUI::widget;
    if(diff != 0) {
        val -= diff;
        mBoxesListScrollArea->verticalScrollBar()->setSliderPosition(val);
    }
    mKeysView->setViewedVerticalRange(val, val + mBoxesListScrollArea->height());
}

void TimelineWidget::setBoxesListWidth(const int width) {
    mMenuWidget->setFixedWidth(width);
    mBoxesListScrollArea->setFixedWidth(width);
}

void TimelineWidget::setBoxRule(const SWT_BoxRule rule) {
    mBoxesListWidget->setCurrentRule(rule);
    emit boxRuleChanged(rule);
}

void TimelineWidget::setTarget(const SWT_Target target) {
    switch(target) {
    case SWT_Target::all:
        mBoxesListWidget->setCurrentTarget(&mDocument, SWT_Target::all);
        break;
    case SWT_Target::canvas:
        mBoxesListWidget->setCurrentTarget(mCurrentScene, SWT_Target::canvas);
        break;
    case SWT_Target::group:
        const auto group = mCurrentScene ? mCurrentScene->getCurrentGroup() :
                                           nullptr;
        mBoxesListWidget->setCurrentTarget(group, SWT_Target::group);
        break;
    }

    emit targetChanged(target);
}

void TimelineWidget::setType(const SWT_Type type) {
    mBoxesListWidget->setCurrentType(type);
    emit typeChanged(type);
}

void TimelineWidget::setSearchText(const QString &text) {
    mBoxesListWidget->setCurrentSearchText(text);
}

void TimelineWidget::setViewedFrameRange(const FrameRange& range) {
    mFrameRangeScrollBar->setViewedFrameRange(range);
    mFrameScrollBar->setDisplayedFrameRange(range);
    mKeysView->setFramesRange(range);
}

void TimelineWidget::setCanvasFrameRange(const FrameRange& range) {
    mFrameRangeScrollBar->setDisplayedFrameRange(range);
    setViewedFrameRange(mFrameRangeScrollBar->getViewedRange());
    mFrameRangeScrollBar->setCanvasFrameRange(range);
    mFrameScrollBar->setCanvasFrameRange(range);
}
