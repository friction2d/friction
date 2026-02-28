/*
#
# Friction - https://friction.graphics
#
# Copyright (c) Ole-André Rodlie and contributors
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3.
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

#include "svgclipboard.h"

#include <QDomDocument>
#include <QGuiApplication>
#include <QClipboard>
#include <QDebug>

using namespace Friction::Ui;

#ifndef Q_OS_MAC
const QString SvgClipBoard::getContent()
{
    return getClipBoard();
}
#endif

const QString SvgClipBoard::getContent(const QMimeData *mime)
{
    if (!mime) { return QString(); }

    static const QStringList formats{"image/svg+xml",
                                     "public.svg-image",
                                     "com.seriflabs.svg",
                                     "com.adobe.svg",
                                     "com.adobe.illustrator.svg",
                                     "com.adobe.illustrator.svgm",
                                     "text/xml"};

    for (const auto &format : formats) {
        if (!mime->hasFormat(format)) { continue; }
        const QString content = mime->data(format);
        if (isSvg(content)) { return content; }
    }

    if (mime->hasText()) {
        const QString text = mime->text();
        if (isSvg(text)) { return text; }
    }

    return QString();
}

bool SvgClipBoard::isSvg(const QString &content)
{
    qDebug() << "SVG in clipboard?" << content;
    if (content.isEmpty() ||
        !content.contains("<svg", Qt::CaseInsensitive)) {
        return false;
    }

    QDomDocument doc;
    if (doc.setContent(content)) {
        const auto root = doc.documentElement();
        return root.tagName().toLower() == "svg" ||
               root.localName().toLower() == "svg";
    }
    return false;
}

const QString SvgClipBoard::getClipBoard()
{
    const auto clipboard = QGuiApplication::clipboard();
    if (clipboard) { return getContent(clipboard->mimeData()); }
    return QString();
}
