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

#include "svganimationimporter.h"

#include <QBuffer>
#include <QCryptographicHash>
#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QMatrix>
#include <QRegularExpression>
#include <QSet>
#include <QUrl>
#include <QtMath>

#include "Animators/qpointfanimator.h"
#include "Animators/qrealanimator.h"
#include "Animators/qrealkey.h"
#include "Animators/coloranimator.h"
#include "Animators/outlinesettingsanimator.h"
#include "Animators/paintsettingsanimator.h"
#include "Animators/SmartPath/smartpathanimator.h"
#include "Animators/transformanimator.h"
#include "Boxes/boundingbox.h"
#include "Boxes/boxrenderdata.h"
#include "Boxes/circle.h"
#include "Boxes/containerbox.h"
#include "Boxes/imagebox.h"
#include "Boxes/rectangle.h"
#include "Boxes/smartvectorpath.h"
#include "PathEffects/dashpatheffect.h"
#include "canvas.h"
#include "exceptions.h"
#include "svgimporter.h"

qsptr<ImageBox> createImageBox(const QString& path);

namespace {

struct AnimationTrack {
    QString targetId;
    QString targetName;
    QString attribute;
    QString type;
    QString calcMode;
    QList<qreal> times;
    QStringList values;
    QList<QList<qreal>> splines;
    qreal begin = 0;
    qreal duration = 0;
    bool repeatIndefinitely = false;
};

struct ImageCandidate {
    QString placeholderId;
    QString name;
    QString path;
};

struct LayerCandidate {
    QString targetId;
    QString targetName;
    SkBlendMode blendMode = SkBlendMode::kSrcOver;
};

struct DashCandidate {
    QString targetId;
    QString targetName;
    qreal size;
};

struct MaskCandidate {
    QString targetId;
    QString targetName;
    SkBlendMode blendMode;
};

const AnimationTrack* findTrack(const QList<AnimationTrack>& tracks,
                                const QString& targetId,
                                const QString& attribute);
qreal parseClock(const QString& value, bool* ok = nullptr);

bool hasAnimationValues(const QDomElement& animation)
{
    const int valuesCount = animation.attribute("values")
            .split(';', Qt::SkipEmptyParts).size();
    return valuesCount >= 2 ||
            (!animation.attribute("from").isEmpty() &&
             !animation.attribute("to").isEmpty());
}

bool supportsAnimation(const QDomElement& animation)
{
    if (animation.tagName() != "animate" &&
        animation.tagName() != "animateTransform") {
        return false;
    }

    bool durationOk = false;
    parseClock(animation.attribute("dur"), &durationOk);
    if (!durationOk || !hasAnimationValues(animation)) { return false; }

    const QDomElement target = animation.parentNode().toElement();
    const QString tag = target.tagName().toLower();
    const QString attribute = animation.attribute("attributeName");
    if (attribute == "opacity") { return true; }
    if (attribute == "transform") {
        const QString type = animation.attribute("type");
        return type == "translate" || type == "scale" || type == "rotate" ||
                type == "skewX" || type == "skewY";
    }

    const QStringList paintedTags{
        "circle", "ellipse", "rect", "path", "polygon", "polyline", "line"
    };
    if (paintedTags.contains(tag) &&
        (attribute == "fill" || attribute == "fill-opacity" ||
         attribute == "stroke" || attribute == "stroke-opacity" ||
         attribute == "stroke-width")) {
        return true;
    }
    if ((tag == "circle" || tag == "ellipse") &&
        (attribute == "cx" || attribute == "cy" || attribute == "rx" ||
         attribute == "ry" || attribute == "r")) {
        return true;
    }
    if (tag == "rect" &&
        (attribute == "x" || attribute == "y" || attribute == "width" ||
         attribute == "height" || attribute == "rx" || attribute == "ry")) {
        return true;
    }
    return tag == "path" && attribute == "d";
}

QString unsupportedDescription(const QDomElement& animation)
{
    const QString attribute = animation.attribute("attributeName");
    if (attribute.isEmpty()) { return animation.tagName(); }
    if (animation.tagName() == "animateTransform") {
        return attribute + ":" + animation.attribute("type");
    }
    return attribute;
}

bool isImportedRootElement(const QDomElement& element)
{
    const QString tag = element.tagName().toLower();
    return tag == "g" || tag == "text" || tag == "circle" ||
            tag == "ellipse" || tag == "rect" || tag == "path" ||
            tag == "polyline" || tag == "polygon" || tag == "line";
}

bool hasTechnicalRoot(const QDomDocument& document)
{
    int importedChildren = 0;
    const QDomElement root = document.firstChildElement("svg");
    if (root.hasAttribute("transform")) { return false; }
    for (QDomElement child = root.firstChildElement(); !child.isNull();
         child = child.nextSiblingElement()) {
        if (child.tagName() == "animate" ||
            child.tagName() == "animateTransform") {
            return false;
        }
        if (isImportedRootElement(child) && ++importedChildren > 1) {
            return true;
        }
    }
    return false;
}

qreal parseClock(const QString& value, bool* ok)
{
    QString text = value.trimmed();
    qreal multiplier = 1;
    if (text.endsWith("ms")) {
        multiplier = 0.001;
        text.chop(2);
    } else if (text.endsWith('s')) {
        text.chop(1);
    }
    bool parsed = false;
    const qreal result = text.toDouble(&parsed) * multiplier;
    if (ok) { *ok = parsed; }
    return parsed ? result : 0;
}

qint64 greatestCommonDivisor(qint64 first, qint64 second)
{
    while (second != 0) {
        const qint64 remainder = first % second;
        first = second;
        second = remainder;
    }
    return first;
}

qreal animationDuration(const QDomDocument& document)
{
    qreal maximumEnd = 0;
    qint64 commonCycleMs = 0;
    constexpr qint64 maximumCommonCycleMs = 60 * 60 * 1000;
    const QStringList tags{"animate", "animateTransform", "animateMotion", "set"};
    for (const QString& tag : tags) {
        const QDomNodeList nodes = document.elementsByTagName(tag);
        for (int i = 0; i < nodes.count(); ++i) {
            const QDomElement animation = nodes.at(i).toElement();
            bool durationOk = false;
            const qreal duration = parseClock(animation.attribute("dur"),
                                              &durationOk);
            bool beginOk = false;
            const qreal begin = parseClock(animation.attribute("begin", "0s"),
                                           &beginOk);
            if (durationOk) {
                const qreal actualBegin = beginOk ? begin : 0;
                maximumEnd = qMax(maximumEnd, duration + actualBegin);
                if (qFuzzyIsNull(actualBegin) &&
                    animation.attribute("repeatCount").trimmed() ==
                    "indefinite") {
                    const qint64 durationMs = qMax<qint64>(
                                1, qRound64(duration * 1000));
                    if (commonCycleMs == 0) {
                        commonCycleMs = durationMs;
                        continue;
                    }
                    const qint64 divisor = greatestCommonDivisor(
                                commonCycleMs, durationMs);
                    const qint64 multiplier = commonCycleMs / divisor;
                    if (multiplier <=
                        maximumCommonCycleMs / durationMs) {
                        commonCycleMs = multiplier * durationMs;
                    }
                }
            }
        }
    }
    return qMax(maximumEnd, commonCycleMs / 1000.);
}

QList<qreal> parseNumbers(const QString& value)
{
    QList<qreal> result;
    const auto parts = value.split(QRegularExpression("[,\\s]+"),
                                   Qt::SkipEmptyParts);
    for (const QString& part : parts) {
        bool ok = false;
        const qreal number = part.toDouble(&ok);
        if (ok) { result.append(number); }
    }
    return result;
}

QList<qreal> parseSemicolonNumbers(const QString& value)
{
    QList<qreal> result;
    for (const QString& part : value.split(';', Qt::SkipEmptyParts)) {
        bool ok = false;
        const qreal number = part.trimmed().toDouble(&ok);
        if (ok) { result.append(number); }
    }
    return result;
}

bool parseTransformList(const QString& value, QMatrix& result)
{
    static const QRegularExpression function(
                QStringLiteral("([A-Za-z]+)\\s*\\(([^)]*)\\)"));
    auto match = function.globalMatch(value);
    int parsedEnd = 0;
    bool parsedAny = false;
    while (match.hasNext()) {
        const auto current = match.next();
        if (!value.mid(parsedEnd, current.capturedStart() - parsedEnd)
                .trimmed().isEmpty()) {
            return false;
        }
        parsedEnd = current.capturedEnd();
        parsedAny = true;

        const QString name = current.captured(1);
        const QList<qreal> values = parseNumbers(current.captured(2));
        if (name.compare("translate", Qt::CaseInsensitive) == 0 &&
            (values.size() == 1 || values.size() == 2)) {
            result.translate(values.at(0),
                             values.size() == 2 ? values.at(1) : 0);
        } else if (name.compare("scale", Qt::CaseInsensitive) == 0 &&
                   (values.size() == 1 || values.size() == 2)) {
            result.scale(values.at(0),
                         values.size() == 2 ? values.at(1) : values.at(0));
        } else if (name.compare("rotate", Qt::CaseInsensitive) == 0 &&
                   (values.size() == 1 || values.size() == 3)) {
            if (values.size() == 3) {
                result.translate(values.at(1), values.at(2));
            }
            result.rotate(values.at(0));
            if (values.size() == 3) {
                result.translate(-values.at(1), -values.at(2));
            }
        } else if (name.compare("skewX", Qt::CaseInsensitive) == 0 &&
                   values.size() == 1) {
            result.shear(qTan(qDegreesToRadians(values.at(0))), 0);
        } else if (name.compare("skewY", Qt::CaseInsensitive) == 0 &&
                   values.size() == 1) {
            result.shear(0, qTan(qDegreesToRadians(values.at(0))));
        } else {
            return false;
        }
    }
    return parsedAny && value.mid(parsedEnd).trimmed().isEmpty();
}

void normalizeStaticTransforms(QDomElement element)
{
    if (element.hasAttribute("transform")) {
        QMatrix matrix;
        if (parseTransformList(element.attribute("transform"), matrix)) {
            element.setAttribute(
                        "transform",
                        QStringLiteral("matrix(%1 %2 %3 %4 %5 %6)")
                        .arg(matrix.m11(), 0, 'g', 15)
                        .arg(matrix.m12(), 0, 'g', 15)
                        .arg(matrix.m21(), 0, 'g', 15)
                        .arg(matrix.m22(), 0, 'g', 15)
                        .arg(matrix.dx(), 0, 'g', 15)
                        .arg(matrix.dy(), 0, 'g', 15));
        }
    }
    for (QDomElement child = element.firstChildElement(); !child.isNull();
         child = child.nextSiblingElement()) {
        normalizeStaticTransforms(child);
    }
}

void normalizePresentationAttributes(QDomElement element)
{
    static const QStringList presentationAttributes{
        "font-family", "font-size", "font-style", "font-weight", "text-anchor",
        "opacity"
    };
    QStringList presentationStyles;
    for (const QString& attribute : presentationAttributes) {
        if (element.hasAttribute(attribute)) {
            presentationStyles.append(attribute + ':' +
                                      element.attribute(attribute));
        }
    }
    if (!presentationStyles.isEmpty()) {
        QString style = element.attribute("style").trimmed();
        if (!style.isEmpty() && !style.startsWith(';')) {
            style.prepend(';');
        }
        element.setAttribute("style", presentationStyles.join(';') + style);
    }
    for (QDomElement child = element.firstChildElement(); !child.isNull();
         child = child.nextSiblingElement()) {
        normalizePresentationAttributes(child);
    }
}

bool parseColor(const QString& value, QColor& color)
{
    const QString text = value.trimmed();
    color = QColor(text);
    if (color.isValid()) { return true; }

    static const QRegularExpression function(
                "^rgba?\\s*\\(([^)]*)\\)$",
                QRegularExpression::CaseInsensitiveOption);
    const auto match = function.match(text);
    if (!match.hasMatch()) { return false; }
    const auto parts = match.captured(1).split(QRegularExpression("[,\\s]+"),
                                                Qt::SkipEmptyParts);
    if (parts.size() < 3) { return false; }
    bool rOk = false;
    bool gOk = false;
    bool bOk = false;
    const int r = parts.at(0).toInt(&rOk);
    const int g = parts.at(1).toInt(&gOk);
    const int b = parts.at(2).toInt(&bOk);
    if (!rOk || !gOk || !bOk) { return false; }
    qreal alpha = 1;
    if (parts.size() > 3) {
        bool alphaOk = false;
        alpha = parts.at(3).toDouble(&alphaOk);
        if (!alphaOk) { return false; }
    }
    color.setRgb(r, g, b);
    color.setAlphaF(qBound(0., alpha, 1.));
    return true;
}

BoundingBox* findBox(BoundingBox* box, const QString& name)
{
    if (!box) { return nullptr; }
    if (box->prp_getName() == name) { return box; }
    const auto container = enve_cast<ContainerBox*>(box);
    if (!container) { return nullptr; }
    for (BoundingBox* child : container->getContainedBoxes()) {
        if (const auto found = findBox(child, name)) { return found; }
    }
    return nullptr;
}

void keepSVGTransformOrigin(BoundingBox* const box)
{
    if (!box) { return; }

    // ImportSVG plans centered pivots for imported boxes. SVG transforms use
    // the coordinate-system origin unless an explicit center is provided.
    // Consume the pending centering before adding animated transforms.
    const auto renderData = box->createRenderData(0);
    if (renderData) {
        renderData->fRelBoundingRectSet = true;
        renderData->fRelBoundingRect = box->getRelBoundingRect();
        box->updateCurrentPreviewDataFromRenderData(renderData.get());
    }
    box->setPivotRelPos(QPointF());
}

QString ensureTargetId(QDomElement& target, int& nextId)
{
    const QString existing = target.attribute("data-friction-animation-target");
    if (!existing.isEmpty()) { return existing; }
    const QString id = QString("__friction_svg_animation_%1").arg(nextId++);
    const QString name = target.attribute("inkscape:label",
                                          target.attribute("id",
                                                           target.tagName()));
    target.setAttribute("data-friction-animation-target", id);
    target.setAttribute("data-friction-animation-name", name);
    target.setAttribute("inkscape:label", id);
    return id;
}

QString imageHref(const QDomElement& image)
{
    return image.attribute("href", image.attribute("xlink:href"));
}

QString resolveImagePath(const QString& href, const QString& svgFilename)
{
    if (href.startsWith("data:", Qt::CaseInsensitive)) {
        const int comma = href.indexOf(',');
        if (comma < 0) { return {}; }
        const QString metadata = href.mid(5, comma - 5);
        const QByteArray payload = href.mid(comma + 1).toLatin1();
        const QByteArray data = metadata.contains(";base64",
                                                  Qt::CaseInsensitive) ?
                    QByteArray::fromBase64(payload) :
                    QByteArray::fromPercentEncoding(payload);
        if (data.isEmpty()) { return {}; }
        QBuffer buffer;
        buffer.setData(data);
        if (!buffer.open(QIODevice::ReadOnly)) { return {}; }
        QImageReader reader(&buffer);
        const QImage image = reader.read();
        if (image.isNull() ||
            reader.error() != QImageReader::UnknownError) {
            return {};
        }

        const QFileInfo svgInfo(svgFilename);
        const QString assetDirName =
                svgInfo.completeBaseName() + "_svg_assets";
        QDir assetDir(svgInfo.absoluteDir().filePath(assetDirName));
        if (!assetDir.exists() && !svgInfo.absoluteDir().mkpath(assetDirName)) {
            return {};
        }
        const QString digest = QString::fromLatin1(
                    QCryptographicHash::hash(data, QCryptographicHash::Sha256)
                    .toHex().left(16));
        const QString path = assetDir.filePath("embedded_" + digest + ".png");
        if (!QFileInfo::exists(path) && !image.save(path, "PNG")) {
            return {};
        }
        return QFileInfo(path).absoluteFilePath();
    }

    const QUrl url(href);
    if (url.isLocalFile()) { return QFileInfo(url.toLocalFile()).absoluteFilePath(); }
    if (!url.scheme().isEmpty()) { return {}; }
    return QFileInfo(QFileInfo(svgFilename).absoluteDir(), href)
            .absoluteFilePath();
}

QString imagePlacementTransform(const QDomElement& image,
                                const QSize& intrinsicSize)
{
    const qreal x = image.attribute("x").toDouble();
    const qreal y = image.attribute("y").toDouble();
    const qreal width = image.attribute("width").isEmpty() ?
                intrinsicSize.width() : image.attribute("width").toDouble();
    const qreal height = image.attribute("height").isEmpty() ?
                intrinsicSize.height() : image.attribute("height").toDouble();
    qreal scaleX = width / intrinsicSize.width();
    qreal scaleY = height / intrinsicSize.height();
    qreal offsetX = x;
    qreal offsetY = y;

    const QString aspect = image.attribute("preserveAspectRatio").simplified();
    if (!aspect.startsWith("none", Qt::CaseInsensitive)) {
        const bool slice = aspect.contains("slice", Qt::CaseInsensitive);
        const qreal scale = slice ? qMax(scaleX, scaleY) : qMin(scaleX, scaleY);
        const qreal renderedWidth = intrinsicSize.width() * scale;
        const qreal renderedHeight = intrinsicSize.height() * scale;
        const qreal remainingWidth = width - renderedWidth;
        const qreal remainingHeight = height - renderedHeight;
        if (aspect.contains("xMax", Qt::CaseInsensitive)) {
            offsetX += remainingWidth;
        } else if (!aspect.contains("xMin", Qt::CaseInsensitive)) {
            offsetX += remainingWidth * 0.5;
        }
        if (aspect.contains("YMax", Qt::CaseInsensitive)) {
            offsetY += remainingHeight;
        } else if (!aspect.contains("YMin", Qt::CaseInsensitive)) {
            offsetY += remainingHeight * 0.5;
        }
        scaleX = scale;
        scaleY = scale;
    }

    return QString("translate(%1 %2) scale(%3 %4)")
            .arg(offsetX, 0, 'g', 16)
            .arg(offsetY, 0, 'g', 16)
            .arg(scaleX, 0, 'g', 16)
            .arg(scaleY, 0, 'g', 16);
}

QList<ImageCandidate> materializeImages(QDomDocument& document,
                                        const QString& svgFilename)
{
    QList<ImageCandidate> result;
    const QDomNodeList images = document.elementsByTagName("image");
    for (int i = images.count() - 1; i >= 0; --i) {
        const QDomElement image = images.at(i).toElement();
        const QString path = resolveImagePath(imageHref(image), svgFilename);
        QImageReader reader(path);
        const QSize intrinsicSize = reader.size();
        if (path.isEmpty() || !intrinsicSize.isValid()) { continue; }

        const QString baseId = QString("__friction_svg_image_%1").arg(i);
        const QString groupId = baseId + "_group";
        const QString placeholderId = baseId + "_placeholder";
        const QString name = image.attribute(
                    "inkscape:label", image.attribute("id", "Image"));
        QDomElement group = document.createElement("g");
        const QDomNamedNodeMap attributes = image.attributes();
        for (int attrId = 0; attrId < attributes.count(); ++attrId) {
            const QDomAttr attr = attributes.item(attrId).toAttr();
            const QString attrName = attr.name();
            if (attrName == "x" || attrName == "y" ||
                attrName == "width" || attrName == "height" ||
                attrName == "href" || attrName == "xlink:href" ||
                attrName == "preserveAspectRatio" ||
                attrName == "id" || attrName == "inkscape:label") {
                continue;
            }
            group.setAttribute(attrName, attr.value());
        }
        if (!group.hasAttribute("transform")) {
            group.setAttribute("transform", "translate(0 0)");
        }
        group.setAttribute("id", groupId);
        group.setAttribute("inkscape:label", name);

        QDomElement placeholder = document.createElement("rect");
        placeholder.setAttribute("x", "0");
        placeholder.setAttribute("y", "0");
        placeholder.setAttribute("width", intrinsicSize.width());
        placeholder.setAttribute("height", intrinsicSize.height());
        placeholder.setAttribute("fill", "none");
        placeholder.setAttribute("transform",
                                 imagePlacementTransform(image, intrinsicSize));
        placeholder.setAttribute("id", placeholderId);
        placeholder.setAttribute("inkscape:label", placeholderId);
        group.appendChild(placeholder);
        while (!image.firstChild().isNull()) {
            group.appendChild(image.firstChild());
        }
        image.parentNode().replaceChild(group, image);
        result.append({placeholderId, name, path});
    }
    return result;
}

void replaceImagePlaceholders(BoundingBox* const root,
                              const QList<ImageCandidate>& candidates)
{
    for (const ImageCandidate& candidate : candidates) {
        BoundingBox* const placeholder = findBox(root, candidate.placeholderId);
        const auto group = placeholder ? placeholder->getParentGroup() : nullptr;
        if (!group || !placeholder) { continue; }

        qsptr<eBoxOrSound> placeholderRef;
        for (const auto& child : group->getContained()) {
            if (child.get() == placeholder) {
                placeholderRef = child;
                break;
            }
        }
        if (!placeholderRef) { continue; }

        const auto image = createImageBox(candidate.path);
        placeholder->copyBoundingBoxDataTo(image.get());
        image->prp_setName(candidate.name);
        group->replaceContained(placeholderRef, image);
    }
}

QString styleProperty(const QDomElement& element, const QString& property)
{
    for (const QString& declaration :
         element.attribute("style").split(';', Qt::SkipEmptyParts)) {
        const int separator = declaration.indexOf(':');
        if (separator < 0) { continue; }
        if (declaration.left(separator).trimmed() == property) {
            return declaration.mid(separator + 1).trimmed();
        }
    }
    return QString();
}

QString presentationProperty(const QDomElement& element,
                             const QString& property)
{
    const QString styled = styleProperty(element, property);
    return styled.isEmpty() ? element.attribute(property) : styled;
}

QString urlReferenceId(const QString& value)
{
    static const QRegularExpression url(
                QStringLiteral("^\\s*url\\(\\s*#([^\\s)]+)\\s*\\)\\s*$"),
                QRegularExpression::CaseInsensitiveOption);
    const auto match = url.match(value);
    return match.hasMatch() ? match.captured(1) : QString();
}

bool isFullMaskBackground(const QDomElement& element)
{
    if (element.tagName().toLower() != "rect") { return false; }
    return element.attribute("width").trimmed() == "100%" &&
            element.attribute("height").trimmed() == "100%";
}

QList<MaskCandidate> materializeMasks(QDomDocument& document)
{
    QList<MaskCandidate> result;
    QHash<QString, QDomElement> masks;
    const QDomNodeList maskNodes = document.elementsByTagName("mask");
    for (int i = 0; i < maskNodes.count(); ++i) {
        const QDomElement mask = maskNodes.at(i).toElement();
        if (!mask.attribute("id").isEmpty()) {
            masks.insert(mask.attribute("id"), mask);
        }
    }

    int nextId = 0;
    const QDomNodeList groups = document.elementsByTagName("g");
    for (int i = 0; i < groups.count(); ++i) {
        QDomElement group = groups.at(i).toElement();
        QString maskValue = presentationProperty(group, "mask");
        const QString maskId = urlReferenceId(maskValue);
        if (maskId.isEmpty() || !masks.contains(maskId)) { continue; }

        const QDomElement mask = masks.value(maskId);
        QList<QDomElement> content;
        bool hasFullBackground = false;
        for (QDomElement child = mask.firstChildElement(); !child.isNull();
             child = child.nextSiblingElement()) {
            if (isFullMaskBackground(child)) {
                hasFullBackground = true;
            } else {
                content.append(child);
            }
        }
        if (content.isEmpty()) { continue; }

        const QString targetId =
                QString("__friction_svg_mask_%1").arg(nextId++);
        const QString targetName = maskId;
        QDomElement importedMask = document.createElement("g");
        for (const QDomElement& child : content) {
            importedMask.appendChild(child.cloneNode(true));
        }
        // ImportSVG flattens single-child groups. A non-element node preserves
        // the container without creating an extra visible object.
        importedMask.appendChild(document.createComment("friction-mask-layer"));
        importedMask.setAttribute("inkscape:label", targetId);
        group.appendChild(importedMask);

        MaskCandidate candidate;
        candidate.targetId = targetId;
        candidate.targetName = targetName;
        candidate.blendMode = hasFullBackground ?
                    SkBlendMode::kDstOut : SkBlendMode::kDstIn;
        result.append(candidate);
    }
    return result;
}

bool parseAbsoluteSVGLength(const QString& value, qreal& result)
{
    static const QRegularExpression length(
                QStringLiteral("^\\s*"
                               "([+-]?(?:\\d+(?:\\.\\d*)?|\\.\\d+)"
                               "(?:[eE][+-]?\\d+)?)"
                               "\\s*(px|pt|pc|mm|cm|in)?\\s*$"),
                QRegularExpression::CaseInsensitiveOption);
    const auto match = length.match(value);
    if (!match.hasMatch()) { return false; }

    bool ok = false;
    result = match.captured(1).toDouble(&ok);
    if (!ok || result < 0) { return false; }

    const QString unit = match.captured(2).toLower();
    if (unit == "pt") {
        result *= 96. / 72.;
    } else if (unit == "pc") {
        result *= 16.;
    } else if (unit == "mm") {
        result *= 96. / 25.4;
    } else if (unit == "cm") {
        result *= 96. / 2.54;
    } else if (unit == "in") {
        result *= 96.;
    }
    return true;
}

bool equivalentDashSize(const QString& value, qreal& result)
{
    const QString text = value.trimmed();
    if (text.isEmpty() || text.compare("none", Qt::CaseInsensitive) == 0) {
        return false;
    }

    qreal total = 0;
    int count = 0;
    for (const QString& part :
         text.split(QRegularExpression("[,\\s]+"), Qt::SkipEmptyParts)) {
        qreal interval = 0;
        if (!parseAbsoluteSVGLength(part, interval)) { return false; }
        total += interval;
        ++count;
    }
    if (!count || qFuzzyIsNull(total)) { return false; }

    // Friction's Dash effect uses one equal dash/gap size. The average SVG
    // interval preserves exact symmetric patterns and approximates the period
    // of patterns that Friction cannot represent directly.
    result = total / count;
    return result >= 0.1;
}

bool isDashTarget(const QDomElement& element)
{
    static const QStringList pathTags{
        "circle", "ellipse", "rect", "path", "polygon", "polyline", "line",
        "text"
    };
    return pathTags.contains(element.tagName().toLower());
}

void collectDashCandidates(QDomElement element,
                           const QString& inheritedDashArray,
                           int& nextId,
                           QList<DashCandidate>& result)
{
    QString dashArray = presentationProperty(element, "stroke-dasharray");
    if (dashArray.isEmpty() ||
        dashArray.compare("inherit", Qt::CaseInsensitive) == 0) {
        dashArray = inheritedDashArray;
    } else if (dashArray.compare("none", Qt::CaseInsensitive) == 0) {
        dashArray.clear();
    }

    qreal dashSize = 0;
    if (isDashTarget(element) && equivalentDashSize(dashArray, dashSize)) {
        DashCandidate candidate;
        candidate.targetId =
                element.attribute("data-friction-animation-target");
        candidate.targetName =
                element.attribute("data-friction-animation-name");
        if (candidate.targetId.isEmpty()) {
            candidate.targetId =
                    QString("__friction_svg_dash_%1").arg(nextId++);
            candidate.targetName =
                    element.attribute("inkscape:label",
                                      element.attribute("id",
                                                        element.tagName()));
            element.setAttribute("inkscape:label", candidate.targetId);
        }
        candidate.size = dashSize;
        result.append(candidate);
    }

    for (QDomElement child = element.firstChildElement(); !child.isNull();
         child = child.nextSiblingElement()) {
        collectDashCandidates(child, dashArray, nextId, result);
    }
}

QList<DashCandidate> collectDashCandidates(QDomDocument& document)
{
    QList<DashCandidate> result;
    int nextId = 0;
    collectDashCandidates(document.documentElement(), QString(), nextId, result);
    return result;
}

bool groupNeedsLayer(const QDomElement& group)
{
    for (QDomElement child = group.firstChildElement(); !child.isNull();
         child = child.nextSiblingElement()) {
        const QString tag = child.tagName();
        if (tag == "animate" || tag == "animateTransform" ||
            tag == "animateMotion" || tag == "set") {
            return true;
        }
    }

    QString opacity = group.attribute("opacity");
    if (opacity.isEmpty()) { opacity = styleProperty(group, "opacity"); }
    bool opacityOk = false;
    const qreal opacityValue = opacity.toDouble(&opacityOk);
    if (opacityOk && !qFuzzyCompare(opacityValue + 1., 2.)) { return true; }

    static const QStringList rasterProperties{
        "filter", "mask", "clip-path", "mix-blend-mode", "isolation"
    };
    for (const QString& property : rasterProperties) {
        QString value = group.attribute(property);
        if (value.isEmpty()) { value = styleProperty(group, property); }
        if (!value.isEmpty() && value != "none" && value != "normal" &&
            value != "auto") {
            return true;
        }
    }
    return false;
}

SkBlendMode svgBlendMode(const QDomElement& element)
{
    const QString value =
            presentationProperty(element, "mix-blend-mode").trimmed().toLower();
    if (value == "multiply") { return SkBlendMode::kMultiply; }
    if (value == "screen") { return SkBlendMode::kScreen; }
    if (value == "overlay") { return SkBlendMode::kOverlay; }
    if (value == "darken") { return SkBlendMode::kDarken; }
    if (value == "lighten") { return SkBlendMode::kLighten; }
    if (value == "color-dodge") { return SkBlendMode::kColorDodge; }
    if (value == "color-burn") { return SkBlendMode::kColorBurn; }
    if (value == "hard-light") { return SkBlendMode::kHardLight; }
    if (value == "soft-light") { return SkBlendMode::kSoftLight; }
    if (value == "difference") { return SkBlendMode::kDifference; }
    if (value == "exclusion") { return SkBlendMode::kExclusion; }
    if (value == "hue") { return SkBlendMode::kHue; }
    if (value == "saturation") { return SkBlendMode::kSaturation; }
    if (value == "color") { return SkBlendMode::kColor; }
    if (value == "luminosity") { return SkBlendMode::kLuminosity; }
    if (value == "plus-lighter" || value == "plus") {
        return SkBlendMode::kPlus;
    }
    return SkBlendMode::kSrcOver;
}

QList<LayerCandidate> collectLayerCandidates(QDomDocument& document)
{
    QList<LayerCandidate> result;
    int nextId = 0;
    const QDomNodeList groups = document.elementsByTagName("g");
    for (int i = 0; i < groups.count(); ++i) {
        QDomElement group = groups.at(i).toElement();
        if (!groupNeedsLayer(group)) { continue; }

        LayerCandidate candidate;
        candidate.targetId =
                group.attribute("data-friction-animation-target");
        candidate.targetName =
                group.attribute("data-friction-animation-name");
        if (candidate.targetId.isEmpty()) {
            candidate.targetId =
                    QString("__friction_svg_layer_%1").arg(nextId++);
            candidate.targetName =
                    group.attribute("inkscape:label",
                                    group.attribute("id", "Group"));
            group.setAttribute("inkscape:label", candidate.targetId);
        }
        candidate.blendMode = svgBlendMode(group);
        result.append(candidate);
    }
    return result;
}

QList<AnimationTrack> collectTracks(QDomDocument& document)
{
    QList<AnimationTrack> result;
    int nextId = 0;
    const QStringList tags{"animate", "animateTransform"};
    for (const QString& tag : tags) {
        const QDomNodeList nodes = document.elementsByTagName(tag);
        for (int i = 0; i < nodes.count(); ++i) {
            const QDomElement animation = nodes.at(i).toElement();
            QDomElement target = animation.parentNode().toElement();
            if (target.isNull()) { continue; }

            AnimationTrack track;
            track.targetId = ensureTargetId(target, nextId);
            track.targetName = target.attribute("data-friction-animation-name");
            track.attribute = animation.attribute("attributeName");
            track.type = animation.attribute("type");
            track.calcMode = animation.attribute("calcMode", "linear");
            track.repeatIndefinitely =
                    animation.attribute("repeatCount")
                    .compare("indefinite", Qt::CaseInsensitive) == 0;

            bool durationOk = false;
            track.duration = parseClock(animation.attribute("dur"), &durationOk);
            if (!durationOk || track.duration <= 0) { continue; }

            bool beginOk = false;
            track.begin = parseClock(animation.attribute("begin", "0s"), &beginOk);
            if (!beginOk) { track.begin = 0; }

            track.values = animation.attribute("values")
                               .split(';', Qt::SkipEmptyParts);
            if (track.values.isEmpty()) {
                const QString from = animation.attribute("from");
                const QString to = animation.attribute("to");
                if (!from.isEmpty() && !to.isEmpty()) {
                    track.values << from << to;
                }
            }
            if (track.values.size() < 2) { continue; }

            track.times = parseSemicolonNumbers(animation.attribute("keyTimes"));
            if (track.times.size() != track.values.size()) {
                track.times.clear();
                const qreal divisor = track.values.size() - 1;
                for (int valueId = 0; valueId < track.values.size(); ++valueId) {
                    track.times.append(valueId / divisor);
                }
            }

            for (const QString& spline :
                 animation.attribute("keySplines").split(';', Qt::SkipEmptyParts)) {
                track.splines.append(parseNumbers(spline));
            }
            result.append(track);
        }
    }
    return result;
}

AnimationTrack repeatedTrack(const AnimationTrack& source,
                             const int repetitions)
{
    if (!source.repeatIndefinitely || repetitions <= 1) { return source; }

    AnimationTrack result = source;
    result.duration *= repetitions;
    result.times.clear();
    result.values.clear();
    result.splines.clear();
    result.repeatIndefinitely = false;

    qreal rotationCycle = 0;
    if (source.attribute == "transform" && source.type == "rotate") {
        const auto first = parseNumbers(source.values.first());
        const auto last = parseNumbers(source.values.last());
        if (!first.isEmpty() && !last.isEmpty()) {
            rotationCycle = last.first() - first.first();
        }
    }

    for (int repetition = 0; repetition < repetitions; ++repetition) {
        for (int valueId = 0; valueId < source.values.size(); ++valueId) {
            if (repetition > 0 && valueId == 0) { continue; }

            result.times.append((repetition + source.times.at(valueId)) /
                                repetitions);
            if (!qFuzzyIsNull(rotationCycle)) {
                const auto numbers = parseNumbers(source.values.at(valueId));
                if (numbers.isEmpty()) { return source; }
                QStringList shifted;
                shifted.append(QString::number(
                                   numbers.first() +
                                   repetition * rotationCycle));
                for (int numberId = 1; numberId < numbers.size(); ++numberId) {
                    shifted.append(QString::number(numbers.at(numberId)));
                }
                result.values.append(shifted.join(' '));
            } else {
                result.values.append(source.values.at(valueId));
            }
        }
        result.splines.append(source.splines);
    }
    return result;
}

QList<AnimationTrack> expandIndefiniteTracks(
        const QList<AnimationTrack>& tracks, const qreal targetDuration)
{
    qreal end = targetDuration;
    for (const AnimationTrack& track : tracks) {
        end = qMax(end, track.begin + track.duration);
    }

    QList<AnimationTrack> result;
    result.reserve(tracks.size());
    for (const AnimationTrack& track : tracks) {
        const qreal available = end - track.begin;
        const int repetitions = qMax(1, qCeil(
                                         available / track.duration -
                                         0.000001));
        result.append(repeatedTrack(track, repetitions));
    }
    return result;
}

void applySpline(QrealAnimator* animator,
                 const int frame0, const int frame1,
                 const qreal value0, const qreal value1,
                 const QList<qreal>& spline)
{
    if (!animator || spline.size() != 4 || frame0 == frame1) { return; }
    auto key0 = animator->anim_getKeyAtRelFrame<QrealKey>(frame0);
    auto key1 = animator->anim_getKeyAtRelFrame<QrealKey>(frame1);
    if (!key0 || !key1) { return; }

    const qreal frameSpan = frame1 - frame0;
    const qreal valueSpan = value1 - value0;
    key0->setC1Enabled(true);
    key0->setC1Frame(frame0 + spline.at(0) * frameSpan);
    key0->setC1Value(value0 + spline.at(1) * valueSpan);
    key1->setC0Enabled(true);
    key1->setC0Frame(frame0 + spline.at(2) * frameSpan);
    key1->setC0Value(value0 + spline.at(3) * valueSpan);
}

void applyScalarTrack(QrealAnimator* animator,
                      const AnimationTrack& track,
                      const qreal fps,
                      const qreal multiplier = 1)
{
    if (!animator) { return; }
    QList<int> frames;
    QList<qreal> values;
    for (int i = 0; i < track.values.size(); ++i) {
        bool ok = false;
        const qreal value = track.values.at(i).trimmed().toDouble(&ok) * multiplier;
        if (!ok) { return; }
        const int frame = qRound((track.begin + track.times.at(i) *
                                  track.duration) * fps);
        frames.append(frame);
        values.append(value);
    }
    for (int i = 0; i < frames.size(); ++i) {
        if (track.calcMode == "discrete" && i > 0 &&
            frames.at(i) - 1 > frames.at(i - 1)) {
            animator->saveValueToKey(frames.at(i) - 1, values.at(i - 1));
        }
        animator->saveValueToKey(frames.at(i), values.at(i));
    }
    if (track.calcMode == "discrete") { return; }
    const QList<qreal> linearSpline{1./3, 1./3, 2./3, 2./3};
    for (int i = 0; i + 1 < frames.size(); ++i) {
        const QList<qreal> spline =
                track.calcMode == "spline" && i < track.splines.size() ?
                    track.splines.at(i) : linearSpline;
        applySpline(animator, frames.at(i), frames.at(i + 1),
                    values.at(i), values.at(i + 1), spline);
    }
}

void applyColorTrack(ColorAnimator* animator, const AnimationTrack& track,
                     const QList<AnimationTrack>& tracks, const qreal fps,
                     const QString& opacityAttribute)
{
    if (!animator) { return; }
    AnimationTrack red = track;
    AnimationTrack green = track;
    AnimationTrack blue = track;
    AnimationTrack alpha = track;
    red.values.clear();
    green.values.clear();
    blue.values.clear();
    alpha.values.clear();
    const bool separateOpacity =
            findTrack(tracks, track.targetId, opacityAttribute);
    for (const QString& value : track.values) {
        QColor color;
        if (!parseColor(value, color)) { return; }
        red.values.append(QString::number(color.redF()));
        green.values.append(QString::number(color.greenF()));
        blue.values.append(QString::number(color.blueF()));
        alpha.values.append(QString::number(color.alphaF()));
    }
    animator->setColorMode(ColorMode::rgb);
    applyScalarTrack(animator->getVal1Animator(), red, fps);
    applyScalarTrack(animator->getVal2Animator(), green, fps);
    applyScalarTrack(animator->getVal3Animator(), blue, fps);
    if (!separateOpacity) {
        applyScalarTrack(animator->getAlphaAnimator(), alpha, fps);
    }
}

void applyPaintTrack(PaintSettingsAnimator* paint,
                     const AnimationTrack& track,
                     const QList<AnimationTrack>& tracks,
                     const qreal fps,
                     const QString& colorAttribute,
                     const QString& opacityAttribute)
{
    if (!paint) { return; }
    if (track.attribute == colorAttribute) {
        for (const QString& value : track.values) {
            if (value.trimmed() == "none") { return; }
        }
        paint->setPaintType(PaintType::FLATPAINT);
        applyColorTrack(paint->getColorAnimator(), track, tracks, fps,
                        opacityAttribute);
    } else if (track.attribute == opacityAttribute) {
        applyScalarTrack(paint->getColorAnimator()->getAlphaAnimator(),
                         track, fps);
    }
}

void applyPointTrack(QPointFAnimator* animator,
                     const AnimationTrack& track,
                     const qreal fps,
                     const bool uniformSingleValue)
{
    if (!animator) { return; }
    AnimationTrack xTrack = track;
    AnimationTrack yTrack = track;
    xTrack.values.clear();
    yTrack.values.clear();
    for (const QString& value : track.values) {
        const auto numbers = parseNumbers(value);
        if (numbers.isEmpty()) { return; }
        xTrack.values.append(QString::number(numbers.at(0)));
        const qreal y = numbers.size() > 1 ? numbers.at(1) :
                        uniformSingleValue ? numbers.at(0) : 0;
        yTrack.values.append(QString::number(y));
    }
    applyScalarTrack(animator->getXAnimator(), xTrack, fps);
    applyScalarTrack(animator->getYAnimator(), yTrack, fps);
}

AnimationTrack offsetTrack(const AnimationTrack& source, const qreal offset)
{
    AnimationTrack result = source;
    result.values.clear();
    for (const QString& value : source.values) {
        bool ok = false;
        const qreal number = value.trimmed().toDouble(&ok);
        if (!ok) { return AnimationTrack(); }
        result.values.append(QString::number(number + offset));
    }
    return result;
}

const AnimationTrack* findTrack(const QList<AnimationTrack>& tracks,
                                const QString& targetId,
                                const QString& attribute)
{
    for (const AnimationTrack& track : tracks) {
        if (track.targetId == targetId && track.attribute == attribute) {
            return &track;
        }
    }
    return nullptr;
}

AnimationTrack sumTracks(const AnimationTrack& primary,
                         const AnimationTrack* secondary,
                         const qreal secondaryFallback)
{
    if (!secondary ||
        secondary->values.size() != primary.values.size() ||
        secondary->times != primary.times ||
        !qFuzzyCompare(secondary->begin + 1, primary.begin + 1) ||
        !qFuzzyCompare(secondary->duration + 1, primary.duration + 1)) {
        return offsetTrack(primary, secondaryFallback);
    }
    AnimationTrack result = primary;
    result.values.clear();
    QList<qreal> primaryValues;
    QList<qreal> secondaryValues;
    for (int i = 0; i < primary.values.size(); ++i) {
        bool primaryOk = false;
        bool secondaryOk = false;
        const qreal primaryValue = primary.values.at(i).trimmed().toDouble(&primaryOk);
        const qreal secondaryValue =
                secondary->values.at(i).trimmed().toDouble(&secondaryOk);
        if (!primaryOk || !secondaryOk) { return AnimationTrack(); }
        primaryValues.append(primaryValue);
        secondaryValues.append(secondaryValue);
        result.values.append(QString::number(primaryValue + secondaryValue));
    }

    const QList<qreal> linearSpline{1./3, 1./3, 2./3, 2./3};
    result.calcMode = "spline";
    result.splines.clear();
    for (int i = 0; i + 1 < result.values.size(); ++i) {
        const auto primarySpline =
                primary.calcMode == "spline" && i < primary.splines.size() ?
                    primary.splines.at(i) : linearSpline;
        const auto secondarySpline =
                secondary->calcMode == "spline" && i < secondary->splines.size() ?
                    secondary->splines.at(i) : linearSpline;
        if (primarySpline.size() != 4 || secondarySpline.size() != 4) {
            result.splines.append(linearSpline);
            continue;
        }

        const qreal primarySpan = primaryValues.at(i + 1) - primaryValues.at(i);
        const qreal secondarySpan =
                secondaryValues.at(i + 1) - secondaryValues.at(i);
        const qreal resultSpan = primarySpan + secondarySpan;
        if (qFuzzyIsNull(resultSpan)) {
            result.splines.append(linearSpline);
            continue;
        }

        QList<qreal> spline = primarySpline;
        if (primary.calcMode != "spline" && secondary->calcMode == "spline") {
            spline[0] = secondarySpline.at(0);
            spline[2] = secondarySpline.at(2);
            spline[1] = (primarySpan * spline.at(0) +
                         secondarySpan * secondarySpline.at(1)) / resultSpan;
            spline[3] = (primarySpan * spline.at(2) +
                         secondarySpan * secondarySpline.at(3)) / resultSpan;
        } else if (primary.calcMode == "spline" &&
                   secondary->calcMode != "spline") {
            spline[1] = (primarySpan * primarySpline.at(1) +
                         secondarySpan * spline.at(0)) / resultSpan;
            spline[3] = (primarySpan * primarySpline.at(3) +
                         secondarySpan * spline.at(2)) / resultSpan;
        } else {
            spline[1] = (primarySpan * primarySpline.at(1) +
                         secondarySpan * secondarySpline.at(1)) / resultSpan;
            spline[3] = (primarySpan * primarySpline.at(3) +
                         secondarySpan * secondarySpline.at(3)) / resultSpan;
        }
        result.splines.append(spline);
    }
    return result;
}

void applyCircleTrack(Circle* circle, const AnimationTrack& track,
                      const qreal fps)
{
    if (track.attribute == "cx") {
        applyScalarTrack(circle->getCenterAnimator()->getXAnimator(), track, fps);
    } else if (track.attribute == "cy") {
        applyScalarTrack(circle->getCenterAnimator()->getYAnimator(), track, fps);
    } else if (track.attribute == "rx") {
        applyScalarTrack(circle->getHRadiusAnimator()->getXAnimator(), track, fps);
    } else if (track.attribute == "ry") {
        applyScalarTrack(circle->getVRadiusAnimator()->getYAnimator(), track, fps);
    } else if (track.attribute == "r") {
        applyScalarTrack(circle->getHRadiusAnimator()->getXAnimator(), track, fps);
        applyScalarTrack(circle->getVRadiusAnimator()->getYAnimator(), track, fps);
    }
}

void applyRectangleTrack(RectangleBox* rectangle, const AnimationTrack& track,
                         const QList<AnimationTrack>& tracks, const qreal fps)
{
    const QPointF topLeft = rectangle->getTopLeftAnimator()->getBaseValue();
    const QPointF bottomRight = rectangle->getBottomRightAnimator()->getBaseValue();
    if (track.attribute == "x") {
        applyScalarTrack(rectangle->getTopLeftAnimator()->getXAnimator(),
                         track, fps);
        if (!findTrack(tracks, track.targetId, "width")) {
            applyScalarTrack(rectangle->getBottomRightAnimator()->getXAnimator(),
                             offsetTrack(track, bottomRight.x() - topLeft.x()), fps);
        }
    } else if (track.attribute == "y") {
        applyScalarTrack(rectangle->getTopLeftAnimator()->getYAnimator(),
                         track, fps);
        if (!findTrack(tracks, track.targetId, "height")) {
            applyScalarTrack(rectangle->getBottomRightAnimator()->getYAnimator(),
                             offsetTrack(track, bottomRight.y() - topLeft.y()), fps);
        }
    } else if (track.attribute == "width") {
        applyScalarTrack(rectangle->getBottomRightAnimator()->getXAnimator(),
                         sumTracks(track, findTrack(tracks, track.targetId, "x"),
                                   topLeft.x()), fps);
    } else if (track.attribute == "height") {
        applyScalarTrack(rectangle->getBottomRightAnimator()->getYAnimator(),
                         sumTracks(track, findTrack(tracks, track.targetId, "y"),
                                   topLeft.y()), fps);
    } else if (track.attribute == "rx") {
        applyScalarTrack(rectangle->getRadiusAnimator()->getXAnimator(),
                         track, fps);
    } else if (track.attribute == "ry") {
        applyScalarTrack(rectangle->getRadiusAnimator()->getYAnimator(),
                         track, fps);
    }
}

void applyPathTrack(SmartVectorPath* vectorPath, const AnimationTrack& track,
                    const qreal fps)
{
    if (track.attribute != "d") { return; }
    const auto collection = vectorPath->getPathAnimator();
    if (!collection || collection->ca_getNumberOfChildren() != 1) { return; }
    const auto animator = collection->getChild(0);
    QList<SmartPathKey*> keys;
    SmartPath previousPath;
    int previousFrame = 0;
    for (int i = 0; i < track.values.size(); ++i) {
        SkPath path;
        const auto pathString = track.values.at(i).trimmed().toStdString();
        if (!SkParsePath::FromSVGString(pathString.c_str(), &path)) { return; }
        const int frame = qRound((track.begin + track.times.at(i) *
                                  track.duration) * fps);
        if (track.calcMode == "discrete" && i > 0 &&
            frame - 1 > previousFrame) {
            const auto holdKey = enve::make_shared<SmartPathKey>(
                        previousPath, frame - 1, animator);
            animator->anim_appendKey(holdKey);
        }
        const auto key = enve::make_shared<SmartPathKey>(SmartPath(path),
                                                         frame, animator);
        animator->anim_appendKey(key);
        keys.append(key.get());
        previousPath = SmartPath(path);
        previousFrame = frame;
    }
    if (track.calcMode != "spline") { return; }
    for (int i = 0; i + 1 < keys.size() && i < track.splines.size(); ++i) {
        const auto& spline = track.splines.at(i);
        if (spline.size() != 4) { continue; }
        const qreal frame0 = keys.at(i)->getRelFrame();
        const qreal frame1 = keys.at(i + 1)->getRelFrame();
        const qreal span = frame1 - frame0;
        keys.at(i)->setC1Enabled(true);
        keys.at(i)->setC1Frame(frame0 + spline.at(0) * span);
        keys.at(i)->setC1Value(frame0 + spline.at(1) * span);
        keys.at(i + 1)->setC0Enabled(true);
        keys.at(i + 1)->setC0Frame(frame0 + spline.at(2) * span);
        keys.at(i + 1)->setC0Value(frame0 + spline.at(3) * span);
    }
}

void applyTrack(BoundingBox* box, const AnimationTrack& track,
                const QList<AnimationTrack>& tracks, const qreal fps)
{
    if (!box) { return; }
    if (const auto circle = enve_cast<Circle*>(box)) {
        applyCircleTrack(circle, track, fps);
    }
    if (const auto rectangle = enve_cast<RectangleBox*>(box)) {
        applyRectangleTrack(rectangle, track, tracks, fps);
    }
    if (const auto vectorPath = enve_cast<SmartVectorPath*>(box)) {
        applyPathTrack(vectorPath, track, fps);
    }
    if (const auto pathBox = enve_cast<PathBox*>(box)) {
        applyPaintTrack(pathBox->getFillSettings(), track, tracks, fps,
                        "fill", "fill-opacity");
        applyPaintTrack(pathBox->getStrokeSettings(), track, tracks, fps,
                        "stroke", "stroke-opacity");
        if (track.attribute == "stroke-width") {
            applyScalarTrack(pathBox->getStrokeSettings()->getLineWidthAnimator(),
                             track, fps);
        }
    }

    const auto transform = box->getBoxTransformAnimator();
    if (!transform) { return; }
    if (track.attribute == "opacity") {
        applyScalarTrack(transform->getOpacityAnimator(), track, fps, 100);
    } else if (track.attribute == "transform" && track.type == "translate") {
        applyPointTrack(transform->getPosAnimator(), track, fps, false);
    } else if (track.attribute == "transform" && track.type == "scale") {
        applyPointTrack(transform->getScaleAnimator(), track, fps, true);
    } else if (track.attribute == "transform" && track.type == "rotate") {
        AnimationTrack rotation = track;
        rotation.values.clear();
        for (const QString& value : track.values) {
            const auto numbers = parseNumbers(value);
            if (numbers.isEmpty()) { return; }
            rotation.values.append(QString::number(numbers.first()));
        }
        applyScalarTrack(transform->getRotAnimator(), rotation, fps);
    } else if (track.attribute == "transform" && track.type == "skewX") {
        applyScalarTrack(transform->getShearAnimator()->getXAnimator(),
                         track, fps, 1. / 45.);
    } else if (track.attribute == "transform" && track.type == "skewY") {
        applyScalarTrack(transform->getShearAnimator()->getYAnimator(),
                         track, fps, 1. / 45.);
    }
}

} // namespace

