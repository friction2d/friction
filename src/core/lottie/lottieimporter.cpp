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
*/

#include "lottie/lottieimporter.h"

#include "Animators/SmartPath/smartpathanimator.h"
#include "Animators/SmartPath/smartpathcollection.h"
#include "Animators/coloranimator.h"
#include "Animators/gradientpoints.h"
#include "Animators/intanimator.h"
#include "Animators/paintsettingsanimator.h"
#include "Animators/qpointfanimator.h"
#include "Animators/qrealanimator.h"
#include "Animators/qstringanimator.h"
#include "Animators/transformanimator.h"
#include "Boxes/circle.h"
#include "Boxes/containerbox.h"
#include "Boxes/imagebox.h"
#include "Boxes/nullobject.h"
#include "Boxes/pathbox.h"
#include "Boxes/rectangle.h"
#include "Boxes/smartvectorpath.h"
#include "Boxes/textbox.h"
#include "PathEffects/dashpatheffect.h"
#include "PathEffects/duplicatepatheffect.h"
#include "PathEffects/subpatheffect.h"
#include "Properties/boolproperty.h"
#include "TransformEffects/parenteffect.h"
#include "canvas.h"
#include "exceptions.h"
#include "paintsettings.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QMap>
#include <QDir>
#include <QSet>
#include <QtEndian>
#include <QtMath>

#include <cstring>

#include <zlib.h>

namespace {

struct KeyValue {
    int frame = 0;
    QJsonValue value;
    bool hold = false;
};

struct PaintStyle {
    bool hasFill = false;
    QColor fill = Qt::black;
    bool hasFillGradient = false;
    bool hasStroke = false;
    QColor stroke = Qt::black;
    bool hasStrokeGradient = false;
    qreal strokeWidth = 1;
};

struct ZipEntry {
    QString name;
    quint16 method = 0;
    quint32 compressedSize = 0;
    quint32 uncompressedSize = 0;
    quint32 localHeaderOffset = 0;
};

SkBlendMode blendModeFromLottie(const int mode)
{
    switch (mode) {
    case 1: return SkBlendMode::kMultiply;
    case 2: return SkBlendMode::kScreen;
    case 3: return SkBlendMode::kOverlay;
    case 4: return SkBlendMode::kDarken;
    case 5: return SkBlendMode::kLighten;
    case 6: return SkBlendMode::kColorDodge;
    case 7: return SkBlendMode::kColorBurn;
    case 8: return SkBlendMode::kHardLight;
    case 9: return SkBlendMode::kSoftLight;
    case 10: return SkBlendMode::kDifference;
    case 11: return SkBlendMode::kExclusion;
    case 12: return SkBlendMode::kHue;
    case 13: return SkBlendMode::kSaturation;
    case 14: return SkBlendMode::kColor;
    case 15: return SkBlendMode::kLuminosity;
    case 16: return SkBlendMode::kPlus;
    default: return SkBlendMode::kSrcOver;
    }
}

using AssetMap = QMap<QString, QJsonObject>;

struct ImportedLayer {
    QJsonObject layer;
    qsptr<BoundingBox> box;
};

QJsonObject firstItemOfType(const QJsonArray& items, const QString& type);

quint16 readLe16(const QByteArray& data, const int offset)
{
    if (offset < 0 || offset + 2 > data.size()) { RuntimeThrow("Invalid ZIP data"); }
    return qFromLittleEndian<quint16>(
                reinterpret_cast<const uchar*>(data.constData() + offset));
}

quint32 readLe32(const QByteArray& data, const int offset)
{
    if (offset < 0 || offset + 4 > data.size()) { RuntimeThrow("Invalid ZIP data"); }
    return qFromLittleEndian<quint32>(
                reinterpret_cast<const uchar*>(data.constData() + offset));
}

int findZipEndOfCentralDirectory(const QByteArray& data)
{
    const int first = qMax(0, data.size() - 65557);
    for (int offset = data.size() - 22; offset >= first; --offset) {
        if (readLe32(data, offset) == 0x06054b50) { return offset; }
    }
    RuntimeThrow("Could not find dotLottie ZIP directory");
}

QList<ZipEntry> readZipEntries(const QByteArray& data)
{
    const int end = findZipEndOfCentralDirectory(data);
    const quint16 entryCount = readLe16(data, end + 10);
    const quint32 directorySize = readLe32(data, end + 12);
    const quint32 directoryOffset = readLe32(data, end + 16);
    if (directoryOffset + directorySize > static_cast<quint32>(data.size())) {
        RuntimeThrow("Invalid dotLottie ZIP directory");
    }

    QList<ZipEntry> entries;
    int offset = directoryOffset;
    for (int i = 0; i < entryCount; ++i) {
        if (readLe32(data, offset) != 0x02014b50) {
            RuntimeThrow("Invalid dotLottie ZIP entry");
        }
        const quint16 nameLength = readLe16(data, offset + 28);
        const quint16 extraLength = readLe16(data, offset + 30);
        const quint16 commentLength = readLe16(data, offset + 32);
        const int nameOffset = offset + 46;
        if (nameOffset + nameLength > data.size()) {
            RuntimeThrow("Invalid dotLottie ZIP file name");
        }

        ZipEntry entry;
        entry.method = readLe16(data, offset + 10);
        entry.compressedSize = readLe32(data, offset + 20);
        entry.uncompressedSize = readLe32(data, offset + 24);
        entry.localHeaderOffset = readLe32(data, offset + 42);
        entry.name = QString::fromUtf8(data.constData() + nameOffset, nameLength);
        entries.append(entry);
        offset = nameOffset + nameLength + extraLength + commentLength;
    }
    return entries;
}

QByteArray inflateRawDeflate(const QByteArray& input, const int outputSize)
{
    QByteArray output(outputSize, Qt::Uninitialized);
    z_stream stream;
    memset(&stream, 0, sizeof(stream));
    stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(input.constData()));
    stream.avail_in = input.size();
    stream.next_out = reinterpret_cast<Bytef*>(output.data());
    stream.avail_out = output.size();

    if (inflateInit2(&stream, -MAX_WBITS) != Z_OK) {
        RuntimeThrow("Could not initialize dotLottie decompression");
    }
    const int result = inflate(&stream, Z_FINISH);
    inflateEnd(&stream);
    if (result != Z_STREAM_END) {
        RuntimeThrow("Could not decompress dotLottie entry");
    }
    output.truncate(stream.total_out);
    return output;
}

QByteArray readZipEntry(const QByteArray& data, const ZipEntry& entry)
{
    const int headerOffset = entry.localHeaderOffset;
    if (readLe32(data, headerOffset) != 0x04034b50) {
        RuntimeThrow("Invalid dotLottie ZIP local entry");
    }
    const quint16 nameLength = readLe16(data, headerOffset + 26);
    const quint16 extraLength = readLe16(data, headerOffset + 28);
    const int dataOffset = headerOffset + 30 + nameLength + extraLength;
    if (dataOffset + entry.compressedSize > static_cast<quint32>(data.size())) {
        RuntimeThrow("Invalid dotLottie ZIP entry data");
    }

    const QByteArray compressed = data.mid(dataOffset, entry.compressedSize);
    if (entry.method == 0) { return compressed; }
    if (entry.method == 8) {
        return inflateRawDeflate(compressed, entry.uncompressedSize);
    }
    RuntimeThrow("Unsupported dotLottie ZIP compression method");
}

QJsonObject jsonObjectFromBytes(const QByteArray& bytes, const QString& filename)
{
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(bytes, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        RuntimeThrow("Cannot parse Lottie file " + filename);
    }
    return document.object();
}

bool isSafeZipPath(const QString& path)
{
    if (path.isEmpty() || path.startsWith(QLatin1Char('/')) ||
        path.contains(QLatin1Char('\\'))) {
        return false;
    }
    const QString clean = QDir::cleanPath(path);
    return clean == path && !path.startsWith(QStringLiteral("../")) &&
            !path.contains(QStringLiteral("/../"));
}

QString dotLottieAssetDir(const QString& filename)
{
    const QFileInfo info(filename);
    return QDir(info.absolutePath()).filePath(
                QStringLiteral("%1_assets").arg(info.completeBaseName()));
}

void extractDotLottieAssets(const QByteArray& zipData,
                            const QMap<QString, ZipEntry>& files,
                            const QString& filename)
{
    const QString outputRoot = dotLottieAssetDir(filename);
    QDir outputDir(outputRoot);
    if (!outputDir.exists() && !QDir().mkpath(outputRoot)) {
        RuntimeThrow("Cannot create dotLottie asset directory");
    }

    for (auto it = files.constBegin(); it != files.constEnd(); ++it) {
        const QString path = it.key();
        if (path == QStringLiteral("manifest.json") ||
            path.endsWith(QStringLiteral(".json"), Qt::CaseInsensitive) ||
            !isSafeZipPath(path)) {
            continue;
        }

        const QFileInfo targetInfo(outputDir.filePath(path));
        if (!QDir().mkpath(targetInfo.absolutePath())) {
            RuntimeThrow("Cannot create dotLottie asset subdirectory");
        }
        QFile target(targetInfo.absoluteFilePath());
        if (!target.open(QIODevice::WriteOnly)) {
            RuntimeThrow("Cannot write dotLottie asset");
        }
        target.write(readZipEntry(zipData, it.value()));
    }
}

QJsonObject readDotLottieRoot(const QString& filename,
                              QString& sourceDir,
                              const bool extractAssets)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        RuntimeThrow("Cannot open Lottie file " + filename);
    }
    const QByteArray zipData = file.readAll();
    const QList<ZipEntry> entries = readZipEntries(zipData);

    QMap<QString, ZipEntry> files;
    for (const ZipEntry& entry : entries) {
        if (!entry.name.endsWith(QLatin1Char('/'))) {
            files.insert(entry.name, entry);
        }
    }

    QString animationPath;
    if (files.contains(QStringLiteral("manifest.json"))) {
        const QJsonObject manifest = jsonObjectFromBytes(
                    readZipEntry(zipData, files.value(QStringLiteral("manifest.json"))),
                    filename + QStringLiteral(":manifest.json"));
        const QJsonArray animations = manifest.value(QStringLiteral("animations")).toArray();
        if (!animations.isEmpty()) {
            const QJsonObject animation = animations.first().toObject();
            animationPath = animation.value(QStringLiteral("url")).toString();
            if (animationPath.isEmpty()) {
                const QString id = animation.value(QStringLiteral("id")).toString();
                if (!id.isEmpty()) {
                    animationPath = QStringLiteral("animations/%1.json").arg(id);
                }
            }
        }
    }

    if (animationPath.isEmpty() || !files.contains(animationPath)) {
        for (const QString& path : files.keys()) {
            if (path.startsWith(QStringLiteral("animations/")) &&
                path.endsWith(QStringLiteral(".json"), Qt::CaseInsensitive)) {
                animationPath = path;
                break;
            }
        }
    }
    if (animationPath.isEmpty() || !files.contains(animationPath)) {
        for (const QString& path : files.keys()) {
            if (path != QStringLiteral("manifest.json") &&
                path.endsWith(QStringLiteral(".json"), Qt::CaseInsensitive)) {
                animationPath = path;
                break;
            }
        }
    }
    if (animationPath.isEmpty() || !files.contains(animationPath)) {
        RuntimeThrow("No animation JSON found in dotLottie file");
    }

    sourceDir = dotLottieAssetDir(filename);
    if (extractAssets) { extractDotLottieAssets(zipData, files, filename); }
    return jsonObjectFromBytes(readZipEntry(zipData, files.value(animationPath)),
                               filename + QStringLiteral(":") + animationPath);
}

QJsonObject readLottieRoot(const QString& filename,
                           QString& sourceDir,
                           const bool extractAssets)
{
    sourceDir = QFileInfo(filename).absolutePath();
    if (filename.endsWith(QStringLiteral(".lottie"), Qt::CaseInsensitive)) {
        return readDotLottieRoot(filename, sourceDir, extractAssets);
    }

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        RuntimeThrow("Cannot open Lottie file " + filename);
    }
    return jsonObjectFromBytes(file.readAll(), filename);
}

AssetMap assetMap(const QJsonArray& assets)
{
    AssetMap result;
    for (const QJsonValue& value : assets) {
        const QJsonObject asset = value.toObject();
        const QString id = asset.value(QStringLiteral("id")).toString();
        if (!id.isEmpty()) { result.insert(id, asset); }
    }
    return result;
}

