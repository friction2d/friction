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

#ifndef TEXTBOX_H
#define TEXTBOX_H
#include "Boxes/pathbox.h"
#include "skia/skiaincludes.h"
#include "../Animators/texteffectcollection.h"
class QStringAnimator;

enum class TextFragmentType : short {
    letter, word, line, whole
};

class CORE_EXPORT TextBox : public PathBox {
    e_OBJECT
protected:
    TextBox();

    void prp_readPropertyXEV_impl(const QDomElement& ele, const Friction::Core::XmlImporter& imp);
    QDomElement prp_writePropertyXEV_impl(const Friction::Core::XmlExporter& exp) const;
public:
    void setupCanvasMenu(PropertyMenu * const menu);
    SkPath getRelativePath(const qreal relFrame) const;

    void writeBoundingBox(eWriteStream& dst) const;
    void readBoundingBox(eReadStream& src);

    bool differenceInEditPathBetweenFrames(
                const int frame1, const int frame2) const;

    void setTextHAlignment(const Qt::Alignment alignment);
    void setTextVAlignment(const Qt::Alignment alignment);

    void setFont(const SkFont &font);
    void setFontSize(const qreal size);
    void setFontFamilyAndStyle(const QString &fontFamily,
                               const SkFontStyle& style);

    stdsptr<BoxRenderData> createRenderData();
    void setupRenderData(const qreal relFrame, const QMatrix& parentM,
                         BoxRenderData * const data,
                         Canvas * const scene);

    SkScalar getFontSize() const;
    const QString& getFontFamily() const;
    const SkFontStyle& getFontStyle() const;
    const QString& getCurrentValue() const;

    void openTextEditor(QWidget* dialogParent);

    bool hasTextEffects() const { return mTextEffects->hasEffects(); };

    void setCurrentValue(const QString &text);

    void saveSVG(SvgExporter& doc, DomEleTask* const task) const;
protected:
    void getMotionBlurProperties(QList<Property*>& list) const;
private:
    void textToPath(const qreal x, const qreal y,
                    const QString& text, SkPath& path) const;

    Qt::Alignment mHAlignment = Qt::AlignLeft;
    Qt::Alignment mVAlignment = Qt::AlignTop;

    QFont mQFont;
    SkFont mFont;
    QString mFamily;
    SkFontStyle mStyle;

    qsptr<StaticComplexAnimator> mSpacingCont;
    qsptr<QrealAnimator> mLetterSpacing;
    qsptr<QrealAnimator> mWordSpacing;
    qsptr<QrealAnimator> mLineSpacing;

    qsptr<QStringAnimator> mText;
    qsptr<TextEffectCollection> mTextEffects;

    TextFragmentType mFragmentsType;
    QList<SkPath> mTextFragments;
};

#endif // TEXTBOX_H
