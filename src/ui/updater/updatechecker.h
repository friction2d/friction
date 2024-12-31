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

#ifndef FRICTION_UPDATE_CHECKER_H
#define FRICTION_UPDATE_CHECKER_H

#include "ui_global.h"

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace Friction
{
    namespace Ui
    {
        class UI_EXPORT UpdateChecker : public QObject
        {
            Q_OBJECT
        public:
            struct fVersion {
                int major;
                int minor;
                int patch;
                double micro;
            };

            explicit UpdateChecker(QObject *parent = nullptr);

            void check();

        private:
            QNetworkAccessManager *mNet;

            bool hasNewVersion(const fVersion &from,
                               const fVersion &to);

            const QString parseVersionXml(const QString &xml,
                                          const bool &devel = false);
            const fVersion parseVersion(const QString &version);

            void handleReply(QNetworkReply *reply);

            void checkXml(const QString &xml);

            bool allowed();
            void checkDate();
        };
    }
}

#endif // FRICTION_UPDATE_CHECKER_H