QString assetFilePath(const QJsonObject& asset, const QString& sourceDir)
{
    const QString file = asset.value(QStringLiteral("p")).toString();
    if (file.isEmpty() || file.startsWith(QStringLiteral("data:"))) { return {}; }

    const QString folder = asset.value(QStringLiteral("u")).toString();
    const QString path = folder + file;
    if (QFileInfo(path).isAbsolute()) { return path; }
    return QDir(sourceDir).filePath(path);
}

QJsonValue propertyValue(const QJsonObject& property)
{
    if (!property.contains(QStringLiteral("k"))) { return {}; }
    const QJsonValue key = property.value(QStringLiteral("k"));
    if (!key.isArray() || property.value(QStringLiteral("a")).toInt() == 0) {
        return key;
    }

    const QJsonArray keys = key.toArray();
    if (keys.isEmpty()) { return {}; }
    return keys.first().toObject().value(QStringLiteral("s"));
}

QList<KeyValue> propertyKeys(const QJsonObject& property)
{
    QList<KeyValue> result;
    const QJsonValue key = property.value(QStringLiteral("k"));
    if (!key.isArray() || property.value(QStringLiteral("a")).toInt() == 0) {
        result.append({0, key, false});
        return result;
    }

    const QJsonArray keys = key.toArray();
    for (const QJsonValue& value : keys) {
        const QJsonObject keyObject = value.toObject();
        if (!keyObject.contains(QStringLiteral("s"))) { continue; }
        result.append({qRound(keyObject.value(QStringLiteral("t")).toDouble()),
                       keyObject.value(QStringLiteral("s")),
                       keyObject.value(QStringLiteral("h")).toInt() == 1});
    }
    return result;
}

qreal numberAt(const QJsonArray& array, const int index, const qreal fallback = 0)
{
    return index < array.size() ? array.at(index).toDouble(fallback) : fallback;
}

qreal scalarValue(const QJsonValue& value, const qreal fallback = 0)
{
    if (value.isArray()) {
        const QJsonArray array = value.toArray();
        return array.isEmpty() ? fallback : array.first().toDouble(fallback);
    }
    return value.toDouble(fallback);
}

QPointF pointValue(const QJsonValue& value, const QPointF& fallback = QPointF())
{
    const QJsonArray array = value.toArray();
    if (array.isEmpty()) { return fallback; }
    return QPointF(numberAt(array, 0, fallback.x()),
                   numberAt(array, 1, fallback.y()));
}

QJsonValue keyValueAtFrame(const QList<KeyValue>& keys,
                           const int frame,
                           const QJsonValue& fallback)
{
    if (keys.isEmpty()) { return fallback; }
    QJsonValue result = fallback;
    for (const KeyValue& key : keys) {
        if (key.frame > frame) { break; }
        result = key.value;
    }
    return result;
}

QList<int> combinedAnimatedFrames(const QList<KeyValue>& first,
                                  const QList<KeyValue>& second)
{
    QList<int> frames;
    const auto appendFrame = [&frames](const KeyValue& key) {
        if (!frames.contains(key.frame)) { frames.append(key.frame); }
    };
    if (first.size() > 1) {
        for (const KeyValue& key : first) { appendFrame(key); }
    }
    if (second.size() > 1) {
        for (const KeyValue& key : second) { appendFrame(key); }
    }
    std::sort(frames.begin(), frames.end());
    return frames;
}

QList<int> combinedAnimatedFrames(const QList<QList<KeyValue>>& keyLists)
{
    QList<int> frames;
    for (const QList<KeyValue>& keys : keyLists) {
        if (keys.size() <= 1) { continue; }
        for (const KeyValue& key : keys) {
            if (!frames.contains(key.frame)) { frames.append(key.frame); }
        }
    }
    std::sort(frames.begin(), frames.end());
    return frames;
}

QColor colorValue(const QJsonValue& value, const QColor& fallback = Qt::black)
{
    const QJsonArray array = value.toArray();
    if (array.size() < 3) { return fallback; }
    QColor color;
    color.setRgbF(qBound(0., numberAt(array, 0), 1.),
                  qBound(0., numberAt(array, 1), 1.),
                  qBound(0., numberAt(array, 2), 1.),
                  qBound(0., numberAt(array, 3, 1), 1.));
    return color;
}

QList<QColor> gradientColors(const QJsonObject& gradient)
{
    const int pointCount = qMax(0, gradient.value(QStringLiteral("p")).toInt());
    const QJsonArray values = propertyValue(
                gradient.value(QStringLiteral("k")).toObject()).toArray();
    QList<QColor> colors;
    for (int i = 0; i < pointCount; ++i) {
        const int offset = i*4;
        if (offset + 3 >= values.size()) { break; }
        QColor color;
        color.setRgbF(qBound(0., values.at(offset + 1).toDouble(), 1.),
                      qBound(0., values.at(offset + 2).toDouble(), 1.),
                      qBound(0., values.at(offset + 3).toDouble(), 1.),
                      1.);
        colors.append(color);
    }
    return colors;
}

QColor firstGradientColor(const QJsonObject& gradient,
                          const QColor& fallback = Qt::black)
{
    const QList<QColor> colors = gradientColors(gradient);
    return colors.isEmpty() ? fallback : colors.first();
}

QColor gradientColorAt(const QJsonValue& value,
                       const int pointIndex,
                       const QColor& fallback = Qt::black)
{
    QJsonArray values;
    if (value.isArray()) {
        values = value.toArray();
    } else if (value.isObject()) {
        values = propertyValue(value.toObject()).toArray();
    }
    const int offset = pointIndex*4;
    if (offset + 3 >= values.size()) { return fallback; }
    QColor color;
    color.setRgbF(qBound(0., values.at(offset + 1).toDouble(), 1.),
                  qBound(0., values.at(offset + 2).toDouble(), 1.),
                  qBound(0., values.at(offset + 3).toDouble(), 1.),
                  fallback.alphaF());
    return color;
}

int importFrame(const int lottieFrame, const int inPoint, Canvas* const scene)
{
    return scene->getMinFrame() + lottieFrame - inPoint;
}

void applyScalarKeys(QrealAnimator* const animator,
                     const QJsonObject& property,
                     Canvas* const scene,
                     const int inPoint,
                     const qreal multiplier = 1)
{
    if (!animator) { return; }
    const QList<KeyValue> keys = propertyKeys(property);
    if (keys.size() == 1) {
        animator->setCurrentBaseValue(scalarValue(keys.first().value) * multiplier);
        return;
    }
    for (int i = 0; i < keys.size(); ++i) {
        const int frame = importFrame(keys.at(i).frame, inPoint, scene);
        if (keys.at(i).hold && i + 1 < keys.size() &&
            keys.at(i).frame + 1 < keys.at(i + 1).frame) {
            animator->saveValueToKey(importFrame(keys.at(i + 1).frame, inPoint, scene) - 1,
                                     scalarValue(keys.at(i).value) * multiplier);
        }
        animator->saveValueToKey(frame, scalarValue(keys.at(i).value) * multiplier);
    }
}

void applyRepeaterCountKeys(QrealAnimator* const animator,
                            const QJsonObject& property,
                            Canvas* const scene,
                            const int inPoint)
{
    if (!animator) { return; }
    const QList<KeyValue> keys = propertyKeys(property);
    auto countValue = [](const QJsonValue& value) {
        return qMax<qreal>(0, qRound(scalarValue(value, 1)) - 1);
    };
    if (keys.size() == 1) {
        animator->setCurrentBaseValue(countValue(keys.first().value));
        return;
    }
    for (int i = 0; i < keys.size(); ++i) {
        const int frame = importFrame(keys.at(i).frame, inPoint, scene);
        if (keys.at(i).hold && i + 1 < keys.size() &&
            keys.at(i).frame + 1 < keys.at(i + 1).frame) {
            animator->saveValueToKey(importFrame(keys.at(i + 1).frame, inPoint, scene) - 1,
                                     countValue(keys.at(i).value));
        }
        animator->saveValueToKey(frame, countValue(keys.at(i).value));
    }
}

void applyPointKeys(QPointFAnimator* const animator,
                    const QJsonObject& property,
                    Canvas* const scene,
                    const int inPoint,
                    const QPointF& multiplier = QPointF(1, 1))
{
    if (!animator) { return; }
    const QList<KeyValue> keys = propertyKeys(property);
    if (keys.size() == 1) {
        const QPointF value = pointValue(keys.first().value);
        animator->setBaseValue(value.x() * multiplier.x(),
                               value.y() * multiplier.y());
        return;
    }
    for (int i = 0; i < keys.size(); ++i) {
        const KeyValue& key = keys.at(i);
        const QPointF value = pointValue(key.value);
        const int frame = importFrame(key.frame, inPoint, scene);
        if (key.hold && i + 1 < keys.size() &&
            key.frame + 1 < keys.at(i + 1).frame) {
            const int endFrame = importFrame(keys.at(i + 1).frame, inPoint, scene) - 1;
            animator->getXAnimator()->saveValueToKey(endFrame, value.x() * multiplier.x());
            animator->getYAnimator()->saveValueToKey(endFrame, value.y() * multiplier.y());
        }
        animator->getXAnimator()->saveValueToKey(frame, value.x() * multiplier.x());
        animator->getYAnimator()->saveValueToKey(frame, value.y() * multiplier.y());
    }
}

QPointF skewToShear(const qreal skewDegrees, const qreal axisDegrees)
{
    const qreal shear = qTan(qDegreesToRadians(skewDegrees));
    const qreal axis = qDegreesToRadians(axisDegrees);
    return QPointF(shear*qCos(axis), shear*qSin(axis));
}

void applySkewKeys(QPointFAnimator* const animator,
                   const QJsonObject& skewProperty,
                   const QJsonObject& axisProperty,
                   Canvas* const scene,
                   const int inPoint)
{
    if (!animator || skewProperty.isEmpty()) { return; }
    const QList<KeyValue> skewKeys = propertyKeys(skewProperty);
    const QList<KeyValue> axisKeys = propertyKeys(axisProperty);
    const QJsonValue baseSkew = propertyValue(skewProperty);
    const QJsonValue baseAxis = propertyValue(axisProperty);
    const QList<int> frames = combinedAnimatedFrames({skewKeys, axisKeys});
    if (frames.isEmpty()) {
        const QPointF shear = skewToShear(scalarValue(baseSkew),
                                          scalarValue(baseAxis));
        animator->setBaseValue(shear);
        return;
    }

    for (const int frame : frames) {
        const QPointF shear = skewToShear(
                    scalarValue(keyValueAtFrame(skewKeys, frame, baseSkew)),
                    scalarValue(keyValueAtFrame(axisKeys, frame, baseAxis)));
        const int importKeyFrame = importFrame(frame, inPoint, scene);
        animator->getXAnimator()->saveValueToKey(importKeyFrame, shear.x());
        animator->getYAnimator()->saveValueToKey(importKeyFrame, shear.y());
    }
}

void applyRectangleGeometryKeys(RectangleBox* const box,
                                const QJsonObject& positionProperty,
                                const QJsonObject& sizeProperty,
                                Canvas* const scene,
                                const int inPoint)
{
    if (!box) { return; }
    const QList<KeyValue> positionKeys = propertyKeys(positionProperty);
    const QList<KeyValue> sizeKeys = propertyKeys(sizeProperty);
    const QPointF basePosition = pointValue(propertyValue(positionProperty));
    const QPointF baseSize = pointValue(propertyValue(sizeProperty));
    const QList<int> frames = combinedAnimatedFrames(positionKeys, sizeKeys);

    if (frames.isEmpty()) {
        box->setTopLeftPos(QPointF(basePosition.x() - baseSize.x()*0.5,
                                   basePosition.y() - baseSize.y()*0.5));
        box->setBottomRightPos(QPointF(basePosition.x() + baseSize.x()*0.5,
                                       basePosition.y() + baseSize.y()*0.5));
        return;
    }

    for (const int frame : frames) {
        const QPointF position = pointValue(
                    keyValueAtFrame(positionKeys, frame, propertyValue(positionProperty)),
                    basePosition);
        const QPointF size = pointValue(
                    keyValueAtFrame(sizeKeys, frame, propertyValue(sizeProperty)),
                    baseSize);
        const int importKeyFrame = importFrame(frame, inPoint, scene);
        box->getTopLeftAnimator()->getXAnimator()->saveValueToKey(
                    importKeyFrame, position.x() - size.x()*0.5);
        box->getTopLeftAnimator()->getYAnimator()->saveValueToKey(
                    importKeyFrame, position.y() - size.y()*0.5);
        box->getBottomRightAnimator()->getXAnimator()->saveValueToKey(
                    importKeyFrame, position.x() + size.x()*0.5);
        box->getBottomRightAnimator()->getYAnimator()->saveValueToKey(
                    importKeyFrame, position.y() + size.y()*0.5);
    }
}

