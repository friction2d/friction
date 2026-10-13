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

#ifndef FRICTION_PORTAL_APPEARANCE_H
#define FRICTION_PORTAL_APPEARANCE_H

#ifdef HAS_DBUS

#include <QObject>
#include <QColor>
#include <QDBusVariant>

#include "core_global.h"

namespace Friction
{
    namespace Core
    {
        class CORE_EXPORT PortalAppearance : public QObject
        {
            Q_OBJECT

        public:
            explicit PortalAppearance(QObject *parent = nullptr);
            virtual ~PortalAppearance() = default;

            QColor currentAccentColor() const;

        signals:
            void accentChanged(const QColor &newColor);

        private slots:
            void onSettingChanged(const QString &namespaceName,
                                  const QString &key,
                                  const QDBusVariant &value);

        private:
            QColor parseAccentColor(const QVariant &variant) const;
            const QColor m_defaultColor;
        };
    }
}

#endif // HAS_DBUS
#endif // FRICTION_PORTAL_APPEARANCE_H
