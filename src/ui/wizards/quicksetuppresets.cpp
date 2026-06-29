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

#include "quicksetuppresets.h"
#include "widgets/checkboxes.h"

using namespace Friction::Ui;

QuickSetupPresetsPage::QuickSetupPresetsPage(QWidget *parent)
    : WizardPage(parent)
{
    setTitle(tr("Friction Presets"));
    setSubTitle(tr("Select the presets you want installed by default."));

    const auto layout = new QFormLayout(this);

    { // render profiles
        const QList<QPair<QString,QString>> profiles {
            {"MP4 (x264)", "001-friction-preset-mp4-h264.conf"},
            {"MP4 (x264) + Audio (MP3)", "002-friction-preset-mp4-h264-mp3.conf"},
            {"ProRes 444", "003-friction-preset-prores-444.conf"},
            {"ProRes 444 + Audio (AAC)", "004-friction-preset-prores-444-aac.conf"},
            {"WebM", "007-friction-preset-webm.conf"},
            {"PNG", "005-friction-preset-png.conf"},
            {"TIFF", "006-friction-preset-tiff.conf"}
            //{"EXR", "008-friction-preset-exr.conf"} // not supported in ffmpeg used in v1.0
        };

        const auto selector = new CheckBoxes(profiles, this);

        layout->addRow(tr("Render Profiles"), selector);

        registerField("renderProfiles", selector,
                      "selectedBoxes", SIGNAL(boxesChanged()));
    }

    addSpace(layout, 20);

    { // expressions
        const QList<QPair<QString,QString>> expressions {
            {"Copy X", "copyX.fexpr"},
            {"Copy Y", "copyY.fexpr"},
            {"Orbit X", "orbitX.fexpr"},
            {"Orbit Y", "orbitY.fexpr"},
            {"Noise", "noise.fexpr"},
            {"Oscillation", "oscillation.fexpr"},
            {"Rotation", "rotation.fexpr"},
            {"Time", "time.fexpr"},
            {"Track Object", "trackObject.fexpr"},
            {"Wave", "wave.fexpr"},
            {"Wiggle", "wiggle.fexpr"},
            {"Frame Remap Loop", "frameRemapLoop.fexpr"},
            {"Frame Remap Loop (bounce)", "frameRemapLoopBounce.fexpr"}
        };

        const auto selector = new CheckBoxes(expressions, this, 3);

        layout->addRow(tr("Expressions"), selector);

        registerField("expressions", selector,
                      "selectedBoxes", SIGNAL(boxesChanged()));
    }
}

bool QuickSetupPresetsPage::isComplete() const
{
    return true;
}
