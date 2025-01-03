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

#include "appsupport.h"
#include "hardwareinfo.h"
#include "ReadWrite/evformat.h"
#include "ReadWrite/filefooter.h"

#include <QApplication>
#include <QDebug>
#include <QIcon>
#include <QPalette>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QMimeDatabase>
#include <QMimeType>
#include <QDomDocument>
#include <QDomElement>
#include <QDirIterator>
#include <QtMath>
#include <QRegularExpression>
#include <QMessageBox>

#include <iostream>
#include <ostream>

extern "C" {
#include <libavutil/log.h>
#include <libavformat/avformat.h>
}

AppSupport::AppSupport(QObject *parent)
    : QObject{parent}
{

}

QVariant AppSupport::getSettings(const QString &group,
                                 const QString &key,
                                 const QVariant &fallback)
{
    if (AppSupport::isAppPortable()) {
        QSettings settings(QString("%1/friction.conf").arg(getAppConfigPath()),
                           QSettings::IniFormat);
        return getSettings(&settings, group, key, fallback);
    }
    QSettings settings;
    return getSettings(&settings, group, key, fallback);
}

QVariant AppSupport::getSettings(QSettings *settings,
                                 const QString &group,
                                 const QString &key,
                                 const QVariant &fallback)
{
    QVariant variant;
    if (!settings) { return variant; }
    settings->beginGroup(group);
    variant = settings->value(key, fallback);
    settings->endGroup();
    return variant;
}

void AppSupport::setSettings(const QString &group,
                             const QString &key,
                             const QVariant &value,
                             bool append)
{
    if (AppSupport::isAppPortable()) {
        QSettings settings(QString("%1/friction.conf").arg(getAppConfigPath()),
                           QSettings::IniFormat);
        setSettings(&settings, group, key, value, append);
        return;
    }
    QSettings settings;
    setSettings(&settings, group, key, value, append);
}

void AppSupport::setSettings(QSettings *settings,
                             const QString &group,
                             const QString &key,
                             const QVariant &value,
                             bool append)
{
    if (!settings) { return; }
    settings->beginGroup(group);
    QVariant result;
    if (append) {
        QVariant orig = getSettings(group, key);
        QMetaType::Type type = static_cast<QMetaType::Type>(orig.type());
        if (!orig.isNull() && type == QMetaType::QStringList) {
            QStringList list = orig.toStringList();
            list.append(value.toString());
            result = list;
        } else if (!orig.isNull() && type == QMetaType::QString) {
            QString text = orig.toString();
            text.append(value.toString());
            result = text;
        } else { append = false; }
    }
    settings->setValue(key, append ? result : value);
    settings->endGroup();
}

const QString AppSupport::getAppName()
{
    return QString::fromUtf8("friction");
}

const QString AppSupport::getAppDisplayName()
{
    return QString::fromUtf8("Friction");
}

const QString AppSupport::getAppDomain()
{
    return QString::fromUtf8("friction.graphics");
}

const QString AppSupport::getAppID()
{
    return QString::fromUtf8("graphics.friction.Friction");
}

const QString AppSupport::getAppUrl()
{
    return QString::fromUtf8("https://friction.graphics");
}

const QString AppSupport::getAppVersion()
{
    QString version = QString::fromUtf8(PROJECT_VERSION);
#ifdef CUSTOM_BUILD
    QString custom(CUSTOM_BUILD);
    if (!custom.isEmpty()) {
        version.append(QString("-%1").arg(custom));
    }
#endif
#ifndef PROJECT_OFFICIAL
#ifndef CUSTOM_BUILD
    version.append("-dev");
#endif
#ifdef PROJECT_COMMIT
    QString commit(PROJECT_COMMIT);
    if (!commit.isEmpty()) {
        version.append(QString("-%1").arg(commit));
    }
#endif
#endif
    return version;
}

const QString AppSupport::getAppBuildInfo(bool html)
{
#if defined(PROJECT_COMMIT) && defined(PROJECT_BRANCH)
    const auto commit = QString::fromUtf8(PROJECT_COMMIT);
    const auto branch = QString::fromUtf8(PROJECT_BRANCH);
    if (commit.isEmpty() || branch.isEmpty()) { return QString(); }
    if (!html) {
        return QString("%1 %2 %3 %4.").arg(tr("Built from"),
                                           commit,
                                           tr("on"),
                                           branch);
    } else {
        return QString("%1 <a href=\"%5/%2\">%2</a> %3 <a href=\"%6/%4\">%4</a>.")
                      .arg(tr("Built from commit"),
                           commit,
                           tr("on branch"),
                           branch,
                           getAppCommitUrl(),
                           getAppBranchUrl());
    }
#else
    Q_UNUSED(html)
#endif
    return QString();
}

