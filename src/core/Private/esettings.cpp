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

#include "esettings.h"

#include "GUI/global.h"
#include "exceptions.h"

#include "smartPointers/stdselfref.h"

struct eSetting
{
    eSetting(const QString& name) : fName(name) {}
    virtual ~eSetting() = default;

    virtual bool setValueString(const QString& value) = 0;
    virtual void writeValue() const = 0;
    virtual void loadDefault() = 0;

    const QString fName;
};

template <typename T>
struct eSettingBase : public eSetting
{
    eSettingBase(T &value,
                 const QString &name,
                 const T &defaultValue)
        : eSetting(name)
        , fValue(value)
        , fDefault(defaultValue) {}

    void loadDefault() { fValue = fDefault; };

    T &fValue;
    const T fDefault;
};

struct eBoolSetting : public eSettingBase<bool>
{
    using eSettingBase<bool>::eSettingBase;

    bool setValueString(const QString& valueStr) {
        const bool value = valueStr == "enabled";
        bool ok = value || valueStr == "disabled";
        if(ok) fValue = value;
        return ok;
    }

    void writeValue() const {
        AppSupport::setSettings("settings", fName, fValue ? "enabled" : "disabled");
    }
};

struct eIntSetting : public eSettingBase<int>
{
    using eSettingBase<int>::eSettingBase;

    bool setValueString(const QString& valueStr) {
        bool ok;
        const int value = valueStr.toInt(&ok);
        if(ok) fValue = value;
        return ok;
    }

    void writeValue() const {
        AppSupport::setSettings("settings", fName, fValue);
    }
};

struct eQrealSetting : public eSettingBase<qreal>
{
    using eSettingBase<qreal>::eSettingBase;

    bool setValueString(const QString& valueStr) {
        bool ok;
        const qreal value = valueStr.toDouble(&ok);
        if(ok) fValue = value;
        return ok;
    }

    void writeValue() const {
        AppSupport::setSettings("settings", fName, fValue);
    }
};

struct eStringSetting : public eSettingBase<QString>
{
    using eSettingBase<QString>::eSettingBase;

    bool setValueString(const QString& valueStr) {
        fValue = valueStr;
        return true;
    }

    void writeValue() const {
        AppSupport::setSettings("settings", fName, fValue);
    }
};

struct eColorSetting : public eSettingBase<QColor>
{
    using eSettingBase<QColor>::eSettingBase;

    bool setValueString(const QString& valueStr) {
        const QString oneVal = QStringLiteral("\\s*(\\d+)\\s*");
        const QString oneValC = QStringLiteral("\\s*(\\d+)\\s*,");

        QRegExp rx("rgba\\("
                        "\\s*(\\d+)\\s*,"
                        "\\s*(\\d+)\\s*,"
                        "\\s*(\\d+)\\s*,"
                        "\\s*(\\d+)\\s*"
                   "\\)",
                   Qt::CaseInsensitive);
        if(rx.exactMatch(valueStr)) {
            rx.indexIn(valueStr);
            const QStringList intRGBA = rx.capturedTexts();
            fValue.setRgb(intRGBA.at(1).toInt(),
                          intRGBA.at(2).toInt(),
                          intRGBA.at(3).toInt(),
                          intRGBA.at(4).toInt());
            return true;
        } else return false;
    }

    void writeValue() const {
        QString col = QString("rgba(%1, %2, %3, %4)").
                              arg(fValue.red()).
                              arg(fValue.green()).
                              arg(fValue.blue()).
                              arg(fValue.alpha());
        AppSupport::setSettings("settings", fName, col);
    }
};

eSettings* eSettings::sInstance = nullptr;

static QList<stdsptr<eSetting>> gSettings;

