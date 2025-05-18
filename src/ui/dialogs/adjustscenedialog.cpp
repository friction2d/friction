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

#include "adjustscenedialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

#include "Private/esettings.h"

AdjustSceneDialog::AdjustSceneDialog(Scene *scene,
                                     const VideoBox::VideoSpecs &specs,
                                     QWidget *parent)
    : Friction::Ui::Dialog(parent)
{
    if (!scene ||
        specs.dim.width() < 1 ||
        specs.dim.height() < 1 ||
        specs.fps < 1) {
        reject();
        return;
    }

    setMinimumWidth(400);
    setWindowTitle(tr("Adjust scene to clip?"));

    const auto mLayout = new QVBoxLayout(this);

    const auto infoWidget = new QWidget(this);
    const auto infoLayout = new QHBoxLayout(infoWidget);

    const auto buttonsWidget = new QWidget(this);
    const auto buttonsLayout = new QHBoxLayout(buttonsWidget);

    const auto pixLabel = new QLabel(this);
    pixLabel->setMaximumWidth(96);
    pixLabel->setPixmap(QIcon::fromTheme("seq_preview").pixmap(96, 96));

    const auto infoLabel = new QLabel(this);
    infoLabel->setTextFormat(Qt::RichText);
    infoLabel->setWordWrap(true);
    infoLabel->setText(QString("<style>th { text-align: left; } table { margin-top: 1em; }</style>"
                               "<h3>%9</h3>"
                               "<table width=\"100%\">"
                               "<tr><th>%13 %10</th><th>%13 %11</th><th>%13 %12</th></tr>"
                               "<tr><td>%1x%2</td><td>%3</td><td>%4</td></tr>"
                               "<tr><td></td><td></td><td></td></tr>"
                               "<tr><th>%14 %10</th><th>%14 %11</th><th>%14 %12</th></tr>"
                               "<tr><td>%5x%6</td><td>%7</td><td>%8</td></tr>"
                               "</table>")
                           .arg(QString::number(specs.dim.width()),
                                QString::number(specs.dim.height()),
                                QString::number(specs.fps),
                                QString::number(specs.range.fMax),
                                QString::number(scene->getCanvasWidth()),
                                QString::number(scene->getCanvasHeight()),
                                QString::number(scene->getFps()),
                                QString::number(scene->getFrameRange().fMax),
                                tr("Adjust scene to clip?"),
                                tr("Dimension"),
                                tr("Fps"),
                                tr("Frames"),
                                tr("Clip"),
                                tr("Scene")));

    const auto buttonYes = new QPushButton(tr("Yes"), this);
    const auto buttonNo = new QPushButton(tr("No"), this);
    const auto buttonAlways = new QPushButton(tr("Always"), this);
    const auto buttonNever = new QPushButton(tr("Never"), this);

    mLayout->addWidget(infoWidget);
    mLayout->addStretch();
    mLayout->addWidget(buttonsWidget);
    infoLayout->addWidget(pixLabel);
    infoLayout->addWidget(infoLabel);
    buttonsLayout->addWidget(buttonAlways);
    buttonsLayout->addWidget(buttonNever);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(buttonYes);
    buttonsLayout->addWidget(buttonNo);

    connect(buttonYes, &QPushButton::clicked,
            this, [scene, &specs, this] {
        scene->setCanvasSize(specs.dim.width(),
                             specs.dim.height());
        scene->setFps(specs.fps);
        scene->setFrameRange(specs.range);
        accept();
    });

    connect(buttonAlways, &QPushButton::clicked,
            this, [scene, &specs, this]() {
        const auto settings = eSettings::sInstance;
        settings->fAdjustSceneFromFirstClip = eSettings::AdjustSceneAlways;
        settings->saveKeyToFile("AdjustSceneFromFirstClip");
        scene->setCanvasSize(specs.dim.width(),
                             specs.dim.height());
        scene->setFps(specs.fps);
        scene->setFrameRange(specs.range);
        accept();
    });

    connect(buttonNo, &QPushButton::clicked,
            this, [this]() { reject(); });

    connect(buttonNever, &QPushButton::clicked,
            this, [this]() {
        const auto settings = eSettings::sInstance;
        settings->fAdjustSceneFromFirstClip = eSettings::AdjustSceneNever;
        settings->saveKeyToFile("AdjustSceneFromFirstClip");
        reject();
    });
}
