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
#include <QDBusMetaType>
#include <QDebug>

using namespace Friction::Core;

struct PortalRGB
{
    double r, g, b;
};
Q_DECLARE_METATYPE(PortalRGB)

const QDBusArgument &operator>>(const QDBusArgument &argument, PortalRGB &color)
{
    argument.beginStructure();
    argument >> color.r >> color.g >> color.b;
    argument.endStructure();
    return argument;
}

QDBusArgument &operator<<(QDBusArgument &argument, const PortalRGB &color)
{
    argument.beginStructure();
    argument << color.r << color.g << color.b;
    argument.endStructure();
    return argument;
}

PortalAppearance::PortalAppearance(QObject *parent)
    : QObject(parent)
    , m_defaultColor(ThemeSupport::getThemeHighlightColor())
{
    qDBusRegisterMetaType<PortalRGB>();
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
    while (val.userType() == qMetaTypeId<QDBusVariant>()) {
        val = val.value<QDBusVariant>().variant();
    }

    if (!val.isValid()) { return m_defaultColor; }

    if (val.userType() == qMetaTypeId<QDBusArgument>()) {
        QDBusArgument arg = val.value<QDBusArgument>();
        if (arg.currentSignature() == QStringLiteral("(ddd)")) {
            PortalRGB color;
            arg >> color;
            return QColor::fromRgbF(qBound(0.0, color.r, 1.0),
                                    qBound(0.0, color.g, 1.0),
                                    qBound(0.0, color.b, 1.0));
        }
    }

    return m_defaultColor;
}

#endif // HAS_DBUS
