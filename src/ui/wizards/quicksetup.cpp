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

#include "quicksetup.h"

#include <QMessageBox>

#include "appsupport.h"
#include "themesupport.h"
#include "Private/esettings.h"
#include "quicksetupgeneral.h"
#include "quicksetuppresets.h"

using namespace Friction::Ui;

QuickSetup::QuickSetup(QWidget *parent)
    : QWizard(parent)
{
    setWindowTitle(tr("Quick Setup"));
    setMinimumWidth(700);

    setWizardStyle(QWizard::ModernStyle);
    setPixmap(QWizard::LogoPixmap,
              QIcon::fromTheme(ThemeSupport::getAppIconName(true)).pixmap(64, 64));

    setPage(0, new QuickSetupGeneralPage(this));
    setPage(1, new QuickSetupPresetsPage(this));
}

void QuickSetup::accept()
{
    if (!AppSupport::isFlatpak()) {
        {
            const auto val = field("CustomBrowserPath");
            if (val.isValid()) {
                const QString path = val.toString().trimmed();
                qDebug() << "browser path" << path;
                if (!path.isEmpty()) { AppSupport::setSettings("settings", "CustomBrowserPath", path); }
            }
        }
    }
    {
        const auto val = field("CustomCachePath");
        if (val.isValid()) {
            const QString path = val.toString().trimmed();
            qDebug() << "cache path" << path;
            if (!path.isEmpty()) { AppSupport::setSettings("settings", "CustomCachePath", path); }
        }
    }
    {
        const auto val = field("ImportFileDirOpt");
        if (val.isValid()) {
            if (val.toInt() == eSettings::ImportFileDirRecent ||
                val.toInt() == eSettings::ImportFileDirProject) {
                qDebug() << "def import dir" << val.toInt();
                AppSupport::setSettings("settings", "ImportFileDirOpt", val.toInt());
            }
        }
    }
    {
        const auto val = field("enableGizmos");
        if (val.isValid()) {
            const bool enabled = val.toBool();
            qDebug() << "gizmos" << enabled;
            AppSupport::setSettings("gizmos", "All", enabled);
            AppSupport::setSettings("gizmos", "Position", enabled);
            AppSupport::setSettings("gizmos", "Rotate", enabled);
            AppSupport::setSettings("gizmos", "Scale", enabled);
            // shear is off by default, so we don't add it here
        }
    }
    {
        const auto val = field("enableSnapping");
        if (val.isValid()) {
            const bool enabled = val.toBool();
            qDebug() << "snapping" << enabled;
            AppSupport::setSettings("grid", "snapEnabled", enabled);
        }
    }
    {
        const auto val = field("enableGrid");
        if (val.isValid()) {
            const bool enabled = val.toBool();
            qDebug() << "grid" << enabled;
            AppSupport::setSettings("grid", "show", enabled);
        }
    }
    {
        const auto res = field("resolution");
        const auto asp = field("aspect");
        if (res.isValid() &&
            asp.isValid() &&
            res.toStringList().size() == 2) {
            int w = 0;
            int h = 0;
            const auto values = res.toStringList();
            switch(asp.toInt()) {
            case 0: // landscape
                w = values.at(0).toInt();
                h = values.at(1).toInt();
                break;
            case 1: // portrait
                w = values.at(1).toInt();
                h = values.at(0).toInt();
                break;
            case 2: // square
                h = values.at(1).toInt();
                w = h;
                break;
            default:;
            }
            if (w > 0 && h > 0) {
                qDebug() << "scene w/h" << w << h;
                AppSupport::setSettings("scene", "DefaultWidth", w);
                AppSupport::setSettings("scene", "DefaultHeight", h);
            }
        }
    }
    {
        const auto fps = field("fps");
        const auto dur = field("duration");
        if (fps.isValid() && dur.isValid()) {
            const double fpsVal = fps.toDouble();
            const int durSeconds = dur.toInt();

            const int totalFrames = static_cast<int>(durSeconds * fpsVal);

            const int startFrame = 0;
            const int endFrame = totalFrames - 1;

            qDebug() << "scene fps" << fpsVal
                     << "scene dur" << durSeconds
                     << "scene min" << startFrame
                     << "scene max" << endFrame;
            AppSupport::setSettings("scene", "DefaultFps", fpsVal);
            AppSupport::setSettings("scene", "DefaultMin", startFrame);
            AppSupport::setSettings("scene", "DefaultMax", endFrame);
        }
    }
    {
        const auto val = field("ramLimit");
        if (val.isValid()) {
            const int percent = val.toInt();
            const auto avail = intMB(AppSupport::getTotalRamBytes());

            const double calculated = (static_cast<double>(avail.fValue) * percent) / 100.0;
            const intMB limit(static_cast<int>(calculated));

            qDebug() << "ram percent" << percent
                     << "ram avail" << avail.fValue
                     << "ram limit" << limit.fValue;
            AppSupport::setSettings("settings", "ramMBCap", limit.fValue);
        }
    }
    {
        const auto val = field("renderProfiles");
        if (val.isValid() && val.toStringList().size() > 0) {
            const auto profiles = val.toStringList();
            qDebug() << "render profiles" << profiles;
            AppSupport::installRenderPresets(true, profiles);
        }
    }
    {
        const auto val = field("expressions");
        if (val.isValid() && val.toStringList().size() > 0) {
            const auto expressions = val.toStringList();
            qDebug() << "expressions" << expressions;
            AppSupport::installExprPresets(true, expressions);
        }
    }
    {
        const auto gpu = AppSupport::getOpenGLInfo();
        const int mssa = gpu.contains("intel", Qt::CaseInsensitive) ? 4 : 16;
        qDebug() << "gpu" << gpu << "mssa" << mssa;
        AppSupport::setSettings("settings", "mssa", mssa);

        // default to cpu mode on included effects
        AppSupport::setSettings("RasterEffects", "BlurHardwareSupport", 0);
        AppSupport::setSettings("RasterEffects", "BrightnessContrastHardwareSupport", 0);
        AppSupport::setSettings("RasterEffects", "ColorizeHardwareSupport", 0);
        AppSupport::setSettings("RasterEffects", "MotionBlurHardwareSupport", 0);
        AppSupport::setSettings("RasterEffects", "NoiseFadeHardwareSupport", 0);
        AppSupport::setSettings("RasterEffects", "ShadowHardwareSupport", 0);
        AppSupport::setSettings("RasterEffects", "WipeHardwareSupport", 0);
    }

    QWizard::accept();
}

void QuickSetup::reject()
{
    const int ret = QMessageBox::question(this,
                                          tr("Cancel Quick Setup?"),
                                          tr("Are you sure you want to quit the quick setup?"));
    if (ret != QMessageBox::Yes) { return; }

    QWizard::reject();
}