void applyRectangleRadiusKeys(RectangleBox* const box,
                              const QJsonObject& radiusProperty,
                              Canvas* const scene,
                              const int inPoint)
{
    if (!box) { return; }
    const QList<KeyValue> keys = propertyKeys(radiusProperty);
    if (keys.size() <= 1) {
        const qreal radius = scalarValue(propertyValue(radiusProperty));
        box->setXRadius(radius);
        box->setYRadius(radius);
        return;
    }

    for (const KeyValue& key : keys) {
        const qreal radius = scalarValue(key.value);
        const int frame = importFrame(key.frame, inPoint, scene);
        box->getRadiusAnimator()->getXAnimator()->saveValueToKey(frame, radius);
        box->getRadiusAnimator()->getYAnimator()->saveValueToKey(frame, radius);
    }
}

void applyEllipseGeometryKeys(Circle* const box,
                              const QJsonObject& positionProperty,
                              const QJsonObject& sizeProperty,
                              Canvas* const scene,
                              const int inPoint)
{
    if (!box) { return; }
    applyPointKeys(box->getCenterAnimator(), positionProperty, scene, inPoint);

    const QList<KeyValue> sizeKeys = propertyKeys(sizeProperty);
    if (sizeKeys.size() <= 1) {
        const QPointF size = pointValue(propertyValue(sizeProperty));
        box->setHorizontalRadius(size.x()*0.5);
        box->setVerticalRadius(size.y()*0.5);
        return;
    }

    for (const KeyValue& key : sizeKeys) {
        const QPointF size = pointValue(key.value);
        const int frame = importFrame(key.frame, inPoint, scene);
        box->getHRadiusAnimator()->getXAnimator()->saveValueToKey(frame, size.x()*0.5);
        box->getVRadiusAnimator()->getYAnimator()->saveValueToKey(frame, size.y()*0.5);
    }
}

void applyColorKeys(ColorAnimator* const animator,
                    const QJsonObject& property,
                    Canvas* const scene,
                    const int inPoint)
{
    if (!animator) { return; }
    animator->setColorMode(ColorMode::rgb);
    const QList<KeyValue> keys = propertyKeys(property);
    if (keys.size() == 1) {
        animator->setColor(colorValue(keys.first().value));
        return;
    }
    for (int i = 0; i < keys.size(); ++i) {
        const KeyValue& key = keys.at(i);
        const QColor color = colorValue(key.value);
        const int frame = importFrame(key.frame, inPoint, scene);
        if (key.hold && i + 1 < keys.size() &&
            key.frame + 1 < keys.at(i + 1).frame) {
            const int endFrame = importFrame(keys.at(i + 1).frame, inPoint, scene) - 1;
            animator->getVal1Animator()->saveValueToKey(endFrame, color.redF());
            animator->getVal2Animator()->saveValueToKey(endFrame, color.greenF());
            animator->getVal3Animator()->saveValueToKey(endFrame, color.blueF());
            animator->getAlphaAnimator()->saveValueToKey(endFrame, color.alphaF());
        }
        animator->getVal1Animator()->saveValueToKey(frame, color.redF());
        animator->getVal2Animator()->saveValueToKey(frame, color.greenF());
        animator->getVal3Animator()->saveValueToKey(frame, color.blueF());
        animator->getAlphaAnimator()->saveValueToKey(frame, color.alphaF());
    }
}

void applyTransform(BoundingBox* const box,
                    const QJsonObject& transform,
                    Canvas* const scene,
                    const int inPoint)
{
    if (!box) { return; }
    const auto animator = box->getBoxTransformAnimator();
    if (!animator) { return; }

    applyPointKeys(animator->getPosAnimator(),
                   transform.value(QStringLiteral("p")).toObject(),
                   scene, inPoint);
    applyPointKeys(animator->getScaleAnimator(),
                   transform.value(QStringLiteral("s")).toObject(),
                   scene, inPoint, QPointF(0.01, 0.01));
    applyScalarKeys(animator->getRotAnimator(),
                    transform.value(QStringLiteral("r")).toObject(),
                    scene, inPoint);
    applyScalarKeys(animator->getRotAnimator(),
                    transform.value(QStringLiteral("rz")).toObject(),
                    scene, inPoint);
    applySkewKeys(animator->getShearAnimator(),
                  transform.value(QStringLiteral("sk")).toObject(),
                  transform.value(QStringLiteral("sa")).toObject(),
                  scene, inPoint);
    applyScalarKeys(animator->getOpacityAnimator(),
                    transform.value(QStringLiteral("o")).toObject(),
                    scene, inPoint);
    applyPointKeys(animator->getPivotAnimator(),
                   transform.value(QStringLiteral("a")).toObject(),
                   scene, inPoint);
}

qreal opacityAtFrame(const QJsonObject& property, const int frame)
{
    const QList<KeyValue> keys = propertyKeys(property);
    if (keys.isEmpty()) { return 100; }
    QJsonValue value = keys.first().value;
    for (const KeyValue& key : keys) {
        if (key.frame > frame) { break; }
        value = key.value;
    }
    return scalarValue(value, 100);
}

void applyLayerVisibilityRange(BoundingBox* const box,
                               const QJsonObject& layer,
                               Canvas* const scene,
                               const int rootInPoint,
                               const int rootOutPoint)
{
    if (!box || !scene) { return; }
    const int inPoint = qRound(layer.value(QStringLiteral("ip")).toDouble(rootInPoint));
    const int outPoint = qRound(layer.value(QStringLiteral("op")).toDouble(rootOutPoint));
    if (inPoint <= rootInPoint && outPoint >= rootOutPoint) { return; }

    const auto transform = box->getBoxTransformAnimator();
    if (!transform || !transform->getOpacityAnimator()) { return; }

    const QJsonObject opacityProperty = layer.value(QStringLiteral("ks")).toObject()
            .value(QStringLiteral("o")).toObject();
    const int firstVisibleFrame = importFrame(inPoint, rootInPoint, scene);
    const int lastVisibleFrame = importFrame(outPoint, rootInPoint, scene) - 1;
    const qreal firstOpacity = opacityAtFrame(opacityProperty, inPoint);
    const qreal lastOpacity = opacityAtFrame(opacityProperty, qMax(inPoint, outPoint - 1));
    const auto opacity = transform->getOpacityAnimator();

    if (firstVisibleFrame > scene->getMinFrame()) {
        opacity->saveValueToKey(firstVisibleFrame - 1, 0);
    }
    opacity->saveValueToKey(firstVisibleFrame, firstOpacity);
    if (lastVisibleFrame >= firstVisibleFrame) {
        opacity->saveValueToKey(lastVisibleFrame, lastOpacity);
    }
    opacity->saveValueToKey(lastVisibleFrame + 1, 0);
}

SkPath pathFromShapeValue(const QJsonValue& value)
{
    QJsonObject object = value.toObject();
    if (object.isEmpty() && value.isArray()) {
        const QJsonArray array = value.toArray();
        if (!array.isEmpty()) {
            object = array.first().toObject();
        }
    }
    const QJsonArray vertices = object.value(QStringLiteral("v")).toArray();
    const QJsonArray inTangents = object.value(QStringLiteral("i")).toArray();
    const QJsonArray outTangents = object.value(QStringLiteral("o")).toArray();
    const bool closed = object.value(QStringLiteral("c")).toBool();
    SkPath path;
    if (vertices.isEmpty()) { return path; }

    const QPointF first = pointValue(vertices.first());
    path.moveTo(first.x(), first.y());
    for (int i = 1; i < vertices.size(); ++i) {
        const QPointF previous = pointValue(vertices.at(i - 1));
        const QPointF current = pointValue(vertices.at(i));
        const QPointF out = pointValue(outTangents.at(i - 1));
        const QPointF in = pointValue(inTangents.at(i));
        if (out.isNull() && in.isNull()) {
            path.lineTo(current.x(), current.y());
        } else {
            path.cubicTo(previous.x() + out.x(), previous.y() + out.y(),
                         current.x() + in.x(), current.y() + in.y(),
                         current.x(), current.y());
        }
    }
    if (closed) {
        const int lastIndex = vertices.size() - 1;
        const QPointF last = pointValue(vertices.last());
        const QPointF out = pointValue(outTangents.at(lastIndex));
        const QPointF in = pointValue(inTangents.first());
        if (!out.isNull() || !in.isNull()) {
            path.cubicTo(last.x() + out.x(), last.y() + out.y(),
                         first.x() + in.x(), first.y() + in.y(),
                         first.x(), first.y());
        }
        path.close();
    }
    return path;
}

SkPath pathFromPolystarValues(const int starType,
                              const int pointCount,
                              const QPointF& center,
                              const qreal rotationDegrees,
                              const qreal outerRadius,
                              const qreal innerRadius)
{
    const int clampedPointCount = qMax(3, pointCount);
    const qreal rotation = qDegreesToRadians(rotationDegrees - 90);
    const int vertexCount = starType == 1 ? clampedPointCount*2 : clampedPointCount;

    SkPath path;
    if (outerRadius <= 0 || vertexCount <= 0) { return path; }

    for (int i = 0; i < vertexCount; ++i) {
        const bool innerPoint = starType == 1 && i % 2 == 1;
        const qreal radius = innerPoint ? innerRadius : outerRadius;
        const qreal angle = rotation + 2*M_PI*i/vertexCount;
        const qreal x = center.x() + qCos(angle)*radius;
        const qreal y = center.y() + qSin(angle)*radius;
        if (i == 0) {
            path.moveTo(x, y);
        } else {
            path.lineTo(x, y);
        }
    }
    path.close();
    return path;
}

SkPath pathFromPolystar(const QJsonObject& shape)
{
    const int starType = qRound(scalarValue(shape.value(QStringLiteral("sy")), 1));
    const int pointCount = qRound(scalarValue(propertyValue(
                                 shape.value(QStringLiteral("pt")).toObject()), 5));
    const QPointF center = pointValue(propertyValue(
                                          shape.value(QStringLiteral("p")).toObject()));
    const qreal rotation = scalarValue(propertyValue(
                                           shape.value(QStringLiteral("r")).toObject()), 0);
    const qreal outerRadius = scalarValue(propertyValue(
                                shape.value(QStringLiteral("or")).toObject()), 0);
    const qreal innerRadius = scalarValue(propertyValue(
                                shape.value(QStringLiteral("ir")).toObject()), outerRadius);
    return pathFromPolystarValues(starType, pointCount, center, rotation,
                                  outerRadius, innerRadius);
}

SkPath pathFromRectangle(const QJsonObject& shape)
{
    const QPointF position = pointValue(propertyValue(
                                            shape.value(QStringLiteral("p")).toObject()));
    const QPointF size = pointValue(propertyValue(
                                        shape.value(QStringLiteral("s")).toObject()));
    const qreal radius = scalarValue(propertyValue(
                                         shape.value(QStringLiteral("r")).toObject()));
    SkPath path;
    const SkRect rect = SkRect::MakeLTRB(position.x() - size.x()*0.5,
                                         position.y() - size.y()*0.5,
                                         position.x() + size.x()*0.5,
                                         position.y() + size.y()*0.5);
    path.addRoundRect(rect, radius, radius);
    return path;
}

SkPath pathFromEllipse(const QJsonObject& shape)
{
    const QPointF position = pointValue(propertyValue(
                                            shape.value(QStringLiteral("p")).toObject()));
    const QPointF size = pointValue(propertyValue(
                                        shape.value(QStringLiteral("s")).toObject()));
    SkPath path;
    const SkRect rect = SkRect::MakeLTRB(position.x() - size.x()*0.5,
                                         position.y() - size.y()*0.5,
                                         position.x() + size.x()*0.5,
                                         position.y() + size.y()*0.5);
    path.addOval(rect);
    return path;
}

SkPath pathFromShapeItem(const QJsonObject& item)
{
    const QString type = item.value(QStringLiteral("ty")).toString();
    if (type == QStringLiteral("sh")) {
        return pathFromShapeValue(propertyValue(
                                      item.value(QStringLiteral("ks")).toObject()));
    }
    if (type == QStringLiteral("sr")) { return pathFromPolystar(item); }
    if (type == QStringLiteral("rc")) { return pathFromRectangle(item); }
    if (type == QStringLiteral("el")) { return pathFromEllipse(item); }
    return {};
}

bool isPathShapeType(const QString& type)
{
    return type == QStringLiteral("sh") || type == QStringLiteral("sr") ||
           type == QStringLiteral("rc") || type == QStringLiteral("el");
}

