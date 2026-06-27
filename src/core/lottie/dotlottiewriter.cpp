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
*/

#include "lottie/dotlottiewriter.h"

#include "exceptions.h"
#include "lottie/lottiejsonoptimizer.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

namespace {

struct ZipEntry {
    ZipEntry(const QByteArray& entryName, const QByteArray& entryData)
        : name(entryName), data(entryData) {}

    QByteArray name;
    QByteArray data;
    QByteArray compressed;
    quint32 crc = 0;
    quint32 offset = 0;
};

void append16(QByteArray& data, const quint16 value)
{
    data.append(static_cast<char>(value & 0xff));
    data.append(static_cast<char>((value >> 8) & 0xff));
}

void append32(QByteArray& data, const quint32 value)
{
    append16(data, static_cast<quint16>(value & 0xffff));
    append16(data, static_cast<quint16>((value >> 16) & 0xffff));
}

quint32 crc32(const QByteArray& data)
{
    quint32 crc = 0xffffffff;
    for (const auto byte : data) {
        crc ^= static_cast<quint8>(byte);
        for (int bit = 0; bit < 8; bit++) {
            crc = (crc >> 1) ^ (0xedb88320 & -(crc & 1));
        }
    }
    return crc ^ 0xffffffff;
}

QByteArray rawDeflate(const QByteArray& data)
{
    const QByteArray compressed = qCompress(data, 9);
    if (compressed.size() < 10) { RuntimeThrow("Could not compress dotLottie entry"); }
    // qCompress adds a four-byte size prefix and a zlib wrapper. ZIP entries
    // require the raw Deflate payload inside that wrapper.
    return compressed.mid(6, compressed.size() - 10);
}

void writeZip(const QString& path, QList<ZipEntry> entries)
{
    QByteArray archive;
    for (auto& entry : entries) {
        entry.crc = crc32(entry.data);
        entry.compressed = rawDeflate(entry.data);
        entry.offset = archive.size();

        append32(archive, 0x04034b50);
        append16(archive, 20);
        append16(archive, 0x0800);
        append16(archive, 8);
        append16(archive, 0);
        append16(archive, 0);
        append32(archive, entry.crc);
        append32(archive, entry.compressed.size());
        append32(archive, entry.data.size());
        append16(archive, entry.name.size());
        append16(archive, 0);
        archive.append(entry.name);
        archive.append(entry.compressed);
    }

    const quint32 directoryOffset = archive.size();
    for (const auto& entry : entries) {
        append32(archive, 0x02014b50);
        append16(archive, 20);
        append16(archive, 20);
        append16(archive, 0x0800);
        append16(archive, 8);
        append16(archive, 0);
        append16(archive, 0);
        append32(archive, entry.crc);
        append32(archive, entry.compressed.size());
        append32(archive, entry.data.size());
        append16(archive, entry.name.size());
        append16(archive, 0);
        append16(archive, 0);
        append16(archive, 0);
        append16(archive, 0);
        append32(archive, 0);
        append32(archive, entry.offset);
        archive.append(entry.name);
    }

    const quint32 directorySize = archive.size() - directoryOffset;
    append32(archive, 0x06054b50);
    append16(archive, 0);
    append16(archive, 0);
    append16(archive, entries.size());
    append16(archive, entries.size());
    append32(archive, directorySize);
    append32(archive, directoryOffset);
    append16(archive, 0);

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate) ||
        file.write(archive) != archive.size()) {
        RuntimeThrow(QStringLiteral("Could not write dotLottie file:\n\"%1\"")
                     .arg(path).toStdString());
    }
}

}

void DotLottieWriter::write(const QString& path,
                            const QString& animationId,
                            const QString& animationName,
                            QJsonObject animation,
                            const QString& assetsPath)
{
    QList<ZipEntry> entries;
    auto assets = animation.value(QStringLiteral("assets")).toArray();
    for (int i = 0; i < assets.size(); i++) {
        auto asset = assets.at(i).toObject();
        // Precomposition assets also live in this array, but only image
        // assets have a path and need to be copied into the archive.
        if (!asset.contains(QStringLiteral("p")) ||
            asset.value(QStringLiteral("p")).toString().isEmpty()) { continue; }
        if (asset.value(QStringLiteral("e")).toInt() != 0) { continue; }

        const QString fileName = asset.value(QStringLiteral("p")).toString();
        QFile image(assetsPath + QStringLiteral("/") + fileName);
        if (!image.open(QIODevice::ReadOnly)) {
            RuntimeThrow(QStringLiteral("Could not read dotLottie image:\n\"%1\"")
                         .arg(image.fileName()).toStdString());
        }
        entries.append({QStringLiteral("images/").append(fileName).toUtf8(),
                        image.readAll()});
        asset.insert(QStringLiteral("u"), QString());
        asset.insert(QStringLiteral("p"), QStringLiteral("images/") + fileName);
        assets.replace(i, asset);
    }
    if (!assets.isEmpty()) { animation.insert(QStringLiteral("assets"), assets); }

    const QJsonDocument animationDoc(LottieJsonOptimizer::optimize(animation));
    entries.prepend({QStringLiteral("animations/%1.json").arg(animationId).toUtf8(),
                     animationDoc.toJson(QJsonDocument::Compact) + '\n'});

    const QJsonObject manifest{
        {QStringLiteral("version"), QStringLiteral("1")},
        {QStringLiteral("generator"), QStringLiteral("Friction")},
        {QStringLiteral("activeAnimationId"), animationId},
        {QStringLiteral("animations"), QJsonArray{
             QJsonObject{
                 {QStringLiteral("id"), animationId},
                 {QStringLiteral("name"), animationName}
             }
         }}
    };
    entries.prepend({QByteArrayLiteral("manifest.json"),
                     QJsonDocument(manifest).toJson(QJsonDocument::Compact) + '\n'});
    writeZip(path, entries);
}
