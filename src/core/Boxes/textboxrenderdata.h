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

#ifndef TEXTBOXRENDERDATA_H
#define TEXTBOXRENDERDATA_H
#include "layerboxrenderdata.h"
#include "pathboxrenderdata.h"

class TextBox;
class PathEffectCaller;

extern qreal textLineX(const Qt::Alignment &alignment,
                       const qreal lineWidth,
                       const qreal maxWidth);

extern qreal horizontalAdvance(const SkFont& font,
                               const QString& str);
extern qreal horizontalAdvance(const SkFont& font, const QString& str,
                               const qreal letterSpacing);
extern qreal horizontalAdvance(const SkFont& font, const QString& str,
                               const qreal letterSpacing, const qreal wordSpacing);

class CORE_EXPORT LetterRenderData : public PathBoxRenderData {
public:
    LetterRenderData(TextBox* const parent);

    void afterQued();

    void initialize(const qreal relFrame,
                    const QPointF &pos,
                    const QString &letter,
                    const SkFont &font,
                    TextBox * const parent,
                    Scene * const scene);

    void applyTransform(const QMatrix& transform);

    QRectF fBoundingRect;
    QPointF fLetterPos;
    QPointF fOriginalPos;

    QList<stdsptr<PathEffectCaller>> fPathEffects;
    QList<stdsptr<PathEffectCaller>> fFillEffects;
    QList<stdsptr<PathEffectCaller>> fOutlineBaseEffects;
    QList<stdsptr<PathEffectCaller>> fOutlineEffects;
};

class CORE_EXPORT WordRenderData : public ContainerBoxRenderData {
public:
    WordRenderData(TextBox* const parent);

    void initialize(const qreal relFrame,
                    const QPointF& pos,
                    const QString& word,
                    const SkFont &font,
                    const qreal letterSpacing,
                    TextBox * const parent,
                    Scene * const scene);

    void applyTransform(const QMatrix &transform);
    void queAllLetters();

    QRectF fBoundingRect;
    QPointF fWordPos;
    QPointF fOriginalPos;
    QList<stdsptr<LetterRenderData>> fLetters;
};

class CORE_EXPORT LineRenderData : public ContainerBoxRenderData {
public:
    LineRenderData(TextBox* const parent);

    void initialize(const qreal relFrame,
                    const QPointF& pos,
                    const QString& line,
                    const SkFont &font,
                    const qreal letterSpacing,
                    const qreal wordSpacing,
                    TextBox * const parent,
                    Scene * const scene);

    void applyTransform(const QMatrix &transform);
    void queAllWords();

    QPointF fLinePos;
    QPointF fOriginalPos;
    QString fString;
    QList<stdsptr<WordRenderData>> fWords;
};

class CORE_EXPORT TextBoxRenderData : public ContainerBoxRenderData {
public:
    TextBoxRenderData(TextBox* const parent);

    void initialize(const QString& text,
                    const SkFont &font,
                    const qreal letterSpacing,
                    const qreal wordSpacing,
                    const qreal lineSpacing,
                    const Qt::Alignment hAlignment,
                    const Qt::Alignment vAlignment,
                    TextBox * const parent,
                    Scene * const scene);

    void queAllLines();

    QList<stdsptr<LineRenderData>> fLines;
};

#endif // TEXTBOXRENDERDATA_H
