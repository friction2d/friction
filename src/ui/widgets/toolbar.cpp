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

#include "toolbar.h"
#include "GUI/global.h"
#include "appsupport.h"

#include <QMenu>

using namespace Friction::Ui;

ToolBar::ToolBar(const QString &title,
                 QWidget *parent,
                 const bool &iconsOnly)
    : QToolBar(title, parent)
    , mIconsOnly(iconsOnly)
{
    setup();
}

ToolBar::ToolBar(const QString &title,
                 const QString &objectName,
                 QWidget *parent,
                 const bool iconsOnly)
    : QToolBar(title, parent)
    , mIconsOnly(iconsOnly)
{
    setObjectName(objectName);
    setup();
}

void ToolBar::updateActions()
{
    const auto disabled = AppSupport::getSettings("ui",
                                                  QString("ToolBarActionsDisabled_%1")
                                                      .arg(objectName())).toStringList();
    for (const auto &act : actions()) {
        if (act->objectName().isEmpty()) { continue; }
        if (disabled.contains(act->objectName())) {
            act->setVisible(false);
        }
    }
}

QAction *ToolBar::addSpacer(const bool &horizontal,
                            const bool &fixed,
                            const int &width,
                            const int &height)
{
    const auto widget = new QWidget(this);
    if (fixed) {
        if (width > 0 || height > 0) {
            if (width > 0) { widget->setFixedWidth(width); }
            if (height > 0) { widget->setFixedHeight(height); }
        }
        else { widget->setProperty("isFixedSpacer", true); };
    }
    else {
        widget->setSizePolicy(horizontal ? QSizePolicy::Expanding : QSizePolicy::Minimum,
                              horizontal ? QSizePolicy::Minimum : QSizePolicy::Expanding);
    }
    return addWidget(widget);
}

void ToolBar::setup()
{
#ifdef Q_OS_MAC
    setStyleSheet(QString("font-size: %1pt;").arg(font().pointSize()));
#endif

    eSizesUI::widget.add(this, [this](const int size) {
        this->setIconSize({size, size});
    });

    if (objectName().isEmpty()) {
        setObjectName(windowTitle().replace(" ", "").simplified());
    }

    mIconsOnly = AppSupport::getSettings("ui",
                                         QString("ToolBarIconsOnly_%1").arg(objectName()),
                                                                            mIconsOnly).toBool();
    setMovable(AppSupport::getSettings("ui",
                                       QString("ToolBarMovable_%1").arg(objectName()),
                                       false).toBool());
    setFocusPolicy(Qt::NoFocus);

    setToolButtonStyle(mIconsOnly ? Qt::ToolButtonIconOnly : Qt::ToolButtonTextBesideIcon);
    connect(this, &QToolBar::orientationChanged,
            this, [this]() {
        if (mIconsOnly) { return; }
        setToolButtonStyle(orientation() == Qt::Horizontal ?
                               Qt::ToolButtonTextBesideIcon :
                               Qt::ToolButtonIconOnly);
    });

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested,
            this, &ToolBar::showContextMenu);

    eSizesUI::widget.add(this, [this](const int size) {
        this->setIconSize({size, size});
    });
}

void ToolBar::showContextMenu(const QPoint &pos)
{
    QMenu menu(this);
    {
        const auto act = menu.addAction(QIcon::fromTheme("window"), windowTitle());
        act->setEnabled(false);
        menu.addSeparator();
    }

    menu.addAction(QIcon::fromTheme(isMovable() ? "locked" : "unlocked"),
                   tr(isMovable() ? "Lock" : "Unlock"),
                   this, [this]() {
        setMovable(!isMovable());
        AppSupport::setSettings("ui",
                                QString("ToolBarMovable_%1").arg(objectName()),
                                isMovable());
    });

    {
        const auto act = menu.addAction(tr("Labels"));
        act->setCheckable(true);
        act->setChecked(!mIconsOnly);
        connect(act, &QAction::triggered,
                this, [this](bool checked) {
            mIconsOnly = !checked;
            setToolButtonStyle(mIconsOnly ?
                                   Qt::ToolButtonIconOnly :
                                   Qt::ToolButtonTextBesideIcon);
            update();
            AppSupport::setSettings("ui",
                                    QString("ToolBarIconsOnly_%1").arg(objectName()),
                                    mIconsOnly);
        });
    }

    const auto disabled = AppSupport::getSettings("ui",
                                                  QString("ToolBarActionsDisabled_%1")
                                                      .arg(objectName())).toStringList();
    const auto acts = actions();
    if (acts.count() > 0) { menu.addSeparator(); }

    for (const auto &act : acts) {
        if (act->objectName().isEmpty()) { continue; }
        const auto mAct = menu.addAction(act->text());
        mAct->setCheckable(true);
        mAct->setData(act->objectName());
        mAct->setChecked(!disabled.contains(act->objectName()));
        connect(mAct, &QAction::triggered,
                this, [this, mAct](bool checked) {
            setEnableAction(mAct->data().toString(), checked);
        });
    }

    menu.exec(mapToGlobal(pos));
}

QAction* ToolBar::addSpacer(QWidget *widget)
{
    if (!widget) { return nullptr; }
    widget->setSizePolicy(QSizePolicy::Expanding,
                          QSizePolicy::Minimum);
    return addWidget(widget);
}

void ToolBar::setEnableAction(const QString &title,
                              const bool &enable)
{
    for (const auto &act : actions()) {
        if (act->objectName().isEmpty()) { continue; }
        if (act->objectName() == title) {
            act->setVisible(enable);
            QStringList disabled = AppSupport::getSettings("ui",
                                                           QString("ToolBarActionsDisabled_%1")
                                                               .arg(objectName())).toStringList();
            if (enable && disabled.contains(title)) {
                disabled.removeAll(title);
            } else if (!enable && !disabled.contains(title)) {
                disabled << title;
            } else { break; }
            AppSupport::setSettings("ui",
                                    QString("ToolBarActionsDisabled_%1").arg(objectName()),
                                    disabled,
                                    false);
            break;
        }
    }
}
