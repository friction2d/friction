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

#include "svgexporter.h"
#include "canvas.h"

#include "appsupport.h"

SvgExporter::SvgExporter(const QString& path,
                         Canvas* const scene,
                         const FrameRange& frameRange,
                         const qreal fps,
                         const bool background,
                         const bool fixedSize,
                         const bool loop,
                         const SkEncodedImageFormat imageFormat,
                         const int imageQuality,
                         bool html,
                         bool blendMix,
                         bool colors11)
    : ComplexTask(INT_MAX, tr("SVG Export"))
    , fScene(scene)
    , fAbsRange(frameRange)
    , fFps(fps)
    , fBackground(background)
    , fFixedSize(fixedSize)
    , fLoop(loop)
    , fImageFormat(imageFormat)
    , fImageQuality(imageQuality)
    , fBlendMix(blendMix)
    , fColors11(colors11)
    , mHtml(html)
    , mOpen(false)
    , mFile(path)
    , mSvg(createElement("svg"))
    , mDefs(createElement("defs"))
{

}

void SvgExporter::nextStep()
{
    if (!mOpen) {
        if (mFile.open(QIODevice::WriteOnly)) {
            mStream.setDevice(&mFile);
            if (mHtml) {
                mStream << QString::fromUtf8("<!DOCTYPE html>") << Qt::endl;
                mStream << QString::fromUtf8("<html>") << Qt::endl;
                mStream << QString::fromUtf8("<head>") << Qt::endl;
                mStream << QString::fromUtf8("<meta charset=\"utf-8\" />") << Qt::endl;
                mStream << QString::fromUtf8("<title>%1</title>").arg(tr("Preview")) << Qt::endl;
                mStream << QString::fromUtf8("<style>html, body { width: 100%; height: 100%; margin: 0; padding: 0; overflow: hidden; } html { background: repeating-conic-gradient(#b0b0b0 0% 25%, transparent 0% 50%) 50% / 40px 40px; } svg { margin: auto; width: 100%; height: 100%; object-fit: contain; overflow: hidden; }</style>") << Qt::endl;
                mStream << QString::fromUtf8("</head>") << Qt::endl;
                mStream << QString::fromUtf8("<body>") << Qt::endl;
            } else {
                mStream << QString::fromUtf8("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>") << Qt::endl;
            }
            mStream << QString::fromUtf8("<!-- Created with %1 - %2 -->").arg(AppSupport::getAppDisplayName(),
                                                                              AppSupport::getAppUrl()) << Qt::endl << Qt::endl;
            fScene->saveSceneSVG(*this);
        } else {
            RuntimeThrow("Could not open:\n\"" + mFile.fileName() + "\"");
        }
        mOpen = true;
    }
    if (mWaitingTasks.isEmpty()) { return finish(); }
    const auto task = mWaitingTasks.takeFirst();
    addTask(task);
}

void SvgExporter::addNextTask(const stdsptr<eTask>& task)
{
    mWaitingTasks << task;
}

void SvgExporter::finish()
{
    if (mOpen) {
        mSvg.appendChild(mDefs);
        mDoc.appendChild(mSvg);
        mStream << mDoc.toString();
        if (mHtml) {
            mStream << QString::fromUtf8("</body>") << Qt::endl;
            mStream << QString::fromUtf8("</html>") << Qt::endl;
        }
        mStream.flush();
        mFile.close();
    }
    setValue(INT_MAX);
}
