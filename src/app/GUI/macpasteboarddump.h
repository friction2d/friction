#pragma once

#include <QString>

namespace Friction {

bool readNativeMacSvgFromPasteboard(QString& svgOut, QString* sourceTypeOut = nullptr);

}
