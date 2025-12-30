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

#include "toolinteract.h"

#include "Private/document.h"
#include "GUI/coloranimatorbutton.h"

#include <QSpinBox>
#include <QLabel>
#include <QWidgetAction>
#include <QMenu>
#include <QHBoxLayout>

using namespace Friction;
using namespace Friction::Ui;

ToolInteract::ToolInteract(QWidget *parent)
    : ToolBar("ToolInteract", parent, true)
{
    setToolButtonStyle(Qt::ToolButtonIconOnly);
    setContextMenuPolicy(Qt::NoContextMenu);
    setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    setWindowTitle(tr("Tool Interact"));

    setupGizmoButton();
    setupSnapButton();
    setupGridButton();
}

void ToolInteract::setupGizmoButton()
{
    const auto button = new QToolButton(this);
    const auto menu = new QMenu(button);

    button->setObjectName("ToolBoxGizmo");
    button->setPopupMode(QToolButton::MenuButtonPopup);
    button->setFocusPolicy(Qt::NoFocus);
    button->setMenu(menu);

    {
        const auto icon = QIcon::fromTheme("gizmos_off");
        const auto iconChecked = QIcon::fromTheme("gizmos_on");
        const auto act = new QAction(tr("Gizmos"), button);
        const auto doc = Document::sInstance;
        const bool visible = doc->getGizmoVisibility(Core::Gizmos::Interact::All);

        const QString gizmosOn = tr("Gizmos in On");
        const QString gizmosOff = tr("Gizmos is Off");

        act->setCheckable(true);
        act->setChecked(visible);
        act->setText(visible ? gizmosOn : gizmosOff);
        act->setIcon(visible ? iconChecked : icon);

        menu->addAction(act);
        button->setDefaultAction(act);

        connect(act, &QAction::triggered,
                this, [doc] () {
            const auto gizmo = Core::Gizmos::Interact::All;
            doc->setGizmoVisibility(gizmo, !doc->getGizmoVisibility(gizmo));
        });
        connect(doc, &Document::gizmoVisibilityChanged,
                this, [act,
                       icon,
                       iconChecked,
                       gizmosOn,
                       gizmosOff] (Core::Gizmos::Interact i, bool visible) {
            const auto gizmo = Core::Gizmos::Interact::All;
            if (gizmo != i) { return; }
            act->blockSignals(true);
            act->setChecked(visible);
            act->setText(visible ? gizmosOn : gizmosOff);
            act->setIcon(visible ? iconChecked : icon);
            act->blockSignals(false);
        });
    }
    menu->addSeparator();

    setupGizmoAction(button, Core::Gizmos::Interact::Position);
    setupGizmoAction(button, Core::Gizmos::Interact::Rotate);
    setupGizmoAction(button, Core::Gizmos::Interact::Scale);
    setupGizmoAction(button, Core::Gizmos::Interact::Shear);

    addWidget(button);
}

void ToolInteract::setupGizmoAction(QToolButton *button,
                                    const Core::Gizmos::Interact &ti)
{
    if (!button) { return; }
    if (!button->menu()) { return; }

    const auto mDocument = Document::sInstance;
    const bool visible = mDocument->getGizmoVisibility(ti);
    QString text;

    switch(ti) {
    case Core::Gizmos::Interact::Position:
        text = tr("Position");
        break;
    case Core::Gizmos::Interact::Rotate:
        text = tr("Rotate");
        break;
    case Core::Gizmos::Interact::Scale:
        text = tr("Scale");
        break;
    case Core::Gizmos::Interact::Shear:
        text = tr("Shear");
        break;
    default: return;
    }

    const auto act = button->menu()->addAction(text);

    act->setCheckable(true);
    act->setChecked(visible);

    connect(act, &QAction::triggered,
            this, [mDocument, ti] () {
        mDocument->setGizmoVisibility(ti, !mDocument->getGizmoVisibility(ti));
    });

    connect(mDocument, &Document::gizmoVisibilityChanged,
            this, [act, ti] (Core::Gizmos::Interact i,
                             bool visible) {
        if (ti != i) { return; }
        act->blockSignals(true);
        act->setChecked(visible);
        act->blockSignals(false);
    });
}

