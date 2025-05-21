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

#include "skqtconversions.h"

#include <QImage>

#include "exceptions.h"
#include "Segments/qcubicsegment2d.h"
#include "skia/skiahelpers.h"

QRect toQRect(const SkIRect &rect) {
    return QRect(rect.left(), rect.top(),
                 rect.width(), rect.height());
}

SkIRect toSkIRect(const QRect &rect) {
    return SkIRect::MakeLTRB(rect.left(), rect.top(),
                             rect.right(), rect.bottom());
}

QRectF toQRectF(const SkRect &rect) {
    return QRectF(toQreal(rect.x()),
                  toQreal(rect.y()),
                  toQreal(rect.width()),
                  toQreal(rect.height()));
}

SkRect toSkRect(const QRectF &rect) {
    return SkRect::MakeXYWH(toSkScalar(rect.x()),
                            toSkScalar(rect.y()),
                            toSkScalar(rect.width()),
                            toSkScalar(rect.height()));
}
// m11 - scaleX
// m12 - skewY
// m21 - skewX
// m22 - scaleY
QMatrix toQMatrix(const SkMatrix &matrix) {
    return QMatrix(toQreal(matrix.getScaleX()),
                   toQreal(matrix.getSkewY()),
                   toQreal(matrix.getSkewX()),
                   toQreal(matrix.getScaleY()),
                   toQreal(matrix.getTranslateX()),
                   toQreal(matrix.getTranslateY()));
}

QTransform toQTransform(const SkMatrix &matrix) {
    return QTransform(toQreal(matrix.getScaleX()),
                      toQreal(matrix.getSkewY()),
                      toQreal(matrix.getSkewX()),
                      toQreal(matrix.getScaleY()),
                      toQreal(matrix.getTranslateX()),
                      toQreal(matrix.getTranslateY()));
}

SkMatrix toSkMatrix(const QMatrix &matrix) {
    SkMatrix skMatrix;
    skMatrix.reset();
    skMatrix.set(SkMatrix::kMScaleX, toSkScalar(matrix.m11()));
    skMatrix.set(SkMatrix::kMScaleY, toSkScalar(matrix.m22()));
    skMatrix.set(SkMatrix::kMSkewX, toSkScalar(matrix.m21()));
    skMatrix.set(SkMatrix::kMSkewY, toSkScalar(matrix.m12()));
    skMatrix.set(SkMatrix::kMTransX, toSkScalar(matrix.dx()));
    skMatrix.set(SkMatrix::kMTransY, toSkScalar(matrix.dy()));
    return skMatrix;
}

SkMatrix toSkMatrix(const QTransform &transform) {
    SkMatrix skMatrix;
    skMatrix.set(SkMatrix::kMScaleX, toSkScalar(transform.m11()));
    skMatrix.set(SkMatrix::kMScaleY, toSkScalar(transform.m22()));
    skMatrix.set(SkMatrix::kMSkewX, toSkScalar(transform.m21()));
    skMatrix.set(SkMatrix::kMSkewY, toSkScalar(transform.m12()));
    skMatrix.set(SkMatrix::kMTransX, toSkScalar(transform.dx()));
    skMatrix.set(SkMatrix::kMTransY, toSkScalar(transform.dy()));
    return skMatrix;
}

SkPaint::Cap QCapToSkCap(const Qt::PenCapStyle &cap) {
    if(cap == Qt::RoundCap) {
        return SkPaint::kRound_Cap;
    }
    if(cap == Qt::SquareCap) {
        return SkPaint::kSquare_Cap;
    }
    return SkPaint::kButt_Cap;
}

SkPaint::Join QJoinToSkJoin(const Qt::PenJoinStyle &join) {
    if(join == Qt::RoundJoin) {
        return SkPaint::kRound_Join;
    }
    if(join == Qt::BevelJoin) {
        return SkPaint::kBevel_Join;
    }
    return SkPaint::kMiter_Join;
}
#include <QPainterPath>
SkPath toSkPath(const QPainterPath &qPath) {
    SkPath path;
    bool firstOther = false;
    SkPoint movePt{0, 0};
    SkPoint endPt{0, 0};
    SkPoint startPt{0, 0};
    const int iMax = qPath.elementCount() - 1;
    for(int i = 0; i <= iMax; i++) {
        const QPainterPath::Element &elem = qPath.elementAt(i);
        const SkPoint toPt{toSkScalar(elem.x), toSkScalar(elem.y)};
        if(elem.isMoveTo()) { // move
            movePt = toPt;
            path.moveTo(movePt);
        } else if(elem.isLineTo()) { // line
            path.lineTo(toPt);
        } else if(elem.isCurveTo()) { // curve
            endPt = toPt;
            firstOther = true;
        } else { // other
            if(firstOther) startPt = toPt;
            else path.cubicTo(endPt, startPt, toPt);
            firstOther = !firstOther;
        }
        if(i == iMax && SkPoint::Distance(movePt, toPt) < 0.001f)
            path.close();
    }
    return path;
}

