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

#include "textboxrenderdata.h"
#include "textbox.h"
#include "PathEffects/patheffectstask.h"
#include "canvas.h"

qreal textLineX(const Qt::Alignment &alignment,
                const qreal lineWidth,
                const qreal maxWidth) {
    if(alignment == Qt::AlignCenter) {
        return (maxWidth - lineWidth)*0.5;
    } else if(alignment == Qt::AlignLeft) {
        return 0;
    } else {// if(alignment == Qt::AlignRight) {
        return maxWidth - lineWidth;
    }
}

qreal horizontalAdvance(const SkFont& font, const QString& str) {
    const SkScalar result = font.measureText(str.utf16(),
                                             str.size()*sizeof(short),
                                             SkTextEncoding::kUTF16);
    return static_cast<qreal>(result);
}

qreal horizontalAdvance(const SkFont& font, const QString& str,
                        const qreal letterSpacing) {
    SkScalar result = font.measureText(str.utf16(),
                                       str.size()*sizeof(short),
                                       SkTextEncoding::kUTF16);
    const qreal fontSize = static_cast<qreal>(font.getSize());
    result += static_cast<SkScalar>(fontSize*letterSpacing*str.length());
    return static_cast<qreal>(result);
}

qreal horizontalAdvance(const SkFont& font, const QString& str,
                        const qreal letterSpacing, const qreal wordSpacing) {
    SkScalar result = font.measureText(str.utf16(),
                                       str.size()*sizeof(short),
                                       SkTextEncoding::kUTF16);
    const qreal fontSize = static_cast<qreal>(font.getSize());
    const int nSpaces = str.count(" ");
    if(nSpaces > 0) {
        const char spaceChar = ' ';
        const SkScalar space = font.measureText(
                    &spaceChar, sizeof(char),
                    SkTextEncoding::kUTF8);
        result += nSpaces*space*static_cast<SkScalar>(wordSpacing - 1);
    }
    const int nNonSpaces = str.length() - nSpaces;
    result += static_cast<SkScalar>(fontSize*letterSpacing*nNonSpaces);
    return static_cast<qreal>(result);
}

LetterRenderData::LetterRenderData(TextBox * const parent) :
    PathBoxRenderData(parent) {
    fParentIsTarget = false;
}

void LetterRenderData::afterQued() {
    if(!fPathEffects.isEmpty() || !fFillEffects.isEmpty() ||
       !fOutlineBaseEffects.isEmpty() || !fOutlineEffects.isEmpty()) {
        const auto pathTask = enve::make_shared<PathEffectsTask>(
                    this, std::move(fPathEffects), std::move(fFillEffects),
                    std::move(fOutlineBaseEffects), std::move(fOutlineEffects));
        pathTask->addDependent(this);
        this->delayDataSet();
        pathTask->queTask();
    }
    BoxRenderData::afterQued();
}

void LetterRenderData::initialize(const qreal relFrame,
                                  const QPointF &pos,
                                  const QString &letter,
                                  const SkFont &font,
                                  TextBox * const parent,
                                  Canvas * const scene) {
    fOriginalPos = pos;
    fLetterPos = pos;
    const auto parentM = parent->getInheritedTransformAtFrame(relFrame);
    parent->BoundingBox::setupWithoutRasterEffects(
                relFrame, parentM, this, scene);
    parent->setupPaintSettings(this, relFrame);
    parent->setupStrokerSettings(this, relFrame);
    SkPath textPath;
    SkiaHelpers::textToPath(font, pos.x(), pos.y(), letter, textPath);

    fPath = textPath;
    fEditPath = textPath;
    fFillPath = textPath;
    fOutlineBasePath = textPath;
    fStroker.strokePath(fOutlineBasePath, &fOutlinePath);

    parent->addPathEffects(relFrame, scene, fPathEffects, fFillEffects,
                           fOutlineBaseEffects, fOutlineEffects);

}

void LetterRenderData::applyTransform(const QMatrix &transform) {
    fRelTransform = transform*fRelTransform;
    fTotalTransform = fRelTransform*fInheritedTransform;
    fLetterPos = transform.map(fLetterPos);
}

WordRenderData::WordRenderData(TextBox * const parent) :
    ContainerBoxRenderData(parent) {
    fParentIsTarget = false;
}

void WordRenderData::initialize(const qreal relFrame,
                                const QPointF &pos,
                                const QString &word,
                                const SkFont &font,
                                const qreal letterSpacing,
                                TextBox * const parent,
                                Canvas * const scene) {
    const auto parentM = parent->getInheritedTransformAtFrame(relFrame);
    parent->BoundingBox::setupWithoutRasterEffects(
                relFrame, parentM, this, scene);

    fOriginalPos = pos;
    fWordPos = pos;
    qreal xPos = pos.x();

    const qreal spacingAdd = static_cast<qreal>(font.getSize())*letterSpacing;

    for(const auto& letterStr : word) {
        const auto letter = enve::make_shared<LetterRenderData>(parent);
        letter->initialize(relFrame, QPointF(xPos, pos.y()),
                           letterStr, font, parent, scene);

        fLetters << letter;

        fChildrenRenderData << letter;
        xPos += horizontalAdvance(font, letterStr) + spacingAdd;
    }
}

