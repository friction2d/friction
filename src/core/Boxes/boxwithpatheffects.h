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

#ifndef BOXWITHPATHEFFECTS_H
#define BOXWITHPATHEFFECTS_H
#include "boundingbox.h"

class PathEffectCollection;
class PathEffectsTask;
class PathEffectCaller;

class CORE_EXPORT BoxWithPathEffects : public BoundingBox {
public:
    BoxWithPathEffects(const QString &name, const eBoxType type);

    void setupCanvasMenu(PropertyMenu * const menu);
    void prp_setupTreeViewMenu(PropertyMenu * const menu);

    void addPathEffect(const qsptr<PathEffect> &effect);
    void addFillPathEffect(const qsptr<PathEffect> &effect);
    void addOutlineBasePathEffect(const qsptr<PathEffect> &effect);
    void addOutlinePathEffect(const qsptr<PathEffect> &effect);

    virtual bool localDifferenceInPathBetweenFrames(
            const int frame1, const int frame2) const;
    virtual bool localDifferenceInOutlinePathBetweenFrames(
            const int frame1, const int frame2) const;
    virtual bool localDifferenceInFillPathBetweenFrames(
            const int frame1, const int frame2) const;

    bool differenceInPathBetweenFrames(
            const int frame1, const int frame2) const;
    bool differenceInOutlinePathBetweenFrames(
            const int frame1, const int frame2) const;
    bool differenceInFillPathBetweenFrames(
            const int frame1, const int frame2) const;

    void setPathEffectsEnabled(const bool enable);
    bool getPathEffectsVisible() const;

    void setFillEffectsEnabled(const bool enable);
    bool getFillEffectsVisible() const;

    void setOutlineBaseEffectsEnabled(const bool enable);
    bool getOutlineBaseEffectsVisible() const;

    void setOutlineEffectsEnabled(const bool enable);
    bool getOutlineEffectsVisible() const;

    PathEffectCollection *getPathEffectsAnimators();
    PathEffectCollection *getFillPathEffectsAnimators();
    PathEffectCollection *getOutlineBasePathEffectsAnimators();
    PathEffectCollection *getOutlinePathEffectsAnimators();

    bool hasBasePathEffects() const;
    bool hasFillEffects() const;
    bool hasOutlineBaseEffects() const;
    bool hasOutlineEffects() const;

    void applyBasePathEffects(const qreal relFrame, SkPath& path,
                              const qreal influence = 1) const;
    void applyFillEffects(const qreal relFrame, SkPath& path,
                          const qreal influence = 1) const;
    void applyOutlineBaseEffects(const qreal relFrame, SkPath& path,
                                 const qreal influence = 1) const;
    void applyOutlineEffects(const qreal relFrame, SkPath& path,
                             const qreal influence = 1) const;

    using PathCallerList = QList<stdsptr<PathEffectCaller>>;
    void addBasePathEffects(const qreal relFrame,
                            PathCallerList& list,
                            const qreal influence = 1) const;
    void addFillEffects(const qreal relFrame,
                        PathCallerList& list,
                        const qreal influence = 1) const;
    void addOutlineBaseEffects(const qreal relFrame,
                               PathCallerList& list,
                               const qreal influence = 1) const;
    void addOutlineEffects(const qreal relFrame,
                           PathCallerList& list,
                           const qreal influence = 1) const;
protected:
    void getMotionBlurProperties(QList<Property*> &list) const;

    qsptr<PathEffectCollection> mPathEffectsAnimators;
    qsptr<PathEffectCollection> mFillPathEffectsAnimators;
    qsptr<PathEffectCollection> mOutlineBasePathEffectsAnimators;
    qsptr<PathEffectCollection> mOutlinePathEffectsAnimators;
};

#endif // BOXWITHPATHEFFECTS_H
