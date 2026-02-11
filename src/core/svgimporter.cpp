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

#include "svgimporter.h"

#include <QtXml/QDomDocument>
#include <QRegularExpression>

#include "Boxes/containerbox.h"
#include "colorhelpers.h"
#include "pointhelpers.h"
#include "Boxes/circle.h"
#include "Boxes/rectangle.h"
#include "Boxes/textbox.h"
#include "Animators/transformanimator.h"
#include "paintsettingsapplier.h"
#include "Boxes/smartvectorpath.h"
#include "Animators/SmartPath/smartpathcollection.h"
#include "Animators/SmartPath/smartpathanimator.h"
#include "MovablePoints/smartnodepoint.h"
#include "Private/document.h"
#include "matrixdecomposition.h"
#include "transformvalues.h"
#include "regexhelpers.h"

#define RGXS REGEX_SPACES

static qreal parseSvgUnit(const QString &str,
                          qreal relativeTo)
{
    QString trimmed = str.trimmed();
    if (trimmed.endsWith("%")) {
        trimmed.remove("%");
        return (trimmed.toDouble() / 100.0) * relativeTo;
    }
    return trimmed.toDouble();
}

class TextSvgAttributes {
public:
    TextSvgAttributes() {}

    void setFontFamily(const QString &family);
    void setFontSize(const int size);
    void setFontSlant(const SkFontStyle::Slant &slant);
    void setFontWeight(const int weight);
    void bolderFontWeight() { mWeight = qMin(900, mWeight + 100); }
    void lighterFontWeight() { mWeight = qMax(100, mWeight - 100); }

    void setFontAlignment(const Qt::Alignment &alignment);

    SkFont getFont() const {
        const SkFontStyle style(mWeight, SkFontStyle::kNormal_Width, mSlant);

        SkFont skFont;
        const auto fmlStdStr = mFamily.toStdString();
        const auto typeface = SkTypeface::MakeFromName(fmlStdStr.c_str(), style);
        skFont.setTypeface(typeface);
        skFont.setSize(mSize);
        return skFont;
    }
private:
    Qt::Alignment mAlignment = Qt::AlignLeft;
    SkScalar mSize = 1;
    SkFontStyle::Slant mSlant = SkFontStyle::kUpright_Slant;
    int mWeight = SkFontStyle::kNormal_Weight;
    QString mFamily;
};

struct SvgGradient {
    Gradient* fGradient;
    qreal fX1;
    qreal fY1;
    qreal fX2;
    qreal fY2;
    QMatrix fTrans;
    GradientType fType;
};

class FillSvgAttributes {
public:
    FillSvgAttributes() {}

    void setColor(const QColor &val);

    void setColorOpacity(const qreal opacity);

    void setPaintType(const PaintType type);

    void setGradient(const SvgGradient& gradient);

    const QColor &getColor() const;
    PaintType getPaintType() const;
    Gradient *getGradient() const;

    void apply(BoundingBox * const box) const;
    void apply(BoundingBox * const box,
               const PaintSetting::Target& target) const;
protected:
    qreal mOpacity = 1;
    QColor mColor;
    PaintType mPaintType = FLATPAINT;
    Gradient *mGradient = nullptr;
    GradientType mGradientType = GradientType::LINEAR;
    QPointF mGradientP1;
    QPointF mGradientP2;
    QMatrix mGradientTransform;
};

class StrokeSvgAttributes : public FillSvgAttributes {
public:
    StrokeSvgAttributes() {
        mPaintType = NOPAINT;
    }

    qreal getLineWidth() const;
    SkPaint::Cap getCapStyle() const;
    SkPaint::Join getJoinStyle() const;
    QPainter::CompositionMode getOutlineCompositionMode() const;

    void setLineWidth(const qreal val);

    void setCapStyle(const SkPaint::Cap capStyle);
    void setJoinStyle(const SkPaint::Join joinStyle);

    void setOutlineCompositionMode(const QPainter::CompositionMode compMode);

    void apply(BoundingBox *box, const qreal scale) const;
protected:
    SkPaint::Cap mCapStyle = SkPaint::kButt_Cap;
    SkPaint::Join mJoinStyle = SkPaint::kMiter_Join;
    QPainter::CompositionMode mOutlineCompositionMode =
            QPainter::CompositionMode_Source;
    qreal mLineWidth = 1;
};

class BoxSvgAttributes {
public:
    void setParent(const BoxSvgAttributes &parent);

    SkPathFillType getFillRule() const;
    const QMatrix &getRelTransform() const;
    const FillSvgAttributes &getFillAttributes() const;
    const StrokeSvgAttributes &getStrokeAttributes() const;
    const TextSvgAttributes &getTextAttributes() const;

    void loadBoundingBoxAttributes(const QDomElement &element);

    bool hasTransform() const;

    void apply(BoundingBox *box) const;
    void setFillAttribute(const QString &value);
    void setStrokeAttribute(const QString &value);
protected:
    SkPathFillType mFillRule = SkPathFillType::kEvenOdd;

    TransformValues mDecomposedTrans;

    qreal mOpacity = 100;

    QMatrix mRelTransform;

    QString mId;
    QString mLabel;

    FillSvgAttributes mFillAttributes;
    StrokeSvgAttributes mStrokeAttributes;
    TextSvgAttributes mTextAttributes;
};

class PathAnimator;
class VectorPathAnimator;
class SmartPathAnimator;

class SmartVectorPath;
class VectorPathSvgAttributes : public BoxSvgAttributes {
public:
    SkPath& path() { return mPath; }

    void apply(SmartVectorPath * const path);

    bool isEmpty() const { return mPath.isEmpty(); }
protected:
    SkPath mPath;
};

struct SvgAttribute {
    SvgAttribute(const QString &nameValueStr) {
        const QStringList nameValueList = nameValueStr.split(":");
        fName = nameValueList.first();
        fValue = nameValueList.last();
    }

    QString getName() const {
        return fName;
    }

    QString getValue() const {
        return fValue;
    }

    QString fName;
    QString fValue;
};

void extractSvgAttributes(const QString &string,
                          QList<SvgAttribute> * const attributesList)
{
    const QStringList attributesStrList = string.split(";",
                                                       Qt::SkipEmptyParts);
    for(const QString &attributeStr : attributesStrList) {
        QString cleanAttribute = attributeStr.trimmed();
        if (cleanAttribute.isEmpty()) { continue; }
        attributesList->append(SvgAttribute(cleanAttribute));
    }
}

bool isColorRGB(const QString &colorStr)
{
    static QRegularExpression rx(RGXS "rgb\\(.*\\)" RGXS,
                                 QRegularExpression::CaseInsensitiveOption);
    return rx.match(colorStr).hasMatch();
}

bool isColorRGBA(const QString &colorStr)
{
    static QRegularExpression rx(RGXS "rgba\\(.*\\)" RGXS,
                                 QRegularExpression::CaseInsensitiveOption);
    return rx.match(colorStr).hasMatch();
}

