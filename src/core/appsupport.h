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

#ifndef APPSUPPORT_H
#define APPSUPPORT_H

#include "core_global.h"

#include <QObject>
#include <QVariant>
#include <QPair>
#include <QStringList>
#include <QSettings>

#include "hardwareenums.h"

class CORE_EXPORT AppSupport : public QObject
{
    Q_OBJECT

public:
    struct ExpressionPreset
    {
        bool valid = false;
        QString definitions;
        QString bindings;
        QString script;
    };
    explicit AppSupport(QObject *parent = nullptr);
    static QVariant getSettings(const QString &group,
                                const QString &key,
                                const QVariant &fallback = QVariant());
    static QVariant getSettings(QSettings *settings,
                                const QString &group,
                                const QString &key,
                                const QVariant &fallback = QVariant());
    static void setSettings(const QString &group,
                            const QString &key,
                            const QVariant &value,
                            bool append = false);
    static void setSettings(QSettings *settings,
                            const QString &group,
                            const QString &key,
                            const QVariant &value,
                            bool append = false);
    static const QString getAppName();
    static const QString getAppDisplayName();
    static const QString getAppDomain();
    static const QString getAppID();
    static const QString getAppUrl();
    static const QString getAppVersion();
    static const QString getAppBuildInfo(bool html = false);
    static const QString getAppDesc();
    static const QString getAppCompany();
    static const QString getAppContributorsUrl();
    static const QString getAppIssuesUrl();
    static const QString getAppLatestReleaseUrl();
    static const QString getAppCommitUrl();
    static const QString getAppBranchUrl();
    static const QString getAppConfigPath();
    static const QString getAppPath();
    static const QString getAppTempPath();
    static const QString getAppOutputProfilesPath();
    static const QString getAppPathEffectsPath();
    static const QString getAppRasterEffectsPath();
    static const QString getAppShaderEffectsPath(bool restore = false);
    static const QString getAppShaderPresetsPath();
    static const QString getAppExPresetsPath();
    static const QString getAppUserExPresetsPath();
    static const QString getSVGO();
    static const QString getSVGOConfig();
    static const QString getFileMimeType(const QString &path);
    static const QString getFileIcon(const QString &path);
    static const QPair<QString,QString> getShaderID(const QString &path);
    static const QStringList getFilesFromPath(const QString &path,
                                              const QStringList &suffix = QStringList());
    static const QString getTimeCodeFromFrame(int frame,
                                              float fps);
    static int getFrameFromTimeCode(const QString &timecode,
                                    float fps);
    static HardwareSupport getRasterEffectHardwareSupport(const QString &effect,
                                                          HardwareSupport fallback);
    static const QString getRasterEffectHardwareSupportString(const QString &effect,
                                                              HardwareSupport fallback);
    static const QByteArray filterShader(QByteArray data);
    static const QStringList getFpsPresets();
    static void saveFpsPresets(const QStringList &presets);
    static void saveFpsPreset(const double value);
    static bool removeFpsPreset(const double value);
    static QPair<bool, bool> getFpsPresetStatus();
    static const QStringList getResolutionPresetsList();
    static const QList<QPair<int, int>> getResolutionPresets();
    static void saveResolutionPresets(const QList<QPair<int, int>> &presets);
    static void saveResolutionPreset(const int w, const int h);
    static bool removeResolutionPreset(const int w, const int h);
    static QPair<bool, bool> getResolutionPresetStatus();
    static const QString filterTextAZW(const QString &text);
    static const QString filterFormatsName(const QString &text);
    static int getProjectVersion(const QString &fileName = QString());
    static const QPair<QStringList,bool> hasWriteAccess();
    static bool isAppPortable();
    static bool isAppImage();
    static bool isWayland();
    static bool isFlatpak();
    static const QString getAppImagePath();
    static bool hasXDGDesktopIntegration();
    static bool setupXDGDesktopIntegration();
    static bool removeXDGDesktopIntegration();
    static void initXDGDesktop(const bool &isRenderer);
    static bool hasArg(int argc,
                       char *argv[],
                       const QString &find);
    static void checkPerms(const bool &isRenderer);
    static void checkFFmpeg(const bool &isRenderer);
    static void initEnv(const bool &isRenderer);
    static QPair<bool,int> handleXDGArgs(const bool &isRenderer,
                                         const QStringList &args);
    static void printVersion();
    static void printHelp(const bool &isRenderer);
    static const ExpressionPreset readEasingPreset(const QString &filename);
    static const QList<QPair<QString,QString>> getEasingPresets();
    static void handlePortableFirstRun();
    static const QString filterId(const QString &input);
};

#endif // APPSUPPORT_H