void ToolInteract::setupSnapButton()
{
    const auto grid = Document::sInstance->getGrid();
    const auto button = new QToolButton(this);
    const auto menu = new QMenu(button);

    button->setObjectName("ToolBoxGizmo");
    button->setPopupMode(QToolButton::MenuButtonPopup);
    button->setFocusPolicy(Qt::NoFocus);
    button->setMenu(menu);

    {
        const auto icon = QIcon::fromTheme("snap_off");
        const auto iconChecked = QIcon::fromTheme("snap_on");
        const QString snapOn = tr("Snapping is On");
        const QString snapOff = tr("Snapping is Off");

        const auto act = new QAction(button);
        const bool snap = grid->getSettings().snapEnabled;

        act->setCheckable(true);
        act->setChecked(snap);
        act->setText(snap ? snapOn : snapOff);
        act->setIcon(snap ? iconChecked : icon);
        act->setShortcut(QKeySequence("Shift+Tab"));

        connect(act, &QAction::triggered,
                this, [grid,
                       act,
                       icon,
                       iconChecked,
                       snapOn,
                       snapOff] (const bool checked) {
            act->setText(checked ? snapOn : snapOff);
            act->setIcon(checked ? iconChecked : icon);
            grid->setOption(Core::Grid::Option::SnapEnabled, checked, true);
        });
        connect(grid, &Core::Grid::changed,
                this, [act,
                       icon,
                       iconChecked,
                       snapOn,
                       snapOff] (const Core::Grid::Settings &settings) {
            if (settings.snapEnabled == act->isChecked()) { return; }
            act->blockSignals(true);
            act->setChecked(settings.snapEnabled);
            act->setText(settings.snapEnabled ? snapOn : snapOff);
            act->setIcon(settings.snapEnabled ? iconChecked : icon);
            act->blockSignals(false);
        });

        menu->addAction(act);
        button->setDefaultAction(act);
    }

    menu->addSeparator();
    setupSnapAction(button, Core::Grid::Option::SnapToCanvas);
    setupSnapAction(button, Core::Grid::Option::SnapToBoxes);
    setupSnapAction(button, Core::Grid::Option::SnapToNodes);
    setupSnapAction(button, Core::Grid::Option::SnapToPivots);
    setupSnapAction(button, Core::Grid::Option::SnapToGrid);
    menu->addSeparator();
    setupSnapAction(button, Core::Grid::Option::AnchorPivot);
    setupSnapAction(button, Core::Grid::Option::AnchorBounds);
    setupSnapAction(button, Core::Grid::Option::AnchorNodes);
    menu->addSeparator();

    addWidget(button);
}

void ToolInteract::setupSnapAction(QToolButton *button,
                                   const Core::Grid::Option &option)
{
    if (!button) { return; }
    if (!button->menu()) { return; }

    const auto grid = Document::sInstance->getGrid();
    const auto act = new QAction(button);

    act->setCheckable(true);
    act->setChecked(grid->getOption(option).toBool());

    switch (option) {
    case Core::Grid::Option::SnapToCanvas:
        act->setText(tr("Snap to Canvas"));
        break;
    case Core::Grid::Option::SnapToBoxes:
        act->setText(tr("Snap to Boxes"));
        break;
    case Core::Grid::Option::SnapToNodes:
        act->setText(tr("Snap to Nodes"));
        break;
    case Core::Grid::Option::SnapToPivots:
        act->setText(tr("Snap to Pivots"));
        break;
    case Core::Grid::Option::SnapToGrid:
        act->setText(tr("Snap to Grid (if visible)"));
        break;
    case Core::Grid::Option::AnchorPivot:
        act->setText(tr("Anchor Pivot"));
        break;
    case Core::Grid::Option::AnchorBounds:
        act->setText(tr("Anchor Bounds"));
        break;
    case Core::Grid::Option::AnchorNodes:
        act->setText(tr("Anchor Nodes"));
        break;
    default:
        qWarning() << "Unknown Snap Option!" << (int)option;
    }

    button->menu()->addAction(act);

    connect(act, &QAction::triggered,
            this, [grid, option] (const bool checked) {
        grid->setOption(option, checked, true);
    });
    connect(grid, &Core::Grid::changed,
            this, [act, option] (const Core::Grid::Settings &settings) {
        bool checked = false;
        switch (option) {
        case Core::Grid::Option::SnapToCanvas:
            checked = settings.snapToCanvas;
            break;
        case Core::Grid::Option::SnapToBoxes:
            checked = settings.snapToBoxes;
            break;
        case Core::Grid::Option::SnapToNodes:
            checked = settings.snapToNodes;
            break;
        case Core::Grid::Option::SnapToPivots:
            checked = settings.snapToPivots;
            break;
        case Core::Grid::Option::SnapToGrid:
            checked = settings.snapToGrid;
            break;
        case Core::Grid::Option::AnchorPivot:
            checked = settings.snapAnchorPivot;
            break;
        case Core::Grid::Option::AnchorBounds:
            checked = settings.snapAnchorBounds;
            break;
        case Core::Grid::Option::AnchorNodes:
            checked = settings.snapAnchorNodes;
            break;
        default: return;
        }
        if (checked == act->isChecked()) { return; }
        act->blockSignals(true);
        act->setChecked(checked);
        act->blockSignals(false);
    });
}

