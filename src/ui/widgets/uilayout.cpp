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

#include "uilayout.h"
#include "appsupport.h"
#include "themesupport.h"
#include "Private/esettings.h"

#include <QLabel>
#include <QPushButton>
#include <QHBoxLayout>
#include <QDebug>

#define UI_CONF_GROUP "uiLayout"
#define UI_CONF_KEY_POS "pos_%1"
#define UI_CONF_KEY_INDEX "index_%1"
#define UI_CONF_KEY_MAIN "uiMain"
#define UI_CONF_KEY_LEFT "uiLeft"
#define UI_CONF_KEY_MIDDLE "uiMiddle"
#define UI_CONF_KEY_RIGHT "uiRight"
#define UI_CONF_KEY_TOP "uiTop"
#define UI_CONF_KEY_BOTTOM "uiBottom"

UIDock::UIDock(QWidget *parent,
               QWidget *widget,
               const QString &label,
               const Position &pos,
               const bool &showLabel,
               const bool &showHeader,
               const bool &darkHeader)
    : QWidget{parent}
    , mLayout(nullptr)
    , mLabel(label)
    , mPos(pos)
    , mIndex(-1)
{
    setObjectName(mLabel);
    setContentsMargins(0, 0, 0, 0);
    mLayout = new QVBoxLayout(this);
    mLayout->setContentsMargins(0, 0, 0, 0);
    mLayout->setMargin(0);
    mLayout->setSpacing(0);

    if (showHeader) {
        const auto headerWidget = new QWidget(this);
        const auto headerLayout = new QHBoxLayout(headerWidget);

        if (darkHeader) {
            headerWidget->setAutoFillBackground(true);
            headerWidget->setPalette(ThemeSupport::getDarkPalette());
        }

        headerWidget->setContentsMargins(0, 0, 0, 0);
        headerWidget->setSizePolicy(QSizePolicy::Expanding,
                                    QSizePolicy::Fixed);
        headerLayout->setContentsMargins(0, 0, 0, 0);
        headerLayout->setMargin(0);
        headerLayout->setSpacing(0);

        const auto leftButton = new QPushButton(this);
        const auto rightButton = new QPushButton(this);
        const auto upButton = new QPushButton(this);
        const auto downButton = new QPushButton(this);

        leftButton->setIcon(QIcon::fromTheme("leftarrow"));
        rightButton->setIcon(QIcon::fromTheme("rightarrow"));
        upButton->setIcon(QIcon::fromTheme("uparrow"));
        downButton->setIcon(QIcon::fromTheme("downarrow"));

        leftButton->setFocusPolicy(Qt::NoFocus);
        rightButton->setFocusPolicy(Qt::NoFocus);
        upButton->setFocusPolicy(Qt::NoFocus);
        downButton->setFocusPolicy(Qt::NoFocus);

        leftButton->setObjectName("FlatButton");
        rightButton->setObjectName("FlatButton");
        upButton->setObjectName("FlatButton");
        downButton->setObjectName("FlatButton");

        leftButton->setToolTip(tr("Left"));
        rightButton->setToolTip(tr("Right"));
        upButton->setToolTip(tr("Up"));
        downButton->setToolTip(tr("Down"));

        leftButton->setSizePolicy(QSizePolicy::Fixed,
                                  QSizePolicy::Fixed);
        rightButton->setSizePolicy(QSizePolicy::Fixed,
                                   QSizePolicy::Fixed);
        upButton->setSizePolicy(QSizePolicy::Fixed,
                                QSizePolicy::Fixed);
        downButton->setSizePolicy(QSizePolicy::Fixed,
                                  QSizePolicy::Fixed);

        headerLayout->addWidget(leftButton);
        headerLayout->addWidget(rightButton);
        headerLayout->addStretch();

        if (showLabel) {
            const auto dockLabel = new QLabel(this);
            dockLabel->setText(mLabel);
            headerLayout->addWidget(dockLabel);
            headerLayout->addStretch();
        }

        headerLayout->addWidget(upButton);
        headerLayout->addWidget(downButton);

        mLayout->addWidget(headerWidget);

        connect(leftButton, &QPushButton::clicked,
                this, [this]() { emit changePosition(mPos, Position::Left); });
        connect(rightButton, &QPushButton::clicked,
                this, [this]() { emit changePosition(mPos, Position::Right); });
        connect(upButton, &QPushButton::clicked,
                this, [this]() { emit changePosition(mPos, Position::Up); });
        connect(downButton, &QPushButton::clicked,
                this, [this]() { emit changePosition(mPos, Position::Down); });
    }
    mLayout->addWidget(widget);
}