bool toColor(const QString &colorStr, QColor &color)
{
    const bool isRGB = isColorRGB(colorStr);
    const bool isRGBA = isColorRGBA(colorStr);
    if (isRGB || isRGBA) {
        QString str = colorStr.toLower().simplified();
        str.remove(isRGB ? "rgb" : "rgba").remove(";").remove("(").remove(")");

        QStringList components;
        for (const QString &part : str.split(",", Qt::SkipEmptyParts)) {
            components << part.trimmed();
        }
        if (components.size() >= 3) {
            const bool isFloat = components[0].contains("%") ||
                                 components[1].contains("%") ||
                                 components[2].contains("%");
            const bool hasAlpha = components.size() == 4;

            qreal r = components[0].contains("%") ? components[0].remove("%").trimmed().toFloat() / 100. :
                                                    components[0].toInt();
            qreal g = components[1].contains("%") ? components[1].remove("%").trimmed().toFloat() / 100. :
                                                    components[1].toInt();
            qreal b = components[2].contains("%") ? components[2].remove("%").trimmed().toFloat() / 100. :
                                                    components[2].toInt();
            qreal a = hasAlpha ? components[3].contains("%") ? components[3].remove("%").trimmed().toFloat() / 100. :
                                 components[3].toFloat() : 1.0;
            if (isFloat) { color.setRgbF(r, g, b); }
            else { color.setRgb(r, g, b); }
            if (hasAlpha) { color.setAlphaF(a); }
        } else { return false; }
    } else {
        if (QColor::isValidColor(colorStr.simplified())) { color = QColor(colorStr.simplified()); }
        else { return false; }
    }
    return true;
}

// '0' is 0x30 and '9' is 0x39
static bool isDigit(ushort ch) {
    static quint16 magic = 0x3ff;
    return ((ch >> 4) == 3) && (magic >> (ch & 15));
}

static qreal toDouble(const QChar *&str) {
    const int maxLen = 255;//technically doubles can go til 308+ but whatever
    char temp[maxLen+1];
    int pos = 0;

    if(*str == QLatin1Char('-')) {
        temp[pos++] = '-';
        ++str;
    } else if(*str == QLatin1Char('+')) {
        ++str;
    }
    while(isDigit(str->unicode()) && pos < maxLen) {
        temp[pos++] = str->toLatin1();
        ++str;
    }
    if(*str == QLatin1Char('.') && pos < maxLen) {
        temp[pos++] = '.';
        ++str;
    }
    while(isDigit(str->unicode()) && pos < maxLen) {
        temp[pos++] = str->toLatin1();
        ++str;
    }
    bool exponent = false;
    if((*str == QLatin1Char('e') || *str == QLatin1Char('E')) && pos < maxLen) {
        exponent = true;
        temp[pos++] = 'e';
        ++str;
        if((*str == QLatin1Char('-') || *str == QLatin1Char('+')) && pos < maxLen) {
            temp[pos++] = str->toLatin1();
            ++str;
        }
        while(isDigit(str->unicode()) && pos < maxLen) {
            temp[pos++] = str->toLatin1();
            ++str;
        }
    }

    temp[pos] = '\0';

    qreal val;
    if(!exponent && pos < 10) {
        int ival = 0;
        const char *t = temp;
        bool neg = false;
        if(*t == '-') {
            neg = true;
            ++t;
        }
        while(*t && *t != '.') {
            ival *= 10;
            ival += (*t) - '0';
            ++t;
        }
        if(*t == '.') {
            ++t;
            int div = 1;
            while(*t) {
                ival *= 10;
                ival += (*t) - '0';
                div *= 10;
                ++t;
            }
            val = static_cast<qreal>(ival)/div;
        } else {
            val = ival;
        }
        if(neg) val = -val;
    } else {
        val = QByteArray::fromRawData(temp, pos).toDouble();
    }
    return val;
}

static qreal toDouble(const QString &str, bool *ok = nullptr) {
    const QChar *c = str.constData();
    const qreal res = toDouble(c);
    if(ok) *ok = (*c == QLatin1Char('\0'));
    return res;
}

//static qreal toDouble(const QStringRef &str, bool *ok = nullptr) {
//    const QChar *c = str.constData();
//    qreal res = toDouble(c);
//    if(ok) {
//        *ok = (c == (str.constData() + str.length()));
//    }
//    return res;
//}

static void parseNumbersArray(const QChar *&str,
                              QVarLengthArray<float, 8> &points) {
    while(str->isSpace()) str++;
    while(isDigit(str->unicode()) ||
          *str == QLatin1Char('-') || *str == QLatin1Char('+') ||
          *str == QLatin1Char('.')) {

        points.append(toSkScalar(toDouble(str)));

        while(str->isSpace()) str++;
        if(*str == QLatin1Char(',')) str++;

        //eat the rest of space
        while(str->isSpace()) str++;
    }
}

bool parsePolylineData(const QString &dataStr,
                       VectorPathSvgAttributes &attributes,
                       const bool isPolygon) {
    float x0 = 0, y0 = 0;              // starting point
    float x = 0, y = 0;                // current point
    const QChar *str = dataStr.constData();
    const QChar *end = str + dataStr.size();

    SkPath& path = attributes.path();
    while (str != end) {
        while(str->isSpace()) ++str;
        QChar endc = *end;
        *const_cast<QChar *>(end) = 0; // parseNumbersArray requires 0-termination that QStringRef cannot guarantee
        QVarLengthArray<float, 8> arg;
        parseNumbersArray(str, arg);
        *const_cast<QChar *>(end) = endc;
        const float *num = arg.constData();
        int count = arg.count();
        bool first = true;
        while(count > 0) {
            x = num[0];
            y = num[1];
            num++;
            num++;
            count -= 2;
            if(count < 0) {
                if(qAbs(x - x0) < 0.001f &&
                   qAbs(y - y0) < 0.001f) {
                    path.close();
                    return true;
                }
            }
            if(first) {
                x0 = x;
                y0 = y;
                path.moveTo({x0, y0});
                first = false;
            } else {
                path.lineTo({x, y});
            }
        }
        if(isPolygon) path.close();
    }
    return true;
}

void loadElement(const QDomElement &element, ContainerBox *parentGroup,
                 const BoxSvgAttributes &parentGroupAttributes,
                 const GradientCreator& gradientCreator);

qsptr<ContainerBox> loadBoxesGroup(const QDomElement &groupElement,
                                   ContainerBox *parentGroup,
                                   const BoxSvgAttributes &attributes,
                                   const GradientCreator& gradientCreator) {
    const QDomNodeList allRootChildNodes = groupElement.childNodes();
    qsptr<ContainerBox> boxesGroup;
    const bool hasTransform = attributes.hasTransform();
    if(allRootChildNodes.count() > 1 || hasTransform || !parentGroup) {
        boxesGroup = enve::make_shared<ContainerBox>(eBoxType::group);
        boxesGroup->planCenterPivotPosition();
        attributes.apply(boxesGroup.get());
        if(parentGroup) parentGroup->addContained(boxesGroup);
    } else {
        boxesGroup = parentGroup->ref<ContainerBox>();
    }

    for(int i = 0; i < allRootChildNodes.count(); i++) {
        const QDomNode iNode = allRootChildNodes.at(i);
        if(iNode.isElement()) {
            loadElement(iNode.toElement(), boxesGroup.get(),
                        attributes, gradientCreator);
        }
    }
    return boxesGroup;
}

