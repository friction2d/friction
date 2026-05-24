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

#include "window.h"
#include "appsupport.h"

#include <QDebug>
#include <QLoggingCategory>
#include <QWindow>

#include "mainwindow.h"

Q_LOGGING_CATEGORY(lcUiWindow, "friction.ui.window", QtWarningMsg)

Window::Window(QWidget *parent,
               QWidget *child,
               const QString title,
               const QString id,
               bool visible,
               bool blockEscKey,
               bool forwardKeys)
    : QDialog(parent)
    , mLayout(nullptr)
    , mBlockEscKey(blockEscKey)
    , mForwardKeys(forwardKeys)
{
    setWindowFlags(Qt::Window |
                   Qt::WindowTitleHint |
                   Qt::WindowSystemMenuHint |
                   Qt::WindowMinimizeButtonHint |
                   Qt::WindowMaximizeButtonHint |
                   Qt::WindowCloseButtonHint);
    setWindowTitle(title);
    setObjectName(id);

    setContentsMargins(0, 0, 0, 0);
    mLayout = new QVBoxLayout(this);
    mLayout->setMargin(0);
    mLayout->addWidget(child);

    loadState(visible);
}

Window::~Window()
{
    //saveState();
}

void Window::focusWindow()
{
    if (!isVisible()) { show(); }
    if (windowHandle()) { windowHandle()->requestActivate(); }
}

void Window::addWidget(QWidget *child)
{
    if (!child) { return; }
    mLayout->addWidget(child);
}

void Window::keyPressEvent(QKeyEvent *event)
{
    if (mForwardKeys) {
        MainWindow::sGetInstance()->processKeyEvent(event);
    }
    if (mBlockEscKey && event->key() == Qt::Key_Escape) { return; }
    QDialog::keyPressEvent(event);
}

void Window::closeEvent(QCloseEvent *event)
{
    qCDebug(lcUiWindow) << "closed window" << objectName();
    saveState();
    emit closed();
    QDialog::closeEvent(event);
}

void Window::loadState(bool visible)
{
    qCDebug(lcUiWindow) << "load window state" << objectName();
    restoreGeometry(AppSupport::getSettings("ui",
                                            QString("%1Geometry")
                                            .arg(objectName())).toByteArray());
    bool isMax = AppSupport::getSettings("ui",
                                         QString("%1Maximized").arg(objectName()),
                                         false).toBool();
    if (!visible) { return; }
    if (isMax) { showMaximized(); }
    else { showNormal(); }
}

void Window::saveState()
{
    qCDebug(lcUiWindow) << "save window state" << objectName();
    AppSupport::setSettings("ui",
                            QString("%1Geometry").arg(objectName()),
                            saveGeometry());
    AppSupport::setSettings("ui",
                            QString("%1Maximized").arg(objectName()),
                            isMaximized());
}
