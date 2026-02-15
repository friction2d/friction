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

#ifndef INTERPOLATIONANIMATORT_H
#define INTERPOLATIONANIMATORT_H
#include "graphanimatort.h"
#include "qrealpoint.h"
#include "interpolationkeyt.h"

template <typename T, typename K = InterpolationKeyT<T>>
class InterpolationAnimatorT :
        public BasedAnimatorT<GraphAnimator, K, T> {
    e_OBJECT
public:
    void graph_getValueConstraints(
            GraphKey *key, const QrealPointType type,
            qreal &minValue, qreal &maxValue) const override {
        if(type == QrealPointType::keyPt) {
            minValue = key->getRelFrame();
            maxValue = minValue;
            //getFrameConstraints(key, type, minValue, maxValue);
        } else {
            minValue = -DBL_MAX;
            maxValue = DBL_MAX;
        }
    }
protected:
    InterpolationAnimatorT(const QString& name) :
        BasedAnimatorT<GraphAnimator, K, T>(name) {}

    T getValueAtRelFrameK(const qreal frame,
                          const K * const prevKey,
                          const K * const nextKey) const override;

    using StringToValue = std::function<void(T&, QStringRef)>;
    void readValuesXEV(const QDomElement& ele, const StringToValue& strToVal);
    using ValueToString = std::function<QString(const T&)>;
    void writeValuesXEV(QDomElement& ele, const ValueToString& valToStr) const;
};

template<typename T, typename K>
T InterpolationAnimatorT<T, K>::getValueAtRelFrameK(
        const qreal frame, const K* const prevKey,
        const K* const nextKey) const {
    T result;
    const qreal prevFrame = prevKey->getRelFrame();
    const qreal nextFrame = nextKey->getRelFrame();
    const qCubicSegment1D seg{prevFrame,
                prevKey->getC1Frame(),
                nextKey->getC0Frame(),
                nextFrame};
    const qreal t = gTFromX(seg, frame);
    const qreal p0y = prevKey->getValueForGraph();
    const qreal p1y = prevKey->getC1Value();
    const qreal p2y = nextKey->getC0Value();
    const qreal p3y = nextKey->getValueForGraph();
    const qreal iFrame = gCubicValueAtT({p0y, p1y, p2y, p3y}, t);
    const qreal tEff = (iFrame - prevFrame)/(nextFrame - prevFrame);
    gInterpolate(prevKey->getValue(), nextKey->getValue(), tEff, result);
    return result;
}