void loadVectorPath(const QDomElement &pathElement,
                    ContainerBox *parentGroup,
                    VectorPathSvgAttributes& attributes) {
    const QString pathStr = pathElement.attribute("d");
    SkParsePath::FromSVGString(pathStr.toStdString().data(), &attributes.path());
    if(attributes.isEmpty()) return;
    const auto vectorPath = enve::make_shared<SmartVectorPath>();
    vectorPath->planCenterPivotPosition();
    attributes.apply(vectorPath.get());
    parentGroup->addContained(vectorPath);
}

void loadPolyline(const QDomElement &pathElement,
                  ContainerBox *parentGroup,
                  VectorPathSvgAttributes &attributes,
                  const bool isPolygon) {
    const QString pathStr = pathElement.attribute("points");
    parsePolylineData(pathStr, attributes, isPolygon);
    if(attributes.isEmpty()) return;
    const auto vectorPath = enve::make_shared<SmartVectorPath>();
    vectorPath->planCenterPivotPosition();
    attributes.apply(vectorPath.get());
    parentGroup->addContained(vectorPath);
}

void loadCircle(const QDomElement &pathElement,
                ContainerBox *parentGroup,
                const BoxSvgAttributes &attributes) {

    const QString cXstr = pathElement.attribute("cx");
    const QString cYstr = pathElement.attribute("cy");
    const QString rStr = pathElement.attribute("r");
    const QString rXstr = pathElement.attribute("rx");
    const QString rYstr = pathElement.attribute("ry");

    qsptr<Circle> circle;
    double rX, rY;
    if(!rStr.isEmpty()) {
        rX = rStr.toDouble();
        rY = rX;
    } else if(!rXstr.isEmpty() && !rYstr.isEmpty()) {
        rX = rXstr.toDouble();
        rY = rYstr.toDouble();
    } else if(!rXstr.isEmpty() || !rYstr.isEmpty()) {
        const qreal rXY = rXstr.isEmpty() ? rYstr.toDouble() : rXstr.toDouble();
        rX = rXY;
        rY = rXY;
    } else return;
    if(isZero4Dec(rX) || isZero4Dec(rY)) return;
    circle = enve::make_shared<Circle>();
    circle->setHorizontalRadius(rX);
    circle->setVerticalRadius(rY);
    circle->setCenter(QPointF(cXstr.toDouble(), cYstr.toDouble()));
    circle->planCenterPivotPosition();

    attributes.apply(circle.data());
    parentGroup->addContained(circle);
}

void loadRect(const QDomElement &pathElement,
              ContainerBox *parentGroup,
              const BoxSvgAttributes &attributes) {

    const QString xStr = pathElement.attribute("x");
    const QString yStr = pathElement.attribute("y");
    const QString wStr = pathElement.attribute("width");
    const QString hStr = pathElement.attribute("height");
    const QString rYstr = pathElement.attribute("ry");
    const QString rXstr = pathElement.attribute("rx");

    const auto rect = enve::make_shared<RectangleBox>();
    rect->planCenterPivotPosition();

    const auto topLeft = QPointF(xStr.toDouble(), yStr.toDouble());
    rect->setTopLeftPos(topLeft);
    rect->setBottomRightPos(topLeft + QPointF(wStr.toDouble(), hStr.toDouble()));
    if(rYstr.isEmpty()) {
        rect->setYRadius(rXstr.toDouble());
        rect->setXRadius(rXstr.toDouble());
    } else if(rXstr.isEmpty()) {
        rect->setYRadius(rYstr.toDouble());
        rect->setXRadius(rYstr.toDouble());
    } else {
        rect->setYRadius(rYstr.toDouble());
        rect->setXRadius(rXstr.toDouble());
    }

    attributes.apply(rect.data());
    parentGroup->addContained(rect);
}

void loadLine(const QDomElement &pathElement,
                  ContainerBox *parentGroup,
                  VectorPathSvgAttributes &attributes) {

    const QString x1Str = pathElement.attribute("x1");
    const QString x2Str = pathElement.attribute("x2");
    const QString y1Str = pathElement.attribute("y1");
    const QString y2Str = pathElement.attribute("y2");

    SkPath& path = attributes.path();
    path.moveTo({toSkScalar(x1Str.toDouble()), toSkScalar(y1Str.toDouble())});
    path.lineTo({toSkScalar(x2Str.toDouble()), toSkScalar(y2Str.toDouble())});

    const auto vectorPath = enve::make_shared<SmartVectorPath>();
    vectorPath->planCenterPivotPosition();
    attributes.apply(vectorPath.get());
    parentGroup->addContained(vectorPath);
}

bool extractTranslation(const QString& str, QMatrix& target) {
    const QRegExp rx1(RGXS "translate\\(" REGEX_SINGLE_FLOAT "\\)" RGXS,
                      Qt::CaseInsensitive);
    if(rx1.exactMatch(str)) {
        rx1.indexIn(str);
        const QStringList capturedTxt = rx1.capturedTexts();
        target.translate(capturedTxt.at(1).toDouble(), 0);
        return true;
    }

    const QRegExp rx2(RGXS "translate\\(" REGEX_TWO_FLOATS "\\)" RGXS,
                      Qt::CaseInsensitive);
    if(rx2.exactMatch(str)) {
        rx2.indexIn(str);
        const QStringList capturedTxt = rx2.capturedTexts();
        target.translate(capturedTxt.at(1).toDouble(),
                         capturedTxt.at(2).toDouble());
        return true;
    }

    return false;
}


bool extractScale(const QString& str, QMatrix& target) {
    const QRegExp rx1(RGXS "scale\\(" REGEX_SINGLE_FLOAT "\\)" RGXS,
                      Qt::CaseInsensitive);
    if(rx1.exactMatch(str)) {
        rx1.indexIn(str);
        const QStringList capturedTxt = rx1.capturedTexts();
        const qreal scale = capturedTxt.at(1).toDouble();
        target.scale(scale, scale);
        return true;
    }

    const QRegExp rx2(RGXS "scale\\(" REGEX_TWO_FLOATS "\\)" RGXS,
                      Qt::CaseInsensitive);
    if(rx2.exactMatch(str)) {
        rx2.indexIn(str);
        const QStringList capturedTxt = rx2.capturedTexts();
        target.scale(capturedTxt.at(1).toDouble(),
                     capturedTxt.at(2).toDouble());
        return true;
    }

    return false;
}

bool extractRotate(const QString& str,
                   QMatrix& target)
{
    const QRegExp rxRotate("rotate\\s*\\(\\s*([^\\s,)]+)(?:[\\s,]+([^\\s,)]+)[\\s,]+([^\\s,)]+))?\\s*\\)",
                           Qt::CaseInsensitive);

    int pos = rxRotate.indexIn(str);
    if (pos != -1) {
        const QStringList captured = rxRotate.capturedTexts();
        bool ok;
        double angle = captured.at(1).toDouble(&ok);

        if (ok) {
            if (!captured.at(2).isEmpty() && !captured.at(3).isEmpty()) {
                double cx = captured.at(2).toDouble();
                double cy = captured.at(3).toDouble();

                target.translate(cx, cy);
                target.rotate(angle);
                target.translate(-cx, -cy);
            } else {
                target.rotate(angle);
            }
            return true;
        }
    }
    return false;
}