qreal repeaterOpacityAt(const QJsonObject& transform, const QString& name,
                        const qreal fallback)
{
    return scalarValue(propertyValue(transform.value(name).toObject()), fallback);
}

QJsonObject repeaterTransform(const QJsonObject& repeater)
{
    QJsonObject transform = repeater.value(QStringLiteral("tr")).toObject();
    if (transform.isEmpty()) {
        transform = firstItemOfType(
                    repeater.value(QStringLiteral("it")).toArray(), QStringLiteral("tr"));
    }
    return transform;
}

bool propertyHasMultipleKeys(const QJsonObject& property)
{
    return propertyKeys(property).size() > 1;
}

bool scalarPropertyIsStaticValue(const QJsonObject& property,
                                 const qreal expected,
                                 const qreal fallback)
{
    if (propertyHasMultipleKeys(property)) { return false; }
    return qAbs(scalarValue(propertyValue(property), fallback) - expected) < 0.0001;
}

bool pointPropertyIsStaticValue(const QJsonObject& property,
                                const QPointF& expected,
                                const QPointF& fallback)
{
    if (propertyHasMultipleKeys(property)) { return false; }
    const QPointF value = pointValue(propertyValue(property), fallback);
    return qAbs(value.x() - expected.x()) < 0.0001 &&
           qAbs(value.y() - expected.y()) < 0.0001;
}

bool canUseNativeDuplicateEffect(const QJsonObject& repeater)
{
    const int copies = qRound(scalarValue(propertyValue(
                             repeater.value(QStringLiteral("c")).toObject()), 1));
    if (copies < 1 || copies > 26) { return false; }

    const QJsonObject offset = repeater.value(QStringLiteral("o")).toObject();
    if (!scalarPropertyIsStaticValue(offset, 0, 0)) { return false; }

    const QJsonObject transform = repeaterTransform(repeater);
    if (transform.isEmpty()) { return true; }

    return pointPropertyIsStaticValue(transform.value(QStringLiteral("s")).toObject(),
                                      QPointF(100, 100), QPointF(100, 100)) &&
           scalarPropertyIsStaticValue(transform.value(QStringLiteral("r")).toObject(), 0, 0) &&
           scalarPropertyIsStaticValue(transform.value(QStringLiteral("so")).toObject(), 100, 100) &&
           scalarPropertyIsStaticValue(transform.value(QStringLiteral("eo")).toObject(), 100, 100);
}

void addNativeDuplicateEffect(SmartVectorPath* const box,
                              const QJsonObject& repeater,
                              Canvas* const scene,
                              const int inPoint)
{
    if (!box) { return; }
    const auto effect = enve::make_shared<DuplicatePathEffect>();
    const QJsonObject transform = repeaterTransform(repeater);

    if (const auto count = effect->ca_getFirstDescendantWithName<IntAnimator>(
                QStringLiteral("count"))) {
        applyRepeaterCountKeys(count, repeater.value(QStringLiteral("c")).toObject(),
                               scene, inPoint);
    }

    if (const auto translation =
            effect->ca_getFirstDescendantWithName<QPointFAnimator>(
                QStringLiteral("translation"))) {
        applyPointKeys(translation,
                       transform.value(QStringLiteral("p")).toObject(),
                       scene, inPoint);
    }

    box->addPathEffect(effect);
}

SkMatrix repeaterMatrix(const QJsonObject& transform, const int copyIndex,
                        const qreal offset)
{
    const qreal amount = copyIndex + offset;
    const QPointF anchor = pointValue(propertyValue(
                                          transform.value(QStringLiteral("a")).toObject()));
    const QPointF position = pointValue(propertyValue(
                                            transform.value(QStringLiteral("p")).toObject()));
    const QPointF scale = pointValue(propertyValue(
                                         transform.value(QStringLiteral("s")).toObject()),
                                     QPointF(100, 100));
    const qreal rotation = scalarValue(propertyValue(
                                           transform.value(QStringLiteral("r")).toObject()));

    SkMatrix matrix;
    matrix.setIdentity();
    matrix.preTranslate(-anchor.x(), -anchor.y());
    matrix.preScale(qPow(scale.x()*0.01, amount),
                    qPow(scale.y()*0.01, amount));
    matrix.preRotate(rotation*amount);
    matrix.preTranslate(anchor.x() + position.x()*amount,
                        anchor.y() + position.y()*amount);
    return matrix;
}

SkPath applyRepeater(const SkPath& source, const QJsonObject& repeater)
{
    const int copies = qMax(1, qRound(scalarValue(propertyValue(
                                    repeater.value(QStringLiteral("c")).toObject()), 1)));
    const qreal offset = scalarValue(propertyValue(
                                         repeater.value(QStringLiteral("o")).toObject()));
    const QJsonObject transform = repeaterTransform(repeater);
    const qreal startOpacity = repeaterOpacityAt(transform, QStringLiteral("so"), 100);
    const qreal endOpacity = repeaterOpacityAt(transform, QStringLiteral("eo"), 100);
    Q_UNUSED(startOpacity)
    Q_UNUSED(endOpacity)

    SkPath result;
    result.setFillType(source.getFillType());
    for (int i = 0; i < copies; ++i) {
        SkPath copy = source;
        copy.transform(repeaterMatrix(transform, i, offset));
        result.addPath(copy);
    }
    return result;
}

SkPath trimPath(const SkPath& source, const QJsonObject& trim)
{
    qreal start = scalarValue(propertyValue(trim.value(QStringLiteral("s")).toObject()), 0);
    qreal end = scalarValue(propertyValue(trim.value(QStringLiteral("e")).toObject()), 100);
    const qreal offset = scalarValue(propertyValue(
                                        trim.value(QStringLiteral("o")).toObject()), 0) / 360. * 100.;
    start = std::fmod(start + offset + 100., 100.);
    end = std::fmod(end + offset + 100., 100.);

    SkPath result;
    result.setFillType(source.getFillType());
    auto addRange = [&source, &result](const qreal from, const qreal to) {
        SkPathMeasure measure(source, false);
        do {
            const SkScalar length = measure.getLength();
            if (length <= 0) { continue; }
            SkPath segment;
            if (measure.getSegment(length*from*0.01, length*to*0.01,
                                   &segment, true)) {
                result.addPath(segment);
            }
        } while (measure.nextContour());
    };

    if (qFuzzyCompare(start, end)) { return result; }
    if (start < end) {
        addRange(start, end);
    } else {
        addRange(start, 100);
        addRange(0, end);
    }
    return result;
}

bool canUseNativeSubPathEffect(const QJsonObject& trim)
{
    const QJsonObject startProperty = trim.value(QStringLiteral("s")).toObject();
    const QJsonObject endProperty = trim.value(QStringLiteral("e")).toObject();
    if (propertyHasMultipleKeys(startProperty) ||
        propertyHasMultipleKeys(endProperty)) {
        return true;
    }

    const qreal start = scalarValue(propertyValue(startProperty), 0);
    const qreal end = scalarValue(propertyValue(endProperty), 100);
    return end >= start;
}

void addNativeSubPathEffect(SmartVectorPath* const box,
                            const QJsonObject& trim,
                            Canvas* const scene,
                            const int inPoint)
{
    if (!box) { return; }
    const auto effect = enve::make_shared<SubPathEffect>();

    if (const auto pathWise =
            effect->ca_getFirstDescendantWithName<BoolProperty>(
                QStringLiteral("path-wise"))) {
        pathWise->setValue(trim.value(QStringLiteral("m")).toInt(1) == 2);
    }
    if (const auto minLength =
            effect->ca_getFirstDescendantWithName<QrealAnimator>(
                QStringLiteral("min length"))) {
        applyScalarKeys(minLength, trim.value(QStringLiteral("s")).toObject(),
                        scene, inPoint);
    }
    if (const auto maxLength =
            effect->ca_getFirstDescendantWithName<QrealAnimator>(
                QStringLiteral("max length"))) {
        applyScalarKeys(maxLength, trim.value(QStringLiteral("e")).toObject(),
                        scene, inPoint);
    }
    if (const auto offset =
            effect->ca_getFirstDescendantWithName<QrealAnimator>(
                QStringLiteral("offset"))) {
        applyScalarKeys(offset, trim.value(QStringLiteral("o")).toObject(),
                        scene, inPoint, 100. / 360.);
    }

    box->addPathEffect(effect);
}

SkPath mergePaths(const QList<SkPath>& paths, const QJsonObject& merge)
{
    if (paths.isEmpty()) { return {}; }
    const int mode = merge.value(QStringLiteral("mm")).toInt(1);
    SkPath result = paths.first();
    for (int i = 1; i < paths.size(); ++i) {
        SkPath out;
        SkPathOp op = kUnion_SkPathOp;
        if (mode == 2) { op = kUnion_SkPathOp; }
        else if (mode == 3) { op = kDifference_SkPathOp; }
        else if (mode == 4) { op = kIntersect_SkPathOp; }
        else if (mode == 5) { op = kXOR_SkPathOp; }
        if (Op(result, paths.at(i), op, &out)) {
            result = out;
        } else {
            result.addPath(paths.at(i));
        }
    }
    return result;
}

QVector<SkScalar> dashIntervals(const QJsonObject& stroke)
{
    QVector<SkScalar> intervals;
    const QJsonArray dash = stroke.value(QStringLiteral("d")).toArray();
    for (const QJsonValue& value : dash) {
        const QJsonObject item = value.toObject();
        const QString name = item.value(QStringLiteral("n")).toString();
        if (name == QStringLiteral("d") || name == QStringLiteral("g")) {
            intervals.append(toSkScalar(qMax<qreal>(0.1, scalarValue(propertyValue(
                                                item.value(QStringLiteral("v")).toObject())))));
        }
    }
    if (intervals.size() == 1) { intervals.append(intervals.first()); }
    if (intervals.size() % 2 == 1) {
        const QVector<SkScalar> copy = intervals;
        intervals += copy;
    }
    return intervals;
}

qreal dashOffset(const QJsonObject& stroke)
{
    const QJsonArray dash = stroke.value(QStringLiteral("d")).toArray();
    for (const QJsonValue& value : dash) {
        const QJsonObject item = value.toObject();
        if (item.value(QStringLiteral("n")).toString() == QStringLiteral("o")) {
            return scalarValue(propertyValue(item.value(QStringLiteral("v")).toObject()));
        }
    }
    return 0;
}

QJsonObject dashEntry(const QJsonObject& stroke, const QString& name)
{
    const QJsonArray dash = stroke.value(QStringLiteral("d")).toArray();
    for (const QJsonValue& value : dash) {
        const QJsonObject item = value.toObject();
        if (item.value(QStringLiteral("n")).toString() == name) { return item; }
    }
    return {};
}

bool canUseNativeDashEffect(const QJsonObject& stroke)
{
    const QJsonArray dash = stroke.value(QStringLiteral("d")).toArray();
    if (dash.isEmpty()) { return false; }

    int dashCount = 0;
    int gapCount = 0;
    int offsetCount = 0;
    for (const QJsonValue& value : dash) {
        const QString name = value.toObject().value(QStringLiteral("n")).toString();
        if (name == QStringLiteral("d")) { ++dashCount; }
        else if (name == QStringLiteral("g")) { ++gapCount; }
        else if (name == QStringLiteral("o")) { ++offsetCount; }
    }
    if (dashCount != 1 || gapCount > 1 || offsetCount > 1) { return false; }

    const QJsonObject offset = dashEntry(stroke, QStringLiteral("o"))
            .value(QStringLiteral("v")).toObject();
    if (!offset.isEmpty() && !scalarPropertyIsStaticValue(offset, 0, 0)) {
        return false;
    }

    const QJsonObject dashValue = dashEntry(stroke, QStringLiteral("d"))
            .value(QStringLiteral("v")).toObject();
    const QJsonObject gapValue = dashEntry(stroke, QStringLiteral("g"))
            .value(QStringLiteral("v")).toObject();
    if (gapValue.isEmpty()) { return true; }
    if (propertyHasMultipleKeys(dashValue) || propertyHasMultipleKeys(gapValue)) {
        return false;
    }

    return qAbs(scalarValue(propertyValue(dashValue)) -
                scalarValue(propertyValue(gapValue))) < 0.0001;
}

void addNativeDashEffect(SmartVectorPath* const box,
                         const QJsonObject& stroke,
                         Canvas* const scene,
                         const int inPoint)
{
    if (!box) { return; }
    const auto effect = enve::make_shared<DashPathEffect>();
    if (const auto size = effect->ca_getFirstDescendantWithName<QrealAnimator>(
                QStringLiteral("size"))) {
        applyScalarKeys(size,
                        dashEntry(stroke, QStringLiteral("d"))
                            .value(QStringLiteral("v")).toObject(),
                        scene, inPoint);
    }
    box->addPathEffect(effect);
}