UIDock::~UIDock()
{
    writeSettings();
}

void UIDock::setPosition(const Position &pos)
{
    mPos = pos;
}

UIDock::Position UIDock::getPosition()
{
    return mPos;
}

void UIDock::setIndex(const int &index)
{
    mIndex = index;
}

int UIDock::getIndex()
{
    return mIndex;
}

const QString UIDock::getLabel()
{
    return mLabel;
}

const QString UIDock::getId()
{
    return AppSupport::filterTextAZW(mLabel);
}

void UIDock::addWidget(QWidget *widget)
{
    mLayout->addWidget(widget);
}

void UIDock::writeSettings()
{
    qDebug() << "==> write dock conf" << mLabel << mPos << mIndex;
    if (eSettings::instance().fRestoreDefaultUi) {
        AppSupport::clearSettings(UI_CONF_GROUP);
        return;
    }
    AppSupport::setSettings(UI_CONF_GROUP,
                            QString(UI_CONF_KEY_POS).arg(getId()),
                            mPos);
    AppSupport::setSettings(UI_CONF_GROUP,
                            QString(UI_CONF_KEY_INDEX).arg(getId()),
                            mIndex);
}

UILayout::UILayout(QWidget *parent)
    : QSplitter{parent}
    , mLeft(nullptr)
    , mMiddle(nullptr)
    , mRight(nullptr)
    , mTop(nullptr)
    , mBottom(nullptr)
{
    setOrientation(Qt::Horizontal);

    mLeft = new QSplitter(this);
    mLeft->setOrientation(Qt::Vertical);
    addWidget(mLeft);

    mMiddle = new QSplitter(this);
    mMiddle->setOrientation(Qt::Vertical);
    addWidget(mMiddle);

    mRight = new QSplitter(this);
    mRight->setOrientation(Qt::Vertical);
    addWidget(mRight);

    mTop = new QSplitter(this);
    mTop->setOrientation(Qt::Horizontal);
    mMiddle->addWidget(mTop);

    mBottom = new QSplitter(this);
    mBottom->setOrientation(Qt::Horizontal);
    mMiddle->addWidget(mBottom);

    setCollapsible(indexOf(mLeft), false);
    setCollapsible(indexOf(mMiddle), false);
    setCollapsible(indexOf(mRight), false);

    mMiddle->setCollapsible(mMiddle->indexOf(mTop), false);
    mMiddle->setCollapsible(mMiddle->indexOf(mBottom), false);
}

UILayout::~UILayout()
{
    updateDocks();
    writeSettings();
}

void UILayout::readSettings()
{
    bool firstrun = AppSupport::getSettings(UI_CONF_GROUP, UI_CONF_KEY_MAIN).isNull();
    restoreState(AppSupport::getSettings(UI_CONF_GROUP, UI_CONF_KEY_MAIN).toByteArray());
    mLeft->restoreState(AppSupport::getSettings(UI_CONF_GROUP, UI_CONF_KEY_LEFT).toByteArray());
    mMiddle->restoreState(AppSupport::getSettings(UI_CONF_GROUP, UI_CONF_KEY_MIDDLE).toByteArray());
    mRight->restoreState(AppSupport::getSettings(UI_CONF_GROUP, UI_CONF_KEY_RIGHT).toByteArray());
    mTop->restoreState(AppSupport::getSettings(UI_CONF_GROUP, UI_CONF_KEY_TOP).toByteArray());
    mBottom->restoreState(AppSupport::getSettings(UI_CONF_GROUP, UI_CONF_KEY_BOTTOM).toByteArray());

    if (firstrun) {
        setSizes({300, 1024, 300});
    }
}

void UILayout::writeSettings()
{
    AppSupport::setSettings(UI_CONF_GROUP, UI_CONF_KEY_MAIN, saveState());
    AppSupport::setSettings(UI_CONF_GROUP, UI_CONF_KEY_LEFT, mLeft->saveState());
    AppSupport::setSettings(UI_CONF_GROUP, UI_CONF_KEY_MIDDLE, mMiddle->saveState());
    AppSupport::setSettings(UI_CONF_GROUP, UI_CONF_KEY_RIGHT, mRight->saveState());
    AppSupport::setSettings(UI_CONF_GROUP, UI_CONF_KEY_TOP, mTop->saveState());
    AppSupport::setSettings(UI_CONF_GROUP, UI_CONF_KEY_BOTTOM, mBottom->saveState());
}