ImportSVGAnimation::Analysis ImportSVGAnimation::analyzeSVGFile(
        const QString& filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        RuntimeThrow("Cannot open file " + filename);
    }
    QDomDocument document;
    if (!document.setContent(&file)) {
        RuntimeThrow("Cannot parse SVG animation file " + filename);
    }

    Analysis result;
    const QStringList tags{"animate", "animateTransform", "animateMotion", "set"};
    for (const QString& tag : tags) {
        const QDomNodeList nodes = document.elementsByTagName(tag);
        for (int i = 0; i < nodes.count(); ++i) {
            const QDomElement animation = nodes.at(i).toElement();
            ++result.totalTracks;
            if (supportsAnimation(animation)) {
                ++result.supportedTracks;
            } else {
                result.unsupported.append(unsupportedDescription(animation));
            }
        }
    }
    result.duration = animationDuration(document);
    result.unsupported.removeDuplicates();
    return result;
}

qsptr<BoundingBox> ImportSVGAnimation::loadSVGFile(const QString& filename,
                                                   Canvas* scene,
                                                   const SceneDurationMode durationMode,
                                                   bool* const technicalRoot)
{
    if (!scene) { RuntimeThrow("SVG animation import requires an active scene"); }
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        RuntimeThrow("Cannot open file " + filename);
    }
    QDomDocument document;
    if (!document.setContent(&file)) {
        RuntimeThrow("Cannot parse SVG animation file " + filename);
    }
    const qreal importedDuration = animationDuration(document);
    const bool documentHasTechnicalRoot = hasTechnicalRoot(document);
    const auto imageCandidates = materializeImages(document, filename);
    normalizePresentationAttributes(document.documentElement());
    normalizeStaticTransforms(document.documentElement());

    const auto maskCandidates = materializeMasks(document);
    const auto tracks = expandIndefiniteTracks(collectTracks(document),
                                               importedDuration);
    const auto layerCandidates = collectLayerCandidates(document);
    const auto dashCandidates = collectDashCandidates(document);
    const auto gradientCreator = [scene]() { return scene->createNewGradient(); };
    const auto result = ImportSVG::loadSVGFile(document, gradientCreator);
    if (!result) { return nullptr; }
    replaceImagePlaceholders(result.get(), imageCandidates);
    if (technicalRoot) {
        *technicalRoot = documentHasTechnicalRoot &&
                enve_cast<ContainerBox*>(result.get());
    }

    QHash<QString, BoundingBox*> targets;
    QSet<QString> preparedTransformTargets;
    for (const AnimationTrack& track : tracks) {
        if (!targets.contains(track.targetId)) {
            BoundingBox* const target = findBox(result.get(), track.targetId);
            targets.insert(track.targetId, target);
        }
        if (track.attribute == "transform" &&
            !preparedTransformTargets.contains(track.targetId)) {
            keepSVGTransformOrigin(targets.value(track.targetId));
            preparedTransformTargets.insert(track.targetId);
        }
        applyTrack(targets.value(track.targetId), track, tracks, scene->getFps());
    }
    for (const DashCandidate& candidate : dashCandidates) {
        const auto path = enve_cast<PathBox*>(
                    findBox(result.get(), candidate.targetId));
        if (!path) { continue; }
        const auto effect = enve::make_shared<DashPathEffect>();
        const auto size =
                effect->ca_getFirstDescendantWithName<QrealAnimator>("size");
        if (!size) { continue; }
        size->setCurrentBaseValue(candidate.size);
        path->addPathEffect(effect);
        path->setPathEffectsEnabled(true);
        if (!candidate.targetName.isEmpty()) {
            path->prp_setName(candidate.targetName);
        }
    }
    for (const LayerCandidate& candidate : layerCandidates) {
        const auto group = enve_cast<ContainerBox*>(
                    findBox(result.get(), candidate.targetId));
        if (!group) { continue; }
        group->promoteToLayer();
        group->setBlendModeSk(candidate.blendMode);
        if (!candidate.targetName.isEmpty()) {
            group->prp_setName(candidate.targetName);
        }
    }
    for (const MaskCandidate& candidate : maskCandidates) {
        const auto maskLayer = enve_cast<ContainerBox*>(
                    findBox(result.get(), candidate.targetId));
        if (!maskLayer) { continue; }
        maskLayer->promoteToLayer();
        maskLayer->setBlendModeSk(candidate.blendMode);
        maskLayer->prp_setName(candidate.targetName);
    }
    for (const AnimationTrack& track : tracks) {
        BoundingBox* const target = targets.value(track.targetId);
        if (target && !track.targetName.isEmpty() &&
            target->prp_getName() != track.targetName) {
            target->prp_setName(track.targetName);
        }
    }
    const int importedLastFrame =
            scene->getMinFrame() +
            qMax(1, qRound(importedDuration * scene->getFps()));
    if (durationMode == SceneDurationMode::fitImportedSVG &&
        importedDuration > 0) {
        scene->setFrameRange({scene->getMinFrame(), importedLastFrame});
    } else if (durationMode == SceneDurationMode::extendIfNeeded &&
               importedLastFrame > scene->getMaxFrame()) {
        scene->setFrameRange({scene->getMinFrame(), importedLastFrame});
    }
    return result;
}