SkPath applyDashPattern(const SkPath& source, const QJsonObject& stroke)
{
    const QVector<SkScalar> intervals = dashIntervals(stroke);
    if (intervals.isEmpty()) { return source; }
    SkPath result;
    SkStrokeRec rec(SkStrokeRec::kHairline_InitStyle);
    SkRect cullRect = source.getBounds();
    const auto effect = SkDashPathEffect::Make(intervals.constData(),
                                               intervals.size(),
                                               toSkScalar(dashOffset(stroke)));
    if (!effect || !effect->filterPath(&result, source, &rec, &cullRect)) {
        return source;
    }
    result.setFillType(source.getFillType());
    return result;
}

void applyPathKeys(SmartVectorPath* const box,
                   const QJsonObject& property,
                   Canvas* const scene,
                   const int inPoint)
{
    if (!box) { return; }
    const auto collection = box->getPathAnimator();
    if (!collection || collection->ca_getNumberOfChildren() != 1) { return; }
    const auto animator = collection->getChild(0);
    const QList<KeyValue> keys = propertyKeys(property);
    if (keys.size() <= 1) { return; }

    for (const KeyValue& key : keys) {
        const SkPath path = pathFromShapeValue(key.value);
        const auto pathKey = enve::make_shared<SmartPathKey>(
                    SmartPath(path), importFrame(key.frame, inPoint, scene),
                    animator);
        animator->anim_appendKey(pathKey);
    }
}

void applyPolystarKeys(SmartVectorPath* const box,
                       const QJsonObject& shape,
                       Canvas* const scene,
                       const int inPoint)
{
    if (!box) { return; }
    const auto collection = box->getPathAnimator();
    if (!collection || collection->ca_getNumberOfChildren() != 1) { return; }
    const auto animator = collection->getChild(0);

    const QJsonObject pointsProperty = shape.value(QStringLiteral("pt")).toObject();
    const QJsonObject positionProperty = shape.value(QStringLiteral("p")).toObject();
    const QJsonObject rotationProperty = shape.value(QStringLiteral("r")).toObject();
    const QJsonObject outerRadiusProperty = shape.value(QStringLiteral("or")).toObject();
    const QJsonObject innerRadiusProperty = shape.value(QStringLiteral("ir")).toObject();
    const QList<KeyValue> pointsKeys = propertyKeys(pointsProperty);
    const QList<KeyValue> positionKeys = propertyKeys(positionProperty);
    const QList<KeyValue> rotationKeys = propertyKeys(rotationProperty);
    const QList<KeyValue> outerRadiusKeys = propertyKeys(outerRadiusProperty);
    const QList<KeyValue> innerRadiusKeys = propertyKeys(innerRadiusProperty);
    if (pointsKeys.size() > 1) {
        return;
    }
    const QList<int> frames = combinedAnimatedFrames({
        positionKeys,
        rotationKeys,
        outerRadiusKeys,
        innerRadiusKeys
    });
    if (frames.isEmpty()) { return; }

    const int starType = qRound(scalarValue(shape.value(QStringLiteral("sy")), 1));
    const QJsonValue basePoints = propertyValue(pointsProperty);
    const QJsonValue basePosition = propertyValue(positionProperty);
    const QJsonValue baseRotation = propertyValue(rotationProperty);
    const QJsonValue baseOuterRadius = propertyValue(outerRadiusProperty);
    const QJsonValue baseInnerRadius = propertyValue(innerRadiusProperty);

    for (const int frame : frames) {
        const int pointCount = qRound(scalarValue(
            keyValueAtFrame(pointsKeys, frame, basePoints), 5));
        const QPointF center = pointValue(
            keyValueAtFrame(positionKeys, frame, basePosition));
        const qreal rotation = scalarValue(
            keyValueAtFrame(rotationKeys, frame, baseRotation), 0);
        const qreal outerRadius = scalarValue(
            keyValueAtFrame(outerRadiusKeys, frame, baseOuterRadius), 0);
        const qreal innerRadius = scalarValue(
            keyValueAtFrame(innerRadiusKeys, frame, baseInnerRadius), outerRadius);
        const SkPath path = pathFromPolystarValues(starType, pointCount, center,
                                                   rotation, outerRadius,
                                                   innerRadius);
        const auto pathKey = enve::make_shared<SmartPathKey>(
                    SmartPath(path), importFrame(frame, inPoint, scene),
                    animator);
        animator->anim_appendKey(pathKey);
    }
}

void applyGradient(PaintSettingsAnimator* const settings,
                   const QJsonObject& gradientStyle,
                   Canvas* const scene,
                   const int inPoint);

SkPaint::Cap strokeCapFromLottie(const int cap)
{
    if (cap == 2) { return SkPaint::kRound_Cap; }
    if (cap == 3) { return SkPaint::kSquare_Cap; }
    return SkPaint::kButt_Cap;
}

SkPaint::Join strokeJoinFromLottie(const int join)
{
    if (join == 2) { return SkPaint::kRound_Join; }
    if (join == 3) { return SkPaint::kBevel_Join; }
    return SkPaint::kMiter_Join;
}

void applyStrokeStyle(OutlineSettingsAnimator* const strokeSettings,
                      const QJsonObject& stroke)
{
    if (!strokeSettings || stroke.isEmpty()) { return; }
    if (stroke.contains(QStringLiteral("lc"))) {
        strokeSettings->setCapStyle(strokeCapFromLottie(
                                        stroke.value(QStringLiteral("lc")).toInt(1)));
    }
    if (stroke.contains(QStringLiteral("lj"))) {
        strokeSettings->setJoinStyle(strokeJoinFromLottie(
                                         stroke.value(QStringLiteral("lj")).toInt(1)));
    }
}

void applyPaint(PathBox* const box,
                const PaintStyle& style,
                Canvas* const scene,
                const int inPoint,
                const QJsonObject& fill = {},
                const QJsonObject& stroke = {},
                const QJsonObject& gradientFill = {},
                const QJsonObject& gradientStroke = {})
{
    if (!box) { return; }
    const auto fillSettings = box->getFillSettings();
    if (fillSettings) {
        if (style.hasFill) {
            fillSettings->setCurrentColor(style.fill);
            if (style.hasFillGradient && !gradientFill.isEmpty()) {
                applyGradient(fillSettings, gradientFill, scene, inPoint);
            } else {
                fillSettings->setPaintType(PaintType::FLATPAINT);
            }
            if (!fill.isEmpty()) {
                applyColorKeys(fillSettings->getColorAnimator(),
                               fill.value(QStringLiteral("c")).toObject(),
                               scene, inPoint);
            }
            const QJsonObject opacitySource = fill.isEmpty() ? gradientFill : fill;
            if (opacitySource.contains(QStringLiteral("o"))) {
                applyScalarKeys(fillSettings->getColorAnimator()->getAlphaAnimator(),
                                opacitySource.value(QStringLiteral("o")).toObject(),
                                scene, inPoint, 0.01);
            }
        } else {
            fillSettings->setPaintType(PaintType::NOPAINT);
        }
    }

    const auto strokeSettings = box->getStrokeSettings();
    if (strokeSettings) {
        if (style.hasStroke) {
            strokeSettings->setCurrentColor(style.stroke);
            strokeSettings->getLineWidthAnimator()->setCurrentBaseValue(
                        style.strokeWidth);
            if (style.hasStrokeGradient && !gradientStroke.isEmpty()) {
                applyGradient(strokeSettings, gradientStroke, scene, inPoint);
            } else {
                strokeSettings->setPaintType(PaintType::FLATPAINT);
            }
            if (!stroke.isEmpty()) {
                applyColorKeys(strokeSettings->getColorAnimator(),
                               stroke.value(QStringLiteral("c")).toObject(),
                               scene, inPoint);
            }
            const QJsonObject strokeSource = stroke.isEmpty() ?
                        gradientStroke : stroke;
            if (strokeSource.contains(QStringLiteral("o"))) {
                applyScalarKeys(strokeSettings->getColorAnimator()->getAlphaAnimator(),
                                strokeSource.value(QStringLiteral("o")).toObject(),
                                scene, inPoint, 0.01);
            }
            if (strokeSource.contains(QStringLiteral("w"))) {
                applyScalarKeys(strokeSettings->getLineWidthAnimator(),
                                strokeSource.value(QStringLiteral("w")).toObject(),
                                scene, inPoint);
            }
            applyStrokeStyle(strokeSettings, strokeSource);
        } else {
            strokeSettings->setPaintType(PaintType::NOPAINT);
        }
    }
}

QJsonObject firstItemOfType(const QJsonArray& items, const QString& type)
{
    for (const QJsonValue& value : items) {
        const QJsonObject item = value.toObject();
        if (item.value(QStringLiteral("hd")).toBool(false)) { continue; }
        if (item.value(QStringLiteral("ty")).toString() == type) { return item; }
    }
    return {};
}

bool hasItemOfType(const QJsonArray& items, const QString& type)
{
    return !firstItemOfType(items, type).isEmpty();
}

bool hasPathModifier(const QJsonArray& items)
{
    if (hasItemOfType(items, QStringLiteral("tm")) ||
        hasItemOfType(items, QStringLiteral("rp")) ||
        hasItemOfType(items, QStringLiteral("mm"))) {
        return true;
    }
    const QJsonObject stroke = firstItemOfType(items, QStringLiteral("st"));
    const QJsonObject gradientStroke = firstItemOfType(items, QStringLiteral("gs"));
    return !stroke.value(QStringLiteral("d")).toArray().isEmpty() ||
           !gradientStroke.value(QStringLiteral("d")).toArray().isEmpty();
}

bool transformOpacityNeedsLayer(const QJsonObject& transform)
{
    const QJsonObject opacity = transform.value(QStringLiteral("o")).toObject();
    if (opacity.isEmpty()) { return false; }
    if (opacity.value(QStringLiteral("a")).toInt() != 0) { return true; }
    return qAbs(scalarValue(propertyValue(opacity), 100) - 100) > 0.0001;
}

void applyGradientColorKeys(SceneBoundGradient* const gradient,
                            const QJsonObject& property,
                            Canvas* const scene,
                            const int inPoint)
{
    if (!gradient) { return; }
    const QList<KeyValue> keys = propertyKeys(property);
    if (keys.size() <= 1) { return; }

    const int colorCount = gradient->ca_getNumberOfChildren();
    for (const KeyValue& key : keys) {
        const int frame = importFrame(key.frame, inPoint, scene);
        for (int i = 0; i < colorCount; ++i) {
            const QColor color = gradientColorAt(key.value, i,
                                                 gradient->getColorAt(i));
            const auto colorAnimator = gradient->getChild(i);
            colorAnimator->getVal1Animator()->saveValueToKey(frame, color.redF());
            colorAnimator->getVal2Animator()->saveValueToKey(frame, color.greenF());
            colorAnimator->getVal3Animator()->saveValueToKey(frame, color.blueF());
            colorAnimator->getAlphaAnimator()->saveValueToKey(frame, color.alphaF());
        }
    }
}

void applyGradient(PaintSettingsAnimator* const settings,
                   const QJsonObject& gradientStyle,
                   Canvas* const scene,
                   const int inPoint)
{
    if (!settings || !scene || gradientStyle.isEmpty()) { return; }
    const QJsonObject gradientProperty =
            gradientStyle.value(QStringLiteral("g")).toObject();
    const QList<QColor> colors = gradientColors(gradientProperty);
    if (colors.isEmpty()) { return; }

    SceneBoundGradient* const gradient = scene->createNewGradient();
    if (!gradient) { return; }
    for (const QColor& color : colors) {
        gradient->addColor(color);
    }

    applyGradientColorKeys(gradient,
                           gradientProperty.value(QStringLiteral("k")).toObject(),
                           scene, inPoint);

    const int gradientType = gradientStyle.value(QStringLiteral("t")).toInt(1);
    settings->setPaintType(PaintType::GRADIENTPAINT);
    settings->setGradientType(gradientType == 2 ?
                              GradientType::RADIAL :
                              GradientType::LINEAR);
    settings->setGradient(gradient);
    settings->setGradientPointsPos(
                pointValue(propertyValue(gradientStyle.value(QStringLiteral("s")).toObject())),
                pointValue(propertyValue(gradientStyle.value(QStringLiteral("e")).toObject())));
    const auto points = settings->getGradientPoints();
    if (points) {
        applyPointKeys(points->startAnimator(),
                       gradientStyle.value(QStringLiteral("s")).toObject(),
                       scene, inPoint);
        applyPointKeys(points->endAnimator(),
                       gradientStyle.value(QStringLiteral("e")).toObject(),
                       scene, inPoint);
    }
}

