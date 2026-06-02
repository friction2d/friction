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
*/

#include "lottie/lottieexporter.h"

#include "appsupport.h"
#include "canvas.h"
#include "exceptions.h"
#include "lottie/lottielayerbuilder.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

LottieExporter::LottieExporter(const QString& path,
                               Canvas* const scene,
                               const FrameRange& frameRange,
                               const qreal fps,
                               const bool background)
    : ComplexTask(INT_MAX, tr("Lottie Export"))
    , mPath(path)
    , mScene(scene)
    , mFrameRange(frameRange)
    , mFps(fps)
    , mBackground(background)
{

}

void LottieExporter::nextStep()
{
    finish();
}

void LottieExporter::finish()
{
    if (!mScene) { RuntimeThrow("No scene selected"); }

    QJsonObject root;
    root.insert(QStringLiteral("v"), QStringLiteral("5.7.11"));
    root.insert(QStringLiteral("fr"), mFps);
    root.insert(QStringLiteral("ip"), mFrameRange.fMin);
    root.insert(QStringLiteral("op"), mFrameRange.fMax + 1);
    root.insert(QStringLiteral("w"), mScene->getCanvasWidth());
    root.insert(QStringLiteral("h"), mScene->getCanvasHeight());
    root.insert(QStringLiteral("nm"), mScene->prp_getName());
    root.insert(QStringLiteral("ddd"), 0);
    root.insert(QStringLiteral("assets"), QJsonArray());
    root.insert(QStringLiteral("markers"), QJsonArray());
    root.insert(QStringLiteral("meta"), QJsonObject{
                    {QStringLiteral("g"),
                     QStringLiteral("%1 - %2").arg(AppSupport::getAppDisplayName(),
                                                   AppSupport::getAppUrl())}
                });

    const LottieLayerBuilder builder(mScene, mFrameRange, mFps);
    root.insert(QStringLiteral("layers"), builder.buildLayers(mBackground));

    QFile file(mPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        RuntimeThrow("Could not open:\n\"" + mPath + "\"");
    }

    const QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.write("\n");
    file.close();
    setValue(INT_MAX);
}
