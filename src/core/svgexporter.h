/*
# enve2d - https://github.com/enve2d
#
# Copyright (c) enve2d developers
# Copyright (C) 2016-2020 Maurycy Liebner
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
*/

#ifndef SVGEXPORTER_H
#define SVGEXPORTER_H

#include "Private/Tasks/complextask.h"
#include "framerange.h"

#include <QDomDocument>
#include <functional>

class Canvas;

class CORE_EXPORT SvgExporter : public ComplexTask
{
public:
    using FrameMapper = std::function<qreal(qreal)>;
    struct FrameMapping {
        FrameMapper mapper;
        bool active = false;
        bool discrete = false;
    };

    class FrameMappingScope {
    public:
        FrameMappingScope(SvgExporter& exp, const FrameMapping& mapping);
        ~FrameMappingScope();
    private:
        SvgExporter& mExp;
        bool mActive = false;
    };

    SvgExporter(const QString& path,
                Canvas* const scene,
                const FrameRange& frameRange,
                const qreal fps,
                const bool background,
                const bool fixedSize,
                const bool loop,
                const SkEncodedImageFormat imageFormat = SkEncodedImageFormat::kPNG,
                const int imageQuality = 100,
                bool html = false,
                bool blendMix = false,
                bool colors11 = false,
                bool optimize = false);

    void nextStep() override;

    void addNextTask(const stdsptr<eTask>& task);

    FrameMapping currentFrameMapping() const;
    qreal mapRelFrame(const qreal frame) const;
    bool hasFrameMapping() const;
    bool forceDiscreteMapping() const;

    Canvas* const fScene;
    const FrameRange fAbsRange;
    const qreal fFps;
    const bool fBackground;
    const bool fFixedSize;
    const bool fLoop;
    const SkEncodedImageFormat fImageFormat;
    const int fImageQuality;
    const bool fBlendMix;
    const bool fColors11;
    const bool fOptimize;

    QDomElement createElement(const QString& tagName)
    {
        return mDoc.createElement(tagName);
    }

    QDomText createTextNode(const QString& text)
    {
        return mDoc.createTextNode(text);
    }

    void addToDefs(const QDomElement& def)
    {
        mDefs.appendChild(def);
    }

    QDomElement& svg()
    {
        return mSvg;
    }

private:
    void finish();
    void pushFrameMapping(const FrameMapping& mapping);
    void popFrameMapping();
    bool mHtml;
    bool mOpen;
    QFile mFile;
    QTextStream mStream;
    QDomDocument mDoc;
    QDomElement mSvg;
    QDomElement mDefs;
    QList<stdsptr<eTask>> mWaitingTasks;
};

#endif // SVGEXPORTER_H