PaintStyle paintStyleForItems(const QJsonArray& items)
{
    PaintStyle style;
    const QJsonObject fill = firstItemOfType(items, QStringLiteral("fl"));
    if (!fill.isEmpty()) {
        style.hasFill = true;
        style.fill = colorValue(propertyValue(fill.value(QStringLiteral("c")).toObject()));
        style.fill.setAlphaF(qBound(0., scalarValue(propertyValue(
                                      fill.value(QStringLiteral("o")).toObject()), 100.) / 100., 1.));
    }
    const QJsonObject gradientFill = firstItemOfType(items, QStringLiteral("gf"));
    if (!gradientFill.isEmpty()) {
        style.hasFill = true;
        style.hasFillGradient = true;
        style.fill = firstGradientColor(gradientFill.value(QStringLiteral("g")).toObject());
        style.fill.setAlphaF(qBound(0., scalarValue(propertyValue(
                                      gradientFill.value(QStringLiteral("o")).toObject()), 100.) / 100., 1.));
    }
    const QJsonObject stroke = firstItemOfType(items, QStringLiteral("st"));
    if (!stroke.isEmpty()) {
        style.hasStroke = true;
        style.stroke = colorValue(propertyValue(stroke.value(QStringLiteral("c")).toObject()));
        style.stroke.setAlphaF(qBound(0., scalarValue(propertyValue(
                                        stroke.value(QStringLiteral("o")).toObject()), 100.) / 100., 1.));
        style.strokeWidth = scalarValue(propertyValue(
                                            stroke.value(QStringLiteral("w")).toObject()), 1);
    }
    const QJsonObject gradientStroke = firstItemOfType(items, QStringLiteral("gs"));
    if (!gradientStroke.isEmpty()) {
        style.hasStroke = true;
        style.hasStrokeGradient = true;
        style.stroke = firstGradientColor(gradientStroke.value(QStringLiteral("g")).toObject());
        style.stroke.setAlphaF(qBound(0., scalarValue(propertyValue(
                                        gradientStroke.value(QStringLiteral("o")).toObject()), 100.) / 100., 1.));
        style.strokeWidth = scalarValue(propertyValue(
                                            gradientStroke.value(QStringLiteral("w")).toObject()), 1);
    }
    return style;
}

qsptr<SmartVectorPath> createPathBox(const QJsonObject& shape,
                                     const PaintStyle& style,
                                     const QJsonObject& fill,
                                     const QJsonObject& stroke,
                                     const QJsonObject& gradientFill,
                                     const QJsonObject& gradientStroke,
                                     Canvas* const scene,
                                     const int inPoint)
{
    const QJsonObject property = shape.value(QStringLiteral("ks")).toObject();
    const SkPath path = pathFromShapeValue(propertyValue(property));
    const auto box = enve::make_shared<SmartVectorPath>();
    box->prp_setName(shape.value(QStringLiteral("nm")).toString(
                         QStringLiteral("Path")));
    box->loadSkPath(path);
    applyPathKeys(box.get(), property, scene, inPoint);
    applyPaint(box.get(), style, scene, inPoint, fill, stroke,
               gradientFill, gradientStroke);
    return box;
}

qsptr<SmartVectorPath> createPathBoxFromPath(const QString& name,
                                             const SkPath& path,
                                             const PaintStyle& style,
                                             const QJsonObject& fill,
                                             const QJsonObject& stroke,
                                             const QJsonObject& gradientFill,
                                             const QJsonObject& gradientStroke,
                                             Canvas* const scene,
                                             const int inPoint)
{
    const auto box = enve::make_shared<SmartVectorPath>();
    box->prp_setName(name.isEmpty() ? QStringLiteral("Path") : name);
    box->loadSkPath(path);
    applyPaint(box.get(), style, scene, inPoint, fill, stroke,
               gradientFill, gradientStroke);
    return box;
}

qsptr<SmartVectorPath> createPolystarBox(const QJsonObject& shape,
                                         const PaintStyle& style,
                                         const QJsonObject& fill,
                                         const QJsonObject& stroke,
                                         const QJsonObject& gradientFill,
                                         const QJsonObject& gradientStroke,
                                         Canvas* const scene,
                                         const int inPoint)
{
    const SkPath path = pathFromPolystar(shape);
    const auto box = enve::make_shared<SmartVectorPath>();
    box->prp_setName(shape.value(QStringLiteral("nm")).toString(
                         QStringLiteral("Polystar")));
    box->loadSkPath(path);
    applyPolystarKeys(box.get(), shape, scene, inPoint);
    applyPaint(box.get(), style, scene, inPoint, fill, stroke,
               gradientFill, gradientStroke);
    return box;
}

qsptr<RectangleBox> createRectangleBox(const QJsonObject& shape,
                                       const PaintStyle& style,
                                       const QJsonObject& fill,
                                       const QJsonObject& stroke,
                                       const QJsonObject& gradientFill,
                                       const QJsonObject& gradientStroke,
                                       Canvas* const scene,
                                       const int inPoint)
{
    const auto box = enve::make_shared<RectangleBox>();
    box->prp_setName(shape.value(QStringLiteral("nm")).toString(
                         QStringLiteral("Rectangle")));
    applyRectangleGeometryKeys(box.get(),
                               shape.value(QStringLiteral("p")).toObject(),
                               shape.value(QStringLiteral("s")).toObject(),
                               scene, inPoint);
    applyRectangleRadiusKeys(box.get(),
                             shape.value(QStringLiteral("r")).toObject(),
                             scene, inPoint);
    applyPaint(box.get(), style, scene, inPoint, fill, stroke,
               gradientFill, gradientStroke);
    return box;
}

qsptr<Circle> createEllipseBox(const QJsonObject& shape,
                               const PaintStyle& style,
                               const QJsonObject& fill,
                               const QJsonObject& stroke,
                               const QJsonObject& gradientFill,
                               const QJsonObject& gradientStroke,
                               Canvas* const scene,
                               const int inPoint)
{
    const auto box = enve::make_shared<Circle>();
    box->prp_setName(shape.value(QStringLiteral("nm")).toString(
                         QStringLiteral("Ellipse")));
    applyEllipseGeometryKeys(box.get(),
                             shape.value(QStringLiteral("p")).toObject(),
                             shape.value(QStringLiteral("s")).toObject(),
                             scene, inPoint);
    applyPaint(box.get(), style, scene, inPoint, fill, stroke,
               gradientFill, gradientStroke);
    return box;
}

void importShapeItems(const QJsonArray& items,
                      ContainerBox* const parent,
                      Canvas* const scene,
                      const int inPoint);

bool importModifiedPathItems(const QJsonArray& items,
                             ContainerBox* const parent,
                             Canvas* const scene,
                             const int inPoint,
                             const PaintStyle& style,
                             const QJsonObject& fill,
                             const QJsonObject& stroke,
                             const QJsonObject& gradientFill,
                             const QJsonObject& gradientStroke,
                             const QString& name)
{
    if (!parent || !hasPathModifier(items)) { return false; }

    QList<SkPath> paths;
    for (const QJsonValue& value : items) {
        const QJsonObject item = value.toObject();
        if (item.value(QStringLiteral("hd")).toBool(false)) { continue; }
        const QString type = item.value(QStringLiteral("ty")).toString();
        if (!isPathShapeType(type)) { continue; }
        paths.append(pathFromShapeItem(item));
    }
    if (paths.isEmpty()) { return false; }

    SkPath path = firstItemOfType(items, QStringLiteral("mm")).isEmpty() ?
                SkPath() : mergePaths(paths, firstItemOfType(items, QStringLiteral("mm")));
    if (path.isEmpty()) {
        for (const SkPath& source : paths) { path.addPath(source); }
    }

    const QJsonObject repeater = firstItemOfType(items, QStringLiteral("rp"));
    const bool nativeRepeater = !repeater.isEmpty() &&
            canUseNativeDuplicateEffect(repeater);
    if (!repeater.isEmpty() && !nativeRepeater) {
        path = applyRepeater(path, repeater);
    }

    const QJsonObject trim = firstItemOfType(items, QStringLiteral("tm"));
    const bool nativeTrim = !trim.isEmpty() && canUseNativeSubPathEffect(trim);
    if (!trim.isEmpty() && !nativeTrim) { path = trimPath(path, trim); }

    const QJsonObject dashSource = !stroke.isEmpty() ? stroke : gradientStroke;
    const bool nativeDash = !dashSource.value(QStringLiteral("d")).toArray().isEmpty() &&
            canUseNativeDashEffect(dashSource);
    if (!dashSource.value(QStringLiteral("d")).toArray().isEmpty() && !nativeDash) {
        path = applyDashPattern(path, dashSource);
    }

    const auto box = createPathBoxFromPath(name, path, style, fill, stroke,
                                           gradientFill, gradientStroke,
                                           scene, inPoint);
    if (nativeRepeater) {
        addNativeDuplicateEffect(box.get(), repeater, scene, inPoint);
    }
    if (nativeTrim) {
        addNativeSubPathEffect(box.get(), trim, scene, inPoint);
    }
    if (nativeDash) {
        addNativeDashEffect(box.get(), dashSource, scene, inPoint);
    }
    parent->addContained(box);
    return true;
}

qsptr<ContainerBox> createGroup(const QJsonObject& group,
                                Canvas* const scene,
                                const int inPoint)
{
    const QJsonArray items = group.value(QStringLiteral("it")).toArray();
    const QJsonObject transform = firstItemOfType(items, QStringLiteral("tr"));
    const auto box = enve::make_shared<ContainerBox>(
                group.value(QStringLiteral("nm")).toString(QStringLiteral("Group")),
                transformOpacityNeedsLayer(transform) ?
                    eBoxType::layer : eBoxType::group);
    importShapeItems(items, box.get(), scene, inPoint);
    if (!transform.isEmpty()) { applyTransform(box.get(), transform, scene, inPoint); }
    return box;
}

qsptr<SmartVectorPath> createMaskPathBox(const QJsonObject& mask,
                                         Canvas* const scene,
                                         const int inPoint)
{
    const QJsonObject property = mask.value(QStringLiteral("pt")).toObject();
    const auto box = enve::make_shared<SmartVectorPath>();
    box->prp_setName(mask.value(QStringLiteral("nm")).toString(
                         QStringLiteral("Mask Path")));
    box->loadSkPath(pathFromShapeValue(propertyValue(property)));
    applyPathKeys(box.get(), property, scene, inPoint);

    if (const auto fillSettings = box->getFillSettings()) {
        fillSettings->setCurrentColor(Qt::white);
        fillSettings->setPaintType(PaintType::FLATPAINT);
        applyScalarKeys(fillSettings->getColorAnimator()->getAlphaAnimator(),
                        mask.value(QStringLiteral("o")).toObject(),
                        scene, inPoint, 0.01);
    }
    if (const auto strokeSettings = box->getStrokeSettings()) {
        strokeSettings->setPaintType(PaintType::NOPAINT);
    }
    return box;
}

void addMaskPathToLayer(ContainerBox* const maskLayer,
                        const QJsonObject& mask,
                        Canvas* const scene,
                        const int inPoint)
{
    if (!maskLayer) { return; }
    const auto path = createMaskPathBox(mask, scene, inPoint);
    if (path) { maskLayer->addContained(path); }
}

void importLayerMasks(const QJsonArray& masks,
                      ContainerBox* const layer,
                      Canvas* const scene,
                      const int inPoint)
{
    if (!layer || masks.isEmpty()) { return; }

    qsptr<ContainerBox> addMaskLayer;
    qsptr<ContainerBox> subtractMaskLayer;
    auto ensureMaskLayer = [&](qsptr<ContainerBox>& maskLayer,
                               const QString& name,
                               const SkBlendMode blendMode) {
        if (!maskLayer) {
            maskLayer = enve::make_shared<ContainerBox>(name, eBoxType::layer);
            maskLayer->setBlendModeSk(blendMode);
        }
        return maskLayer.get();
    };

    for (const QJsonValue& value : masks) {
        const QJsonObject mask = value.toObject();
        const QString mode = mask.value(QStringLiteral("mode")).toString(
                    QStringLiteral("a"));
        if (mode == QStringLiteral("n") || mode == QStringLiteral("i")) {
            continue;
        }

        const bool inverted = mask.value(QStringLiteral("inv")).toBool(false);
        const bool subtract = mode == QStringLiteral("s");
        if (subtract != inverted) {
            addMaskPathToLayer(ensureMaskLayer(subtractMaskLayer,
                                               QStringLiteral("Subtract Masks"),
                                               SkBlendMode::kDstOut),
                               mask, scene, inPoint);
        } else {
            addMaskPathToLayer(ensureMaskLayer(addMaskLayer,
                                               QStringLiteral("Add Masks"),
                                               SkBlendMode::kDstIn),
                               mask, scene, inPoint);
        }
    }

    if (addMaskLayer && addMaskLayer->getContainedBoxesCount() > 0) {
        layer->addContained(addMaskLayer);
    }
    if (subtractMaskLayer && subtractMaskLayer->getContainedBoxesCount() > 0) {
        layer->addContained(subtractMaskLayer);
    }
}

