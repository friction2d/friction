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

#include "canvas.h"
#include "exceptions.h"
#include "lottie/lottiejsonoptimizer.h"
#include "lottie/lottielayerbuilder.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

LottieExporter::LottieExporter(const QString& path,
                               Canvas* const scene,
                               const FrameRange& frameRange,
                               const qreal fps,
                               const bool background,
                               const bool embedImages,
                               const bool svgRendererFix,
                               const bool nativeText)
    : ComplexTask(INT_MAX, tr("Lottie Export"))
    , mPath(path)
    , mScene(scene)
    , mFrameRange(frameRange)
    , mFps(fps)
    , mBackground(background)
    , mEmbedImages(embedImages)
    , mSvgRendererFix(svgRendererFix)
    , mNativeText(nativeText)
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

    const LottieLayerBuilder builder(mScene,
                                     mFrameRange,
                                     mFps,
                                     mPath,
                                     mEmbedImages,
                                     mSvgRendererFix,
                                     mNativeText);
    const auto fonts = builder.buildFonts();
    if (!fonts.value(QStringLiteral("list")).toArray().isEmpty()) {
        root.insert(QStringLiteral("fonts"), fonts);
    }
    const auto assets = builder.buildAssets();
    if (!assets.isEmpty()) {
        root.insert(QStringLiteral("assets"), assets);
    }
    root.insert(QStringLiteral("layers"), builder.buildLayers(mBackground));

    QFile file(mPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        RuntimeThrow("Could not open:\n\"" + mPath + "\"");
    }

    const QJsonDocument doc(LottieJsonOptimizer::optimize(root));
    file.write(doc.toJson(QJsonDocument::Compact));
    file.write("\n");
    file.close();
    setValue(INT_MAX);
}
