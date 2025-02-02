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

#ifndef COLORANIMATOR_H
#define COLORANIMATOR_H

#include "staticcomplexanimator.h"
#include "../colorhelpers.h"
#include "qrealanimator.h"

class CORE_EXPORT ColorAnimator : public StaticComplexAnimator {
    Q_OBJECT
    e_OBJECT
protected:
    ColorAnimator(const QString& name = "color");

    QDomElement prp_writePropertyXEV_impl(const Friction::Core::XevExporter& exp) const;
    void prp_readPropertyXEV_impl(const QDomElement& ele, const Friction::Core::XevImporter& imp);
public:
    QJSValue prp_getBaseJSValue(QJSEngine& e) const;
    QJSValue prp_getBaseJSValue(QJSEngine& e, const qreal relFrame) const;
    QJSValue prp_getEffectiveJSValue(QJSEngine& e) const;
    QJSValue prp_getEffectiveJSValue(QJSEngine& e, const qreal relFrame) const;

    void prp_writeProperty_impl(eWriteStream& dst) const;
    void prp_readProperty_impl(eReadStream& src);
    void prp_setupTreeViewMenu(PropertyMenu * const menu);

    QColor getBaseColor() const;
    QColor getBaseColor(const qreal relFrame) const;
    QColor getColor() const;
    QColor getColor(const qreal relFrame) const;
    void setColor(const QColor& col);

    void setColorMode(const ColorMode colorMode);

    void startVal1Transform();
    void startVal2Transform();
    void startVal3Transform();
    void startAlphaTransform();

    void setCurrentVal1Value(const qreal val1);
    void setCurrentVal2Value(const qreal val2);
    void setCurrentVal3Value(const qreal val3);
    void setCurrentAlphaValue(const qreal alpha);

    void duplicateVal1AnimatorFrom(QrealAnimator *source);
    void duplicateVal2AnimatorFrom(QrealAnimator *source);
    void duplicateVal3AnimatorFrom(QrealAnimator *source);
    void duplicateAlphaAnimatorFrom(QrealAnimator *source);

    ColorMode getColorMode() {
        return mColorMode;
    }

    QrealAnimator *getVal1Animator() {
        return mVal1Animator.get();
    }

    QrealAnimator *getVal2Animator() {
        return mVal2Animator.get();
    }

    QrealAnimator *getVal3Animator() {
        return mVal3Animator.get();
    }

    QrealAnimator *getAlphaAnimator() {
        return mAlphaAnimator.get();
    }

    void saveColorSVG(SvgExporter &exp,
                      QDomElement &parent,
                      const FrameRange &visRange,
                      const QString &name,
                      bool rgba = true,
                      bool a = false) const;
private:
    ColorMode mColorMode = ColorMode::rgb;

    qsptr<QrealAnimator> mVal1Animator =
            QrealAnimator::sCreate0to1Animator("");
    qsptr<QrealAnimator> mVal2Animator =
            QrealAnimator::sCreate0to1Animator("");
    qsptr<QrealAnimator> mVal3Animator =
            QrealAnimator::sCreate0to1Animator("");
    qsptr<QrealAnimator> mAlphaAnimator =
            QrealAnimator::sCreate0to1Animator("alpha");
signals:
    void colorChanged(const QColor&);
    void colorModeChanged(ColorMode);
};

#endif // COLORANIMATOR_H
