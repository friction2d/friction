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

#ifndef FRICTION_WIZARD_PAGE_H
#define FRICTION_WIZARD_PAGE_H

#include "ui_global.h"

#include <QWizardPage>
#include <QFormLayout>

namespace Friction
{
    namespace Ui
    {
        class UI_EXPORT WizardPage : public QWizardPage
        {
            Q_OBJECT

        public:
            explicit WizardPage(QWidget *parent = nullptr);

            void addSpace(QFormLayout *layout,
                          const int &height = 10);
        };
    }
}

#endif // FRICTION_WIZARD_PAGE_H
