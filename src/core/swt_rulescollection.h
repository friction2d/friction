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

#ifndef SWT_RULESCOLLECTION_H
#define SWT_RULESCOLLECTION_H

#include <QString>

#include "core_global.h"

enum class SWT_BoxRule : short {
    all,
    selected,
    visible,
    hidden,
    locked,
    unlocked,
    animated,
    notAnimated
};

enum class SWT_Target : short {
    canvas,
    group,
    all
};

enum class SWT_Type : short {
    all,
    graphics,
    sound
};

enum class SWT_ParamRule : short {
    all,
    animated,
    animatedOnly
};

struct CORE_EXPORT SWT_RulesCollection {
    SWT_RulesCollection();
    SWT_RulesCollection(const SWT_BoxRule rule,
                        const bool alwaysShowChildren,
                        const SWT_Target target,
                        const SWT_Type type,
                        const SWT_ParamRule paramRule,
                        const QString &searchString);

    SWT_BoxRule fRule = SWT_BoxRule::all;
    bool fAlwaysShowChildren = false;
    SWT_Target fTarget = SWT_Target::canvas;
    SWT_Type fType = SWT_Type::all;
    SWT_ParamRule fParamRule = SWT_ParamRule::all;
    QString fSearchString = "";
};

#endif // SWT_RULESCOLLECTION_H