void WordRenderData::applyTransform(const QMatrix &transform) {
    fRelTransform = transform*fRelTransform;
    fTotalTransform = fRelTransform*fInheritedTransform;
    fWordPos = transform.map(fWordPos);

    for(const auto& letter : qAsConst(fLetters)) {
        letter->applyTransform(transform);
    }
}

void WordRenderData::queAllLetters() {
    for(const auto& letter : qAsConst(fLetters)) {
        letter->queTask();
        letter->addDependent(this);
    }
}

LineRenderData::LineRenderData(TextBox * const parent) :
    ContainerBoxRenderData(parent) {
    fParentIsTarget = false;
}

void LineRenderData::initialize(const qreal relFrame,
                                const QPointF &pos,
                                const QString &line,
                                const SkFont &font,
                                const qreal letterSpacing,
                                const qreal wordSpacing,
                                TextBox * const parent,
                                Canvas * const scene) {
    fOriginalPos = pos;
    fLinePos = pos;
    fString = line;
    const auto parentM = parent->getInheritedTransformAtFrame(relFrame);
    parent->BoundingBox::setupRenderData(relFrame, parentM, this, scene);

    qreal xPos = pos.x();
    const qreal spaceX = horizontalAdvance(font, " ")*wordSpacing;

    const auto wordFinished =
            [this, &xPos, &pos, &line, &font, parent, relFrame,
            letterSpacing, scene](const int i0, const int i) {
        const QString wordStr = line.mid(i0, i - i0 + 1);
        const auto word = enve::make_shared<WordRenderData>(parent);
        word->initialize(relFrame, QPointF(xPos, pos.y()), wordStr, font,
                         letterSpacing, parent, scene);
        fWords << word;
        fChildrenRenderData << word;
        xPos += horizontalAdvance(font, wordStr, letterSpacing);
    };

    int i0 = 0;
    int nSpaces = 0;
    for(int i = 0; i < line.length(); i++) {
        if(line.at(i) == ' ') {
            if(nSpaces == 0 && i != 0) wordFinished(i0, i - 1);
            nSpaces++;
            i0 = i + 1;
            xPos += spaceX;
            continue;
        }
        nSpaces = 0;
    }
    if(i0 < line.length()) wordFinished(i0, line.length() - 1);
}

void LineRenderData::applyTransform(const QMatrix &transform) {
    fRelTransform = transform*fRelTransform;
    fTotalTransform = fRelTransform*fInheritedTransform;
    fLinePos = transform.map(fLinePos);

    for(const auto& word : qAsConst(fWords)) {
        word->applyTransform(transform);
    }
}

void LineRenderData::queAllWords() {
    for(const auto& word : qAsConst(fWords)) {
        word->queAllLetters();
        word->queTask();
        word->addDependent(this);
    }
}

TextBoxRenderData::TextBoxRenderData(TextBox* const parent) :
    ContainerBoxRenderData(parent) {}

void TextBoxRenderData::initialize(const QString &text,
                                   const SkFont &font,
                                   const qreal letterSpacing,
                                   const qreal wordSpacing,
                                   const qreal lineSpacing,
                                   const Qt::Alignment hAlignment,
                                   const Qt::Alignment vAlignment,
                                   TextBox * const parent,
                                   Canvas* const scene) {
    const QStringList lines = text.split(QRegExp("\n|\r\n|\r"));

    qreal maxWidth = 0;

    for(const auto& line : lines) {
        const qreal lineWidth = horizontalAdvance(font, line, letterSpacing,
                                                  wordSpacing);
        if(lineWidth > maxWidth) maxWidth = lineWidth;
    }

    const qreal lineInc = static_cast<qreal>(font.getSpacing())*lineSpacing;

    qreal xTranslate;
    if(hAlignment == Qt::AlignLeft) xTranslate = 0;
    else if(hAlignment == Qt::AlignRight) xTranslate = -maxWidth;
    else /*if(hAlignment == Qt::AlignCenter)*/ xTranslate = -0.5*maxWidth;

    SkFontMetrics metrics;
    font.getMetrics(&metrics);
    const qreal height = (lines.count() - 1)*lineInc +
            static_cast<qreal>(metrics.fAscent + metrics.fDescent);
    qreal yTranslate;
    if(vAlignment == Qt::AlignTop) yTranslate = 0;
    else if(vAlignment == Qt::AlignBottom) yTranslate = -height;
    else /*if(vAlignment == Qt::AlignCenter)*/ yTranslate = -0.5*height;

    qreal yPos = yTranslate;
    for(const auto& lineStr : lines) {
        const qreal lineWidth = horizontalAdvance(font, lineStr, letterSpacing,
                                                  wordSpacing);
        const qreal xPos = textLineX(hAlignment, lineWidth, maxWidth) + xTranslate;
        const auto line = enve::make_shared<LineRenderData>(parent);
        line->initialize(fRelFrame, QPointF(xPos, yPos), lineStr,
                         font, letterSpacing, wordSpacing, parent, scene);
        fLines << line;
        fChildrenRenderData << line;
        yPos += lineInc;
    }
}

void TextBoxRenderData::queAllLines() {
    for(const auto& line : qAsConst(fLines)) {
        line->queAllWords();
        line->queTask();
        line->addDependent(this);
    }
}