bool layerHasMasks(const QJsonObject& layer)
{
    return !layer.value(QStringLiteral("masksProperties")).toArray().isEmpty();
}

qsptr<ContainerBox> wrapLayerContentWithMasks(const QJsonObject& layer,
                                              const qsptr<BoundingBox>& content,
                                              Canvas* const scene,
                                              const int inPoint,
                                              const int outPoint)
{
    if (!content) { return {}; }
    const auto box = enve::make_shared<ContainerBox>(
                layer.value(QStringLiteral("nm")).toString(
                    QStringLiteral("Masked Layer")),
                eBoxType::layer);
    box->addContained(content);
    importLayerMasks(layer.value(QStringLiteral("masksProperties")).toArray(),
                     box.get(), scene, inPoint);
    applyTransform(box.get(), layer.value(QStringLiteral("ks")).toObject(),
                   scene, inPoint);
    applyLayerVisibilityRange(box.get(), layer, scene, inPoint, outPoint);
    return box;
}

void importShapeItems(const QJsonArray& items,
                      ContainerBox* const parent,
                      Canvas* const scene,
                      const int inPoint)
{
    if (!parent) { return; }
    const PaintStyle style = paintStyleForItems(items);
    const QJsonObject fill = firstItemOfType(items, QStringLiteral("fl"));
    const QJsonObject stroke = firstItemOfType(items, QStringLiteral("st"));
    const QJsonObject gradientFill = firstItemOfType(items, QStringLiteral("gf"));
    const QJsonObject gradientStroke = firstItemOfType(items, QStringLiteral("gs"));

    if (importModifiedPathItems(items, parent, scene, inPoint, style, fill, stroke,
                                gradientFill, gradientStroke, parent->prp_getName())) {
        return;
    }

    for (const QJsonValue& value : items) {
        const QJsonObject item = value.toObject();
        if (item.value(QStringLiteral("hd")).toBool(false)) { continue; }
        const QString type = item.value(QStringLiteral("ty")).toString();
        qsptr<eBoxOrSound> imported;
        if (type == QStringLiteral("gr")) {
            imported = createGroup(item, scene, inPoint);
        } else if (type == QStringLiteral("sh")) {
            imported = createPathBox(item, style, fill, stroke,
                                     gradientFill, gradientStroke,
                                     scene, inPoint);
        } else if (type == QStringLiteral("sr")) {
            imported = createPolystarBox(item, style, fill, stroke,
                                         gradientFill, gradientStroke,
                                         scene, inPoint);
        } else if (type == QStringLiteral("rc")) {
            imported = createRectangleBox(item, style, fill, stroke,
                                          gradientFill, gradientStroke,
                                          scene, inPoint);
        } else if (type == QStringLiteral("el")) {
            imported = createEllipseBox(item, style, fill, stroke,
                                        gradientFill, gradientStroke,
                                        scene, inPoint);
        }
        if (imported) { parent->addContained(imported); }
    }
}

qsptr<BoundingBox> importShapeLayer(const QJsonObject& layer,
                                    Canvas* const scene,
                                    const int inPoint,
                                    const int outPoint)
{
    const auto box = enve::make_shared<ContainerBox>(
                layer.value(QStringLiteral("nm")).toString(QStringLiteral("Shape Layer")),
                eBoxType::layer);
    importShapeItems(layer.value(QStringLiteral("shapes")).toArray(),
                     box.get(), scene, inPoint);
    importLayerMasks(layer.value(QStringLiteral("masksProperties")).toArray(),
                     box.get(), scene, inPoint);
    applyTransform(box.get(), layer.value(QStringLiteral("ks")).toObject(),
                   scene, inPoint);
    applyLayerVisibilityRange(box.get(), layer, scene, inPoint, outPoint);
    return box;
}

QColor solidColor(const QJsonObject& layer)
{
    QColor color(layer.value(QStringLiteral("sc")).toString(QStringLiteral("#ffffff")));
    return color.isValid() ? color : QColor(Qt::white);
}

QList<KeyValue> textDocumentKeys(const QJsonObject& layer)
{
    const QJsonObject text = layer.value(QStringLiteral("t")).toObject();
    const QJsonObject document = text.value(QStringLiteral("d")).toObject();
    const QJsonArray keys = document.value(QStringLiteral("k")).toArray();
    QList<KeyValue> result;
    for (const QJsonValue& value : keys) {
        const QJsonObject key = value.toObject();
        const QJsonObject documentData = key.value(QStringLiteral("s")).toObject();
        if (documentData.isEmpty()) { continue; }
        result.append({qRound(key.value(QStringLiteral("t")).toDouble()),
                       documentData,
                       true});
    }
    return result;
}

QJsonObject firstTextDocumentData(const QList<KeyValue>& keys)
{
    return keys.isEmpty() ? QJsonObject() : keys.first().value.toObject();
}

Qt::Alignment textHAlignmentFromLottie(const int alignment)
{
    if (alignment == 1) { return Qt::AlignRight; }
    if (alignment == 2) { return Qt::AlignCenter; }
    return Qt::AlignLeft;
}

PaintStyle textPaintStyle(const QJsonObject& document)
{
    PaintStyle style;
    style.hasFill = document.contains(QStringLiteral("fc"));
    style.fill = colorValue(document.value(QStringLiteral("fc")), Qt::black);
    style.hasStroke = document.contains(QStringLiteral("sc")) &&
            document.value(QStringLiteral("sw")).toDouble(0) > 0;
    style.stroke = colorValue(document.value(QStringLiteral("sc")), Qt::black);
    style.strokeWidth = document.value(QStringLiteral("sw")).toDouble(1);
    return style;
}

void applyTextDocumentKeys(TextBox* const box,
                           const QList<KeyValue>& keys,
                           Canvas* const scene,
                           const int inPoint)
{
    if (!box || keys.isEmpty()) { return; }
    box->setCurrentValue(keys.first().value.toObject().value(
                             QStringLiteral("t")).toString());
    if (keys.size() <= 1) { return; }

    const auto textAnimator =
            box->ca_getFirstDescendantWithName<QStringAnimator>(
                QStringLiteral("text"));
    if (!textAnimator) { return; }
    for (const KeyValue& key : keys) {
        const QString text = key.value.toObject().value(
                    QStringLiteral("t")).toString();
        const auto stringKey = enve::make_shared<QStringKey>(
                    text, importFrame(key.frame, inPoint, scene), textAnimator);
        textAnimator->anim_appendKey(stringKey);
    }
}

qreal textTrackingValue(const QJsonObject& document)
{
    return document.value(QStringLiteral("tr")).toDouble(0) / 1000.;
}

qreal textLineSpacingValue(const QJsonObject& document)
{
    const qreal fontSize = document.value(QStringLiteral("s")).toDouble(64);
    if (fontSize <= 0) { return 1; }
    return document.value(QStringLiteral("lh")).toDouble(fontSize) / fontSize;
}

void applyTextSpacingKeys(TextBox* const box,
                          const QList<KeyValue>& keys,
                          Canvas* const scene,
                          const int inPoint)
{
    if (!box || keys.isEmpty()) { return; }
    const auto letterSpacing =
            box->ca_getFirstDescendantWithName<QrealAnimator>(
                QStringLiteral("letters"));
    const auto lineSpacing =
            box->ca_getFirstDescendantWithName<QrealAnimator>(
                QStringLiteral("lines"));
    if (!letterSpacing && !lineSpacing) { return; }

    const QJsonObject first = keys.first().value.toObject();
    if (letterSpacing) {
        letterSpacing->setCurrentBaseValue(textTrackingValue(first));
    }
    if (lineSpacing && first.contains(QStringLiteral("lh"))) {
        lineSpacing->setCurrentBaseValue(textLineSpacingValue(first));
    }
    if (keys.size() <= 1) { return; }

    for (int i = 0; i < keys.size(); ++i) {
        const KeyValue& key = keys.at(i);
        const QJsonObject document = key.value.toObject();
        const int frame = importFrame(key.frame, inPoint, scene);
        const qreal tracking = textTrackingValue(document);
        if (letterSpacing) {
            if (key.hold && i + 1 < keys.size() &&
                key.frame + 1 < keys.at(i + 1).frame) {
                letterSpacing->saveValueToKey(
                            importFrame(keys.at(i + 1).frame, inPoint, scene) - 1,
                            tracking);
            }
            letterSpacing->saveValueToKey(frame, tracking);
        }
        if (lineSpacing && document.contains(QStringLiteral("lh"))) {
            const qreal lineSpacingValue = textLineSpacingValue(document);
            if (key.hold && i + 1 < keys.size() &&
                key.frame + 1 < keys.at(i + 1).frame) {
                lineSpacing->saveValueToKey(
                            importFrame(keys.at(i + 1).frame, inPoint, scene) - 1,
                            lineSpacingValue);
            }
            lineSpacing->saveValueToKey(frame, lineSpacingValue);
        }
    }
}

qsptr<BoundingBox> importTextLayer(const QJsonObject& layer,
                                   Canvas* const scene,
                                   const int inPoint,
                                   const int outPoint)
{
    const QList<KeyValue> documentKeys = textDocumentKeys(layer);
    const QJsonObject document = firstTextDocumentData(documentKeys);
    if (document.isEmpty()) { return {}; }

    const auto box = enve::make_shared<TextBox>();
    box->prp_setName(layer.value(QStringLiteral("nm")).toString(
                         QStringLiteral("Text Layer")));
    applyTextDocumentKeys(box.get(), documentKeys, scene, inPoint);
    applyTextSpacingKeys(box.get(), documentKeys, scene, inPoint);
    box->setFontSize(document.value(QStringLiteral("s")).toDouble(64));
    const QString fontFamily = document.value(QStringLiteral("f")).toString();
    if (!fontFamily.isEmpty()) {
        box->setFontFamilyAndStyle(fontFamily, SkFontStyle());
    }
    box->setTextHAlignment(textHAlignmentFromLottie(
                               document.value(QStringLiteral("j")).toInt(0)));
    box->setTextVAlignment(Qt::AlignTop);
    applyPaint(box.get(), textPaintStyle(document), scene, inPoint);

    if (layerHasMasks(layer)) {
        return wrapLayerContentWithMasks(layer, box, scene, inPoint, outPoint);
    }

    applyTransform(box.get(), layer.value(QStringLiteral("ks")).toObject(),
                   scene, inPoint);
    applyLayerVisibilityRange(box.get(), layer, scene, inPoint, outPoint);
    return box;
}

qsptr<BoundingBox> importSolidLayer(const QJsonObject& layer,
                                    Canvas* const scene,
                                    const int inPoint,
                                    const int outPoint)
{
    const qreal width = qMax<qreal>(1, layer.value(QStringLiteral("sw")).toDouble(1));
    const qreal height = qMax<qreal>(1, layer.value(QStringLiteral("sh")).toDouble(1));
    const auto box = enve::make_shared<RectangleBox>();
    box->prp_setName(layer.value(QStringLiteral("nm")).toString(
                         QStringLiteral("Solid Layer")));
    box->setTopLeftPos(QPointF(0, 0));
    box->setBottomRightPos(QPointF(width, height));

    PaintStyle style;
    style.hasFill = true;
    style.fill = solidColor(layer);
    applyPaint(box.get(), style, scene, inPoint);

    if (layerHasMasks(layer)) {
        return wrapLayerContentWithMasks(layer, box, scene, inPoint, outPoint);
    }

    applyTransform(box.get(), layer.value(QStringLiteral("ks")).toObject(),
                   scene, inPoint);
    applyLayerVisibilityRange(box.get(), layer, scene, inPoint, outPoint);
    return box;
}