bool extractWholeMatrix(const QString& str, QMatrix& target) {
    const QRegExp rx(RGXS "matrix\\("
                         REGEX_FIRST_FLOAT
                         REGEX_INNER_FLOAT
                         REGEX_INNER_FLOAT
                         REGEX_INNER_FLOAT
                         REGEX_INNER_FLOAT
                         REGEX_LAST_FLOAT
                     "\\)" RGXS, Qt::CaseInsensitive);
    if(rx.exactMatch(str)) {
        rx.indexIn(str);
        const QStringList capturedTxt = rx.capturedTexts();
        target.setMatrix(capturedTxt.at(1).toDouble(),
                         capturedTxt.at(2).toDouble(),
                         capturedTxt.at(3).toDouble(),
                         capturedTxt.at(4).toDouble(),
                         capturedTxt.at(5).toDouble(),
                         capturedTxt.at(6).toDouble());
        return true;
    }
    return false;
}

QMatrix getMatrixFromString(const QString &str) {
    QMatrix matrix;
    if(str.isEmpty()) return matrix;
    const bool found = str.isEmpty() ||
                       extractWholeMatrix(str, matrix) ||
                       extractTranslation(str, matrix) ||
                       extractScale(str, matrix) ||
                       extractRotate(str, matrix);
    if(!found) qDebug() << "getMatrixFromString - could not extract "
                           "values from string:" << str;
    return matrix;
}

static QMap<QString, SvgGradient> gGradients;
//            to       from
static QMap<QString, QStringList> gUnresolvedGradientLinks;

void applyGradientToAttributes(const QDomElement &element,
                               BoxSvgAttributes &attributes,
                               const GradientCreator& gradientCreator,
                               const QPointF &offset = QPointF(0,0),
                               bool isFill = true)
{
    QString gradId;
    QString attrName = isFill ? "fill" : "stroke";
    QString attrVal = element.attribute(attrName).trimmed();

    if (attrVal.isEmpty() && element.hasAttribute("style")) {
        QString style = element.attribute("style");
        int idx = style.indexOf(attrName + ":url(#");
        if (idx != -1) {
            int start = style.indexOf('#', idx) + 1;
            int end = style.indexOf(')', start);
            gradId = style.mid(start, end - start);
        }
    } else if (attrVal.startsWith("url(#")) {
        gradId = attrVal.mid(5, attrVal.lastIndexOf(')') - 5);
    }

    gradId = gradId.remove("'").remove("\"").trimmed();

    if (!gradId.isEmpty() && gGradients.contains(gradId)) {
        const SvgGradient &templateGrad = gGradients[gradId];

        QMatrix correction;
        if (!offset.isNull()) {
            correction.translate(-offset.x(), -offset.y());
        }

        QPointF p1 = correction.map(QPointF(templateGrad.fX1,
                                            templateGrad.fY1));
        QPointF p2 = correction.map(QPointF(templateGrad.fX2,
                                            templateGrad.fY2));

        QMatrix adjustedTrans = templateGrad.fTrans;
        if (!offset.isNull()) {
            QMatrix invCorrection;
            invCorrection.translate(offset.x(), offset.y());
            adjustedTrans = invCorrection * templateGrad.fTrans * correction;
        }

        Gradient* newGradInstance = gradientCreator();
        qreal opacity = 1.0;
        QString opAttr = isFill ? "fill-opacity" : "stroke-opacity";
        if (element.hasAttribute(opAttr)) { opacity = element.attribute(opAttr).toDouble(); }

        for (const QGradientStop &stop : templateGrad.fGradient->getQGradientStops()) {
            QColor c = stop.second;
            c.setAlphaF(c.alphaF() * opacity);
            newGradInstance->addColor(c);
        }
        newGradInstance->updateQGradientStops();

        SvgGradient instance = {
            newGradInstance,
            p1.x(), p1.y(), p2.x(), p2.y(),
            adjustedTrans, templateGrad.fType
        };

        if (isFill) {
            auto& fill = const_cast<FillSvgAttributes&>(attributes.getFillAttributes());
            fill.setGradient(instance);
            fill.setPaintType(GRADIENTPAINT);
        } else {
            auto& stroke = const_cast<StrokeSvgAttributes&>(attributes.getStrokeAttributes());
            stroke.setGradient(instance);
            stroke.setPaintType(GRADIENTPAINT);
        }

        qDebug() << "--- Gradient Debug ---";
        qDebug() << "ID:" << gradId << "Offset:" << offset;
        qDebug() << "Raw P1:" << QPointF(templateGrad.fX1, templateGrad.fY1);
        qDebug() << "Final Matrix Translation:" << adjustedTrans.dx() << adjustedTrans.dy();
    }
}


void loadText(const QDomElement &textElement,
              ContainerBox *parentGroup,
              const BoxSvgAttributes &attributes,
              const GradientCreator& gradientCreator)
{
    // inkscape?
    bool hasInkscapeLines = false;
    QDomNodeList tspans = textElement.elementsByTagName("tspan");
    for (int i = 0; i < tspans.count(); i++) {
        if (tspans.at(i).toElement().attribute("sodipodi:role") == "line") {
            hasInkscapeLines = true;
            break;
        }
    }

    if (hasInkscapeLines) {
        for (int i = 0; i < tspans.count(); i++) {
            QDomElement tspan = tspans.at(i).toElement();
            QString content = tspan.text();
            if (content.trimmed().isEmpty()) { continue; }

            BoxSvgAttributes lineAttributes = attributes;
            lineAttributes.loadBoundingBoxAttributes(tspan);

            QString xStr = tspan.attribute("x", textElement.attribute("x"))
                               .split(QRegularExpression("\\s+")).first().remove("px");
            QString yStr = tspan.attribute("y", textElement.attribute("y"))
                               .split(QRegularExpression("\\s+")).first().remove("px");
            QPointF pos(xStr.toDouble(), yStr.toDouble());

            applyGradientToAttributes(tspan,
                                      lineAttributes,
                                      gradientCreator,
                                      pos,
                                      true); // fill
            applyGradientToAttributes(tspan,
                                      lineAttributes,
                                      gradientCreator,
                                      pos,
                                      false); // stroke

            const auto textBox = enve::make_shared<TextBox>();
            textBox->planCenterPivotPosition();
            textBox->moveByRel(pos);
            textBox->setCurrentValue(content);

            lineAttributes.apply(textBox.data());
            parentGroup->addContained(textBox);
        }
    } else {
        QString combinedText;
        for (QDomNode n = textElement.firstChild(); !n.isNull(); n = n.nextSibling()) {
            if (n.isText()) { combinedText += n.toText().data(); }
            else if (n.isElement() && n.toElement().tagName() == "tspan") {
                QDomElement tspan = n.toElement();
                // Boxy uses tspan for new line
                if (tspan.hasAttribute("dy") && !combinedText.isEmpty()) {
                    combinedText += "\n";
                }
                if (!tspan.text().trimmed().isEmpty()) {
                    combinedText += tspan.text();
                }
            }
        }

        combinedText.remove(QChar(0x200B)); // Boxy

        if (combinedText.trimmed().isEmpty()) { return; }

        QString xStr = textElement.attribute("x")
                           .split(QRegularExpression("\\s+")).first().remove("px");
        QString yStr = textElement.attribute("y")
                           .split(QRegularExpression("\\s+")).first().remove("px");
        QPointF pos(xStr.toDouble(), yStr.toDouble());

        BoxSvgAttributes textAttributes = attributes;

        applyGradientToAttributes(textElement,
                                  textAttributes,
                                  gradientCreator,
                                  pos, true); // fill
        applyGradientToAttributes(textElement,
                                  textAttributes,
                                  gradientCreator,
                                  pos, false); // stroke

        const auto textBox = enve::make_shared<TextBox>();
        textBox->planCenterPivotPosition();
        textBox->moveByRel(pos);
        textBox->setCurrentValue(combinedText);

        textAttributes.apply(textBox.data());
        parentGroup->addContained(textBox);
    }
}

