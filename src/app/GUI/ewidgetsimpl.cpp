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

// Fork of enve - Copyright (C) 2016-2020 Maurycy Liebner

#include "ewidgetsimpl.h"

#include "widgets/colorsettingswidget.h"

using namespace Friction;

eWidgetsImpl::eWidgetsImpl() {}

QWidget *eWidgetsImpl::colorWidget(QWidget * const parent,
                                   const QColor &iniColor,
                                   QObject * const receiver,
                                   const Func<void (const ColorSetting &)> &slot) {
    const auto wid = new Ui::ColorSettingsWidget(parent,
                                                 true,
                                                 true,
                                                 true,
                                                 false,
                                                 "widget");
    wid->setDisplayedColor(iniColor);
    if(slot) {
        QObject::connect(wid, &Ui::ColorSettingsWidget::colorSettingSignal,
                         receiver, slot);
    }
    return wid;
}

QWidget *eWidgetsImpl::colorWidget(QWidget * const parent,
                                   ColorAnimator * const target) {
    const auto wid = new Ui::ColorSettingsWidget(parent,
                                                 true,
                                                 true,
                                                 true,
                                                 false,
                                                 "widget");
    wid->setTarget(target);
    return wid;
}

QWidget *eWidgetsImpl::colorWidget(QWidget * const parent,
                                   ColorAnimator * const target,
                                   const bool showColorMode)
{
    const auto wid = new Ui::ColorSettingsWidget(parent,
                                                 true,
                                                 true,
                                                 true,
                                                 false,
                                                 "widget");
    wid->setTarget(target);
    wid->setColorModeVisible(showColorMode);
    return wid;
}