QPainterPath toQPainterPath(const SkPath& path) {
    QPainterPath qPath;
    SkPath::RawIter iter = SkPath::RawIter(path);

    SkPoint pts[4];
    for(;;) {
        auto verb = iter.next(pts);
        switch(verb) {
            case SkPath::kMove_Verb: {
                const SkPoint pt = pts[0];
                qPath.moveTo(toQPointF(pt));
            }
                break;
            case SkPath::kLine_Verb: {
                const SkPoint pt = pts[1];

                qPath.lineTo(toQPointF(pt));
            }
                break;
            case SkPath::kConic_Verb: {
                const QPointF p0 = toQPointF(pts[0]);
                const QPointF p1 = toQPointF(pts[1]);
                const QPointF p2 = toQPointF(pts[2]);
                const qreal weight = SkScalarToDouble(iter.conicWeight());

                const auto seg = qCubicSegment2D::sFromConic(p0, p1, p2, weight);
                pts[1] = toSkPoint(seg.c1());
                pts[2] = toSkPoint(seg.c2());
                pts[3] = toSkPoint(seg.p3());
                verb = SkPath::kCubic_Verb;
                continue;
            }
            case SkPath::kCubic_Verb: {
                const SkPoint endPt = pts[1];
                const SkPoint startPt = pts[2];
                const SkPoint targetPt = pts[3];
                qPath.cubicTo(toQPointF(endPt),
                              toQPointF(startPt),
                              toQPointF(targetPt));
            }
                break;
            case SkPath::kClose_Verb:
                qPath.closeSubpath();
                break;
            case SkPath::kQuad_Verb: {
                const SkPoint ctrlPt = pts[1];
                const SkPoint targetPt = pts[2];
                qPath.quadTo(toQPointF(ctrlPt),
                             toQPointF(targetPt));
            }
                break;
            case SkPath::kDone_Verb:
                return qPath;
        }
    }
}

SkColor toSkColor(const QColor &qcol) {
    return SkColorSetARGB(static_cast<U8CPU>(qcol.alpha()),
                          static_cast<U8CPU>(qcol.red()),
                          static_cast<U8CPU>(qcol.green()),
                          static_cast<U8CPU>(qcol.blue()));
}

void switchSkQ(const QPointF &qPos, SkPoint &skPos) {
    skPos = toSkPoint(qPos);
}

void switchSkQ(const SkPoint &skPos, QPointF &qPos) {
    qPos = toQPointF(skPos);
}

void switchSkQ(const qreal q, float &sk) {
    sk = toSkScalar(q);
}

void switchSkQ(const float sk, qreal &q) {
    q = toQreal(sk);
}

QFont::Style toQStyle(const SkFontStyle::Slant& style) {
    switch(style) {
    case SkFontStyle::kOblique_Slant:
        return QFont::StyleOblique;
    case SkFontStyle::kItalic_Slant:
        return QFont::StyleItalic;
    case SkFontStyle::kUpright_Slant:
        return QFont::StyleNormal;
    }
    return QFont::StyleNormal;
}

int SkFontWeightToQFontWeight(int skWeight) {
    int qWeight;
    switch(skWeight) {
    case SkFontStyle::kThin_Weight: qWeight = QFont::Thin; break;
    case SkFontStyle::kExtraLight_Weight: qWeight = QFont::ExtraLight; break;
    case SkFontStyle::kLight_Weight: qWeight = QFont::Light; break;
    case SkFontStyle::kNormal_Weight: qWeight = QFont::Normal; break;
    case SkFontStyle::kMedium_Weight: qWeight = QFont::Medium; break;
    case SkFontStyle::kSemiBold_Weight: qWeight = QFont::DemiBold; break;
    case SkFontStyle::kBold_Weight: qWeight = QFont::Bold; break;
    case SkFontStyle::kExtraBold_Weight: qWeight = QFont::ExtraBold; break;
    case SkFontStyle::kBlack_Weight: qWeight = QFont::Black; break;
    default: {
        qWeight = qRound(skWeight/10.0);
        if(qWeight >= 0 && qWeight < 30) {
            qWeight -= 10;
        } else if(qWeight >= 45 && qWeight < 50) {
            qWeight += 10;
        }
    }
    }

    if(qWeight < 0) qWeight = 0;

    return qWeight;
}

