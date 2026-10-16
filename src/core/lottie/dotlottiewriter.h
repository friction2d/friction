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
*/

#ifndef DOTLOTTIEWRITER_H
#define DOTLOTTIEWRITER_H

#include "core_global.h"

#include <QJsonObject>

class CORE_EXPORT DotLottieWriter
{
public:
    static void write(const QString& path,
                      const QString& animationId,
                      const QString& animationName,
                      QJsonObject animation,
                      const QString& assetsPath);
};

#endif // DOTLOTTIEWRITER_H
