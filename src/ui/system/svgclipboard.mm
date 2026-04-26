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

#ifdef Q_OS_MAC
#import <AppKit/AppKit.h>

using namespace Friction::Ui;

const QString SvgClipBoard::getContent()
{
    @autoreleasepool{
        NSPasteboard* pb = [NSPasteboard generalPasteboard];
        if (!pb) { return QString(); }
        NSArray<NSString*>* types = @[@"public.svg-image",
                                      @"org.inkscape.svg",
                                      @"com.seriflabs.svg",
                                      @"com.adobe.svg",
                                      @"com.adobe.illustrator.svg",
                                      @"com.adobe.illustrator.svgm",
                                      NSPasteboardTypeString];
        for (NSString* type in types) {
            NSString* str = [pb stringForType:type];
            if (!str) { continue; }
            const QString qStr = QString::fromUtf8([str UTF8String]);
            if (isSvg(qStr)) { return qStr; }
        }
    }
    return getClipBoard();
}
#endif