QFont toQFont(const SkFont &skfont, const int skPPI, const int qPPI) {
    const auto typeface = skfont.getTypefaceOrDefault();
    const auto skStyle = typeface->fontStyle();
    const QFont::Style style = toQStyle(skStyle.slant());

    int stretch;
    switch(skStyle.width()) {
    case SkFontStyle::kUltraCondensed_Width:
        stretch = 56; break;
    case SkFontStyle::kExtraCondensed_Width:
        stretch = 68; break;
    case SkFontStyle::kCondensed_Width:
        stretch = 81; break;
    case SkFontStyle::kSemiCondensed_Width:
        stretch = 94; break;
    case SkFontStyle::kNormal_Width:
        stretch = 106; break;
    case SkFontStyle::kSemiExpanded_Width:
        stretch = 118; break;
    case SkFontStyle::kExpanded_Width:
        stretch = 137; break;
    case SkFontStyle::kExtraExpanded_Width:
        stretch = 175; break;
    default:
        stretch = 200;
    }

    const int weight = SkFontWeightToQFontWeight(skStyle.weight());

    QFont qFont;
    SkString family;
    typeface->getFamilyName(&family);
    qFont.setFamily(family.c_str());
    qFont.setStretch(stretch);
    qFont.setWeight(weight);
    qFont.setStyle(style);
    qFont.setPointSizeF(skfont.getSize()*skPPI/qPPI);
    return qFont;
}

QFont toQFont(const SkFont& skfont) {
    return toQFont(skfont, 72, 72);
}

SkFontStyle::Slant toSkSlant(const QFont::Style& style) {
    switch(style) {
    case QFont::StyleOblique:
        return SkFontStyle::kOblique_Slant;
    case QFont::StyleItalic:
        return SkFontStyle::kItalic_Slant;
    case QFont::StyleNormal:
        return SkFontStyle::kUpright_Slant;
    }
    return SkFontStyle::kUpright_Slant;
}

int QFontWeightToSkFontWeight(const int qWeight) {
    int skWeight = qWeight;
    switch(qWeight) {
    case QFont::Thin: skWeight = 100; break;
    case QFont::ExtraLight: skWeight = 200; break;
    case QFont::Light: skWeight = 300; break;
    case QFont::Normal: skWeight = 400; break;
    case QFont::Medium: skWeight = 500; break;
    case QFont::DemiBold: skWeight = 600; break;
    case QFont::Bold: skWeight = 700; break;
    case QFont::ExtraBold: skWeight = 800; break;
    case QFont::Black: skWeight = 900; break;
    default: {
        if(skWeight >= 0 && skWeight < 30) {
            skWeight += 10;
        } else if(skWeight >= 45 && skWeight < 50) {
            skWeight -= 10;
        }
        skWeight = 100*qRound(skWeight/10.0);
    }
    }

    if(skWeight > 900) skWeight = 900;
    return skWeight;
}

SkFont toSkFont(const QFont &qfont, const int qPPI, const int skPPI) {
    const auto slant = toSkSlant(qfont.style());
    SkFontStyle::Width width{SkFontStyle::kNormal_Width};
    const int qStretch = qfont.stretch();

    if(qStretch <= 56) {
        width = SkFontStyle::kUltraCondensed_Width;
    } else if(qStretch <= 68) {
        width = SkFontStyle::kExtraCondensed_Width;
    } else if(qStretch <= 81) {
        width = SkFontStyle::kCondensed_Width;
    } else if(qStretch <= 94) {
        width = SkFontStyle::kSemiCondensed_Width;
    } else if(qStretch <= 106) {
        width = SkFontStyle::kNormal_Width;
    } else if(qStretch <= 118) {
        width = SkFontStyle::kSemiExpanded_Width;
    } else if(qStretch <= 137) {
        width = SkFontStyle::kExpanded_Width;
    } else if(qStretch <= 175) {
        width = SkFontStyle::kExtraExpanded_Width;
    } else {
        width = SkFontStyle::kUltraExpanded_Width;
    }

    const int weight = QFontWeightToSkFontWeight(qfont.weight());
    const SkFontStyle style(weight, width, slant);

    SkFont skFont;
    const auto fmlStdStr = qfont.family().toStdString();
    const auto typeface = SkTypeface::MakeFromName(fmlStdStr.c_str(), style);
    skFont.setTypeface(typeface);
    const qreal ptSize = qfont.pointSizeF()*qPPI/skPPI;
    skFont.setSize(toSkScalar(ptSize));
    return skFont;
}

