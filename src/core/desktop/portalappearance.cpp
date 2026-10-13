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

#ifdef HAS_DBUS

#include "portalappearance.h"
#include "themesupport.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusArgument>
#include <QDebug>

using namespace Friction::Core;

PortalAppearance::PortalAppearance(QObject *parent)
    : QObject(parent)
    , m_defaultColor(ThemeSupport::getThemeHighlightColor())
{
    bool connected = QDBusConnection::sessionBus().connect(QStringLiteral("org.freedesktop.portal.Desktop"),
                                                           QStringLiteral("/org/freedesktop/portal/desktop"),
                                                           QStringLiteral("org.freedesktop.portal.Settings"),
                                                           QStringLiteral("SettingChanged"),
                                                           this,
                                                           SLOT(onSettingChanged(QString,
                                                                                 QString,
                                                                                 QDBusVariant)));

    if (!connected) {
        qWarning() << "Failed to connect to XDG Portal SettingChanged.";
    }
}

QColor PortalAppearance::currentAccentColor() const
{
    QDBusInterface portal(QStringLiteral("org.freedesktop.portal.Desktop"),
                          QStringLiteral("/org/freedesktop/portal/desktop"),
                          QStringLiteral("org.freedesktop.portal.Settings"),
                          QDBusConnection::sessionBus());

    if (!portal.isValid()) {
        qWarning() << "Failed to connect to XDG Portal Settings";
        return m_defaultColor;
    }

    QDBusReply<QDBusVariant> reply = portal.call(QStringLiteral("ReadOne"),
                                                 QStringLiteral("org.freedesktop.appearance"),
                                                 QStringLiteral("accent-color"));

    if (reply.isValid()) {
        return parseAccentColor(reply.value().variant());
    }

    return m_defaultColor;
}

void PortalAppearance::onSettingChanged(const QString &namespaceName,
                                        const QString &key,
                                        const QDBusVariant &value)
{
    // We only care about accent-color changes (for now)
    if (namespaceName == QStringLiteral("org.freedesktop.appearance") &&
        key == QStringLiteral("accent-color")) {
        QColor newColor = parseAccentColor(value.variant());
        qDebug() << "org.freedesktop.appearance accent-color changed" << newColor;
        emit accentChanged(newColor);
    }
}

QColor PortalAppearance::parseAccentColor(const QVariant &variant) const
{
    if (!variant.isValid()) { return m_defaultColor; }
    QVariant val = variant;
    if (val.canConvert<QDBusVariant>()) {
        val = val.value<QDBusVariant>().variant();
    }

    if (val.canConvert<QDBusArgument>()) {
        QDBusArgument arg = val.value<QDBusArgument>();
        if (arg.currentType() == QDBusArgument::StructureType) {
            double r = 0.0, g = 0.0, b = 0.0;
            const QDBusArgument copy(arg);
            const_cast<QDBusArgument&>(arg).beginStructure();
            const_cast<QDBusArgument&>(arg) >> r >> g >> b;
            const_cast<QDBusArgument&>(arg).endStructure();

            if (r >= 0.0 && r <= 1.0 &&
                g >= 0.0 && g <= 1.0 &&
                b >= 0.0 && b <= 1.0) {
                return QColor::fromRgbF(r, g, b);
            }
        } else if (arg.currentType() == QDBusArgument::ArrayType) {
            QList<double> rgbValues;
            arg >> rgbValues;
            if (rgbValues.size() >= 3) {
                if (rgbValues[0] >= 0.0 && rgbValues[0] <= 1.0 &&
                    rgbValues[1] >= 0.0 && rgbValues[1] <= 1.0 &&
                    rgbValues[2] >= 0.0 && rgbValues[2] <= 1.0) {
                    return QColor::fromRgbF(rgbValues.at(0),
                                            rgbValues.at(1),
                                            rgbValues.at(2));
                }
            }
        }
    }

    return m_defaultColor;
}

#endif // HAS_DBUS
