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

#ifndef SETTINGS_H
#define SETTINGS_H

#include <QtCore>
#include <QColor>
#include <QAction>

#include "skia/skiaincludes.h"
#include "efiltersettings.h"
#include "memorystructs.h"
#include "appsupport.h"
#include "grid.h"
#include "themesupport.h"
#include "Expressions/expressionpresets.h"

enum class GpuVendor {
    intel,
    amd,
    nvidia,
    unrecognized
};

enum class AccPreference : int {
    cpuStrongPreference,
    cpuSoftPreference,
    defaultPreference,
    gpuSoftPreference,
    gpuStrongPreference
};

class CORE_EXPORT eSettings : public QObject
{
    Q_OBJECT

public:
    enum AdjustSceneArgs {
        AdjustSceneAsk,
        AdjustSceneAlways,
        AdjustSceneNever
    };
    enum ImportFileDirOpt {
        ImportFileDirRecent,
        ImportFileDirProject
    };
    eSettings(const int cpuThreads,
              const intKB ramKB);

    // accessors
    static intMB sRamMBCap();
    static int sCpuThreadsCapped();
    static const QString& sSettingsDir();
    static const QString& sIconsDir();

    static const eSettings &instance();
    static eSettings* sInstance;

    void loadDefaults();
    void loadFromFile();
    void saveToFile();
    void saveKeyToFile(const QString &key);

    // general
    QString fUserSettingsDir;

    // performance settings
    const int fCpuThreads;
    int fCpuThreadsCap = 0; // <= 0 - use all available threads

    const intKB fRamKB;
    intMB fRamMBCap = intMB(0); // <= 0 - cap at 80 %

    AccPreference fAccPreference = AccPreference::defaultPreference;
    bool fPathGpuAcc = true;

    // MSAA
    int fInternalMultisampleCount = 4;

    int fImportFileDirOpt = ImportFileDirRecent;

    bool fHddCache = true;
    QString fHddCacheFolder = ""; // "" - use system default temporary files folder
    intMB fHddCacheMBCap = intMB(0); // <= 0 - no cap

    // history
    int fUndoCap = 25; // <= 0 - no cap

    enum class AutosaveTarget {
        dedicated_folder,
        same_folder
    };

    int fQuickSaveCap = 5; // <= 0 - no cap
    int fAutoQuickSaveMin = 0; // <= 0 - disabled
    AutosaveTarget fQuickSaveTarget = AutosaveTarget::same_folder;

    // ui settings
    qreal fInterfaceScaling;
    qreal fCurrentInterfaceDPI;
    bool fDefaultInterfaceScaling;

    // restore ui on quit
    bool fRestoreDefaultUi = false;

    // canvas settings
    bool fCanvasRtlSupport;

    qreal fPathNodeScaling;
    QColor fPathNodeColor;
    QColor fPathNodeSelectedColor;

    qreal fPathDissolvedNodeScaling;
    QColor fPathDissolvedNodeColor;
    QColor fPathDissolvedNodeSelectedColor;

    qreal fPathControlScaling;
    QColor fPathControlColor;
    QColor fPathControlSelectedColor;

    int fAdjustSceneFromFirstClip = AdjustSceneAsk;

    int fDefaultFillStrokeIndex = 0;

    bool fPreviewCache = true;

    // timeline settings
    bool fTimelineAlternateRow = true;
    QColor fTimelineAlternateRowColor = QColor(0, 0, 0, 25);
    bool fTimelineHighlightRow = true;
    QColor fTimelineHighlightRowColor = ThemeSupport::getThemeHighlightColor(15);

    QColor fObjectKeyframeColor;
    QColor fPropertyGroupKeyframeColor;
    QColor fPropertyKeyframeColor;
    QColor fSelectedKeyframeColor;

    QColor fVisibilityRangeColor = ThemeSupport::getThemeRangeColor();
    QColor fSelectedVisibilityRangeColor = ThemeSupport::getThemeRangeSelectedColor();
    QColor fAnimationRangeColor = QColor(0, 0, 0, 55);

    // command palette
    QList<QAction*> fCommandPalette;
    QList<QString> fCommandHistory;

    // expressions presets
    Friction::Core::ExpressionPresets fExpressions;

    // grid settings
    Friction::Core::Grid::Settings fGrid = Friction::Core::Grid::loadSettings();

    // last used stroke color
    QColor fLastUsedStrokeColor = AppSupport::getSettings("FillStroke",
                                                          "LastStrokeColor",
                                                          ThemeSupport::getThemeObjectColor()).value<QColor>();

    // last used stoke size
    qreal fLastUsedStrokeWidth = AppSupport::getSettings("FillStroke",
                                                         "LastStrokeWidth",
                                                         10.).toDouble();

    // last used fill color
    QColor fLastUsedFillColor = AppSupport::getSettings("FillStroke",
                                                        "LastFillColor",
                                                        QColor(Qt::white)).value<QColor>();

    // last fill/stroke state
    bool fLastFillFlatEnabled = AppSupport::getSettings("FillStroke",
                                                        "LastFillFlat",
                                                        false).toBool();
    bool fLastStrokeFlatEnabled = AppSupport::getSettings("FillStroke",
                                                          "LastStrokeFlat",
                                                          true).toBool();

signals:
    void settingsChanged();

private:
    QString mIconsDir;
};

#endif // SETTINGS_H
