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

#ifndef FRICTION_UNITY_LAUNCHER_ENTRY_H
#define FRICTION_UNITY_LAUNCHER_ENTRY_H

#include "ui_global.h"

#include <QObject>
#include <QString>

namespace Friction
{
    namespace Ui
    {
        class UI_EXPORT UnityLauncherEntry : public QObject
        {
            Q_OBJECT
        public:
            explicit UnityLauncherEntry(const QString &desktopFileName,
                                        QObject *parent = nullptr);

        public slots:
            void setProgress(double progress);
            void setProgressVisible(bool visible);
            void setCount(qint64 count);
            void setCountVisible(bool visible);
            void setUrgent(bool urgent);

        private:
            void sendUpdate();

            QString mAppUri;
            double mProgress;
            bool mProgressVisible;
            qint64 mCount;
            bool mCountVisible;
            bool mUrgent;
        };
    }
}

#endif // FRICTION_UNITY_LAUNCHER_ENTRY_H
