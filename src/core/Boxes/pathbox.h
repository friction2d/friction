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

#ifndef PATHBOX_H
#define PATHBOX_H
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include "Boxes/boxwithpatheffects.h"
#include "Animators/paintsettingsanimator.h"
//#include "Paint/autotiledsurface.h"
#include "pathboxrenderdata.h"
//#include "libmypaintincludes.h"
#include "Animators/qcubicsegment1danimator.h"
class SmartVectorPath;
class GradientPoints;
class SkStroke;
class PathEffectCollection;
class PathEffect;

class CORE_EXPORT PathBox : public BoxWithPathEffects {
    typedef qCubicSegment1DAnimator::Action SegAction;
    e_OBJECT
    e_DECLARE_TYPE(PathBox)
protected:
    PathBox(const QString& name, const eBoxType type);
public:
    virtual bool differenceInEditPathBetweenFrames(
            const int frame1, const int frame2) const = 0;
    virtual SkPath getRelativePath(const qreal relFrame) const = 0;

    HardwareSupport hardwareSupport() const;

    OutlineSettingsAnimator *getStrokeSettings() const;
    FillSettingsAnimator *getFillSettings() const;

    SmartVectorPath *objectToVectorPathBox();
    SmartVectorPath *strokeToVectorPathBox();

    bool relPointInsidePath(const QPointF &relPos) const;

    void drawHoveredSk(SkCanvas *canvas, const float invScale);

    void setupRenderData(const qreal relFrame, const QMatrix& parentM,
                         BoxRenderData * const data,
                         Scene * const scene);
    stdsptr<BoxRenderData> createRenderData() {
        return enve::make_shared<PathBoxRenderData>(this);
    }
    void updateCurrentPreviewDataFromRenderData(
            BoxRenderData *renderData);

    typedef QList<stdsptr<PathEffectCaller>> PathEffectsCList;
    void addPathEffects(
            const qreal relFrame, Scene* const scene,
            PathEffectsCList &pathEffects,
            PathEffectsCList &fillEffects,
            PathEffectsCList &outlineBaseEffects,
            PathEffectsCList &outlineEffects);

    void setupStrokerSettings(PathBoxRenderData * const pathData,
                              const qreal relFrame);
    void setupPaintSettings(PathBoxRenderData * const pathData,
                            const qreal relFrame);

    void duplicateStrokeSettingsFrom(
            OutlineSettingsAnimator * const strokeSettings);
    void duplicateFillSettingsFrom(
            FillSettingsAnimator * const fillSettings);
    void duplicateStrokeSettingsNotAnimatedFrom(
            OutlineSettingsAnimator * const strokeSettings);
    void duplicateFillSettingsNotAnimatedFrom(
            FillSettingsAnimator * const fillSettings);
    void duplicatePaintSettingsFrom(FillSettingsAnimator * const fillSettings,
                                    OutlineSettingsAnimator * const strokeSettings);

    SkPath getParentCoordinatesPath(const qreal relFrame) const;
    SkPath getAbsolutePath(const qreal relFrame) const;
    SkPath getAbsolutePath() const;
    const SkPath &getRelativePath() const;
    void setOutlineAffectedByScale(const bool bT);

    void copyDataToOperationResult(PathBox * const targetBox) const;
    void copyPathBoxDataTo(PathBox * const targetBox) const;

    bool differenceInOutlinePathBetweenFrames(
            const int frame1, const int frame2) const;

    void setPathsOutdated(const UpdateReason reason);
    void setOutlinePathOutdated(const UpdateReason reason);
    void setFillPathOutdated(const UpdateReason reason);

    void savePathBoxSVG(SvgExporter& exp, QDomElement& ele,
                        const FrameRange& visRange) const;
    void saveFillSettingsSVG(SvgExporter& exp, QDomElement& ele,
                             const FrameRange& visRange) const;
    void saveStrokeSettingsSVG(SvgExporter& exp, QDomElement& ele,
                               const FrameRange& visRange,
                               const bool asFill = false) const;
protected:
    bool mOutlineAffectedByScale = true;
    bool mCurrentPathsOutdated = true;
    bool mCurrentOutlinePathOutdated = true;
    bool mCurrentFillPathOutdated = true;

    qreal mCurrentPathsFrame = 0;

    SkPath mEditPathSk;
    SkPath mPathSk;
    SkPath mFillPathSk;
    SkPath mOutlineBasePathSk;
    SkPath mOutlinePathSk;

    GradientPoints* mFillGradientPoints = nullptr;
    GradientPoints* mStrokeGradientPoints = nullptr;

    qsptr<FillSettingsAnimator> mFillSettings;
    qsptr<OutlineSettingsAnimator> mStrokeSettings;
};

#endif // PATHBOX_H