template<typename T, typename K>
void InterpolationAnimatorT<T, K>::readValuesXEV(
        const QDomElement& ele, const StringToValue& strToVal) {
    const bool hasValues = ele.hasAttribute("values");
    const bool hasFrames = ele.hasAttribute("frames");
    const bool hasKeys = hasValues && hasFrames;
    if(hasKeys) {
        const QString valueStrs = ele.attribute("values");
        const QString frameStrs = ele.attribute("frames");
        const QString ctrlModeStrs = ele.attribute("ctrlModes");
        const QString ctrlValueStrs = ele.attribute("ctrlValues");

        const auto values = valueStrs.splitRef(';');
        const auto framess = frameStrs.splitRef(';');
        const auto ctrlModes = ctrlModeStrs.splitRef(';');
        const auto ctrlValuess = ctrlValueStrs.splitRef(';');

        if(values.count() != framess.count())
            RuntimeThrow("The values count does not match the frames count");
        if(ctrlModes.count() != framess.count())
            RuntimeThrow("The ctrlModes count does not match the frames count");
        if(ctrlValuess.count() != framess.count())
            RuntimeThrow("The ctrlValues count does not match the frames count");
        const int iMax = values.count();
        for(int i = 0; i < iMax; i++) {
            const auto& value = values[i];
            const auto frames = framess[i].split(' ');
            if(frames.count() != 3) {
                RuntimeThrow("Invalid frames count " + framess[i].toString());
            }
            const auto ctrlValues = ctrlValuess[i].split(' ');
            if(ctrlValues.count() != 2) {
                RuntimeThrow("Invalid ctrlValues count " + ctrlValuess[i].toString());
            }
            const auto ctrlModeStr = ctrlModes[i];
            const auto ctrlMode = XmlExportHelpers::stringToEnum<CtrlsMode>(
                                        ctrlModeStr, CtrlsMode::smooth,
                                        CtrlsMode::corner);

            const int frame = XmlExportHelpers::stringToInt(frames[1]);
            const auto key = enve::make_shared<K>(frame, this);
            auto& keyValue = key->getValue();
            strToVal(keyValue, value);

            const auto f0Str = frames[0];
            const auto f2Str = frames[2];

            const auto& v0Str = ctrlValues[0];
            const auto& v2Str = ctrlValues[1];

            const bool c0Enabled = f0Str != '*' && v0Str != '*';
            const bool c1Enabled = f2Str != '*' && v2Str != '*';

            key->setC0Enabled(c0Enabled);
            key->setC1Enabled(c1Enabled);

            key->setC0FrameVar(c0Enabled ? XmlExportHelpers::stringToDouble(f0Str) : frame);
            key->setC1FrameVar(c1Enabled ? XmlExportHelpers::stringToDouble(f2Str) : frame);

            key->setC0ValueVar(c0Enabled ? XmlExportHelpers::stringToDouble(v0Str) : frame);
            key->setC1ValueVar(c1Enabled ? XmlExportHelpers::stringToDouble(v2Str) : frame);

            key->setCtrlsMode(ctrlMode);

            this->anim_appendKey(key);
        }
    } else if(ele.hasAttribute("value")) {
        const QString value = ele.attribute("value");
        strToVal(this->mCurrentValue, &value);
    } else RuntimeThrow("No values/frames and no value provided");
}

template<typename T, typename K>
void InterpolationAnimatorT<T, K>::writeValuesXEV(
        QDomElement& ele, const ValueToString& valToStr) const {
    if(this->anim_hasKeys()) {
        QString values;
        QString frames;
        QString ctrlModes;
        QString ctrlValues;

        const QString framesBP = QStringLiteral("%1 %2 %3");
        const QString ctrlValsBP = QStringLiteral("%1 %2");
        const auto& keys = this->anim_getKeys();
        for(const auto &key : keys) {
            const auto smKey = static_cast<K*>(key);
            const QString v = valToStr(smKey->getValue());

            const qreal fc0 = smKey->getC0Frame();
            const qreal vc0 = smKey->getC0Value();
            const int f = smKey->getRelFrame();
            const qreal fc1 = smKey->getC1Frame();
            const qreal vc1 = smKey->getC1Value();

            const QString fc0Str = smKey->getC0Enabled() ?
                                       QString::number(fc0) : "*";
            const QString fc1Str = smKey->getC1Enabled() ?
                                       QString::number(fc1) : "*";

            const QString vc0Str = smKey->getC0Enabled() ?
                                       QString::number(vc0) : "*";
            const QString vc1Str = smKey->getC1Enabled() ?
                                       QString::number(vc1) : "*";

            if(!values.isEmpty()) values += ';';
            values += v;
            if(!frames.isEmpty()) frames += ';';
            frames += framesBP.arg(fc0Str).arg(f).arg(fc1Str);
            if(!ctrlModes.isEmpty()) ctrlModes += ';';
            const auto ctrlMode = smKey->getCtrlsMode();
            ctrlModes += QString::number(static_cast<int>(ctrlMode));
            if(!ctrlValues.isEmpty()) ctrlValues += ';';
            ctrlValues += ctrlValsBP.arg(vc0Str, vc1Str);
        }
        ele.setAttribute("values", values);
        ele.setAttribute("frames", frames);
        ele.setAttribute("ctrlModes", ctrlModes);
        ele.setAttribute("ctrlValues", ctrlValues);
    } else ele.setAttribute("value", valToStr(this->mCurrentValue));
}

#endif // INTERPOLATIONANIMATORT_H