const QString AppSupport::getAppDesc()
{
    return QString::fromUtf8("Motion Graphics");
}

const QString AppSupport::getAppCompany()
{
    return getAppName();
}

const QString AppSupport::getAppContributorsUrl()
{
    return QString::fromUtf8("https://github.com/friction2d/friction/graphs/contributors");
}

const QString AppSupport::getAppIssuesUrl()
{
    return QString::fromUtf8("https://github.com/friction2d/friction/issues");
}

const QString AppSupport::getAppLatestReleaseUrl()
{
    return QString::fromUtf8("https://friction.graphics/news");
}

const QString AppSupport::getAppCommitUrl()
{
    return QString::fromUtf8("https://github.com/friction2d/friction/commit");
}

const QString AppSupport::getAppBranchUrl()
{
    return QString::fromUtf8("https://github.com/friction2d/friction/tree");
}

const QString AppSupport::getAppConfigPath()
{
    QString path = QString::fromUtf8("%1/%2")
                   .arg(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation),
                        getAppName());
    if (isAppPortable()) {
        const QString appPath = getAppPath();
        if (QFileInfo(appPath).isWritable()) { path = QString("%1/config").arg(appPath); }
/*#ifdef Q_OS_LINUX
        const QString appimage = getAppImagePath();
        if (!appimage.isEmpty() && QFileInfo(appimage).isWritable()) { path = QString("%1.config").arg(appimage); }
#endif*/
    }
    QDir dir(path);
    if (!dir.exists()) { dir.mkpath(path); }
    return path;
}

const QString AppSupport::getAppPath()
{
    return QApplication::applicationDirPath();
}

const QString AppSupport::getAppTempPath()
{
#ifdef Q_OS_LINUX
    if (isFlatpak()) {
        QString path = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        if (!path.isEmpty()) {
            path.append("/friction-temp");
            if (!QFile::exists(path)) {
                QDir dir(path);
                dir.mkpath(path);
            }
            if (QFile::exists(path)) { return path; }
        }
    }
#endif
    return QDir::tempPath();
}

const QString AppSupport::getAppOutputProfilesPath()
{
    QString path = QString::fromUtf8("%1/OutputProfiles").arg(getAppConfigPath());
    QDir dir(path);
    if (!dir.exists()) { dir.mkpath(path); }
    return path;
}

const QString AppSupport::getAppPathEffectsPath()
{
    QString path = QString::fromUtf8("%1/PathEffects").arg(getAppConfigPath());
    QDir dir(path);
    if (!dir.exists()) { dir.mkpath(path); }
    return path;
}

const QString AppSupport::getAppRasterEffectsPath()
{
    QString path = QString::fromUtf8("%1/RasterEffects").arg(getAppConfigPath());
    QDir dir(path);
    if (!dir.exists()) { dir.mkpath(path); }
    return path;
}

const QString AppSupport::getAppShaderEffectsPath(bool restore)
{
    QString def = QString::fromUtf8("%1/ShaderEffects").arg(getAppConfigPath());
    QString path = restore ? def : getSettings("settings",
                                               "CustomShaderPath",
                                               def).toString();
    QDir dir(path);
    if (!dir.exists() && path.startsWith(getAppConfigPath())) { dir.mkpath(path); }
    return path;
}

const QString AppSupport::getAppShaderPresetsPath()
{
    QString path1 = getAppPath();
    QString path2 = path1;
    path1.append(QString::fromUtf8("/plugins/shaders"));
    path2.append(QString::fromUtf8("/../share/friction/plugins/shaders"));
    if (QFile::exists(path1)) { return path1; }
    if (QFile::exists(path2)) { return path2; }
    return QString();
}

const QString AppSupport::getAppExPresetsPath()
{
    QString path1 = getAppPath();
    QString path2 = path1;
    path1.append(QString::fromUtf8("/presets/expressions"));
    path2.append(QString::fromUtf8("/../share/friction/presets/expressions"));
    if (QFile::exists(path1)) { return path1; }
    if (QFile::exists(path2)) { return path2; }
    return QString();
}

const QString AppSupport::getAppUserExPresetsPath()
{
    QString path = QString::fromUtf8("%1/ExpressionPresets").arg(getAppConfigPath());
    QDir dir(path);
    if (!dir.exists()) { dir.mkpath(path); }
    return path;
}

const QString AppSupport::getSVGO()
{
#if defined(Q_OS_WIN)
    const QString svgo = "svgo-win.exe";
#elif defined(Q_OS_LINUX)
    const QString svgo = "svgo-linux";
#elif defined(Q_OS_MAC)
    const QString svgo = "svgo-macos";
#else
    const QString svgo = "svgo";
#endif
    const QString path = QString("%1/%2").arg(getAppPath(), svgo);
    if (QFile::exists(path)) { return path; }
    return QStandardPaths::findExecutable("svgo");
}

