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

#include "quicksetupgeneral.h"

#include <QLineEdit>
#include <QToolButton>
#include <QComboBox>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>

#include "appsupport.h"

using namespace Friction::Ui;

QuickSetupGeneralPage::QuickSetupGeneralPage(QWidget *parent)
    : WizardPage(parent)
{
    setTitle(tr("Welcome to Friction"));
    setSubTitle(tr("Set your core preferences to get started right away."));

    const auto layout = new QFormLayout(this);

    { // custom browser path
        const auto wid = new QWidget(this);
        wid->setContentsMargins(0, 0, 0, 0);

        const auto lay = new QHBoxLayout(wid);
        lay->setContentsMargins(0, 0, 0, 0);

        const auto browserPath = new QLineEdit(this);
        browserPath->setReadOnly(true);
        browserPath->setPlaceholderText(tr("System Default"));
        browserPath->setFocusPolicy(Qt::NoFocus);

        const auto browserPathBtn = new QToolButton(this);
        browserPathBtn->setText("...");

        const QString customBrowser = AppSupport::getSettings("settings",
                                                              "CustomBrowserPath").toString();

        if (!customBrowser.trimmed().isEmpty()) {
            browserPath->setText(customBrowser);
        }

        lay->addWidget(browserPath);
        lay->addWidget(browserPathBtn);

        layout->addRow(tr("Web Browser"), wid);

        registerField("CustomBrowserPath", browserPath);

        connect(browserPathBtn, &QToolButton::clicked,
                [browserPath, this]() {
                    QString file = AppSupport::getOpenFile(this,
                                                           tr("Select Browser Executable"),
                                                           QString(),
                                                           tr("Executables (*);;All Files (*)"));
                    if (!file.isEmpty()) { browserPath->setText(file); }
        });
    }

    { // custom cache path
        const auto wid = new QWidget(this);
        wid->setContentsMargins(0, 0, 0, 0);

        const auto lay = new QHBoxLayout(wid);
        lay->setContentsMargins(0, 0, 0, 0);

        const auto cachePath = new QLineEdit(this);
        cachePath->setReadOnly(true);
        cachePath->setPlaceholderText(tr("System Default"));
        cachePath->setFocusPolicy(Qt::NoFocus);

        const auto cachePathBtn = new QToolButton(this);
        cachePathBtn->setText("...");

        const QString customCache = AppSupport::getSettings("settings",
                                                            "CustomCachePath").toString();

        if (!customCache.trimmed().isEmpty()) {
            cachePath->setText(customCache);
        }

        lay->addWidget(cachePath);
        lay->addWidget(cachePathBtn);

        layout->addRow(tr("Cache Path"), wid);

        registerField("CustomCachePath", cachePath);

        connect(cachePathBtn, &QToolButton::clicked,
                [cachePath, this]() {
                    QString dir = AppSupport::getExistingDirectory(this,
                                                                   tr("Select Cache Folder"),
                                                                   cachePath->text());
                    if (!dir.isEmpty()) { cachePath->setText(dir); }
        });

    }

    { // import dir
        const auto combo = new QComboBox(this);
        combo->addItems({tr("Last used directory"), tr("Project directory")});
        registerField("ImportFileDirOpt", combo);
        layout->addRow(tr("Default Import Directory"), combo);
    }

    addSpace(layout);

    { // memory usage
        const auto wid = new QWidget(this);
        wid->setContentsMargins(0, 0, 0, 0);

        const auto lay = new QHBoxLayout(wid);
        lay->setContentsMargins(0, 0, 0, 0);

        const auto slider = new QSlider(Qt::Horizontal, wid);
        slider->setRange(10, 95);

        const double ramMax = static_cast<double>(AppSupport::getTotalRamBytes().fValue);
        const auto ramLabel = new QLabel(wid);

        lay->addWidget(slider);
        lay->addWidget(ramLabel);

        layout->addRow(tr("Memory Usage"), wid);

        registerField("ramLimit", slider, "value");

        connect(slider, &QSlider::valueChanged, [this,
                                                 ramLabel,
                                                 ramMax](int v) {
            const auto l = this->locale();
            const double p = ramMax * (v/100.);
            const QString r = l.formattedDataSize(p * 1024);
            ramLabel->setText(tr("%1% (%2)").arg(QString::number(v), r));
        });

        slider->setValue(70);
    }

    addSpace(layout, 20);

    { // default scene
        const QList<double> fpsList{24., 25., 30.,
                                    50., 60., 90., 120.};

        const QList<QStringList> resList{{"1280", "720", "HD (720p)"},
                                         {"1920", "1080", "FHD (1080p)"},
                                         {"2560", "1440", "QHD (1440p)"},
                                         {"3840", "2160", "UHD (2160p)"}};

        const QList<QPair<QString,int>> aspectList{{tr("Landscape"), 0},
                                                    {tr("Portrait"), 1},
                                                    {tr("Square"), 2}};

        const auto pickAspect = new QComboBox(this);
        const auto pickResolution = new QComboBox(this);
        const auto pickFps = new QComboBox(this);
        const auto pickDur = new QSpinBox(this);

        pickDur->setMinimum(1);
        pickDur->setValue(1);
        pickDur->setSuffix(tr(" min"));

        for (const auto &res : resList) {
            pickResolution->addItem(res.at(2),
                                    QStringList{res.at(0),
                                                res.at(1)});
        }
        pickResolution->setCurrentIndex(pickResolution->findText("FHD (1080p)"));

        for (const auto &fps : fpsList) {
            pickFps->addItem(QString::number(fps), fps);
        }
        pickFps->setCurrentIndex(pickFps->findData(30.));

        for (const auto &aspect : aspectList) {
            pickAspect->addItem(aspect.first, aspect.second);
        }

        const auto wid = new QWidget(this);
        wid->setContentsMargins(0, 0, 0, 0);

        const auto lay = new QHBoxLayout(wid);
        lay->setContentsMargins(0, 0, 0, 0);

        lay->addWidget(pickResolution);
        lay->addWidget(pickAspect);

        layout->addRow(tr("Default Resolution"), wid);
        layout->addRow(tr("Default FPS"), pickFps);
        layout->addRow(tr("Default Duration"), pickDur);

        registerField("resolution", pickResolution, "currentData");
        registerField("fps", pickFps, "currentData");
        registerField("aspect", pickAspect);
        registerField("duration", pickDur);
    }

    addSpace(layout, 20);

    { // gizmos
        auto addOption = [&](const QString &name,
                             const QString &field,
                             const QString &desc) {
            const auto check = new QCheckBox(name);
            const auto hint = new QLabel(desc);
            check->setChecked(field == "enableGrid" ? false : true);
            hint->setWordWrap(true);
            hint->setObjectName("wizardHint");
            registerField(field, check);
            layout->addRow(check, hint);
        };

        addOption(tr("Gizmos"),
                  "enableGizmos",
                  tr("Show interactive controls to move, rotate, and scale objects in the viewport."));
        addOption(tr("Snapping"),
                  "enableSnapping",
                  tr("Snap objects precisely to points, edges, and other elements during alignment."));
        addOption(tr("Grid"),
                  "enableGrid",
                  tr("Display a background grid for structural layout and proportions."));
    }
}

bool QuickSetupGeneralPage::isComplete() const
{
    return true;
}
