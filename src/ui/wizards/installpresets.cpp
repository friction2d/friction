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

#include "installpresets.h"

#include "appsupport.h"
#include "themesupport.h"
#include "quicksetuppresets.h"

using namespace Friction::Ui;

InstallPresets::InstallPresets(QWidget *parent)
    : QWizard(parent)
{
    setWindowTitle(tr("Install Presets"));
    setMinimumWidth(700);

    setWizardStyle(QWizard::ModernStyle);
    setPixmap(QWizard::LogoPixmap,
              QIcon::fromTheme(ThemeSupport::getAppIconName(true)).pixmap(64, 64));

    setPage(0, new QuickSetupPresetsPage(this));
}

void InstallPresets::accept()
{
    {
        const auto val = field("renderProfiles");
        if (val.isValid() && !val.toStringList().isEmpty()) {
            AppSupport::installRenderPresets(true, val.toStringList());
        }
    }
    {
        const auto val = field("expressions");
        if (val.isValid() && !val.toStringList().isEmpty()) {
            AppSupport::installExprPresets(true, val.toStringList());
        }
    }

    QWizard::accept();
}

void InstallPresets::reject()
{
    QWizard::reject();
}
