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

#ifndef ANIMATORT_H
#define ANIMATORT_H

#include "animator.h"
#include "key.h"
#include "differsinterpolate.h"
#include "keyt.h"
#include "basedanimatort.h"
#include "XML/xmlexporthelpers.h"

template <typename T>
class AnimatorT : public BasedAnimatorT<Animator, KeyT<T>, T> {
public:
protected:
    AnimatorT(const QString& name) :
        BasedAnimatorT<Animator, KeyT<T>, T>(name) {}

    using StringToValue = std::function<void(T&, const QStringRef&)>;
    void readValuesXML(const QDomElement& ele, const StringToValue& strToVal);
    using ValueToString = std::function<QString(const T&)>;
    void writeValuesXML(QDomElement& ele, const ValueToString& valToStr) const;
};

template<typename T>
void AnimatorT<T>::readValuesXML(
        const QDomElement& ele, const StringToValue& strToVal) {
    const bool hasValues = ele.hasAttribute("values");
    const bool hasFrames = ele.hasAttribute("frames");
    const bool hasKeys = hasValues && hasFrames;
    if(hasKeys) {
        const QString valueStrs = ele.attribute("values");
        const QString frameStrs = ele.attribute("frames");

        const auto values = valueStrs.splitRef(';');
        const auto framess = frameStrs.splitRef(';');
        if(values.count() != framess.count())
            RuntimeThrow("The values count does not match the frames count");
        const int iMax = values.count();
        for(int i = 0; i < iMax; i++) {
            const auto& value = values[i];
            const auto frames = framess[i].split(' ');
            if(frames.count() != 3) {
                RuntimeThrow("Invalid frames count " + framess[i].toString());
            }

            const int frame = Friction::Core::XmlExportHelpers::stringToInt(frames[1]);
            const auto key = enve::make_shared<KeyT<T>>(frame, this);
            auto& keyValue = key->getValue();
            strToVal(keyValue, value);
            this->anim_appendKey(key);
        }
    } else if(ele.hasAttribute("value")) {
        const QString value = ele.attribute("value");
        strToVal(this->mCurrentValue, &value);
    } else RuntimeThrow("No values/frames and no value provided");
}

template<typename T>
void AnimatorT<T>::writeValuesXML(
        QDomElement& ele, const ValueToString& valToStr) const {
    if(this->anim_hasKeys()) {
        QString values;
        QString frames;
        const auto& keys = this->anim_getKeys();
        for(const auto &key : keys) {
            const auto smKey = static_cast<KeyT<T>*>(key);
            const QString v = valToStr(smKey->getValue());

            const int f = smKey->getRelFrame();

            if(!values.isEmpty()) values += ';';
            values += v;
            if(!frames.isEmpty()) frames += ';';
            frames += QString::number(f);
        }
        ele.setAttribute("values", values);
        ele.setAttribute("frames", frames);
    } else ele.setAttribute("value", valToStr(this->mCurrentValue));
}

#endif // ANIMATORT_H
