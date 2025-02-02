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

#ifndef BOXTARGETPROPERTY_H
#define BOXTARGETPROPERTY_H
#include <QtCore>
#include "Properties/property.h"
#include "conncontextptr.h"
class BoundingBox;
class QrealAnimator;

class BoxTargetProperty;

class CORE_EXPORT BoxTargetProperty : public Property {
    Q_OBJECT
    e_OBJECT
protected:
    BoxTargetProperty(const QString& name);

    QDomElement prp_writePropertyXML_impl(const Friction::Core::XmlExporter& exp) const;
    void prp_readPropertyXML_impl(const QDomElement& ele, const Friction::Core::XmlImporter& imp);
public:
    bool SWT_dropSupport(const QMimeData* const data);
    bool SWT_drop(const QMimeData* const data);

    void prp_writeProperty_impl(eWriteStream &dst) const;
    void prp_readProperty_impl(eReadStream& src);

    BoundingBox *getTarget() const;
    void setTarget(BoundingBox * const box);
    void setTargetAction(BoundingBox * const box);

    using Validator = std::function<bool(BoundingBox*)>;

    const auto& validator() const
    { return mValidator; }

    template <typename T>
    void setValidator();
    void setValidator(const Validator& validator)
    { mValidator = validator; }
signals:
    void setActionStarted(BoundingBox*, BoundingBox*);
    void setActionFinished(BoundingBox*, BoundingBox*);
    void targetSet(BoundingBox*);
private:
    std::function<bool(BoundingBox*)> mValidator = nullptr;
    ConnContextQPtr<BoundingBox> mTarget_d;
};

template <typename T>
void BoxTargetProperty::setValidator() {
    const auto validator = [](BoundingBox* const box) {
        return enve_cast<T*>(box);
    };
    setValidator(validator);
}

#endif // BOXTARGETPROPERTY_H