void loadElement(const QDomElement &element,
                 ContainerBox *parentGroup,
                 const BoxSvgAttributes &parentGroupAttributes,
                 const GradientCreator& gradientCreator)
{
    const QString tagName = element.tagName();
    if ((tagName == "linearGradient" || tagName == "radialGradient") &&
        gGradients.contains(element.attribute("id"))) {
        qDebug() << "gradient already added, ignore" << tagName;
        return;
    }

    if(tagName == "defs") {
        const QDomNodeList allRootChildNodes = element.childNodes();
        for(int i = 0; i < allRootChildNodes.count(); i++) {
            const QDomNode iNode = allRootChildNodes.at(i);
            if(iNode.isElement()) {
                loadElement(iNode.toElement(), parentGroup,
                            parentGroupAttributes, gradientCreator);
            }
        }
        return;
    } else if(tagName == "linearGradient" || tagName == "radialGradient") {
        GradientType type;
        if(tagName == "linearGradient") {
            type = GradientType::LINEAR;
        } else { //if(tagName == "radialGradient") {
            type = GradientType::RADIAL;
        }
        const QString id = element.attribute("id");
        qDebug() << "found gradient id" << id;
        QString linkId = element.attribute("xlink:href");
        Gradient* gradient = nullptr;
        if(linkId.isEmpty()) {
            gradient = gradientCreator();
            const QDomNodeList allRootChildNodes = element.childNodes();
            for(int i = 0; i < allRootChildNodes.count(); i++) {
                const QDomNode iNode = allRootChildNodes.at(i);
                if(!iNode.isElement()) continue;
                const QDomElement elem = iNode.toElement();
                if(elem.tagName() != "stop") continue;
                QString stopColorS;
                QString stopOpacityS;
                const QString stopStyle = elem.attribute("style");
                QList<SvgAttribute> attributesList;
                extractSvgAttributes(stopStyle, &attributesList);
                for(const auto& attr : attributesList) {
                    if(attr.fName == "stop-color") {
                        stopColorS = attr.fValue;

                    } else if(attr.fName == "stop-opacity") {
                        stopOpacityS = attr.fValue;
                    }
                }
                if(stopColorS.isEmpty()) {
                    stopColorS = elem.attribute("stop-color");
                }
                if(stopOpacityS.isEmpty()) {
                    stopOpacityS = elem.attribute("stop-opacity");
                }

                QColor stopColor;
                toColor(stopColorS, stopColor);
                if(!stopOpacityS.isEmpty()) {
                    stopColor.setAlphaF(toDouble(stopOpacityS));
                }

                gradient->addColor(stopColor);
            }
        } else {
            if(linkId.at(0) == "#") linkId.remove(0, 1);
            const auto it = gGradients.find(linkId);
            if(it == gGradients.end()) {
                gUnresolvedGradientLinks[linkId].append(id);
                gradient = nullptr;
            } else {
                gradient = it.value().fGradient;
            }
        }
        const auto it = gUnresolvedGradientLinks.find(id);
        if(it != gUnresolvedGradientLinks.end()) {
            if(gradient) {
                for(const auto& linking : it.value()) {
                    auto& grad = gGradients[linking];
                    grad.fGradient = gradient;
                    grad.fType = type;
                }
            } else {
                gUnresolvedGradientLinks[linkId] = it.value();
            }
        }

        switch(type) {
        case GradientType::LINEAR:
        case GradientType::RADIAL:
        {
            QDomElement svgRoot = element.ownerDocument().documentElement();
            QStringList viewBox = svgRoot.attribute("viewBox").split(QRegularExpression("\\s+"),
                                                                     Qt::SkipEmptyParts);
            qreal viewW = 1.0;
            qreal viewH = 1.0;

            if (viewBox.size() >= 4) {
                viewW = viewBox.at(2).toDouble();
                viewH = viewBox.at(3).toDouble();
            } else {
                viewW = svgRoot.attribute("width", "1").toDouble();
                viewH = svgRoot.attribute("height", "1").toDouble();
            }
            if (viewW <= 0) { viewW = 1.0; }
            if (viewH <= 0) { viewH = 1.0; }

            QPointF p1, p2;
            const QString units = element.attribute("gradientUnits");
            const QString gradTrans = element.attribute("gradientTransform");
            QMatrix trans = getMatrixFromString(gradTrans);

            if (units == "userSpaceOnUse") {
                if (type == GradientType::LINEAR) {
                    p1 = QPointF(element.attribute("x1", "0").toDouble(),
                                 element.attribute("y1", "0").toDouble());
                    p2 = QPointF(element.attribute("x2", "1").toDouble(),
                                 element.attribute("y2", "1").toDouble());
                } else {
                    qreal cx = element.attribute("cx", "0").toDouble();
                    qreal cy = element.attribute("cy", "0").toDouble();
                    qreal r  = element.attribute("r", "1").toDouble();
                    p1 = QPointF(cx, cy);
                    p2 = QPointF(cx + r, cy);
                }
            } else {
                // objectBoundingBox:
                if (type == GradientType::LINEAR) {
                    p1 = QPointF(parseSvgUnit(element.attribute("x1"), viewW),
                                 parseSvgUnit(element.attribute("y1"), viewH));
                    p2 = QPointF(parseSvgUnit(element.attribute("x2"), viewW),
                                 parseSvgUnit(element.attribute("y2"), viewH));
                } else {
                    const qreal cx = parseSvgUnit(element.attribute("cx"), viewW);
                    const qreal cy = parseSvgUnit(element.attribute("cy"), viewH);
                    const qreal r = parseSvgUnit(element.attribute("r"),
                                                 (viewW + viewH) * 0.5);
                    p1 = QPointF(cx, cy);
                    p2 = QPointF(cx + r, cy + r);
                }

                if (!gradTrans.isEmpty()) {
                    p1 = QPointF(p1.x() / viewW, p1.y() / viewH);
                    p2 = QPointF(p2.x() / viewW, p2.y() / viewH);
                    p1 = trans.map(p1);
                    p2 = trans.map(p2);
                    p1 = QPointF(p1.x() * viewW, p1.y() * viewH);
                    p2 = QPointF(p2.x() * viewW, p2.y() * viewH);

                    trans = QMatrix();
                }
            }
            gGradients.insert(id, {gradient,
                                   p1.x(), p1.y(),
                                   p2.x(), p2.y(),
                                   trans, type});
            break;
        }
        }
    } else if(tagName == "path" || tagName == "polyline" || tagName == "polygon" || tagName == "line") {
        VectorPathSvgAttributes attributes;
        attributes.setParent(parentGroupAttributes);
        attributes.loadBoundingBoxAttributes(element);
        applyGradientToAttributes(element, attributes, gradientCreator);
        if(tagName == "path") {
            loadVectorPath(element, parentGroup, attributes);
        } else if(tagName == "polyline") {
            loadPolyline(element, parentGroup, attributes, false);
        } else if(tagName == "polygon") {
            loadPolyline(element, parentGroup, attributes, true);
        } else if(tagName == "line") {
            loadLine(element, parentGroup, attributes);
        }
    } else if(tagName == "g" || tagName == "text" ||
              tagName == "circle" || tagName == "ellipse" ||
              tagName == "rect") {
        BoxSvgAttributes attributes;
        attributes.setParent(parentGroupAttributes);
        attributes.loadBoundingBoxAttributes(element);
        applyGradientToAttributes(element, attributes, gradientCreator);
        if(tagName == "g") {
            const auto group = loadBoxesGroup(element, parentGroup,
                                              attributes, gradientCreator);
            if(group->getContainedBoxesCount() == 0)
                group->removeFromParent_k();
        } else if(tagName == "circle" || tagName == "ellipse") {
            loadCircle(element, parentGroup, attributes);
        } else if(tagName == "rect") {
            loadRect(element, parentGroup, attributes);
        } else if(tagName == "text") {
            loadText(element, parentGroup, attributes, gradientCreator);
        }
    } else qDebug() << "Unrecognized tagName \"" + tagName + "\"";
}

