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

#ifndef FRICTION_SVGO_H
#define FRICTION_SVGO_H

#include "core_global.h"

#include <QDomDocument>
#include <QString>

namespace Friction
{
    namespace Core
    {
        class CORE_EXPORT SVGO
        {
        public:
            static QString optimize(const QString &svg);

        private:
            static bool recursiveOptimize(QDomElement &element);
            static void removeUselessDefs(QDomDocument &doc);
            static void removeProcessingInstructions(QDomDocument &doc);
            static int countElementChildren(const QDomElement &element);
            static QString minify(const QString &xml);
        };
    }
}

#endif // FRICTION_SVGO_H
