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

#include "qstringanimator.h"

#include "undoredo.h"
#include "simplemath.h"
#include "svgexporthelpers.h"
#include "appsupport.h"

QStringAnimator::QStringAnimator(const QString &name) :
    SteppedAnimator<QString>(name) {}

QDomElement createTextElement(SvgExporter& exp, const QString& text) {
    auto textEle = exp.createElement("text");

    const QStringList lines = text.split(QRegExp("\n|\r\n|\r"));
    for(int i = 0; i < lines.count(); i++) {
        const auto& line = lines.at(i);
        auto tspan = exp.createElement("tspan");
        if(i != 0) tspan.setAttribute("dy", "1.2em");
        tspan.setAttribute("x", 0);
        const auto textNode = exp.createTextNode(line);
        tspan.appendChild(textNode);
        textEle.appendChild(tspan);
    }

    return textEle;
}

void QStringAnimator::saveSVG(SvgExporter& exp, QDomElement& parent,
                              const PropSetter& propSetter) const {
    const auto relRange = prp_absRangeToRelRange(exp.fAbsRange);
    const int span = exp.fAbsRange.span();
    if(exp.hasFrameMapping()) {
        QString currentValue;
        FrameRange currentRange{FrameRange::EMIN, FrameRange::EMIN};
        for(int i = relRange.fMin; i <= relRange.fMax; i++) {
            const qreal mapped = exp.mapRelFrame(i);
            const auto value = getValueAtRelFrame(mapped);
            if(i == relRange.fMin) {
                currentValue = value;
                currentRange = {i, i};
            } else if(value == currentValue) {
                currentRange.fMax = i;
            } else {
                auto ele = createTextElement(exp, currentValue);
                propSetter(ele);
                const auto absRange = prp_relRangeToAbsRange(currentRange);
                SvgExportHelpers::assignVisibility(exp, ele, exp.fAbsRange*absRange);
                parent.appendChild(ele);
                currentValue = value;
                currentRange = {i, i};
            }
        }

        if(currentRange.fMin != FrameRange::EMIN) {
            auto ele = createTextElement(exp, currentValue);
            propSetter(ele);
            const auto absRange = prp_relRangeToAbsRange(currentRange);
            SvgExportHelpers::assignVisibility(exp, ele, exp.fAbsRange*absRange);
            parent.appendChild(ele);
        }
        return;
    }

    const auto idRange = prp_getIdenticalRelRange(relRange.fMin);
    if(idRange.inRange(relRange) || span == 1) {
        auto ele = createTextElement(exp, getValueAtRelFrame(relRange.fMin));
        propSetter(ele);
        parent.appendChild(ele);
    } else {
        int i = relRange.fMin;
        while(true) {
            const auto iRange = exp.fAbsRange*prp_getIdenticalAbsRange(i);

            auto ele = createTextElement(exp, getValueAtRelFrame(i));
            propSetter(ele);
            SvgExportHelpers::assignVisibility(exp, ele, iRange);
            parent.appendChild(ele);

            if(iRange.fMax >= relRange.fMax) break;
            i = prp_nextDifferentRelFrame(i);
        }
    }
}

void QStringAnimator::prp_readPropertyXEV_impl(
        const QDomElement& ele, const XevImporter& imp) {
    if(ele.hasAttribute("frames")) {
        const auto framesStr = ele.attribute("frames");
        const auto frameStrs = framesStr.splitRef(' ', Qt::SkipEmptyParts);

        for(const QStringRef& frame : frameStrs) {
            const int iFrame = XmlExportHelpers::stringToInt(frame);
            imp.processAsset(frame + ".txt", [&](QIODevice* const src) {
                QString value;
                QTextStream stream(src);
                value = stream.readAll();
                const auto key = enve::make_shared<QStringKey>(value, iFrame, this);
                anim_appendKey(key);
            });
        }
    } else {
        imp.processAsset("value.txt", [&](QIODevice* const src) {
            QString value;
            QTextStream stream(src);
            value = stream.readAll();
            setCurrentValue(value);
        });
    }
}

void saveTextXEV(const QString& path, const XevExporter& exp,
                 const QString& txt) {
    exp.processAsset(path, [&](QIODevice* const dst) {
        QTextStream stream(dst);
        stream << txt;
    }, false);
}

QDomElement QStringAnimator::prp_writePropertyXEV_impl(const XevExporter& exp) const {
    auto result = exp.createElement("Text");
    if(anim_hasKeys()) {
        QString frames;
        for(const auto& key : anim_getKeys()) {
            const auto txtKey = static_cast<QStringKey*>(key);
            const auto frameStr = QString::number(key->getRelFrame());
            if(!frames.isEmpty()) frames += ' ';
            frames += frameStr;
            saveTextXEV(frameStr + ".txt", exp, txtKey->getValue());
        }
        result.setAttribute("frames", frames);
    } else {
        saveTextXEV("value.txt", exp, getCurrentValue());
    }

    return result;
}