void ToolInteract::setupGridButton()
{
    const auto grid = Document::sInstance->getGrid();
    const auto button = new QToolButton(this);
    const auto menu = new QMenu(button);

    button->setObjectName("ToolBoxGizmo");
    button->setPopupMode(QToolButton::MenuButtonPopup);
    button->setFocusPolicy(Qt::NoFocus);
    button->setMenu(menu);

    {
        const auto icon = QIcon::fromTheme("grid_off");
        const auto iconChecked = QIcon::fromTheme("grid_on");

        const QString gridOn = tr("Grid is On");
        const QString gridOff = tr("Grid is Off");

        const auto act = new QAction(tr("Grid"), button);
        const bool gridShow = grid->getSettings().show;

        act->setCheckable(true);
        act->setChecked(gridShow);
        act->setText(gridShow ? gridOn : gridOff);
        act->setIcon(gridShow ? iconChecked : icon);

        menu->addAction(act);
        button->setDefaultAction(act);

        connect(act, &QAction::triggered,
                this, [grid,
                       act,
                       icon,
                       iconChecked,
                       gridOn,
                       gridOff] (const bool checked) {
            act->setText(checked ? gridOn : gridOff);
            act->setIcon(checked ? iconChecked : icon);
            grid->setOption(Core::Grid::Option::Show,
                            checked, true);
        });
        connect(grid, &Core::Grid::changed,
                this, [act,
                       icon,
                       iconChecked,
                       gridOn,
                       gridOff] (const Core::Grid::Settings &settings) {
            if (settings.show == act->isChecked()) { return; }
            act->blockSignals(true);
            act->setChecked(settings.snapEnabled);
            act->setText(settings.snapEnabled ? gridOn : gridOff);
            act->setIcon(settings.snapEnabled ? iconChecked : icon);
            act->blockSignals(false);
        });
    }

    menu->addSeparator();
    const auto optMenu = menu->addMenu(QIcon::fromTheme("preferences"),
                                       tr("Settings"));

    menu->addSeparator();
    setupGridAction(button, Core::Grid::Option::SizeX);
    setupGridAction(button, Core::Grid::Option::SizeY);
    menu->addSeparator();
    setupGridAction(button, Core::Grid::Option::OriginX);
    setupGridAction(button, Core::Grid::Option::OriginY);
    menu->addSeparator();
    setupGridAction(button, Core::Grid::Option::MajorEveryX);
    setupGridAction(button, Core::Grid::Option::MajorEveryY);
    menu->addSeparator();

    {
        const auto act = new QWidgetAction(this);
        const auto wid = new QWidget(this);
        const auto lay = new QHBoxLayout(wid);
        const auto spin = new QSpinBox(wid);
        const auto label = new QLabel(tr("Threshold"), wid);

        wid->setContentsMargins(0, 0, 0, 0);
        lay->setMargin(4);

        lay->addWidget(label);
        lay->addWidget(spin);

        act->setDefaultWidget(wid);
        menu->addAction(act);

        spin->setRange(0, 9999);
        spin->setValue(grid->getSettings().snapThresholdPx);

        connect(spin, qOverload<int>(&QSpinBox::valueChanged),
                this, [grid] (const int value) {
            grid->setOption(Core::Grid::Option::SnapThresholdPx,
                            value, true);
        });
        connect(grid, &Core::Grid::changed,
                this, [spin] (const Core::Grid::Settings &settings) {
            if (settings.snapThresholdPx == spin->value()) { return; }
            spin->blockSignals(true);
            spin->setValue(settings.snapThresholdPx);
            spin->blockSignals(false);
        });
    }

    menu->addSeparator();
    setupGridAction(button, Core::Grid::Option::Color);
    menu->addSeparator();
    setupGridAction(button, Core::Grid::Option::DrawOnTop);
    menu->addSeparator();

    {
        const auto act = new QAction(QIcon::fromTheme("loop_back"),
                                     tr("Reset Grid"), this);
        optMenu->addAction(act);
        connect(act, &QAction::triggered,
                this, [grid] () {
            auto settings = grid->getSettings();
            const auto fallback = Core::Grid::Settings();
            settings.sizeX = fallback.sizeX;
            settings.sizeY = fallback.sizeY;
            settings.originX = fallback.originX;
            settings.originY = fallback.originY;
            settings.snapThresholdPx = fallback.snapThresholdPx;
            settings.majorEveryX = fallback.majorEveryX;
            settings.majorEveryY = fallback.majorEveryY;
            settings.color = fallback.color;
            settings.colorMajor = fallback.colorMajor;
            settings.drawOnTop = fallback.drawOnTop;
            grid->setSettings(settings);
        });
    }

    {
        const auto act = new QAction(QIcon::fromTheme("loop_back"),
                                     tr("Reset Default"), this);
        optMenu->addAction(act);
        connect(act, &QAction::triggered,
                this, [grid] () {
            auto settings = eSettings::instance().fGrid;
            const auto fallback = Core::Grid::Settings();
            settings.sizeX = fallback.sizeX;
            settings.sizeY = fallback.sizeY;
            settings.originX = fallback.originX;
            settings.originY = fallback.originY;
            settings.snapThresholdPx = fallback.snapThresholdPx;
            settings.majorEveryX = fallback.majorEveryX;
            settings.majorEveryY = fallback.majorEveryY;
            settings.color = fallback.color;
            settings.colorMajor = fallback.colorMajor;
            settings.drawOnTop = fallback.drawOnTop;
            grid->saveSettings(settings);
            eSettings::sInstance->fGrid = settings;
        });
    }

    optMenu->addSeparator();

    {
        const auto act = new QAction(QIcon::fromTheme("file_folder"),
                                     tr("Load from Default"), this);
        optMenu->addAction(act);
        connect(act, &QAction::triggered,
                this, [grid] () {
            auto settings = grid->getSettings();
            const auto fallback = eSettings::instance().fGrid;
            settings.sizeX = fallback.sizeX;
            settings.sizeY = fallback.sizeY;
            settings.originX = fallback.originX;
            settings.originY = fallback.originY;
            settings.snapThresholdPx = fallback.snapThresholdPx;
            settings.majorEveryX = fallback.majorEveryX;
            settings.majorEveryY = fallback.majorEveryY;
            settings.color = fallback.color;
            settings.colorMajor = fallback.colorMajor;
            settings.drawOnTop = fallback.drawOnTop;
            grid->setSettings(settings);
        });
    }

    {
        const auto act = new QAction(QIcon::fromTheme("disk_drive"),
                                     tr("Save as Default"), this);
        optMenu->addAction(act);
        connect(act, &QAction::triggered,
                this, [grid] () {
            const auto settings = grid->getSettings();
            auto defaults = eSettings::instance().fGrid;
            defaults.sizeX = settings.sizeX;
            defaults.sizeY = settings.sizeY;
            defaults.originX = settings.originX;
            defaults.originY = settings.originY;
            defaults.snapThresholdPx = settings.snapThresholdPx;
            defaults.majorEveryX = settings.majorEveryX;
            defaults.majorEveryY = settings.majorEveryY;
            defaults.color = settings.color;
            defaults.colorMajor = settings.colorMajor;
            defaults.drawOnTop = settings.drawOnTop;
            grid->saveSettings(defaults);
            eSettings::sInstance->fGrid = defaults;
        });
    }

    addWidget(button);
}