SkFont toSkFont(const QFont& qfont) {
    return toSkFont(qfont, 72, 72);
}

uint16_t floatToHalfFloat(const float float32) {
    uint32_t fltInt32;
    memcpy(&fltInt32, &float32, sizeof(float));

    uint16_t fltInt16 = (fltInt32 >> 31) << 5;
    uint16_t tmp = (fltInt32 >> 23) & 0xff;
    tmp = (tmp - 0x70) & ((uint32_t)((int32_t)(0x70 - tmp) >> 4) >> 27);
    fltInt16 = (fltInt16 | tmp) << 10;
    fltInt16 |= (fltInt32 >> 13) & 0x3ff;
    return fltInt16;
}

uint16_t uint16ToHalfFloat(const uint16_t uint16) {
    return floatToHalfFloat(float(uint16)/USHRT_MAX);
}

sk_sp<SkImage> toSkImage(const QImage &qImg) {
    const QImage::Format format = qImg.format();
    const int width = qImg.width();
    const int height = qImg.height();
    switch(format) {
    case QImage::Format_RGBA8888:
    case QImage::Format_RGBA8888_Premultiplied: {
        SkColorType colorType;
        SkAlphaType alphaType;
        if(format == QImage::Format_RGBA8888_Premultiplied) {
            colorType = kRGBA_8888_SkColorType;
            alphaType = kPremul_SkAlphaType;
        } else { // QImage::Format_RGBA8888:
            colorType = kRGBA_8888_SkColorType;
            alphaType = kUnpremul_SkAlphaType;
        }
        const auto info = SkImageInfo::Make(width, height, colorType,
                                            alphaType, nullptr);
        SkBitmap bitmap;
        bitmap.allocPixels(info, qImg.bytesPerLine());
        const uint8_t* const src = reinterpret_cast<const uint8_t*>(qImg.bits());
        const uint8_t* const end = src + qImg.width() * qImg.height() * 4;
        uint8_t* dst = static_cast<uint8_t*>(bitmap.getPixels());
        std::for_each(src, end, [&dst](const uint8_t c) {
            *dst++ = c;
        });
        return SkiaHelpers::transferDataToSkImage(bitmap);
    } break;
    case QImage::Format_ARGB32:
    case QImage::Format_ARGB32_Premultiplied: {
        SkAlphaType alphaType;
        if(format == QImage::Format_ARGB32_Premultiplied) {
            alphaType = kPremul_SkAlphaType;
        } else { // QImage::Format_ARGB32:
            alphaType = kUnpremul_SkAlphaType;
        }
        const auto info = SkImageInfo::Make(width, height,
                                            kRGBA_8888_SkColorType,
                                            alphaType, nullptr);
        SkBitmap bitmap;
        bitmap.allocPixels(info, qImg.bytesPerLine());
        const QRgb* const src = reinterpret_cast<const QRgb*>(qImg.bits());
        const QRgb* const end = src + qImg.width() * qImg.height();
        uint8_t* dst = static_cast<uint8_t*>(bitmap.getPixels());
        std::for_each(src, end, [&dst](const QRgb& c) {
            *dst++ = qRed(c);
            *dst++ = qGreen(c);
            *dst++ = qBlue(c);
            *dst++ = qAlpha(c);
        });
        return SkiaHelpers::transferDataToSkImage(bitmap);
    }
    case QImage::Format_RGBA64:
    case QImage::Format_RGBA64_Premultiplied: {
        SkAlphaType alphaType;
        if(format == QImage::Format_RGBA64_Premultiplied) {
            alphaType = kPremul_SkAlphaType;
        } else { // QImage::Format_RGBA64:
            alphaType = kUnpremul_SkAlphaType;
        }
        const auto info = SkImageInfo::Make(width, height,
                                            kRGBA_F16Norm_SkColorType,
                                            alphaType, nullptr);
        SkBitmap bitmap;
        bitmap.allocPixels(info, qImg.bytesPerLine());
        const uint16_t* const src = reinterpret_cast<const uint16_t*>(qImg.bits());
        const uint16_t* const end = src + qImg.width() * qImg.height() * 4;
        float* dst = static_cast<float*>(bitmap.getPixels());
        std::for_each(src, end, [&dst](const uint16_t c) {
            *dst++ = uint16ToHalfFloat(c);
        });
        return SkiaHelpers::transferDataToSkImage(bitmap);
    }
    default: RuntimeThrow("Unsupported format");
    }
}