eSettings::eSettings(const int cpuThreads,
                     const intKB ramKB)
    : fUserSettingsDir(AppSupport::getAppConfigPath())
    , fCpuThreads(cpuThreads)
    , fRamKB(ramKB)
{
    Q_ASSERT(!sInstance);
    sInstance = this;

    gSettings << std::make_shared<eIntSetting>(
                     fCpuThreadsCap,
                     "cpuThreadsCap", 0);
    gSettings << std::make_shared<eIntSetting>(
                     reinterpret_cast<int&>(fRamMBCap),
                     "ramMBCap", 0);
    gSettings << std::make_shared<eIntSetting>(
                     reinterpret_cast<int&>(fAccPreference),
                     "accPreference",
                     static_cast<int>(AccPreference::defaultPreference));
    gSettings << std::make_shared<eBoolSetting>(
                     fPathGpuAcc,
                     "pathGpuAcc", true);
    gSettings << std::make_shared<eIntSetting>(
                     fInternalMultisampleCount,
                     "msaa", 4);
    gSettings << std::make_shared<eBoolSetting>(
                     fHddCache,
                     "hddCache", true);
    gSettings << std::make_shared<eIntSetting>(
                     reinterpret_cast<int&>(fHddCacheMBCap),
                     "hddCacheMBCap", 0);

    gSettings << std::make_shared<eQrealSetting>(
                     fInterfaceScaling,
                     "interfaceScaling", 1.);
    gSettings << std::make_shared<eBoolSetting>(
                     fDefaultInterfaceScaling,
                     "defaultInterfaceScaling", true);

    gSettings << std::make_shared<eIntSetting>(fImportFileDirOpt,
                                               "ImportFileDirOpt",
                                               ImportFileDirRecent);

    gSettings << std::make_shared<eBoolSetting>(
                     fCanvasRtlSupport,
                     "rtlTextSupport", false);

    gSettings << std::make_shared<eColorSetting>(
                     fPathNodeColor,
                     "pathNodeColor",
                     QColor(170, 240, 255));
    gSettings << std::make_shared<eColorSetting>(
                     fPathNodeSelectedColor,
                     "pathNodeSelectedColor",
                     QColor(0, 200, 255));
    gSettings << std::make_shared<eQrealSetting>(
                     fPathNodeScaling,
                     "pathNodeScaling", 1.);

    gSettings << std::make_shared<eColorSetting>(
                     fPathDissolvedNodeColor,
                     "pathDissolvedNodeColor",
                     QColor(255, 120, 120));
    gSettings << std::make_shared<eColorSetting>(
                     fPathDissolvedNodeSelectedColor,
                     "pathDissolvedNodeSelectedColor",
                     QColor(255, 0, 0));
    gSettings << std::make_shared<eQrealSetting>(
                     fPathDissolvedNodeScaling,
                     "pathDissolvedNodeScaling", 1.);

    gSettings << std::make_shared<eColorSetting>(
                     fPathControlColor,
                     "pathControlColor",
                     QColor(255, 175, 175));
    gSettings << std::make_shared<eColorSetting>(
                     fPathControlSelectedColor,
                     "pathControlSelectedColor",
                     QColor(255, 0, 0));
    gSettings << std::make_shared<eQrealSetting>(
                     fPathControlScaling,
                     "pathControlScaling", 1.);

    gSettings << std::make_shared<eIntSetting>(fAdjustSceneFromFirstClip,
                                               "AdjustSceneFromFirstClip",
                                               AdjustSceneAsk);

    gSettings << std::make_shared<eIntSetting>(fDefaultFillStrokeIndex,
                                              "DefaultFillStrokeIndex",
                                               0);

    gSettings << std::make_shared<eBoolSetting>(fPreviewCache,
                                                "PreviewCache",
                                                true);
    /*gSettings << std::make_shared<eBoolSetting>(
                     fTimelineAlternateRow,
                     "timelineAlternateRow", true);
    gSettings << std::make_shared<eColorSetting>(
                     fTimelineAlternateRowColor,
                     "timelineAlternateRowColor",
                     QColor(0, 0, 0, 25));
    gSettings << std::make_shared<eBoolSetting>(
                     fTimelineHighlightRow,
                     "timelineHighlightRow", true);
    gSettings << std::make_shared<eColorSetting>(
                     fTimelineHighlightRowColor,
                     "timelineHighlightRowColor",
                     QColor(255, 0, 0, 15));*/

    gSettings << std::make_shared<eColorSetting>(
                     fObjectKeyframeColor,
                     "objectKeyframeColor",
                     ThemeSupport::getThemeColorBlue());
    gSettings << std::make_shared<eColorSetting>(
                     fPropertyGroupKeyframeColor,
                     "propertyGroupKeyframeColor",
                     ThemeSupport::getThemeColorGreen());
    gSettings << std::make_shared<eColorSetting>(
                     fPropertyKeyframeColor,
                     "propertyKeyframeColor",
                     ThemeSupport::getThemeColorRed());
    gSettings << std::make_shared<eColorSetting>(
                     fSelectedKeyframeColor,
                     "selectedKeyframeColor",
                     ThemeSupport::getThemeColorYellow());

    /*gSettings << std::make_shared<eColorSetting>(
                     fVisibilityRangeColor,
                     "visibilityRangeColor",
                     QColor(131, 92, 255, 255));
    gSettings << std::make_shared<eColorSetting>(
                     fSelectedVisibilityRangeColor,
                     "selectedVisibilityRangeColor",
                     QColor(116, 18, 98, 255));
    gSettings << std::make_shared<eColorSetting>(
                     fAnimationRangeColor,
                     "animationRangeColor",
                     QColor(0, 0, 0, 55));*/

    loadDefaults();

    eSizesUI::widget.add(this, [this](const int size) {
        mIconsDir = sSettingsDir() + "/icons/" + QString::number(size);
    });
}

int eSettings::sCpuThreadsCapped()
{
    if (sInstance->fCpuThreadsCap > 0) {
        return sInstance->fCpuThreadsCap;
    }
    return sInstance->fCpuThreads;
}

intMB eSettings::sRamMBCap()
{
    if (sInstance->fRamMBCap.fValue > 0) { return sInstance->fRamMBCap; }
    auto mbTot = intMB(sInstance->fRamKB);
    mbTot.fValue *= 8; mbTot.fValue /= 10;
    return mbTot;
}

const QString &eSettings::sSettingsDir()
{
    return sInstance->fUserSettingsDir;
}

const QString &eSettings::sIconsDir()
{
    return sInstance->mIconsDir;
}

const eSettings &eSettings::instance()
{
    return *sInstance;
}

void eSettings::loadDefaults()
{
    for (auto& setting : gSettings) {
        setting->loadDefault();
    }
}

void eSettings::loadFromFile()
{
    loadDefaults();

    for(auto &setting : gSettings) {
        QString val = AppSupport::getSettings("settings",
                                              setting->fName,
                                              QString()).toString();
        if (!val.isEmpty()) { setting->setValueString(val); }
    }

    eSizesUI::font.updateSize();
    eSizesUI::widget.updateSize();
}

void eSettings::saveToFile()
{
    for (const auto& setting : qAsConst(gSettings)) {
        setting->writeValue();
    }
}

void eSettings::saveKeyToFile(const QString &key)
{
    for (const auto& setting : qAsConst(gSettings)) {
        if (key == setting->fName) {
            setting->writeValue();
            return;
        }
    }
}
