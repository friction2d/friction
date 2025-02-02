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

#ifndef SCENE_BINDING_H
#define SCENE_BINDING_H

#include "propertybindingbase.h"

class CORE_EXPORT SceneBinding : public PropertyBindingBase
{
public:
    enum SceneBindingType {
        SceneBindingFps,
        SceneBindingWidth,
        SceneBindingHeight,
        SceneBindingRangeMin,
        SceneBindingRangeMax
    };

    SceneBinding(const Property* const context,
                 const SceneBindingType &binding);

    static qsptr<SceneBinding> sCreate(const Property* const context,
                                       const SceneBindingType &binding);

    QJSValue getJSValue(QJSEngine& e);
    QJSValue getJSValue(QJSEngine& e,
                        const qreal relFrame);

    FrameRange identicalRelRange(const int absFrame);
    FrameRange nextNonUnaryIdenticalRelRange(const int absFrame);
    QString path() const;

private:
    SceneBindingType mBindingType;
};

#endif // SCENE_BINDING_H
