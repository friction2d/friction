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

#include "mainwindow.h"

#include "GUI/BoxesList/boxscrollwidget.h"

void MainWindow::setupMenuExtras()
{
    const auto menu = new QMenu(this);

    // add rotate
    {
        const auto act = menu->addAction(QIcon::fromTheme("image-x-generic"/* TODO: find new (blender) icon! */),
                                         tr("Rotate selected ..."));
        act->setData("cmd:rotate");
        act->setToolTip(QString("<p><b>%1</b> %2 <i>%3</i></p>").arg(tr("Rotate"),
                                                                     tr("selected items by"),
                                                                     tr("AMOUNT")));
        cmdAddAction(act);
    }
    // add scale
    {
        const auto act = menu->addAction(QIcon::fromTheme("image-x-generic"/* TODO: find new (blender) icon! */),
                                         tr("Scale selected ..."));
        act->setData("cmd:scale");
        act->setToolTip(QString("<p><b>%1</b> %2 <i>%3</i></p>").arg(tr("Scale"),
                                                                     tr("selected items by"),
                                                                     tr("AMOUNT")));
        cmdAddAction(act);
    }
    // add move
    {
        const auto act = menu->addAction(QIcon::fromTheme("image-x-generic"/* TODO: find new (blender) icon! */),
                                         tr("Move selected ..."));
        act->setData("cmd:move");
        act->setToolTip(QString("<p><b>%1</b> %2 <i>%3</i></p>").arg(tr("Move"),
                                                                     tr("selected items by"),
                                                                     tr("AMOUNT (XxY)")));
        cmdAddAction(act);
    }
    // add marker
    {
        const auto act = menu->addAction(QIcon::fromTheme("sequence"/* TODO: find new (blender) icon! */),
                                         tr("Marker at ..."));
        act->setData("cmd:marker");
        act->setToolTip(QString("<p><b>%1</b> %2 <i>%3</i><br><br>%4</p>").arg(tr("Marker"),
                                                                               tr("at"),
                                                                               tr("TIME TITLE."),
                                                                               tr("Time can be frame, sec (<i>X</i><b>s</b>) or min (<i>X</i><b>m</b>)."
                                                                                  " Use <b>,</b> to add multiple markers. Title is optional.")));
        cmdAddAction(act);
    }
    // clear markers
    {
        const auto act = menu->addAction(QIcon::fromTheme("trash"/* TODO: find new (blender) icon! */),
                                         tr("Clear Markers"));
        connect(act, &QAction::triggered,
                this, [this](){
                    const auto scene = *mDocument.fActiveScene;
                    if (scene) { scene->clearMarkers(); }
                });
        cmdAddAction(act);
    }
    // add in
    {
        const auto act = menu->addAction(QIcon::fromTheme("range-in"),
                                         tr("In at ..."));
        act->setData("cmd:in");
        act->setToolTip(QString("<p><b>%1</b> %2 <i>%3</i></p>").arg(tr("Frame In"),
                                                                     tr("at"),
                                                                     tr("TIME (frame/sec/min).")));
        cmdAddAction(act);
    }
    // add out
    {
        const auto act = menu->addAction(QIcon::fromTheme("range-out"),
                                         tr("Out at ..."));
        act->setData("cmd:out");
        act->setToolTip(QString("<p><b>%1</b> %2 <i>%3</i></p>").arg(tr("Frame Out"),
                                                                     tr("at"),
                                                                     tr("TIME (frame/sec/min).")));
        cmdAddAction(act);
    }
    // clear in/out
    {
        const auto act = menu->addAction(QIcon::fromTheme("trash"/* TODO: find new (blender) icon! */),
                                         tr("Clear In"));
        connect(act, &QAction::triggered,
                this, [this](){
                    const auto scene = *mDocument.fActiveScene;
                    if (!scene) { return; }
                    scene->setFrameIn(false, 0);
                });
        cmdAddAction(act);
    }
    {
        const auto act = menu->addAction(QIcon::fromTheme("trash"/* TODO: find new (blender) icon! */),
                                         tr("Clear Out"));
        connect(act, &QAction::triggered,
                this, [this](){
                    const auto scene = *mDocument.fActiveScene;
                    if (!scene) { return; }
                    scene->setFrameOut(false, 0);
                });
        cmdAddAction(act);
    }
    {
        const auto act = menu->addAction(QIcon::fromTheme("range-clear"),
                                         tr("Clear In/Out"));
        connect(act, &QAction::triggered,
                this, [this](){
                    const auto scene = *mDocument.fActiveScene;
                    if (!scene) { return; }
                    scene->setFrameIn(false, 0);
                    scene->setFrameOut(false, 0);
                });
        cmdAddAction(act);
    }
    // center pivot
    {
        const auto act = menu->addAction(QIcon::fromTheme("pivot-align-center"),
                                         tr("Center Pivot"));
        connect(act, &QAction::triggered, this, [this]() {
            const auto scene = *mDocument.fActiveScene;
            if (!scene) { return; }
            scene->centerPivotForSelected();
        });
        cmdAddAction(act);
    }
    // create link
    {
        const auto act = menu->addAction(QIcon::fromTheme("linked"),
                                         tr("Create Link"));
        connect(act, &QAction::triggered, this, [this]() {
            const auto scene = *mDocument.fActiveScene;
            if (!scene) { return; }
            scene->createLinkBoxForSelected();
        });
        cmdAddAction(act);
    }
    // align
    {
        const int alignTotal = 40;

        const QString alignTextDefault = tr("Align %1 %2 Relative to %3");
        const QString alignGeometry = tr("Geometry");
        const QString alignPivot = tr("Pivot");
        const QString alignScene = tr("Scene");
        const QString alignLast = tr("Last Selected");

        const QString alignCenter = tr("Center");
        const QString alignCenterIcon = "pivot-align-center";

        const QString alignLeft = tr("Left");
        const QString alignLeftIcon = "pivot-align-left";

        const QString alignRight = tr("Right");
        const QString alignRightIcon = "pivot-align-right";

        const QString alignTop = tr("Top");
        const QString alignTopIcon = "pivot-align-top";

        const QString alignBottom = tr("Bottom");
        const QString alignBottomIcon = "pivot-align-bottom";

        const QString alignHCenter = tr("Horizontal Center");
        const QString alignHCenterIcon = "pivot-align-hcenter";

        const QString alignVCenter = tr("Vertical Center");
        const QString alignVCenterIcon = "pivot-align-vcenter";

        for (int i = 0; i < alignTotal; i++) {
            QString alignString;
            QString pivotString;
            QString relString;
            QString iconString = "alignCenter";
            AlignPivot pivot = AlignPivot::geometry;
            AlignRelativeTo rel = AlignRelativeTo::scene;
            Qt::Alignment align = Qt::AlignLeft;
            bool alignBoth = false;

            switch(i) {
            case 0: // Align Center - Geometry Relative to Scene -----------------------------------------------------------------------------------
                alignString = alignCenter;
                pivotString = alignGeometry;
                relString = alignScene;
                pivot = AlignPivot::geometry;
                rel = AlignRelativeTo::scene;
                iconString = alignCenterIcon;
                alignBoth = true;
                align = Qt::AlignHCenter;
                break;
            case 1: // Align Center - Pivot Relative to Scene
                alignString = alignCenter;
                pivotString = alignPivot;
                relString = alignScene;
                pivot = AlignPivot::pivot;
                rel = AlignRelativeTo::scene;
                iconString = alignCenterIcon;
                alignBoth = true;
                align = Qt::AlignHCenter;
                break;
            case 2: // Align Center - Geometry Relative to Last Selected
                alignString = alignCenter;
                pivotString = alignGeometry;
                relString = alignLast;
                pivot = AlignPivot::geometry;
                rel = AlignRelativeTo::lastSelected;
                iconString = alignCenterIcon;
                alignBoth = true;
                align = Qt::AlignHCenter;
                break;
            case 3: // Align Center - Pivot Relative to Last Selected
                alignString = alignCenter;
                pivotString = alignPivot;
                relString = alignLast;
                pivot = AlignPivot::pivot;
                rel = AlignRelativeTo::lastSelected;
                iconString = alignCenterIcon;
                alignBoth = true;
                align = Qt::AlignHCenter;
                break;
            case 4: // Align Left - Geometry Relative to Scene -----------------------------------------------------------------------------------
                alignString = alignLeft;
                pivotString = alignGeometry;
                relString = alignScene;
                pivot = AlignPivot::geometry;
                rel = AlignRelativeTo::scene;
                iconString = alignLeftIcon;
                alignBoth = false;
                align = Qt::AlignLeft;
                break;
            case 5: // Align Left - Pivot Relative to Scene
                alignString = alignLeft;
                pivotString = alignPivot;
                relString = alignScene;
                pivot = AlignPivot::pivot;
                rel = AlignRelativeTo::scene;
                iconString = alignLeftIcon;
                alignBoth = false;
                align = Qt::AlignLeft;
                break;
            case 6: // Align Left - Geometry Relative to Last Selected
                alignString = alignLeft;
                pivotString = alignGeometry;
                relString = alignLast;
                pivot = AlignPivot::geometry;
                rel = AlignRelativeTo::lastSelected;
                iconString = alignLeftIcon;
                alignBoth = false;
                align = Qt::AlignLeft;
                break;
            case 7: // Align Left - Pivot Relative to Last Selected
                alignString = alignLeft;
                pivotString = alignPivot;
                relString = alignLast;
                pivot = AlignPivot::pivot;
                rel = AlignRelativeTo::lastSelected;
                iconString = alignLeftIcon;
                alignBoth = false;
                align = Qt::AlignLeft;
                break;
            case 8: // Align Right - Geometry Relative to Scene -----------------------------------------------------------------------------------
                alignString = alignRight;
                pivotString = alignGeometry;
                relString = alignScene;
                pivot = AlignPivot::geometry;
                rel = AlignRelativeTo::scene;
                iconString = alignRightIcon;
                alignBoth = false;
                align = Qt::AlignRight;
                break;
            case 9: // Align Right - Pivot Relative to Scene
                alignString = alignRight;
                pivotString = alignPivot;
                relString = alignScene;
                pivot = AlignPivot::pivot;
                rel = AlignRelativeTo::scene;
                iconString = alignRightIcon;
                alignBoth = false;
                align = Qt::AlignRight;
                break;
            case 10: // Align Right - Geometry Relative to Last Selected
                alignString = alignRight;
                pivotString = alignGeometry;
                relString = alignLast;
                pivot = AlignPivot::geometry;
                rel = AlignRelativeTo::lastSelected;
                iconString = alignRightIcon;
                alignBoth = false;
                align = Qt::AlignRight;
                break;
            case 11: // Align Right - Pivot Relative to Last Selected
                alignString = alignRight;
                pivotString = alignPivot;
                relString = alignLast;
                pivot = AlignPivot::pivot;
                rel = AlignRelativeTo::lastSelected;
                iconString = alignRightIcon;
                alignBoth = false;
                align = Qt::AlignRight;
                break;
            case 12: // Align Top - Geometry Relative to Scene -----------------------------------------------------------------------------------
                alignString = alignTop;
                pivotString = alignGeometry;
                relString = alignScene;
                pivot = AlignPivot::geometry;
                rel = AlignRelativeTo::scene;
                iconString = alignTopIcon;
                alignBoth = false;
                align = Qt::AlignTop;
                break;
            case 13: // Align Top - Pivot Relative to Scene
                alignString = alignTop;
                pivotString = alignPivot;
                relString = alignScene;
                pivot = AlignPivot::pivot;
                rel = AlignRelativeTo::scene;
                iconString = alignTopIcon;
                alignBoth = false;
                align = Qt::AlignTop;
                break;
            case 14: // Align Top - Geometry Relative to Last Selected
                alignString = alignTop;
                pivotString = alignGeometry;
                relString = alignLast;
                pivot = AlignPivot::geometry;
                rel = AlignRelativeTo::lastSelected;
                iconString = alignTopIcon;
                alignBoth = false;
                align = Qt::AlignTop;
                break;
            case 15: // Align Top - Pivot Relative to Last Selected
                alignString = alignTop;
                pivotString = alignPivot;
                relString = alignLast;
                pivot = AlignPivot::pivot;
                rel = AlignRelativeTo::lastSelected;
                iconString = alignTopIcon;
                alignBoth = false;
                align = Qt::AlignTop;
                break;
            case 16: // Align Bottom - Geometry Relative to Scene -----------------------------------------------------------------------------------
                alignString = alignBottom;
                pivotString = alignGeometry;
                relString = alignScene;
                pivot = AlignPivot::geometry;
                rel = AlignRelativeTo::scene;
                iconString = alignBottomIcon;
                alignBoth = false;
                align = Qt::AlignBottom;
                break;
            case 17: // Align Bottom - Pivot Relative to Scene
                alignString = alignBottom;
                pivotString = alignPivot;
                relString = alignScene;
                pivot = AlignPivot::pivot;
                rel = AlignRelativeTo::scene;
                iconString = alignBottomIcon;
                alignBoth = false;
                align = Qt::AlignBottom;
                break;
            case 18: // Align Bottom - Geometry Relative to Last Selected
                alignString = alignBottom;
                pivotString = alignGeometry;
                relString = alignLast;
                pivot = AlignPivot::geometry;
                rel = AlignRelativeTo::lastSelected;
                iconString = alignBottomIcon;
                alignBoth = false;
                align = Qt::AlignBottom;
                break;
            case 19: // Align Bottom - Pivot Relative to Last Selected
                alignString = alignBottom;
                pivotString = alignPivot;
                relString = alignLast;
                pivot = AlignPivot::pivot;
                rel = AlignRelativeTo::lastSelected;
                iconString = alignBottomIcon;
                alignBoth = false;
                align = Qt::AlignBottom;
                break;
            case 20: // Align HCenter - Geometry Relative to Scene -----------------------------------------------------------------------------------
                alignString = alignHCenter;
                pivotString = alignGeometry;
                relString = alignScene;
                pivot = AlignPivot::geometry;
                rel = AlignRelativeTo::scene;
                iconString = alignHCenterIcon;
                alignBoth = false;
                align = Qt::AlignHCenter;
                break;
            case 21: // Align HCenter - Pivot Relative to Scene
                alignString = alignHCenter;
                pivotString = alignPivot;
                relString = alignScene;
                pivot = AlignPivot::pivot;
                rel = AlignRelativeTo::scene;
                iconString = alignHCenterIcon;
                alignBoth = false;
                align = Qt::AlignHCenter;
                break;
            case 22: // Align HCenter - Geometry Relative to Last Selected
                alignString = alignHCenter;
                pivotString = alignGeometry;
                relString = alignLast;
                pivot = AlignPivot::geometry;
                rel = AlignRelativeTo::lastSelected;
                iconString = alignHCenterIcon;
                alignBoth = false;
                align = Qt::AlignHCenter;
                break;
            case 23: // Align HCenter - Pivot Relative to Last Selected
                alignString = alignHCenter;
                pivotString = alignPivot;
                relString = alignLast;
                pivot = AlignPivot::pivot;
                rel = AlignRelativeTo::lastSelected;
                iconString = alignHCenterIcon;
                alignBoth = false;
                align = Qt::AlignHCenter;
                break;
            case 24: // Align VCenter - Geometry Relative to Scene -----------------------------------------------------------------------------------
                alignString = alignVCenter;
                pivotString = alignGeometry;
                relString = alignScene;
                pivot = AlignPivot::geometry;
                rel = AlignRelativeTo::scene;
                iconString = alignVCenterIcon;
                alignBoth = false;
                align = Qt::AlignVCenter;
                break;
            case 25: // Align VCenter - Pivot Relative to Scene
                alignString = alignVCenter;
                pivotString = alignPivot;
                relString = alignScene;
                pivot = AlignPivot::pivot;
                rel = AlignRelativeTo::scene;
                iconString = alignVCenterIcon;
                alignBoth = false;
                align = Qt::AlignVCenter;
                break;
            case 26: // Align VCenter - Geometry Relative to Last Selected
                alignString = alignVCenter;
                pivotString = alignGeometry;
                relString = alignLast;
                pivot = AlignPivot::geometry;
                rel = AlignRelativeTo::lastSelected;
                iconString = alignVCenterIcon;
                alignBoth = false;
                align = Qt::AlignVCenter;
                break;
            case 27: // Align VCenter - Pivot Relative to Last Selected
                alignString = alignVCenter;
                pivotString = alignPivot;
                relString = alignLast;
                pivot = AlignPivot::pivot;
                rel = AlignRelativeTo::lastSelected;
                iconString = alignVCenterIcon;
                alignBoth = false;
                align = Qt::AlignVCenter;
                break;
            case 28:  // Pivot alignment to Bounding Box - VCenter -----------------------------------------------------------------------------------
                alignString = alignVCenter;
                pivotString = alignPivot;
                relString = alignLast;
                pivot = AlignPivot::pivotItself;
                rel = AlignRelativeTo::boundingBox;
                iconString = alignVCenterIcon;
                alignBoth = false;
                align = Qt::AlignVCenter;
                break;
            case 29:  // Pivot alignment to Bounding Box - HCenter
                alignString = alignVCenter;
                pivotString = alignPivot;
                relString = alignLast;
                pivot = AlignPivot::pivotItself;
                rel = AlignRelativeTo::boundingBox;
                iconString = alignVCenterIcon;
                alignBoth = false;
                align = Qt::AlignHCenter;
                break;
            case 30:  // Pivot alignment to Bounding Box - Left
                alignString = alignVCenter;
                pivotString = alignPivot;
                relString = alignLast;
                pivot = AlignPivot::pivotItself;
                rel = AlignRelativeTo::boundingBox;
                iconString = alignVCenterIcon;
                alignBoth = false;
                align = Qt::AlignLeft;
                break;
            case 31:  // Pivot alignment to Bounding Box - Right
                alignString = alignVCenter;
                pivotString = alignPivot;
                relString = alignLast;
                pivot = AlignPivot::pivotItself;
                rel = AlignRelativeTo::boundingBox;
                iconString = alignVCenterIcon;
                alignBoth = false;
                align = Qt::AlignRight;
                break;
            case 32:  // Pivot alignment to Bounding Box - Top
                alignString = alignVCenter;
                pivotString = alignPivot;
                relString = alignLast;
                pivot = AlignPivot::pivotItself;
                rel = AlignRelativeTo::boundingBox;
                iconString = alignVCenterIcon;
                alignBoth = false;
                align = Qt::AlignTop;
                break;
            case 33:  // Pivot alignment to Bounding Box - Bottom
                alignString = alignVCenter;
                pivotString = alignPivot;
                relString = alignLast;
                pivot = AlignPivot::pivotItself;
                rel = AlignRelativeTo::boundingBox;
                iconString = alignVCenterIcon;
                alignBoth = false;
                align = Qt::AlignBottom;
                break;
            case 34:  // Pivot alignment to Scene - VCenter -----------------------------------------------------------------------------------
                alignString = alignVCenter;
                pivotString = alignPivot;
                relString = alignLast;
                pivot = AlignPivot::pivotItself;
                rel = AlignRelativeTo::scene;
                iconString = alignVCenterIcon;
                alignBoth = false;
                align = Qt::AlignVCenter;
                break;
            case 35:  // Pivot alignment to Scene - HCenter
                alignString = alignVCenter;
                pivotString = alignPivot;
                relString = alignLast;
                pivot = AlignPivot::pivotItself;
                rel = AlignRelativeTo::scene;
                iconString = alignVCenterIcon;
                alignBoth = false;
                align = Qt::AlignHCenter;
                break;
            case 36:  // Pivot alignment to Scene - Left
                alignString = alignVCenter;
                pivotString = alignPivot;
                relString = alignLast;
                pivot = AlignPivot::pivotItself;
                rel = AlignRelativeTo::scene;
                iconString = alignVCenterIcon;
                alignBoth = false;
                align = Qt::AlignLeft;
                break;
            case 37:  // Pivot alignment to Scene - Right
                alignString = alignVCenter;
                pivotString = alignPivot;
                relString = alignLast;
                pivot = AlignPivot::pivotItself;
                rel = AlignRelativeTo::scene;
                iconString = alignVCenterIcon;
                alignBoth = false;
                align = Qt::AlignRight;
                break;
            case 38:  // Pivot alignment to Scene - Top
                alignString = alignVCenter;
                pivotString = alignPivot;
                relString = alignLast;
                pivot = AlignPivot::pivotItself;
                rel = AlignRelativeTo::scene;
                iconString = alignVCenterIcon;
                alignBoth = false;
                align = Qt::AlignTop;
                break;
            case 39:  // Pivot alignment to Scene - Bottom
                alignString = alignVCenter;
                pivotString = alignPivot;
                relString = alignLast;
                pivot = AlignPivot::pivotItself;
                rel = AlignRelativeTo::scene;
                iconString = alignVCenterIcon;
                alignBoth = false;
                align = Qt::AlignBottom;
                break;
            default:
                return;
            }

            const auto act = menu->addAction(QIcon::fromTheme(iconString),
                                             QString(alignTextDefault).arg(alignString,
                                                                           pivotString,
                                                                           relString));
            connect(act, &QAction::triggered, this, [this,
                                                     align,
                                                     pivot,
                                                     rel,
                                                     alignBoth]() {
                const auto scene = *mDocument.fActiveScene;
                if (!scene) { return; }
                scene->alignSelectedBoxes(align, pivot, rel);
                if (alignBoth && (align == Qt::AlignVCenter || align == Qt::AlignHCenter)) {
                    scene->alignSelectedBoxes(align == Qt::AlignVCenter ? Qt::AlignHCenter : Qt::AlignVCenter,
                                              pivot,
                                              rel);
                }
                mDocument.actionFinished();
            });
            cmdAddAction(act);
        }
    }
    // Easing
    {
        const QIcon easeIcon(QIcon::fromTheme("easing"));
        const auto presets = AppSupport::getEasingPresets();
        for (const auto &preset : presets) {
            const auto presetAct = new QAction(easeIcon, preset.second, this);
            presetAct->setData(preset.first);
            cmdAddAction(presetAct);
            connect(presetAct, &QAction::triggered,
                    this, [this, presetAct]() {
                const auto scene = *mDocument.fActiveScene;
                if (!scene) { return; }
                scene->setEasingAction(presetAct->text());
            });
        }
    }
}