qsptr<BoundingBox> importImageLayer(const QJsonObject& layer,
                                    Canvas* const scene,
                                    const int inPoint,
                                    const int outPoint,
                                    const AssetMap& assets,
                                    const QString& sourceDir)
{
    const QString refId = layer.value(QStringLiteral("refId")).toString();
    const QString path = assetFilePath(assets.value(refId), sourceDir);
    if (path.isEmpty()) { return {}; }

    const auto box = enve::make_shared<ImageBox>();
    box->prp_setName(layer.value(QStringLiteral("nm")).toString(
                         QFileInfo(path).completeBaseName()));
    box->setFilePath(path);
    if (layerHasMasks(layer)) {
        return wrapLayerContentWithMasks(layer, box, scene, inPoint, outPoint);
    }

    applyTransform(box.get(), layer.value(QStringLiteral("ks")).toObject(),
                   scene, inPoint);
    applyLayerVisibilityRange(box.get(), layer, scene, inPoint, outPoint);
    return box;
}

qsptr<BoundingBox> importNullLayer(const QJsonObject& layer,
                                   Canvas* const scene,
                                   const int inPoint,
                                   const int outPoint)
{
    const auto box = enve::make_shared<NullObject>();
    box->prp_setName(layer.value(QStringLiteral("nm")).toString(
                         QStringLiteral("Null Layer")));
    applyTransform(box.get(), layer.value(QStringLiteral("ks")).toObject(),
                   scene, inPoint);
    applyLayerVisibilityRange(box.get(), layer, scene, inPoint, outPoint);
    return box;
}

void importLayersToContainer(const QJsonArray& layers,
                             ContainerBox* const parent,
                             Canvas* const scene,
                             const int inPoint,
                             const int outPoint,
                             const AssetMap& assets,
                             const QString& sourceDir,
                             QSet<QString>& assetStack);

qsptr<BoundingBox> importPrecompLayer(const QJsonObject& layer,
                                      Canvas* const scene,
                                      const int inPoint,
                                      const int outPoint,
                                      const AssetMap& assets,
                                      const QString& sourceDir,
                                      QSet<QString>& assetStack)
{
    const QString refId = layer.value(QStringLiteral("refId")).toString();
    if (refId.isEmpty() || !assets.contains(refId) || assetStack.contains(refId)) {
        return {};
    }

    const QJsonObject asset = assets.value(refId);
    const auto box = enve::make_shared<ContainerBox>(
                layer.value(QStringLiteral("nm")).toString(
                    asset.value(QStringLiteral("nm")).toString(QStringLiteral("Precomp"))),
                eBoxType::layer);

    assetStack.insert(refId);
    importLayersToContainer(asset.value(QStringLiteral("layers")).toArray(),
                            box.get(), scene, inPoint, outPoint, assets,
                            sourceDir, assetStack);
    assetStack.remove(refId);

    importLayerMasks(layer.value(QStringLiteral("masksProperties")).toArray(),
                     box.get(), scene, inPoint);
    applyTransform(box.get(), layer.value(QStringLiteral("ks")).toObject(),
                   scene, inPoint);
    applyLayerVisibilityRange(box.get(), layer, scene, inPoint, outPoint);
    return box;
}

qsptr<BoundingBox> importLayer(const QJsonObject& layer,
                               Canvas* const scene,
                               const int inPoint,
                               const int outPoint,
                               const AssetMap& assets,
                               const QString& sourceDir,
                               QSet<QString>& assetStack)
{
    if (layer.value(QStringLiteral("hd")).toBool(false)) { return {}; }
    const int type = layer.value(QStringLiteral("ty")).toInt(-1);
    qsptr<BoundingBox> box;
    if (type == 0) {
        box = importPrecompLayer(layer, scene, inPoint, outPoint,
                                 assets, sourceDir, assetStack);
    } else if (type == 1) {
        box = importSolidLayer(layer, scene, inPoint, outPoint);
    } else if (type == 2) {
        box = importImageLayer(layer, scene, inPoint, outPoint,
                               assets, sourceDir);
    } else if (type == 3) {
        box = importNullLayer(layer, scene, inPoint, outPoint);
    } else if (type == 4) {
        box = importShapeLayer(layer, scene, inPoint, outPoint);
    } else if (type == 5) {
        box = importTextLayer(layer, scene, inPoint, outPoint);
    }
    if (box && layer.contains(QStringLiteral("bm"))) {
        box->setBlendModeSk(blendModeFromLottie(
                                layer.value(QStringLiteral("bm")).toInt(0)));
    }
    return box;
}

bool layerUsesTrackMatte(const QJsonObject& layer)
{
    const int matteType = layer.value(QStringLiteral("tt")).toInt(0);
    return matteType > 0 && matteType <= 4;
}

SkBlendMode trackMatteBlendMode(const int matteType)
{
    return (matteType == 2 || matteType == 4) ?
                SkBlendMode::kDstOut : SkBlendMode::kDstIn;
}

QString trackMatteName(const QJsonObject& layer)
{
    const QString name = layer.value(QStringLiteral("nm")).toString();
    return name.isEmpty() ?
                QStringLiteral("Track Matte") :
                QStringLiteral("%1 Track Matte").arg(name);
}

qsptr<BoundingBox> importTrackMatteLayer(const QJsonObject& layer,
                                         const QJsonObject& matteLayer,
                                         Canvas* const scene,
                                         const int inPoint,
                                         const int outPoint,
                                         const AssetMap& assets,
                                         const QString& sourceDir,
                                         QSet<QString>& assetStack,
                                         QList<ImportedLayer>& importedLayers)
{
    const auto target = importLayer(layer, scene, inPoint, outPoint,
                                    assets, sourceDir, assetStack);
    const auto matte = importLayer(matteLayer, scene, inPoint, outPoint,
                                   assets, sourceDir, assetStack);
    if (!target) { return {}; }
    if (!matte) { return target; }

    matte->setBlendModeSk(trackMatteBlendMode(
                              layer.value(QStringLiteral("tt")).toInt(0)));
    const auto container = enve::make_shared<ContainerBox>(
                trackMatteName(layer), eBoxType::layer);
    container->addContained(target);
    container->addContained(matte);

    importedLayers.append({layer, target});
    importedLayers.append({matteLayer, matte});
    return container;
}

void applyLayerParenting(const QList<ImportedLayer>& importedLayers)
{
    QMap<int, BoundingBox*> layerByIndex;
    for (const ImportedLayer& imported : importedLayers) {
        if (!imported.box) { continue; }
        const int index = imported.layer.value(QStringLiteral("ind")).toInt(-1);
        if (index >= 0) { layerByIndex.insert(index, imported.box.get()); }
    }

    for (const ImportedLayer& imported : importedLayers) {
        if (!imported.box) { continue; }
        const int parentIndex = imported.layer.value(QStringLiteral("parent")).toInt(-1);
        BoundingBox* const target = layerByIndex.value(parentIndex, nullptr);
        if (!target || target == imported.box.get()) { continue; }

        const auto effect = enve::make_shared<ParentEffect>();
        imported.box->addTransformEffect(effect);
        if (const auto targetProperty =
                effect->ca_getFirstDescendantWithName<BoxTargetProperty>(
                    QStringLiteral("target"))) {
            targetProperty->setTarget(target);
        }
    }
}

void importLayersToContainer(const QJsonArray& layers,
                             ContainerBox* const parent,
                             Canvas* const scene,
                             const int inPoint,
                             const int outPoint,
                             const AssetMap& assets,
                             const QString& sourceDir,
                             QSet<QString>& assetStack)
{
    if (!parent) { return; }
    QList<ImportedLayer> importedLayers;
    for (int i = layers.size() - 1; i >= 0; --i) {
        const QJsonObject layer = layers.at(i).toObject();
        qsptr<BoundingBox> imported;
        if (layerUsesTrackMatte(layer) && i > 0) {
            const QJsonObject matteLayer = layers.at(i - 1).toObject();
            imported = importTrackMatteLayer(layer, matteLayer, scene,
                                             inPoint, outPoint, assets,
                                             sourceDir, assetStack,
                                             importedLayers);
            --i;
        } else {
            imported = importLayer(layer, scene, inPoint, outPoint,
                                   assets, sourceDir, assetStack);
            if (imported) { importedLayers.append({layer, imported}); }
        }
        if (imported) {
            parent->addContained(imported);
        }
    }
    applyLayerParenting(importedLayers);
}

void analyzeShapes(const QJsonArray& shapes, ImportLottie::Analysis& result)
{
    for (const QJsonValue& value : shapes) {
        const QJsonObject shape = value.toObject();
        if (shape.value(QStringLiteral("hd")).toBool(false)) { continue; }
        const QString type = shape.value(QStringLiteral("ty")).toString();
        if (type == QStringLiteral("gr")) {
            analyzeShapes(shape.value(QStringLiteral("it")).toArray(), result);
            continue;
        }
        if (type == QStringLiteral("tr") || type == QStringLiteral("fl") ||
            type == QStringLiteral("gf") || type == QStringLiteral("st") ||
            type == QStringLiteral("gs")) {
            continue;
        }
        ++result.totalShapes;
        if (type == QStringLiteral("sh") || type == QStringLiteral("sr") ||
            type == QStringLiteral("rc") ||
            type == QStringLiteral("el")) {
            ++result.supportedShapes;
        } else {
            result.unsupported.append(QStringLiteral("shape:%1").arg(type));
        }
    }
}

void analyzeLayers(const QJsonArray& layers,
                   const AssetMap& assets,
                   ImportLottie::Analysis& result,
                   QSet<QString>& assetStack)
{
    for (const QJsonValue& value : layers) {
        const QJsonObject layer = value.toObject();
        if (layer.value(QStringLiteral("hd")).toBool(false)) { continue; }
        ++result.totalLayers;
        const int type = layer.value(QStringLiteral("ty")).toInt(-1);
        if (type == 0) {
            ++result.supportedLayers;
            const QString refId = layer.value(QStringLiteral("refId")).toString();
            if (!refId.isEmpty() && assets.contains(refId) &&
                !assetStack.contains(refId)) {
                assetStack.insert(refId);
                analyzeLayers(assets.value(refId).value(QStringLiteral("layers")).toArray(),
                              assets, result, assetStack);
                assetStack.remove(refId);
            }
        } else if (type == 1 || type == 2 || type == 3 || type == 5) {
            ++result.supportedLayers;
        } else if (type == 4) {
            ++result.supportedLayers;
            analyzeShapes(layer.value(QStringLiteral("shapes")).toArray(), result);
        } else {
            result.unsupported.append(QStringLiteral("layer:%1").arg(type));
        }
    }
}

} // namespace

ImportLottie::Analysis ImportLottie::analyzeFile(const QString& filename)
{
    QString sourceDir;
    const QJsonObject root = readLottieRoot(filename, sourceDir, false);
    Analysis result;
    result.fps = root.value(QStringLiteral("fr")).toDouble(0);
    result.firstFrame = qRound(root.value(QStringLiteral("ip")).toDouble(0));
    result.lastFrame = qRound(root.value(QStringLiteral("op")).toDouble(0)) - 1;
    result.duration = result.fps > 0 ?
                (result.lastFrame - result.firstFrame + 1) / result.fps : 0;
    result.size = QSize(root.value(QStringLiteral("w")).toInt(),
                        root.value(QStringLiteral("h")).toInt());

    const AssetMap assets = assetMap(root.value(QStringLiteral("assets")).toArray());
    QSet<QString> assetStack;
    analyzeLayers(root.value(QStringLiteral("layers")).toArray(),
                  assets, result, assetStack);
    result.unsupported.removeDuplicates();
    return result;
}

qsptr<BoundingBox> ImportLottie::loadFile(const QString& filename,
                                          Canvas* const scene,
                                          const SceneDurationMode durationMode)
{
    if (!scene) { RuntimeThrow("Lottie import requires an active scene"); }
    QString sourceDir;
    const QJsonObject root = readLottieRoot(filename, sourceDir, true);
    const int inPoint = qRound(root.value(QStringLiteral("ip")).toDouble(0));
    const int outPoint = qRound(root.value(QStringLiteral("op")).toDouble(0));
    const auto result = enve::make_shared<ContainerBox>(
                QFileInfo(filename).completeBaseName(), eBoxType::group);

    const AssetMap assets = assetMap(root.value(QStringLiteral("assets")).toArray());
    QSet<QString> assetStack;
    importLayersToContainer(root.value(QStringLiteral("layers")).toArray(),
                            result.get(), scene, inPoint, outPoint,
                            assets, sourceDir, assetStack);

    const int importedLastFrame = scene->getMinFrame() + qMax(1, outPoint - inPoint);
    if (durationMode == SceneDurationMode::fitImportedLottie && outPoint > inPoint) {
        scene->setFrameRange({scene->getMinFrame(), importedLastFrame});
    } else if (durationMode == SceneDurationMode::extendIfNeeded &&
               importedLastFrame > scene->getMaxFrame()) {
        scene->setFrameRange({scene->getMinFrame(), importedLastFrame});
    }
    return result;
}