bool getUrlId(const QString &urlStr, QString *id) {
    const QRegExp rx = QRegExp(RGXS "url\\(\\s*#(.*)\\)" RGXS, Qt::CaseInsensitive);
    if(rx.exactMatch(urlStr)) {
        rx.indexIn(urlStr);
        const QStringList capturedTxt = rx.capturedTexts();
        *id = capturedTxt.at(1);
        return true;
    }

    return false;
}

bool getGradientFromString(const QString &colorStr,
                           FillSvgAttributes * const target) {
    const QRegExp rx = QRegExp(RGXS "url\\(\\s*(.*)\\s*\\)" RGXS, Qt::CaseInsensitive);
    if(rx.exactMatch(colorStr)) {
        const QStringList capturedTxt = rx.capturedTexts();
        QString id = capturedTxt.at(1);
        if(id.at(0) == '#') id.remove(0, 1);
        const auto it = gGradients.find(id);
        if(it != gGradients.end()) {
            target->setGradient(it.value());
            return true;
        }
    }
    return false;
}

bool getFlatColorFromString(const QString &colorStr, FillSvgAttributes *target) {
    QColor color(colorStr);
    if(!color.isValid()) {
        if(!toColor(colorStr, color)) return false;
    }
    target->setColor(color);
    target->setPaintType(FLATPAINT);
    return true;
}

qsptr<BoundingBox> ImportSVG::loadSVGFile(const QDomDocument& src,
                                          const GradientCreator& gradientCreator)
{
    const QDomElement rootElement = src.firstChildElement("svg");
    if(rootElement.isNull()) RuntimeThrow("File does not have svg root element");

    // Pre-load gradients
    QDomNodeList gradients = src.elementsByTagName("linearGradient");
    qDebug() << "found linearGradients:" << gradients.count();
    for (int i = 0; i < gradients.count(); i++) {
        loadElement(gradients.at(i).toElement(),
                    nullptr,
                    BoxSvgAttributes(),
                    gradientCreator);
    }
    gradients = src.elementsByTagName("radialGradient");
    qDebug() << "found radialGradients:" << gradients.count();
    for (int i = 0; i < gradients.count(); i++) {
        loadElement(gradients.at(i).toElement(),
                    nullptr,
                    BoxSvgAttributes(),
                    gradientCreator);
    }

    BoxSvgAttributes attributes;
    const auto result = loadBoxesGroup(rootElement, nullptr,
                                       attributes, gradientCreator);
    gGradients.clear();
    auto it = gUnresolvedGradientLinks.begin();
    while(it != gUnresolvedGradientLinks.end()) {
        qDebug() << "unresolved gradient links to " + it.key() + ":";
        qDebug() << it.value().join(", ");
        it++;
    }
    gUnresolvedGradientLinks.clear();
    if(result->getContainedBoxesCount() == 1) {
        return qSharedPointerCast<BoundingBox>(
                    result->takeContained_k(0));
    } else if(result->getContainedBoxesCount() == 0) {
        return nullptr;
    }
    return result;
}

qsptr<BoundingBox> ImportSVG::loadSVGFile(
        const QByteArray& src,
        const GradientCreator& gradientCreator) {
    QDomDocument document;
    if(!document.setContent(src))
        RuntimeThrow("Cannot set file as QDomDocument content");
    return loadSVGFile(document, gradientCreator);
}

qsptr<BoundingBox> ImportSVG::loadSVGFile(
        QIODevice* const src,
        const GradientCreator& gradientCreator) {
    QDomDocument document;
    if(!document.setContent(src))
        RuntimeThrow("Cannot set file as QDomDocument content");
    return loadSVGFile(document, gradientCreator);
}

qsptr<BoundingBox> ImportSVG::loadSVGFile(
        const QString &filename,
        const GradientCreator& gradientCreator) {
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        RuntimeThrow("Cannot open file " + filename);
    return loadSVGFile(&file, gradientCreator);
}

void BoxSvgAttributes::setParent(const BoxSvgAttributes &parent) {
    mFillAttributes = parent.getFillAttributes();
    mStrokeAttributes = parent.getStrokeAttributes();
    mTextAttributes = parent.getTextAttributes();
    mFillRule = parent.getFillRule();
}

SkPathFillType BoxSvgAttributes::getFillRule() const {
    return mFillRule;
}

const QMatrix &BoxSvgAttributes::getRelTransform() const {
    return mRelTransform;
}

const FillSvgAttributes &BoxSvgAttributes::getFillAttributes() const {
    return mFillAttributes;
}

const StrokeSvgAttributes &BoxSvgAttributes::getStrokeAttributes() const {
    return mStrokeAttributes;
}

const TextSvgAttributes &BoxSvgAttributes::getTextAttributes() const {
    return mTextAttributes;
}

void BoxSvgAttributes::setFillAttribute(const QString &value) {
    if(value.contains("none")) {
        mFillAttributes.setPaintType(NOPAINT);
    } else if(getFlatColorFromString(value, &mFillAttributes)) {
    } else if (value.contains("url(#")) {
    } else if(getGradientFromString(value, &mFillAttributes)) {
    } else {
        qDebug() << "setFillAttribute - format not recognised:" << value;
    }
}

void BoxSvgAttributes::setStrokeAttribute(const QString &value) {
    if(value.contains("none")) {
        mStrokeAttributes.setPaintType(NOPAINT);
    } else if(getFlatColorFromString(value, &mStrokeAttributes)) {
    } else if(getGradientFromString(value, &mStrokeAttributes)) {
    } else {
        qDebug() << "setStrokeAttribute - format not recognised:" << value;
    }
}