void UILayout::addDocks(std::vector<Item> items)
{
    if (items.size() <= 0) { return; }
    std::vector<Item> leftDock, rightDock, topDock, bottomDock;

    for (auto item : items) {
        if (!item.widget) { continue; }
        qDebug() << "==> setup new dock" << item.label;
        QString keyPos = QString(UI_CONF_KEY_POS).arg(AppSupport::filterTextAZW(item.label));
        QString keyIndex = QString(UI_CONF_KEY_INDEX).arg(AppSupport::filterTextAZW(item.label));

        const auto confIndex = AppSupport::getSettings(UI_CONF_GROUP, keyIndex);
        if (confIndex.isValid()) { item.index = confIndex.toInt(); }

        const auto confPos = AppSupport::getSettings(UI_CONF_GROUP, keyPos);
        if (confPos.isValid()) { item.pos = confPos.toInt(); }

        switch (item.pos) {
        case UIDock::Position::Left:
            leftDock.push_back(item);
            break;
        case UIDock::Position::Right:
            rightDock.push_back(item);
            break;
        case UIDock::Position::Up:
            topDock.push_back(item);
            break;
        case UIDock::Position::Down:
            bottomDock.push_back(item);
            break;
        }
    }

    if (leftDock.size() > 0) { std::sort(leftDock.begin(), leftDock.end()); }
    if (rightDock.size() > 0) { std::sort(rightDock.begin(), rightDock.end()); }
    if (topDock.size() > 0) { std::sort(topDock.begin(), topDock.end()); }
    if (bottomDock.size() > 0) { std::sort(bottomDock.begin(), bottomDock.end()); }

    for (const auto &item : leftDock) {
        qDebug() << "==> add to left dock" << item.label << item.index;
        addDock(item);
    }
    for (const auto &item : rightDock) {
        qDebug() << "==> add to right dock" << item.label << item.index;
        addDock(item);
    }
    for (const auto &item : topDock) {
        qDebug() << "==> add to top dock" << item.label << item.index;
        addDock(item);
    }
    for (const auto &item : bottomDock) {
        qDebug() << "==> add to bottom dock" << item.label << item.index;
        addDock(item);
    }

    updateDocks();
    readSettings();
}

void UILayout::setDockVisible(const QString &label,
                              bool visible)
{
    emit updateDockVisibility(label, visible);
}

void UILayout::addDockWidget(const QString &label, QWidget *widget)
{
    if (!widget) { return; }
    emit updateDockWidget(label, widget);
}

void UILayout::addDock(const Item &item)
{
    if (!item.widget) { return; }
    qDebug() << "==> adding dock" << item.label << item.pos << item.index;
    const auto dock = new UIDock(this,
                                 item.widget,
                                 item.label,
                                 static_cast<UIDock::Position>(item.pos),
                                 item.showLabel,
                                 item.showHeader,
                                 item.darkHeader);
    switch (item.pos) {
    case UIDock::Position::Left:
        mLeft->addWidget(dock);
        mLeft->setCollapsible(mLeft->indexOf(dock), false);
        break;
    case UIDock::Position::Right:
        mRight->addWidget(dock);
        mRight->setCollapsible(mRight->indexOf(dock), false);
        break;
    case UIDock::Position::Up:
        mTop->addWidget(dock);
        mTop->setCollapsible(mTop->indexOf(dock), false);
        break;
    case UIDock::Position::Down:
        mBottom->addWidget(dock);
        mBottom->setCollapsible(mBottom->indexOf(dock), false);
        break;
    }
    connectDock(dock);
}

