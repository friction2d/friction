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

#include "framespinbox.h"
#include "appsupport.h"

void FrameSpinBox::setDisplayTimeCode(const bool &enabled)
{
    setSingleStep(enabled && mFps > 0. ? mFps : 1);
    mDisplayTimecode = enabled;
    setSpecialValueText(enabled ? "00:00:00:00" : "");
    updateGeometry();
}

void FrameSpinBox::updateFps(const qreal &fps)
{
    mFps = fps;
}

QString FrameSpinBox::textFromValue(int value) const
{
    return mDisplayTimecode ? AppSupport::getTimeCodeFromFrame(value, mFps) : QSpinBox::textFromValue(value);
}

void FrameSpinBox::fixup(QString &str) const
{
    if (mDisplayTimecode) {
        str = QString::number(AppSupport::getFrameFromTimeCode(str, mFps));
    }
    QSpinBox::fixup(str);
}

void FrameSpinBox::wheelEvent(QWheelEvent *event)
{
    const int val = value();

    QSpinBox::wheelEvent(event);

    if (val != value()) {
        emit wheelValueChanged(value());
    }
}