QString stripPx(const QString& val) {
    QString res = val;
    return res.remove("px").remove("pt").remove("em").trimmed();
}

void BoxSvgAttributes::loadBoundingBoxAttributes(const QDomElement &element) {
    QList<SvgAttribute> styleAttributes;
    const QString styleAttributesStr = element.attribute("style");
    extractSvgAttributes(styleAttributesStr, &styleAttributes);
    for(const SvgAttribute &attribute : styleAttributes) {
        const QString name = attribute.getName();
        if(name.isEmpty()) continue;
        const QString value = attribute.getValue();
        if(value == "inherit") continue;
        switch (name.at(0).unicode()) {
        case 'c':
            if(name == "color") {
                mFillAttributes.setPaintType(FLATPAINT);
                QColor color = mFillAttributes.getColor();
                color.setAlphaF(toDouble(value));
                mFillAttributes.setColor(color);
            } else if(name == "color-opacity") {
                QColor color = mFillAttributes.getColor();
                color.setAlphaF(toDouble(value));
                mFillAttributes.setColor(color);
            } else if(name == "comp-op") {
                //compOp = value;
            }
            break;

        case 'd':
            if(name == "display") {
                //display = value;
            }
            break;

        case 'f':
            if(name == "fill") {
                setFillAttribute(value);
            } else if(name == "fill-rule") {
                if(value == "nonzero") {
                    mFillRule = SkPathFillType::kWinding;
                } else { // "evenodd"
                    mFillRule = SkPathFillType::kEvenOdd;
                }
            } else if(name == "fill-opacity") {
                mFillAttributes.setColorOpacity(toDouble(value));
            } else if(name == "font-family") {
                QString stripQuotes = value;
                stripQuotes.remove("'");
                mTextAttributes.setFontFamily(stripQuotes);
            } else if(name == "font-size") {
                mTextAttributes.setFontSize(qRound(stripPx(value).toDouble()));
            } else if(name == "font-style") {
                if(value == "normal") {
                    mTextAttributes.setFontSlant(SkFontStyle::kUpright_Slant);
                } else if(value == "italic") {
                    mTextAttributes.setFontSlant(SkFontStyle::kItalic_Slant);
                } else if(value == "oblique") {
                    mTextAttributes.setFontSlant(SkFontStyle::kOblique_Slant);
                }
            } else if(name == "font-weight") {
                if(value == "normal") {
                    mTextAttributes.setFontWeight(SkFontStyle::kNormal_Weight);
                } else if(value == "bold") {
                    mTextAttributes.setFontWeight(SkFontStyle::kBold_Weight);
                } else if(value == "bolder") {
                    mTextAttributes.bolderFontWeight();
                } else if(value == "lighter") {
                    mTextAttributes.lighterFontWeight();
                } else {
                    bool ok;
                    const int val = value.toInt(&ok);
                    if(ok) mTextAttributes.setFontWeight(val);
                    else qDebug() << "Unrecognized font-weight '" + value + "'";
                }
            } else if(name == "font-variant") {
                //fontVariant = value;
            }
            break;

        case 'i':
            if(name == "id") mId = value;
            break;
        case 'o':
            if(name == "opacity") {
                mOpacity = toDouble(value)*100.;
            } else if(name == "offset") {
                //offset = value;
            }
            break;

        case 's':
            if(name.contains("stroke")) {
                if(name == "stroke") {
                    setStrokeAttribute(value);
                } else if(name == "stroke-dasharray") {
                    //strokeDashArray = value;
                } else if(name == "stroke-dashoffset") {
                    //strokeDashOffset = value;
                } else if(name == "stroke-linecap") {
                    if(value == "butt") {
                        mStrokeAttributes.setCapStyle(SkPaint::kButt_Cap);
                    } else if(value == "round") {
                        mStrokeAttributes.setCapStyle(SkPaint::kRound_Cap);
                    } else {
                        mStrokeAttributes.setCapStyle(SkPaint::kSquare_Cap);
                    }
                } else if(name == "stroke-linejoin") {
                    if(value == "miter") {
                        mStrokeAttributes.setJoinStyle(SkPaint::kMiter_Join);
                    } else if(value == "round") {
                        mStrokeAttributes.setJoinStyle(SkPaint::kRound_Join);
                    } else {
                        mStrokeAttributes.setJoinStyle(SkPaint::kBevel_Join);
                    }
                } else if(name == "stroke-miterlimit") {
                    //mStrokeAttributes.setMiterLimit(toDouble(value));
                } else if(name == "stroke-opacity") {
                    mStrokeAttributes.setColorOpacity(toDouble(value));
                } else if(name == "stroke-width") {
                    if(value.contains("%")) {
                        mStrokeAttributes.setLineWidth(
                                    mStrokeAttributes.getLineWidth()*
                                    value.toDouble()/100.);
                    } else {
                        mStrokeAttributes.setLineWidth(
                                    stripPx(value).toDouble());
                    }
                }
            } else if(name == "stop-color") {
                //stopColor = value;
            } else if(name == "stop-opacity") {
                //stopOpacity = value;
            }
            break;
        case 't':
            if(name == "text-anchor") {
                if(value == "end") {
                    mTextAttributes.setFontAlignment(Qt::AlignLeft);
                } else if(value == "middle") {
                    mTextAttributes.setFontAlignment(Qt::AlignHCenter);
                } else if(value == "start") {
                    mTextAttributes.setFontAlignment(Qt::AlignRight);
                }
            } else if(name == "transform") {
                mRelTransform = getMatrixFromString(value)*mRelTransform;
            }
            break;

        case 'v':
            if(name == "vector-effect") {
                //vectorEffect = value;
            } else if(name == "visibility") {
                //visibility = value;
            }
            break;

        case 'x':
            if(name == "xml:id") {
                mId = value;
            }
            break;

        default:
            break;
        }
    }

    mId = element.attribute("id", mId);
    mLabel = element.attribute("inkscape:label", mLabel);

    const QString fillAttributesStr = element.attribute("fill");
    if(!fillAttributesStr.isEmpty()) setFillAttribute(fillAttributesStr);

    const QString fillOp = element.attribute("fill-opacity");
    if(!fillOp.isEmpty()) mFillAttributes.setColorOpacity(toDouble(fillOp));

    const QString strokeAttributesStr = element.attribute("stroke");
    if(!strokeAttributesStr.isEmpty()) setStrokeAttribute(strokeAttributesStr);

    const QString strokeOp = element.attribute("stroke-opacity");
    if(!strokeOp.isEmpty()) mFillAttributes.setColorOpacity(toDouble(strokeOp));

    const QString strokeWidth = element.attribute("stroke-width").simplified();
    if (mStrokeAttributes.getPaintType() != NOPAINT) {
        if (strokeWidth.isEmpty() || strokeWidth.contains("%")) {
            if (mStrokeAttributes.getLineWidth() < 1.0) {
                mStrokeAttributes.setLineWidth(1.0); // spec says 1 as default
            }
            // TODO: A percentage value is always computed as a percentage of the normalized viewBox diagonal length.
        } else {
            mStrokeAttributes.setLineWidth(stripPx(strokeWidth).simplified().toDouble());
        }
    }

    const QString matrixStr = element.attribute("transform");
//    const QString transCenterX = element.attribute("inkscape:transform-center-x");
//    const QString transCenterY = element.attribute("inkscape:transform-center-y");
//    QPointF transCenter;
//    if(!transCenterX.isEmpty()) transCenter.setX(toDouble(transCenterX));
//    if(!transCenterY.isEmpty()) transCenter.setY(toDouble(transCenterY));
    if(!matrixStr.isEmpty()) {
        mRelTransform = getMatrixFromString(matrixStr)*mRelTransform;
    }

    mDecomposedTrans = MatrixDecomposition::decompose(mRelTransform);
}