void ToolInteract::setupGridAction(QToolButton *button,
                                   const Core::Grid::Option &option)
{
    if (!button) { return; }
    if (!button->menu()) { return; }

    const auto grid = Document::sInstance->getGrid();
    const auto& settings = grid->getSettings();

    if (option == Core::Grid::Option::DrawOnTop) {
        const auto act = new QAction(tr("Draw on Top"), button);

        act->setCheckable(true);
        act->setChecked(settings.drawOnTop);
        button->menu()->addAction(act);

        connect(act, &QAction::triggered,
                this, [grid, option] (const bool checked) {
            grid->setOption(option, checked, false);
        });
        connect(grid, &Core::Grid::changed,
                this, [act] (const Core::Grid::Settings &settings) {
            const bool checked = settings.drawOnTop;
            if (checked == act->isChecked()) { return; }
            act->blockSignals(true);
            act->setChecked(checked);
            act->blockSignals(false);
        });
    } else if (option == Core::Grid::Option::Color) {
        const auto act = new QWidgetAction(this);
        const auto wid = new QWidget(this);
        const auto lay = new QHBoxLayout(wid);
        const auto label = new QLabel(tr("Colors"), wid);
        const auto color1 = new ColorAnimatorButton(settings.color, wid);
        const auto color2 = new ColorAnimatorButton(settings.colorMajor, wid);

        wid->setContentsMargins(0, 0, 0, 0);
        lay->setContentsMargins(5, 2, 10, 2);

        lay->addWidget(label);
        lay->addWidget(color1);
        lay->addWidget(color2);

        act->setDefaultWidget(wid);
        button->menu()->addAction(act);

        connect(color1, &ColorAnimatorButton::colorChanged,
                this, [grid] (const QColor &color) {
            grid->setOption(Core::Grid::Option::Color,
                            color, false);
        });
        connect(color2, &ColorAnimatorButton::colorChanged,
                this, [grid] (const QColor &color) {
            grid->setOption(Core::Grid::Option::ColorMajor,
                            color, false);
        });
        connect(grid, &Core::Grid::changed,
                this, [color1, color2] (const Core::Grid::Settings &settings) {
            if (settings.color != color1->color()) {
                color1->blockSignals(true);
                color1->setColor(settings.color);
                color1->blockSignals(false);
            }
            if (settings.colorMajor != color2->color()) {
                color2->blockSignals(true);
                color2->setColor(settings.colorMajor);
                color2->blockSignals(false);
            }
        });
    } else {
        const auto act = new QWidgetAction(this);
        const auto wid = new QWidget(this);
        const auto lay = new QHBoxLayout(wid);
        const auto spin = new QSpinBox(wid);

        QString labelText;
        spin->setRange(0, 9999);

        switch (option) {
        case Core::Grid::Option::SizeX:
            spin->setValue(settings.sizeX);
            labelText = tr("Size X");
            break;
        case Core::Grid::Option::SizeY:
            spin->setValue(settings.sizeY);
            labelText = tr("Size Y");
            break;
        case Core::Grid::Option::OriginX:
            spin->setValue(settings.originX);
            labelText = tr("Origin X");
            break;
        case Core::Grid::Option::OriginY:
            spin->setValue(settings.originY);
            labelText = tr("Origin Y");
            break;
        case Core::Grid::Option::MajorEveryX:
            spin->setValue(settings.majorEveryX);
            labelText = tr("Major X");
            break;
        case Core::Grid::Option::MajorEveryY:
            spin->setValue(settings.majorEveryY);
            labelText = tr("Major Y");
            break;
        default:
            qWarning() << "Unknown Grid Option!" << (int)option;
        }

        const auto label = new QLabel(labelText, wid);

        wid->setContentsMargins(0, 0, 0, 0);
        lay->setContentsMargins(5, 2, 5, 2);

        lay->addWidget(label);
        lay->addWidget(spin);

        act->setDefaultWidget(wid);
        button->menu()->addAction(act);

        connect(spin, qOverload<int>(&QSpinBox::valueChanged),
                this, [grid, option] (const int value){
            grid->setOption(option, value, false);
        });
        connect(grid, &Core::Grid::changed,
                this, [spin, option] (const Core::Grid::Settings &settings) {
            int value = -1;
            switch (option) {
            case Core::Grid::Option::SizeX:
                value = settings.sizeX;
                break;
            case Core::Grid::Option::SizeY:
                value = settings.sizeY;
                break;
            case Core::Grid::Option::OriginX:
                value = settings.originX;
                break;
            case Core::Grid::Option::OriginY:
                value = settings.originY;
                break;
            case Core::Grid::Option::MajorEveryX:
                value = settings.majorEveryX;
                break;
            case Core::Grid::Option::MajorEveryY:
                value = settings.majorEveryY;
                break;
            default: return;
            }
            if (value == spin->value()) { return; }
            spin->blockSignals(true);
            spin->setValue(value);
            spin->blockSignals(false);
        });
    }
}