const QString AppSupport::getSVGOConfig()
{
    QString filename = "svgo.config.js";
    QString path = getAppConfigPath() + QDir::separator() + filename;
    if (!QFile::exists(path)) {
        QString config;
        QFile file(":/config/" + filename);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            config = file.readAll();
            file.close();
        }
        if (!config.isEmpty()) {
            QFile file(path);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                file.write(config.toUtf8());
                file.close();
            }
        }
    }
    return path;
}

const QString AppSupport::getFileMimeType(const QString &path)
{
    QMimeDatabase db;
    QMimeType type = db.mimeTypeForFile(path);
    return type.name();
}

const QString AppSupport::getFileIcon(const QString &path)
{
    QString fileIcon = "file_blank";
    QString mime = getFileMimeType(path);
    if (mime.startsWith("image")) { fileIcon = "file_image"; }
    else if (mime.startsWith("video")) { fileIcon = "file_movie"; }
    else if (mime.startsWith("audio")) { fileIcon = "file_sound"; }
    return fileIcon;
}

const QPair<QString, QString> AppSupport::getShaderID(const QString &path)
{
    QPair<QString,QString> result;

    QFile greFile(path);
    if (!greFile.exists()) { return result; }
    if (!greFile.open(QIODevice::ReadOnly)) { return result; }

    QDomDocument document;
    QString errMsg;
    if (!document.setContent(&greFile, &errMsg)) {
        greFile.close();
        return result;
    }
    greFile.close();

    QDomElement root = document.firstChildElement();
    if (root.tagName() != "ShaderEffect") { return result; }

    const QString effectName = root.attribute("name");
    const QString menuPath = root.attribute("menuPath");
    result.first = effectName;
    result.second = menuPath;

    return result;
}

const QStringList AppSupport::getFilesFromPath(const QString &path,
                                               const QStringList &suffix)
{
    QStringList result;
    if (path.isEmpty() || !QFile::exists(path)) { return result; }
    QDirIterator it(path, suffix, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) { result << it.next(); }
    return result;
}

const QString AppSupport::getTimeCodeFromFrame(int frame,
                                               float fps)
{
    if (fps <= 0) { return "00:00:00:00"; }
    bool negative = frame < 0;
    if (negative) { frame = qAbs(frame); }
    double totalSeconds = static_cast<double>(frame) / fps;
    int hours = static_cast<int>(totalSeconds / 3600);
    int minutes = static_cast<int>((totalSeconds - hours * 3600) / 60);
    int seconds = static_cast<int>(totalSeconds - hours * 3600 - minutes * 60);
    int frames = static_cast<int>((totalSeconds - static_cast<int>(totalSeconds)) * fps);
    QString timecode = QString("%1:%2:%3:%4")
                           .arg(hours, 2, 10, QChar('0'))
                           .arg(minutes, 2, 10, QChar('0'))
                           .arg(seconds, 2, 10, QChar('0'))
                           .arg(frames, 2, 10, QChar('0'));
    if (negative) { timecode.prepend("-"); }
    //qDebug() << frame << "=" << timecode;
    return timecode;
}

int AppSupport::getFrameFromTimeCode(const QString &timecode,
                                     float fps)
{
    const auto list = timecode.split(":");
    bool negative = timecode.startsWith("-");
    if (fps > 0. && list.count() == 4) {
        int hh = negative ? QString(list.at(0)).replace("-", "").toInt() : list.at(0).toInt();
        int mm = list.at(1).toInt();
        int ss = list.at(2).toInt();
        int ff = list.at(3).toInt();
        double totalSeconds = hh * 3600 + mm * 60 + ss + static_cast<double>(ff) / fps;
        int frame = static_cast<int>(totalSeconds * fps);
        //qDebug() << timecode << "=" << negative ? -frame : frame;
        return negative ? -frame : frame;
    } else {
        // assume it's just a frame number
        return timecode.toInt();
    }
    return 0;
}

HardwareSupport AppSupport::getRasterEffectHardwareSupport(const QString &effect,
                                                           HardwareSupport fallback)
{
    if (effect.isEmpty()) { return fallback; }
    const QVariant variant = getSettings("RasterEffects",
                                         QString("%1HardwareSupport").arg(effect));
    if (variant.isValid()) {
        HardwareSupport supported = static_cast<HardwareSupport>(variant.toInt());
        if (supported == HardwareSupport::hardwareDefault) { return fallback; }
        else { return supported; }
    }
    return fallback;
}

