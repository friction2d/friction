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

#ifndef PROPERTYBINDING_H
#define PROPERTYBINDING_H

#include "propertybindingbase.h"

#include "conncontextptr.h"
#include "simpletask.h"
#include "smartPointers/ememory.h"

class CORE_EXPORT PropertyBinding : public PropertyBindingBase {
public:
    using Validator = std::function<bool(Property*)>;
private:
    PropertyBinding(const Validator& validator,
                    const Property* const context);
public:
    static qsptr<PropertyBinding> sCreate(const QString& binding,
                                          const Validator& validator,
                                          const Property* const context);
    static qsptr<PropertyBinding> sCreate(Property* const prop);
    template <class T>
    static bool sValidateClass(const Validator& validator,
                               Property* const prop);

    template <class T>
    static Validator sWrapValidatorForClass(const Validator& validator);

    QJSValue getJSValue(QJSEngine& e);
    QJSValue getJSValue(QJSEngine& e, const qreal relFrame);
    QJSValue getJSValueAtAbsFrame(QJSEngine& e, const qreal absFrame);

    FrameRange identicalRelRange(const int absFrame);
    FrameRange nextNonUnaryIdenticalRelRange(const int absFrame);

    QString path() const { return mPath; }

    bool dependsOn(const Property* const prop);
    bool isValid() const { return mBindPathValid; }

    void setPath(const QString& path);
    Property* getBindProperty() const { return mBindProperty.get(); }
private:
    static Property* sFindPropertyToBind(const QString& binding,
                                         const Validator& validator,
                                         const Property* const context);
    void reloadBindProperty();
    void updateBindPath();
    bool bindProperty(const QString& path, Property * const newBinding);
    void setBindPathValid(const bool valid);
    SimpleTaskScheduler pathChanged;

    bool mBindPathValid = false;
    QString mPath;
    ConnContextQPtr<Property> mBindProperty;
    const Validator mValidator;
};

template <class T>
bool PropertyBinding::sValidateClass(const Validator& validator,
                                     Property* const prop) {
    return enve_cast<T*>(prop) && (!validator || validator(prop));
}

template <class T>
PropertyBinding::Validator
PropertyBinding::sWrapValidatorForClass(const Validator& validator) {
    return [validator](Property* const prop) {
        return sValidateClass<T>(validator, prop);
    };
}

#endif // PROPERTYBINDING_H
