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
#include "lottie/dotlottiewriter.h"
#include "lottie/lottiejsonoptimizer.h"
#include "lottie/lottielayerbuilder.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTemporaryDir>

#include <memory>

namespace {

QString safeAnimationId(QString name)
{
    name = name.toLower();
    name.replace(QRegularExpression(QStringLiteral("[^a-z0-9_-]+")),
                 QStringLiteral("_"));
    name.remove(QRegularExpression(QStringLiteral("^_+|_+$")));
    return name.isEmpty() ? QStringLiteral("animation") : name;
}

}

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
    const bool dotLottie = mPath.endsWith(QStringLiteral(".lottie"),
                                         Qt::CaseInsensitive);
    std::unique_ptr<QTemporaryDir> dotLottieDir;
    if (dotLottie) {
        dotLottieDir = std::make_unique<QTemporaryDir>();
        if (!dotLottieDir->isValid()) {
            RuntimeThrow("Could not create temporary dotLottie directory");
        }
    }
    const QString builderPath = dotLottie ?
                QDir(dotLottieDir->path()).filePath(QStringLiteral("animation.json")) :
                mPath;

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
                                     builderPath,
                                     dotLottie ? false : mEmbedImages,
                                     mSvgRendererFix,
                                     mNativeText);
    const auto fonts = builder.buildFonts();
    if (!fonts.value(QStringLiteral("list")).toArray().isEmpty()) {
        root.insert(QStringLiteral("fonts"), fonts);
    }
    // Group layers discover their precomposition assets while being built.
    root.insert(QStringLiteral("layers"), builder.buildLayers(mBackground));
    const auto assets = builder.buildAssets();
    if (!assets.isEmpty()) {
        root.insert(QStringLiteral("assets"), assets);
    }

    if (dotLottie) {
        const QString animationName = mScene->prp_getName();
        DotLottieWriter::write(mPath,
                               safeAnimationId(animationName),
                               animationName,
                               root,
                               QDir(dotLottieDir->path()).filePath(
                                   QStringLiteral("animation_assets")));
    } else {
        QFile file(mPath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            RuntimeThrow("Could not open:\n\"" + mPath + "\"");
        }

        const QJsonDocument doc(LottieJsonOptimizer::optimize(root));
        file.write(doc.toJson(QJsonDocument::Compact));
        file.write("\n");
        file.close();
    }
    setValue(INT_MAX);
}