const QString AppSupport::getRasterEffectHardwareSupportString(const QString &effect,
                                                               HardwareSupport fallback)
{
    HardwareSupport supported = getRasterEffectHardwareSupport(effect, fallback);
    QString result;
    switch (supported) {
    case HardwareSupport::cpuOnly:
        result = tr("CPU-only");
        break;
    case HardwareSupport::cpuPreffered:
        result = tr("CPU preffered");
        break;
    case HardwareSupport::gpuOnly:
        result = tr("GPU-only");
        break;
    case HardwareSupport::gpuPreffered:
        result = tr("GPU preffered");
        break;
    default:;
    }
    return result;
}

const QByteArray AppSupport::filterShader(QByteArray data)
{
#if defined(Q_OS_WIN)
    if (HardwareInfo::sGpuVendor() == GpuVendor::intel) {
        return data.replace("texture2D", "texture");
    }
    return data;
#elif defined(Q_OS_MAC)
    return data.replace("texture2D", "texture");
#else
    return data;
#endif
}

const QStringList AppSupport::getFpsPresets()
{
    QStringList presets = getSettings("presets",
                                      "fps",
                                      QStringList()
                                          << "24"
                                          << "25"
                                          << "30"
                                          << "50"
                                          << "60").toStringList();
    return presets;
}

void AppSupport::saveFpsPresets(const QStringList &presets)
{
    setSettings("presets", "fps", presets);
}

void AppSupport::saveFpsPreset(const double value)
{
    if (value <= 0) { return; }
    auto presets = getFpsPresets();
    if (presets.contains(QString::number(value))) { return; }
    presets << QString::number(value);
    setSettings("presets", "fps", presets);
}

bool AppSupport::removeFpsPreset(const double value)
{
    if (value <= 0) { return false; }
    auto presets = getFpsPresets();
    if (!presets.removeAll(QString::number(value))) { return false; }
    setSettings("presets", "fps", presets);
    return true;
}

QPair<bool, bool> AppSupport::getFpsPresetStatus()
{
    QPair<bool, bool> status;
    status.first = getSettings("presets", "EnableFPS", true).toBool();
    status.second = getSettings("presets", "EnableFPSAuto", true).toBool();
    return status;
}

const QStringList AppSupport::getResolutionPresetsList()
{
    QStringList presets = getSettings("presets",
                                      "resolution",
                                      QStringList()
                                          << "1280x720"
                                          << "1920x1080"
                                          << "2560x1440"
                                          << "3840x2160").toStringList();
    return presets;
}

const QList<QPair<int, int> > AppSupport::getResolutionPresets()
{
    const auto presets = getResolutionPresetsList();
    QList<QPair<int, int>> l;
    for (auto &v: presets) {
        auto res = v.split("x");
        if (res.count() != 2) { continue; }
        const auto val = QPair<int, int>(res.at(0).trimmed().toInt(),
                                         res.at(1).trimmed().toInt());
        if (l.contains(val) || val.first <= 0 || val.second <= 0) { continue; }
        l.append(val);
    }
    std::sort(l.begin(), l.end());
    return l;
}

void AppSupport::saveResolutionPresets(const QList<QPair<int, int> > &presets)
{
    QStringList list;
    for (const auto &preset: presets) {
        if (preset.first <= 0 || preset.second <= 0) { continue; }
        const auto val = QString("%1x%2").arg(preset.first).arg(preset.second);
        if (list.contains(val)) { continue; }
        list << val;
    }
    setSettings("presets", "resolution", list);
}

void AppSupport::saveResolutionPreset(const int w,
                                      const int h)
{
    if (w <= 0 || h <= 0) { return; }
    const auto v = QString("%1x%2").arg(w).arg(h);
    auto presets = getResolutionPresetsList();
    if (presets.contains(v)) { return; }
    presets << v;
    setSettings("presets", "resolution", presets);
}

bool AppSupport::removeResolutionPreset(const int w,
                                        const int h)
{
    if (w <= 0 || h <= 0) { return false; }
    const auto v = QString("%1x%2").arg(w).arg(h);
    auto presets = getResolutionPresetsList();
    if (!presets.removeAll(v)) { return false; }
    setSettings("presets", "resolution", presets);
    return true;
}

QPair<bool, bool> AppSupport::getResolutionPresetStatus()
{
    QPair<bool, bool> status;
    status.first = getSettings("presets", "EnableResolutions", true).toBool();
    status.second = getSettings("presets", "EnableResolutionsAuto", true).toBool();
    return status;
}

const QString AppSupport::filterTextAZW(const QString &text)
{
    QRegularExpression regex("\\s|\\W");
    QString output = text;
    return output.replace(regex, "");
}

const QString AppSupport::filterFormatsName(const QString &text)
{
    QString output(text);
    if (output == "image2") { output = "image"; }
    else if (output == "image2 sequence") { output = "Image Sequence"; }
    return output;
}

