#include "macpasteboarddump.h"

#import <AppKit/AppKit.h>

namespace Friction {

bool readNativeMacSvgFromPasteboard(QString& svgOut, QString* sourceTypeOut) {
#ifdef Q_OS_MAC
    @autoreleasepool {
        NSPasteboard* pb = [NSPasteboard generalPasteboard];
        if (!pb) return false;
        NSArray<NSPasteboardItem*>* items = [pb pasteboardItems];
        if (!items || items.count < 1) return false;

        NSPasteboardItem* item = items.firstObject;
        if (!item) return false;

        NSArray<NSString*>* preferredTypes = @[
            @"org.inkscape.svg",
            @"public.svg-image"
        ];

        for (NSString* type in preferredTypes) {
            NSString* str = [item stringForType:type];
            if (!str || str.length == 0) continue;
            const QString text = QString::fromUtf8(str.UTF8String ? str.UTF8String : "");
            if (!text.contains("<svg")) continue;
            svgOut = text;
            if (sourceTypeOut) {
                *sourceTypeOut = QString::fromUtf8(type.UTF8String ? type.UTF8String : "");
            }
            return true;
        }

        NSArray<NSPasteboardType>* types = [item types];
        for (NSPasteboardType type in types) {
            NSData* data = [item dataForType:type];
            if (!data || data.length == 0) continue;
            const QByteArray bytes(static_cast<const char*>(data.bytes),
                                   static_cast<int>(data.length));
            const int pos = bytes.indexOf("<svg");
            if (pos < 0) continue;
            svgOut = QString::fromUtf8(bytes.mid(pos));
            if (sourceTypeOut) {
                *sourceTypeOut = QString::fromUtf8(type.UTF8String ? type.UTF8String : "");
            }
            return true;
        }
        return false;
    }
#else
    Q_UNUSED(svgOut)
    Q_UNUSED(sourceTypeOut)
    return false;
#endif
}

}
