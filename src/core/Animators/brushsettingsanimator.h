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

#ifndef BRUSHSETTINGSANIMATOR_H
#define BRUSHSETTINGSANIMATOR_H
#include "Animators/staticcomplexanimator.h"
#include "Animators/qcubicsegment1danimator.h"
#include "Paint/simplebrushwrapper.h"

struct UpdateStrokeSettings;

class CORE_EXPORT BrushSettingsAnimator : public StaticComplexAnimator {
    typedef qCubicSegment1DAnimator::Action SegAction;
    e_OBJECT
    Q_OBJECT
    BrushSettingsAnimator();
protected:
    void prp_readPropertyXML_impl(const QDomElement& ele, const Friction::Core::XmlImporter& imp);
    QDomElement prp_writePropertyXML_impl(const Friction::Core::XmlExporter& exp) const;
public:    
    void prp_writeProperty_impl(eWriteStream &dst) const;
    void prp_readProperty_impl(eReadStream &src);

    qCubicSegment1DAnimator * getWidthAnimator() const
    { return mWidthCurve.data(); }

    qCubicSegment1DAnimator * getPressureAnimator() const
    { return mPressureCurve.data(); }

    qCubicSegment1DAnimator * getSpacingAnimator() const
    { return mSpacingCurve.data(); }

    qCubicSegment1DAnimator * getTimeAnimator() const
    { return mTimeCurve.data(); }

    SimpleBrushWrapper* getBrush() const
    { return mBrush; }

    void setBrush(SimpleBrushWrapper * const brush);

    void applyWidthAction(const SegAction& action)
    { action.apply(mWidthCurve.get()); }

    void applyPressureAction(const SegAction& action)
    { action.apply(mPressureCurve.get()); }

    void applySpacingAction(const SegAction& action)
    { action.apply(mSpacingCurve.get()); }

    void applyTimeAction(const SegAction& action)
    { action.apply(mTimeCurve.get()); }

    void setStrokeBrushWidthCurve(const qCubicSegment1D& curve)
    { mWidthCurve->setCurrentValue(curve); }

    void setStrokeBrushPressureCurve(const qCubicSegment1D& curve)
    { mPressureCurve->setCurrentValue(curve); }

    void setStrokeBrushSpacingCurve(const qCubicSegment1D& curve)
    { mSpacingCurve->setCurrentValue(curve); }

    void setStrokeBrushTimeCurve(const qCubicSegment1D& curve)
    { mTimeCurve->setCurrentValue(curve); }

    void setupStrokeSettings(const qreal relFrame,
                             UpdateStrokeSettings& settings);
signals:
    void brushChanged(SimpleBrushWrapper* brush);
private:
    qsptr<qCubicSegment1DAnimator> mWidthCurve =
            enve::make_shared<qCubicSegment1DAnimator>("width");
    qsptr<qCubicSegment1DAnimator> mPressureCurve =
            enve::make_shared<qCubicSegment1DAnimator>("pressure");
    qsptr<qCubicSegment1DAnimator> mSpacingCurve =
            enve::make_shared<qCubicSegment1DAnimator>("spacing");
    qsptr<qCubicSegment1DAnimator> mTimeCurve =
            enve::make_shared<qCubicSegment1DAnimator>("time");

    stdptr<SimpleBrushWrapper> mBrush;
};

#endif // BRUSHSETTINGSANIMATOR_H