int AppSupport::getProjectVersion(const QString &fileName)
{
    if (fileName.isEmpty()) { return EvFormat::version; }
    if (!QFile::exists(fileName)) { return 0; }
    QFile file(fileName);
    if (file.open(QIODevice::ReadOnly)) {
        try {
            const int version = FileFooter::sReadEvFileVersion(&file);
            file.close();
            return version;
        } catch(...) {
            file.close();
        }
    }
    return 0;
}

const QPair<QStringList, bool> AppSupport::hasWriteAccess()
{
    QPair<QStringList,bool> result(QStringList(), true);

    QStringList dirs;
    dirs << getAppConfigPath();
    dirs << getAppOutputProfilesPath();
    dirs << getAppShaderEffectsPath();
    for (const auto &dir : dirs) {
        if (dir.isEmpty()) { continue; }
        QFileInfo info(dir);
        if (!info.isDir() ||
            !info.exists() ||
            !info.isReadable() ||
            !info.isWritable()) {
            result.second = false;
            result.first << info.absoluteFilePath();
        }
    }

    return result;
}

bool AppSupport::isAppPortable()
{
    const QString path = getAppPath();
/*#ifdef Q_OS_LINUX
    const QString appimage = getAppImagePath();
    if (!appimage.isEmpty()) {
        return QFile::exists(appimage);// && QFileInfo(appimage).isWritable();
    }
#endif*/
    return QFile::exists(QString("%1/portable.txt").arg(path)) && QFileInfo(path).isWritable();
}

bool AppSupport::isAppImage()
{
    return !getAppImagePath().simplified().isEmpty();
}

bool AppSupport::isWayland()
{
    return QGuiApplication::platformName().startsWith("wayland");
}

bool AppSupport::isFlatpak()
{
    return !QString(qgetenv("container")).isEmpty();
}

const QString AppSupport::getAppImagePath()
{
    return QString(qgetenv("APPIMAGE"));
}

bool AppSupport::hasXDGDesktopIntegration()
{
    const bool ignoreXDG = getSettings("portable", "ignoreXDG", false).toBool();
    if (ignoreXDG) { return true; }

    QString path = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    if (path.isEmpty() ||
        !path.startsWith(QDir::homePath())) { path = QString("%1/.local/share").arg(QDir::homePath()); }

    QStringList files;
    QString desktop = "applications/graphics.friction.Friction.desktop";
    files << desktop;
    files << "mime/packages/graphics.friction.Friction.xml";
    files << "icons/hicolor/scalable/apps/graphics.friction.Friction.svg";
    files << "icons/hicolor/256x256/apps/graphics.friction.Friction.png";
    files << "icons/hicolor/scalable/mimetypes/application-x-graphics.friction.Friction.svg";
    files << "icons/hicolor/256x256/mimetypes/application-x-graphics.friction.Friction.png";

    for (const auto &file : files) {
        if (!QFile::exists(QString("%1/%2").arg(path, file))) {
            qDebug() << "not found!" << file;
            return false;
        }
    }

    QString desktopFilePath(QString("%1/%2").arg(path, desktop));
    if (QFile::exists(desktopFilePath)) {
        QString appPath(qApp->applicationFilePath());
        const QString appimage = getAppImagePath();
        if (!appimage.isEmpty() && QFile::exists(appimage)) { appPath = appimage; }
        QSettings dotDesktop(desktopFilePath, QSettings::IniFormat);
        dotDesktop.beginGroup("Desktop Entry");
        QString exec = dotDesktop.value("Exec").toString().simplified();
        dotDesktop.endGroup();
        if (exec.isEmpty() || !exec.startsWith(appPath)) { return false; }
    }
    return true;
}

