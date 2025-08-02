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

#ifndef FRAMESPINBOX_H
#define FRAMESPINBOX_H

#include "ui_global.h"

#include <QSpinBox>

class UI_EXPORT FrameSpinBox : public QSpinBox
{
    Q_OBJECT

public:
    using QSpinBox::QSpinBox;
    void setDisplayTimeCode(const bool &enabled);
    void updateFps(const qreal &fps);
    QString textFromValue(int value) const override;
    void fixup(QString &str) const override;

signals:
    void wheelValueChanged(int value);

protected:
    void wheelEvent(QWheelEvent *event) override;

private:
    bool mDisplayTimecode = false;
    qreal mFps = 0;
};

#endif // FRAMESPINBOX_H