bool BoxSvgAttributes::hasTransform() const {
    return !(isZero4Dec(mRelTransform.dx()) &&
             isZero4Dec(mRelTransform.dy()) &&
             isZero4Dec(mRelTransform.m11() - 1) &&
             isZero4Dec(mRelTransform.m22() - 1) &&
             isZero4Dec(mRelTransform.m12()) &&
             isZero4Dec(mRelTransform.m21()));
}

#include "Animators/paintsettingsanimator.h"

void FillSvgAttributes::setColor(const QColor &val) {
    mColor = val;
    mColor.setAlphaF(val.alphaF()*mOpacity);
    setPaintType(FLATPAINT);
}

void FillSvgAttributes::setColorOpacity(const qreal opacity) {
    mColor.setAlphaF(opacity);
    mOpacity = opacity;
}

void FillSvgAttributes::setPaintType(const PaintType type) {
    mPaintType = type;
}

void FillSvgAttributes::setGradient(const SvgGradient& gradient) {
    mGradientType = gradient.fType;
    mGradient = gradient.fGradient;
    mGradientP1 = gradient.fTrans.map(QPointF{gradient.fX1, gradient.fY1});
    mGradientP2 = gradient.fTrans.map(QPointF{gradient.fX2, gradient.fY2});
    mGradientTransform = gradient.fTrans;
    if(!mGradient) return;
    setPaintType(GRADIENTPAINT);
}

const QColor &FillSvgAttributes::getColor() const { return mColor; }

PaintType FillSvgAttributes::getPaintType() const { return mPaintType; }

Gradient *FillSvgAttributes::getGradient() const { return mGradient; }

void FillSvgAttributes::apply(BoundingBox *box) const {
    apply(box, PaintSetting::FILL);
}

void FillSvgAttributes::apply(BoundingBox * const box,
                              const PaintSetting::Target& target) const {
    const auto pathBox = enve_cast<PathBox*>(box);
    if(!pathBox) return;
    if(mPaintType == FLATPAINT) {
        ColorSetting colorSetting(ColorMode::rgb, ColorParameter::all,
                                  mColor.redF(), mColor.greenF(),
                                  mColor.blueF(), mColor.alphaF(),
                                  ColorSettingType::change);
        ColorPaintSetting(target, colorSetting).apply(pathBox);
    } else if(mPaintType == GRADIENTPAINT) {
        GradientPaintSetting(target, mGradient).apply(pathBox);
        GradientTypePaintSetting(target, mGradientType).apply(pathBox);
        GradientPtsPosSetting(target, mGradientP1, mGradientP2).apply(pathBox);
        GradientTransformSetting(target, mGradientTransform).apply(pathBox);
    }
    PaintTypePaintSetting(target, mPaintType).apply(pathBox);
}

qreal StrokeSvgAttributes::getLineWidth() const {
    return mLineWidth;
}

SkPaint::Cap StrokeSvgAttributes::getCapStyle() const {
    return mCapStyle;
}

SkPaint::Join StrokeSvgAttributes::getJoinStyle() const {
    return mJoinStyle;
}

QPainter::CompositionMode StrokeSvgAttributes::getOutlineCompositionMode() const {
    return mOutlineCompositionMode;
}

void StrokeSvgAttributes::setLineWidth(const qreal val) {
    mLineWidth = val;
}

void StrokeSvgAttributes::setCapStyle(const SkPaint::Cap capStyle) {
    mCapStyle = capStyle;
}

void StrokeSvgAttributes::setJoinStyle(const SkPaint::Join joinStyle) {
    mJoinStyle = joinStyle;
}

void StrokeSvgAttributes::setOutlineCompositionMode(const QPainter::CompositionMode compMode) {
    mOutlineCompositionMode = compMode;
}

void StrokeSvgAttributes::apply(BoundingBox *box, const qreal scale) const {
    box->strokeWidthAction(QrealAction::sMakeSet(mLineWidth*scale));
    box->setStrokeJoinStyle(mJoinStyle);
    box->setStrokeCapStyle(mCapStyle);
    FillSvgAttributes::apply(box, PaintSetting::OUTLINE);
    //box->setStrokePaintType(mPaintType, mColor, mGradient);
}

void BoxSvgAttributes::apply(BoundingBox *box) const
{
    if (!mLabel.isEmpty()) { box->prp_setName(mLabel); }
    else if (!mId.isEmpty()) { box->prp_setName(mId); }

    if (const auto path = enve_cast<PathBox*>(box)) {
        const qreal m11 = mRelTransform.m11();
        const qreal m12 = mRelTransform.m12();
        const qreal m21 = mRelTransform.m21();
        const qreal m22 = mRelTransform.m22();

        const qreal sxAbs = qSqrt(m11*m11 + m21*m21);
        const qreal syAbs = qSqrt(m12*m12 + m22*m22);
        mStrokeAttributes.apply(path, (sxAbs + syAbs)*0.5);
        mFillAttributes.apply(path);
        if (const auto text = enve_cast<TextBox*>(box)) {
            text->setFont(mTextAttributes.getFont());
        }
    }
    const auto transAnim = box->getBoxTransformAnimator();
    transAnim->setOpacity(mOpacity);
    transAnim->translate(mDecomposedTrans.fMoveX, mDecomposedTrans.fMoveY);
    transAnim->setScale(mDecomposedTrans.fScaleX, mDecomposedTrans.fScaleY);
    transAnim->setRotation(mDecomposedTrans.fRotation);
    transAnim->setShear(mDecomposedTrans.fShearX, mDecomposedTrans.fShearY);
}

void VectorPathSvgAttributes::apply(SmartVectorPath * const path) {
    SmartPathCollection* const pathAnimator = path->getPathAnimator();
    pathAnimator->loadSkPath(mPath);
    pathAnimator->setFillType(mFillRule);
    BoxSvgAttributes::apply(path);
}

void TextSvgAttributes::setFontFamily(const QString &family) {
    mFamily = family;
}

void TextSvgAttributes::setFontSize(const int size) {
    mSize = size > 0 ? size : 1;
}

void TextSvgAttributes::setFontSlant(const SkFontStyle::Slant &slant) {
    mSlant = slant;
}

void TextSvgAttributes::setFontWeight(const int weight) {
    mWeight = weight;
}

void TextSvgAttributes::setFontAlignment(const Qt::Alignment &alignment) {
    mAlignment = alignment;
}