bool AppSupport::setupXDGDesktopIntegration()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    if (path.isEmpty() ||
        !path.startsWith(QDir::homePath())) { path = QString("%1/.local/share").arg(QDir::homePath()); }

    if (!QFile::exists(path)) {
        QDir dir(path);
        if (!dir.mkpath(path)) { return false; }
    }

    const QString resDesktop = ":/xdg/friction.desktop";
    const QString resMime = ":/xdg/friction.xml";
    const QString resSvg = ":/icons/hicolor/scalable/apps/graphics.friction.Friction.svg";
    const QString resPng = ":/icons/hicolor/256x256/apps/graphics.friction.Friction.png";

    {
        const QString dirName(QString("%1/applications").arg(path));
        const QString fileName(QString("%1/graphics.friction.Friction.desktop").arg(dirName));
        if (!QFile::exists(dirName)) {
            QDir dir(dirName);
            if (!dir.mkpath(dirName)) { return false; }
        }
        bool wrongPath = false;
        QSettings desktop(fileName, QSettings::IniFormat);
        desktop.beginGroup("Desktop Entry");
        QString exec = desktop.value("Exec").toString().simplified();
        if (!exec.startsWith(getAppPath())) { wrongPath = true; }
        desktop.endGroup();
        if (!QFile::exists(fileName) || wrongPath) {
            QFile file(fileName);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
                QFile res(resDesktop);
                QString appPath(qApp->applicationFilePath());
                const QString appimage = getAppImagePath();
                if (!appimage.isEmpty() && QFile::exists(appimage)) { appPath = appimage; }
                if (res.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    file.write(res.readAll().replace("__FRICTION__",
                                                     appPath.toUtf8()));
                    res.close();
                } else { return false; }
                file.close();
            } else { return false; }
        }
    }
    {
        const QString dirName(QString("%1/mime/packages").arg(path));
        const QString fileName(QString("%1/graphics.friction.Friction.xml").arg(dirName));
        if (!QFile::exists(dirName)) {
            QDir dir(dirName);
            if (!dir.mkpath(dirName)) { return false; }
        }
        if (!QFile::exists(fileName)) {
            QFile file(fileName);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QFile res(resMime);
                if (res.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    file.write(res.readAll());
                    res.close();
                } else { return false; }
                file.close();
            } else { return false; }
        }
    }
    {
        const QString dirName(QString("%1/icons/hicolor/scalable/apps").arg(path));
        const QString fileName(QString("%1/graphics.friction.Friction.svg").arg(dirName));
        if (!QFile::exists(dirName)) {
            QDir dir(dirName);
            if (!dir.mkpath(dirName)) { return false; }
        }
        if (!QFile::exists(fileName)) {
            QFile file(fileName);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QFile res(resSvg);
                if (res.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    file.write(res.readAll());
                    res.close();
                } else { return false; }
                file.close();
            } else { return false; }
        }
    }
    {
        const QString dirName(QString("%1/icons/hicolor/256x256/apps").arg(path));
        const QString fileName(QString("%1/graphics.friction.Friction.png").arg(dirName));
        if (!QFile::exists(dirName)) {
            QDir dir(dirName);
            if (!dir.mkpath(dirName)) { return false; }
        }
        if (!QFile::exists(fileName)) {
            QFile file(fileName);
            if (file.open(QIODevice::WriteOnly)) {
                QFile res(resPng);
                if (res.open(QIODevice::ReadOnly)) {
                    file.write(res.readAll());
                    res.close();
                } else { return false; }
                file.close();
            } else { return false; }
        }
    }
    {
        const QString dirName(QString("%1/icons/hicolor/scalable/mimetypes").arg(path));
        const QString fileName(QString("%1/application-x-graphics.friction.Friction.svg").arg(dirName));
        if (!QFile::exists(dirName)) {
            QDir dir(dirName);
            if (!dir.mkpath(dirName)) { return false; }
        }
        if (!QFile::exists(fileName)) {
            QFile file(fileName);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QFile res(resSvg);
                if (res.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    file.write(res.readAll());
                    res.close();
                } else { return false; }
                file.close();
            } else { return false; }
        }
    }
    {
        const QString dirName(QString("%1/icons/hicolor/256x256/mimetypes").arg(path));
        const QString fileName(QString("%1/application-x-graphics.friction.Friction.png").arg(dirName));
        if (!QFile::exists(dirName)) {
            QDir dir(dirName);
            if (!dir.mkpath(dirName)) { return false; }
        }
        if (!QFile::exists(fileName)) {
            QFile file(fileName);
            if (file.open(QIODevice::WriteOnly)) {
                QFile res(resPng);
                if (res.open(QIODevice::ReadOnly)) {
                    file.write(res.readAll());
                    res.close();
                } else { return false; }
                file.close();
            } else { return false; }
        }
    }

    if (isWayland()) {
        QGuiApplication::setDesktopFileName(getAppID());
    }
    return true;
}

bool AppSupport::removeXDGDesktopIntegration()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    if (path.isEmpty() ||
        !path.startsWith(QDir::homePath())) { path = QString("%1/.local/share").arg(QDir::homePath()); }

    QStringList files;
    files << "applications/graphics.friction.Friction.desktop";
    files << "mime/packages/graphics.friction.Friction.xml";
    files << "icons/hicolor/scalable/apps/graphics.friction.Friction.svg";
    files << "icons/hicolor/256x256/apps/graphics.friction.Friction.png";
    files << "icons/hicolor/scalable/mimetypes/application-x-graphics.friction.Friction.svg";
    files << "icons/hicolor/256x256/mimetypes/application-x-graphics.friction.Friction.png";

    for (const auto &file : files) {
        QFile fileName(QString("%1/%2").arg(path, file));
        if (fileName.exists()) {
            if (!fileName.remove()) {
                qWarning() << "Failed to remove" << fileName;
                return false;
            }
        }
    }
    return true;
}

