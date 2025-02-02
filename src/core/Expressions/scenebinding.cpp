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

#include "scenebinding.h"

SceneBinding::SceneBinding(const Property* const context,
                           const SceneBinding::SceneBindingType &binding)
    : PropertyBindingBase(context)
    , mBindingType(binding)
{

}

qsptr<SceneBinding> SceneBinding::sCreate(const Property* const context,
                                          const SceneBindingType &binding)
{
    const auto result = new SceneBinding(context, binding);
    return qsptr<SceneBinding>(result);
}
    
QJSValue SceneBinding::getJSValue(QJSEngine& e)
{
    Q_UNUSED(e);
    if (mContext) {
        qreal val = 0.0;
        switch (mBindingType) {
        case SceneBindingFps:
            val = mContext->prp_getSceneFPS();
            break;
        case SceneBindingWidth:
            val = mContext->prp_getSceneWidth();
            break;
        case SceneBindingHeight:
            val = mContext->prp_getSceneHeight();
            break;
        case SceneBindingRangeMin:
            val = mContext->prp_getSceneRangeMin();
            break;
        case SceneBindingRangeMax:
            val = mContext->prp_getSceneRangeMax();
            break;
        }
        return QJSValue(val);
    }
    return QJSValue::NullValue;
}

QJSValue SceneBinding::getJSValue(QJSEngine& e,
                                  const qreal relFrame)
{
    Q_UNUSED(relFrame);
    return getJSValue(e);
}

FrameRange SceneBinding::identicalRelRange(const int absFrame)
{
    Q_UNUSED(absFrame);
    return FrameRange::EMINMAX;
}

FrameRange SceneBinding::nextNonUnaryIdenticalRelRange(const int absFrame)
{
    Q_UNUSED(absFrame);
    return FrameRange::EMINMAX;
}

QString SceneBinding::path() const
{
    switch (mBindingType) {
    case SceneBindingFps:
        return "$scene.fps";
    case SceneBindingWidth:
        return "$scene.width";
    case SceneBindingHeight:
        return "$scene.height";
    case SceneBindingRangeMin:
        return "$scene.rangeMin";
    case SceneBindingRangeMax:
        return "$scene.rangeMax";
    }
    return QString();
}
