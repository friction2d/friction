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

#ifndef PATHEFFECTMENUCREATOR_H
#define PATHEFFECTMENUCREATOR_H

#include "core_global.h"

#include <functional>
#include "smartPointers/selfref.h"

class PathEffect;

struct CORE_EXPORT PathEffectMenuCreator
{
    template <typename T> using Func = std::function<T>;
    template <typename T> using Creator = Func<qsptr<T>()>;
    using EffectCreator = Creator<PathEffect>;
    using EffectAdder = Func<void(const QString&,
                                  const EffectCreator&)>;
    static void forEveryEffect(const EffectAdder& add);
};

#endif // PATHEFFECTMENUCREATOR_H