void AppSupport::initXDGDesktop(const bool &isRenderer)
{
    if ((!isAppPortable() && !isAppImage()) || isRenderer) { return; }
    if (AppSupport::hasXDGDesktopIntegration()) { return; }
    QString appPath("friction");
    const QString appimage = AppSupport::getAppImagePath().simplified();
    if (!appimage.isEmpty()) { appPath = appimage.split("/").takeLast(); }
    const auto ask = QMessageBox::question(nullptr,
                                           QObject::tr("Setup Desktop Integration"),
                                           QObject::tr("Would you like to setup desktop integration?"
                                                       " This will add Friction to your application launcher"
                                                       " and add required mime types.<br><br>"
                                                       "You also can manage the desktop integration with:"
                                                       "<br><br><code>%1 --xdg-install</code>"
                                                       "<br><code>%1 --xdg-remove</code>").arg(appPath));
    if (ask == QMessageBox::Yes) {
        if (!AppSupport::setupXDGDesktopIntegration()) {
            QMessageBox::warning(nullptr,
                                 QObject::tr("Desktop Integration Failed"),
                                 QObject::tr("Failed to install the required files for desktop integration,"
                                             " please check your permissions."));
        }
    } else {
        AppSupport::setSettings("portable", "ignoreXDG", true);
    }
}

bool AppSupport::hasArg(int argc,
                        char *argv[],
                        const QString &find)
{
    for (int i = 0; i < argc; i++) {
        const QString val = argv[i];
        if (val.contains(find)) { return true; }
    }
    return false;
}

void AppSupport::checkPerms(const bool &isRenderer)
{
    const auto perms = hasWriteAccess();
    if (perms.second) { return; }
    if (isRenderer) {
        qWarning() << QObject::tr("Friction needs read/write access to:\n- %1").arg(perms.first.join("\n- "));
    } else {
        QMessageBox::warning(nullptr,
                             QObject::tr("Permission issue"),
                             QObject::tr("Friction needs read/write access to:<br><br>- %1").arg(perms.first.join("<br>- ")));
    }
}

void AppSupport::checkFFmpeg(const bool &isRenderer)
{
    av_log_set_level(AV_LOG_ERROR);
#ifndef QT_DEBUG
    const QString warning = QObject::tr("Friction is built against an unsupported FFmpeg version. Use at own risk and don't report any issues upstream.");
    if (avformat_version() >= 3812708) {
        if (isRenderer) { qWarning() << warning; }
        else {
            QMessageBox::critical(nullptr,
                                  QObject::tr("Unsupported FFmpeg version"),
                                  warning);
        }
    }
#else
    Q_UNUSED(isRenderer)
#endif
}

void AppSupport::initEnv(const bool &isRenderer)
{
    Q_UNUSED(isRenderer)
#if defined(Q_OS_WIN)
    // windows theme integration
#if QT_VERSION < QT_VERSION_CHECK(6, 5, 0)
    // Set window title bar color based on dark/light theme
    // https://www.qt.io/blog/dark-mode-on-windows-11-with-qt-6.5
    // https://learn.microsoft.com/en-us/answers/questions/1161597/how-to-detect-windows-application-dark-mode
    QSettings registry("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                       QSettings::NativeFormat);
    if (registry.value("AppsUseLightTheme", 0).toInt() == 0) { qputenv("QT_QPA_PLATFORM", "windows:darkmode=1"); }
#endif
#elif defined(Q_OS_LINUX)
#ifndef FRICTION_EGL
    qputenv("QT_QPA_PLATFORM", "xcb");
#endif
#endif
}

QPair<bool, int> AppSupport::handleXDGArgs(const bool &isRenderer,
                                           const QStringList &args)
{
    QPair<bool,int> status(false, 0);
    if (!AppSupport::isAppPortable() || isRenderer) { return status; }
    if (args.contains("--xdg-remove")) {
        const bool removedXDG = AppSupport::removeXDGDesktopIntegration();
        qWarning() << "Removed XDG Integration:" << removedXDG;
        status.first = true;
        status.second = removedXDG ? 0 : -1;
    } else if (args.contains("--xdg-install")) {
        const bool installedXDG = AppSupport::setupXDGDesktopIntegration();
        qWarning() << "Installed XDG Integration:" << installedXDG;
        status.first = true;
        status.second = installedXDG ? 0 : -1;
    }
    return status;
}

void AppSupport::printVersion()
{
    std::cout << QString("%1 %2 - %3").arg(getAppDisplayName(),
                                           getAppVersion(),
                                           getAppUrl()).toStdString() << std::endl;
}

void AppSupport::printHelp(const bool &isRenderer)
{
    Q_UNUSED(isRenderer)
}

const AppSupport::ExpressionPreset AppSupport::readEasingPreset(const QString &filename)
{
    ExpressionPreset preset;
    preset.valid = false;
    if (!QFile::exists(filename)) { return preset; }
    QFile file(filename);
    QString js;
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        js = file.readAll();
        file.close();
    }
    if (js.isEmpty()) { return preset; }
    QStringList parts = js.split("/*_FRICTION_EXPRESSION_PRESET_*/",
                                 Qt::SkipEmptyParts);
    if (parts.size() != 3) { return preset; }
    preset.valid = true;
    preset.definitions = parts.at(0).trimmed();
    preset.bindings = parts.at(1).trimmed();
    preset.script = parts.at(2).trimmed();
    return preset;
}

