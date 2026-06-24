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

#ifndef FRICTION_TOOLBUTTON_H
#define FRICTION_TOOLBUTTON_H

#include "ui_global.h"
#include "enterevent.h"

#include <QToolButton>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QMimeData>
#include <QList>
#include <QUrl>

namespace Friction
{
    namespace Ui
    {
        class UI_EXPORT ToolButton : public QToolButton
        {
            Q_OBJECT
        public:
            explicit ToolButton(QWidget *parent = nullptr,
                                bool autoPopup = true)
            : QToolButton(parent)
            , mAutoPopup(autoPopup)
            {

            }

        public slots:
            void setAutoPopup(const bool &state)
            {
                mAutoPopup = state;
            }

        signals:
            void droppedUrls(const QList<QUrl> &urls);

        private:
            bool mAutoPopup;

        protected:
            void enterEvent(QtEnterEvent *event) override
            {
                if (mAutoPopup) { showMenu(); }
                QToolButton::enterEvent(event);
            }
            void dropEvent(QDropEvent *event) override
            {
                if (event->mimeData()->hasUrls()) {
                    emit droppedUrls(event->mimeData()->urls());
                    event->acceptProposedAction();
                }
            }
            void dragEnterEvent(QDragEnterEvent *event) override
            {
                if (event->mimeData()->hasUrls()) { event->acceptProposedAction(); }
            }
            void dragMoveEvent(QDragMoveEvent *event) override
            {
                event->acceptProposedAction();
            }
        };
    }
}

#endif // FRICTION_TOOLBUTTON_H