void UILayout::connectDock(UIDock *dock)
{
    if (!dock) { return; }
    connect(this,
            &UILayout::updateDockVisibility,
            this,
            [dock](const QString &label,
                   bool visible) {
                if (dock->getLabel() == label) { dock->setVisible(visible); }
            });
    connect(this,
            &UILayout::updateDockWidget,
            this,
            [dock](const QString &label,
                   QWidget *widget) {
                if (dock->getLabel() == label) {
                    dock->addWidget(widget);
                    dock->setVisible(true);
                }
            });
    connect(dock,
            &UIDock::changePosition,
            this,
            [this, dock](const UIDock::Position &pos,
                         const UIDock::Position &trigger)
            {
                int index = -1;
                int count = -1;
                switch (pos) {
                case UIDock::Position::Left:
                    index = mLeft->indexOf(dock);
                    count = mLeft->count();
                    break;
                case UIDock::Position::Right:
                    index = mRight->indexOf(dock);
                    count = mRight->count();
                    break;
                case UIDock::Position::Up:
                    index = mTop->indexOf(dock);
                    count = mTop->count();
                    break;
                case UIDock::Position::Down:
                    index = mBottom->indexOf(dock);
                    count = mBottom->count();
                    break;
                }
                if (index < 0 || count <= 0) { return; }
                switch (pos) {
                case UIDock::Position::Left:
                    if (trigger == UIDock::Position::Down && (index + 1) < count) {
                        mLeft->insertWidget(index + 1, dock);
                    } else if (trigger == UIDock::Position::Up && index > 0) {
                        mLeft->insertWidget(index - 1, dock);
                    } else if (trigger == UIDock::Position::Right) {
                        mTop->insertWidget(0, dock);
                    } else if (trigger == UIDock::Position::Left) {
                        mRight->addWidget(dock);
                    }
                    break;
                case UIDock::Position::Right:
                    if (trigger == UIDock::Position::Down && (index + 1) < count) {
                        mRight->insertWidget(index + 1, dock);
                    } else if (trigger == UIDock::Position::Up && index > 0) {
                        mRight->insertWidget(index - 1, dock);
                    } else if (trigger == UIDock::Position::Right) {
                        mLeft->addWidget(dock);
                    } else if (trigger == UIDock::Position::Left) {
                        mTop->addWidget(dock);
                    }
                    break;
                case UIDock::Position::Up:
                    if (trigger == UIDock::Position::Right && (index + 1) < count) {
                        mTop->insertWidget(index + 1, dock);
                    } else if (trigger == UIDock::Position::Right && (index + 1) >= count) {
                        mRight->insertWidget(index + 1, dock);
                    } else if (trigger == UIDock::Position::Left && index > 0) {
                        mTop->insertWidget(index - 1, dock);
                    } else if (trigger == UIDock::Position::Left && index == 0) {
                        mLeft->insertWidget(index - 1, dock);
                    } else if (trigger == UIDock::Position::Down && index == 0) {
                        mBottom->insertWidget(0, dock);
                    } else if (trigger == UIDock::Position::Down) {
                        mBottom->addWidget(dock);
                    }
                    break;
                case UIDock::Position::Down:
                    if (trigger == UIDock::Position::Right && (index + 1) < count) {
                        mBottom->insertWidget(index + 1, dock);
                    } else if (trigger == UIDock::Position::Right && (index + 1) >= count) {
                        mRight->insertWidget(index + 1, dock);
                    } else if (trigger == UIDock::Position::Left && index > 0) {
                        mBottom->insertWidget(index - 1, dock);
                    } else if (trigger == UIDock::Position::Left && index == 0) {
                        mLeft->insertWidget(index - 1, dock);
                    } else if (trigger == UIDock::Position::Up && index == 0) {
                        mTop->insertWidget(0, dock);
                    } else if (trigger == UIDock::Position::Up) {
                        mTop->addWidget(dock);
                    }
                    break;
                }
                updateDocks();
            });
}

void UILayout::updateDock(QSplitter *container,
                          const UIDock::Position &pos)
{
    if (!container) { return; }
    for (int i = 0; i < container->count(); ++i) {
        UIDock *dock = qobject_cast<UIDock*>(container->widget(i));
        if (!dock) { continue; }
        container->setCollapsible(container->indexOf(dock), false);
        dock->setPosition(pos);
        dock->setIndex(container->indexOf(dock));
        qDebug() << "==> update dock" << dock->getLabel() << dock->getPosition() << dock->getIndex();
    }
}

void UILayout::updateDocks()
{
    updateDock(mLeft, UIDock::Position::Left);
    updateDock(mRight, UIDock::Position::Right);
    updateDock(mTop, UIDock::Position::Up);
    updateDock(mBottom, UIDock::Position::Down);
}