const QList<QPair<QString, QString> > AppSupport::getEasingPresets()
{
    QList<QPair<QString, QString> > presets;

    presets.push_back({tr("Ease In Back"), ":/easing/presets/easeInBack.js"});
    presets.push_back({tr("Ease In Bounce"), ":/easing/presets/easeInBounce.js"});
    presets.push_back({tr("Ease In Circ"), ":/easing/presets/easeInCirc.js"});
    presets.push_back({tr("Ease In Cubic"), ":/easing/presets/easeInCubic.js"});
    presets.push_back({tr("Ease In Elastic"), ":/easing/presets/easeInElastic.js"});
    presets.push_back({tr("Ease In Expo"), ":/easing/presets/easeInExpo.js"});
    presets.push_back({tr("Ease In Quad"), ":/easing/presets/easeInQuad.js"});
    presets.push_back({tr("Ease In Quart"), ":/easing/presets/easeInQuart.js"});
    presets.push_back({tr("Ease In Quint"), ":/easing/presets/easeInQuint.js"});
    presets.push_back({tr("Ease In Sine"), ":/easing/presets/easeInSine.js"});

    presets.push_back({tr("Ease Out Back"), ":/easing/presets/easeOutBack.js"});
    presets.push_back({tr("Ease Out Bounce"), ":/easing/presets/easeOutBounce.js"});
    presets.push_back({tr("Ease Out Circ"), ":/easing/presets/easeOutCirc.js"});
    presets.push_back({tr("Ease Out Cubic"), ":/easing/presets/easeOutCubic.js"});
    presets.push_back({tr("Ease Out Elastic"), ":/easing/presets/easeOutElastic.js"});
    presets.push_back({tr("Ease Out Expo"), ":/easing/presets/easeOutExpo.js"});
    presets.push_back({tr("Ease Out Quad"), ":/easing/presets/easeOutQuad.js"});
    presets.push_back({tr("Ease Out Quart"), ":/easing/presets/easeOutQuart.js"});
    presets.push_back({tr("Ease Out Quint"), ":/easing/presets/easeOutQuint.js"});
    presets.push_back({tr("Ease Out Sine"), ":/easing/presets/easeOutSine.js"});

    presets.push_back({tr("Ease In/Out Back"), ":/easing/presets/easeInOutBack.js"});
    presets.push_back({tr("Ease In/Out Bounce"), ":/easing/presets/easeInOutBounce.js"});
    presets.push_back({tr("Ease In/Out Circ"), ":/easing/presets/easeInOutCirc.js"});
    presets.push_back({tr("Ease In/Out Cubis"), ":/easing/presets/easeInOutCubic.js"});
    presets.push_back({tr("Ease In/Out Elastic"), ":/easing/presets/easeInOutElastic.js"});
    presets.push_back({tr("Ease In/Out Expo"), ":/easing/presets/easeInOutExpo.js"});
    presets.push_back({tr("Ease In/Out Quad"), ":/easing/presets/easeInOutQuad.js"});
    presets.push_back({tr("Ease In/Out Quart"), ":/easing/presets/easeInOutQuart.js"});
    presets.push_back({tr("Ease In/Out Quint"), ":/easing/presets/easeInOutQuint.js"});
    presets.push_back({tr("Ease In/Out Sine"), ":/easing/presets/easeInOutSine.js"});

    return presets;
}

void AppSupport::handlePortableFirstRun()
{
    if (!isAppPortable()) { return; }
    const bool firstRun = getSettings("portable", "PortableFirstRun", true).toBool();
    if (!firstRun) { return; }
    QMessageBox::information(nullptr,
                             tr("Portable Mode"),
                             tr("You are in portable mode, your config directory is:"
                                "<br><br><code>%1</code>").arg(getAppConfigPath()));
    setSettings("portable", "PortableFirstRun", false);
}

const QString AppSupport::filterId(const QString &input)
{
    return QString(input).simplified().replace(" ", "");
}
