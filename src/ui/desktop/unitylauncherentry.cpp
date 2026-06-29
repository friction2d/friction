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

#include "unitylauncherentry.h"

#ifdef HAS_DBUS
#include <QDBusMessage>
#include <QDBusConnection>
#endif

using namespace Friction::Ui;

UnityLauncherEntry::UnityLauncherEntry(const QString &desktopFileName,
                                       QObject *parent)
    : QObject(parent)
    , mProgress(0.0)
    , mProgressVisible(false)
    , mCount(0)
    , mCountVisible(false)
    , mUrgent(false)
{
    mAppUri = QStringLiteral("application://%1").arg(desktopFileName);
}

void UnityLauncherEntry::setProgress(double progress)
{
    const double clampedProgress = qBound(0.0, progress, 1.0);
    if (qFuzzyCompare(mProgress, clampedProgress)) { return; }

    mProgress = clampedProgress;
    sendUpdate();
}

void UnityLauncherEntry::setProgressVisible(bool visible)
{
    if (mProgressVisible == visible) { return; }
    mProgressVisible = visible;
    sendUpdate();
}

void UnityLauncherEntry::setCount(qint64 count)
{
    if (mCount == count) { return; }
    mCount = count;
    sendUpdate();
}

void UnityLauncherEntry::setCountVisible(bool visible)
{
    if (mCountVisible == visible) { return; }
    mCountVisible = visible;
    sendUpdate();
}

void UnityLauncherEntry::setUrgent(bool urgent)
{
    if (mUrgent == urgent) { return; }
    mUrgent = urgent;
    sendUpdate();
}

void UnityLauncherEntry::sendUpdate()
{
#ifdef HAS_DBUS
    QVariantMap properties;
    properties.insert(QStringLiteral("progress"), mProgress);
    properties.insert(QStringLiteral("progress-visible"), mProgressVisible);
    properties.insert(QStringLiteral("count"), mCount);
    properties.insert(QStringLiteral("count-visible"), mCountVisible);
    properties.insert(QStringLiteral("urgent"), mUrgent);

    QDBusMessage message = QDBusMessage::createSignal(
        QStringLiteral("/com/canonical/unity/launcherentry/1"),
        QStringLiteral("com.canonical.Unity.LauncherEntry"),
        QStringLiteral("Update")
    );

    message << mAppUri << properties;
    QDBusConnection::sessionBus().send(message);
#endif
}