void MainWindow::setupPropertiesActions()
{
    const auto menu = mViewMenu->addMenu(QIcon::fromTheme("filter"),
                                         tr("Properties Filter"));
    const int defaultRule = AppSupport::getSettings("ui",
                                                    "propertiesFilter",
                                                    (int)SWT_BoxRule::selected).toInt();
    const auto ruleActionAdder = [this, menu, defaultRule](const SWT_BoxRule rule,
                                                           const QString& text) {
        const auto slot = [this, rule]() {
            mObjectSettingsWidget->setCurrentRule(rule);
            AppSupport::setSettings("ui", "propertiesFilter", (int)rule);
        };
        const auto action = menu->addAction(text, this, slot);
        action->setCheckable(true);
        action->setChecked((int)rule == defaultRule);
        connect(mObjectSettingsWidget, &BoxScrollWidget::boxRuleChanged,
                action, [action, rule](const SWT_BoxRule setRule) {
                    action->setChecked(rule == setRule);
                });
        return action;
    };

    const auto group = new QActionGroup(this);
    group->addAction(ruleActionAdder(SWT_BoxRule::all, tr("All")));
    group->addAction(ruleActionAdder(SWT_BoxRule::selected, tr("Selected")));
    group->addAction(ruleActionAdder(SWT_BoxRule::animated, tr("Animated")));
    group->addAction(ruleActionAdder(SWT_BoxRule::notAnimated, tr("Not Animated")));
    group->addAction(ruleActionAdder(SWT_BoxRule::visible, tr("Visible")));
    group->addAction(ruleActionAdder(SWT_BoxRule::hidden, tr("Hidden")));
    group->addAction(ruleActionAdder(SWT_BoxRule::unlocked, tr("Unlocked")));
    group->addAction(ruleActionAdder(SWT_BoxRule::locked, tr("Locked")));
}
