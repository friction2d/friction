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

#include "updatechecker.h"
#include "appsupport.h"

#include <QXmlStreamReader>
#include <QMessageBox>
#include <QIcon>
#include <QPixmap>
#include <QPushButton>
#include <QDesktopServices>
#include <QDateTime>
#include <QTimer>
#include <QDebug>

#define UPDATE_INTERVAL_DAYS 7

using namespace Friction::Ui;

UpdateChecker::UpdateChecker(QObject *parent)
    : QObject{parent}
    , mNet(nullptr)
{
    mNet = new QNetworkAccessManager(this);
    connect(mNet, &QNetworkAccessManager::finished,
            this, &UpdateChecker::handleReply);

    if (allowed()) { checkDate(); }
}

void UpdateChecker::check()
{
    AppSupport::setSettings("updater",
                            "last",
                            QDateTime::currentDateTime());
    const auto internalVersion = parseVersion(AppSupport::getAppVersion());
    if (internalVersion.major == 0) { return; }

    qDebug() << "check for updates ...";
    mNet->get(QNetworkRequest(QUrl("http://friction.graphics/update.xml")));
}

bool UpdateChecker::hasNewVersion(const fVersion &from,
                                  const fVersion &to)
{
    if (to.major < from.major) { return false; }
    if (to.minor < from.minor) { return false; }
    if (to.patch < from.patch) { return false; }
    if (to.micro < from.micro) { return false; }
    if (to.major == from.major &&
        to.minor == from.minor &&
        to.patch == from.patch &&
        to.micro == from.micro) { return false; }
    return true;
}

const QString UpdateChecker::parseVersionXml(const QString &xml,
                                             const bool &devel)
{
    const QString branch = devel ? "devel" : "stable";
    QXmlStreamReader r(xml);
    while (r.readNextStartElement()) {
        while (r.readNextStartElement()) {
            if (r.qualifiedName().toString() == branch) { return r.readElementText(); }
            else { r.readElementText(); }
        }
    }
    return QString();
}

const UpdateChecker::fVersion UpdateChecker::parseVersion(const QString &version)
{
    fVersion output{0, 0, 0, 0};
    if (version.simplified().isEmpty()) { return output; }

    QStringList versionList;
    if (version.contains("-")) {
        const auto list = version.simplified().split("-");
        if (list.count() == 2) {
            const QString custom = list.at(1).simplified();
            const bool isBeta = custom.startsWith("beta.");
            const bool isRc = custom.startsWith("rc.");
            if (isBeta || isRc) {
                double devel = custom.split(".").takeLast().toInt();
                if (devel > 0) {
                    versionList = list.at(0).simplified().split(".");
                    versionList << QString::number(isBeta ? devel / 100 : devel / 10);
                }
            }
        }
    } else { versionList = version.simplified().split("."); }

    if (versionList.count() >= 3) {
        output.major = versionList.at(0).toInt();
        output.minor = versionList.at(1).toInt();
        output.patch = versionList.at(2).toInt();
        if (versionList.count() >= 4) {
            output.micro = versionList.at(3).toDouble();
        }
    }

    return output;
}

void UpdateChecker::handleReply(QNetworkReply *reply)
{
    if (!reply) { return; }
    const QString xml = reply->readAll();
    reply->deleteLater();

    checkXml(xml);
}

void UpdateChecker::checkXml(const QString &xml)
{
    if (xml.isEmpty()) { return; }

    const QString appVersion = AppSupport::getAppVersion();
    const QString xmlVersion = parseVersionXml(xml, (appVersion.contains("-beta.") ||
                                                     appVersion.contains("-rc.")));
    const auto internalVersion = parseVersion(appVersion);
    const auto updateVersion = parseVersion(xmlVersion);

    if (!hasNewVersion(internalVersion,
                       updateVersion)) { return; }

    QMessageBox box;
    box.setWindowTitle(tr("Update available"));
    box.setIconPixmap(QPixmap(QIcon::fromTheme(AppSupport::getAppID()).pixmap(64)));
    box.setText(tr("<h3>Friction %1</h3>"
                   "<p>Friction %1 is now available for download. "
                   "See release notes for more information.</p>").arg(xmlVersion));
    box.addButton(tr("Ignore"),
                  QMessageBox::NoRole);
    const auto never = box.addButton(tr("Disable"),
                                     QMessageBox::ResetRole);
    const auto download = box.addButton(tr("Download"),
                                        QMessageBox::YesRole);
    download->setFocus();

    box.exec();

    if (box.clickedButton() == download) {
        QString url = QString("https://friction.graphics/releases/friction-%1.html").arg(QString(xmlVersion).replace(".", ""));
        QDesktopServices::openUrl(QUrl::fromUserInput(url));
    } else if (box.clickedButton() == never) {
        AppSupport::setSettings("updater",
                                "check",
                                false);
    }
}

bool UpdateChecker::allowed()
{
    const int interval = AppSupport::getSettings("updater",
                                                 "interval",
                                                 UPDATE_INTERVAL_DAYS).toInt();
    const bool firstRun = !AppSupport::getSettings("updater",
                                                   "check").isValid();
    if (firstRun) {
        const int ask = QMessageBox::question(nullptr,
                                              tr("Update Checker"),
                                              tr("<h3>Enable Update Checker?</h3>"
                                              "<p>Friction will check for updates every %1 days.</p>").arg(interval));
        const bool allow = (ask == QMessageBox::Yes);
        AppSupport::setSettings("updater",
                                "check",
                                allow);
        return allow;
    }
    return AppSupport::getSettings("updater",
                                   "check",
                                   false).toBool();
}

void UpdateChecker::checkDate()
{
    const bool firstRun = !AppSupport::getSettings("updater",
                                                   "last").toDateTime().isValid();
    QDateTime lastCheck = AppSupport::getSettings("updater",
                                                  "last",
                                                  QDateTime::currentDateTime()).toDateTime();
    const int interval = AppSupport::getSettings("updater",
                                                 "interval",
                                                 UPDATE_INTERVAL_DAYS).toInt();
    const bool doCheck = (lastCheck.daysTo(QDateTime::currentDateTime()) >= interval || firstRun);

    if (doCheck) {
        QTimer::singleShot(5000, this, [this]() { check(); });
    }
}
