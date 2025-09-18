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

#include "paintsettingsanimator.h"
#include "undoredo.h"
#include "colorhelpers.h"
#include "skia/skqtconversions.h"
#include "skia/skiaincludes.h"
#include "Boxes/pathbox.h"
#include "gradientpoints.h"
#include "gradient.h"
#include "Private/document.h"
#include "canvas.h"
#include "transformanimator.h"
#include "simpletask.h"
#include "qpointfanimator.h"

PaintSettingsAnimator::PaintSettingsAnimator(const QString &name,
                                             BoundingBox * const parent) :
    ComplexAnimator(name),
    mTarget_k(parent) {
    mGradientPoints = enve::make_shared<GradientPoints>(parent);
    mGradientTransform = enve::make_shared<AdvancedTransformAnimator>();
}

void PaintSettingsAnimator::prp_writeProperty_impl(eWriteStream& dst) const {
    mColor->prp_writeProperty_impl(dst);
    dst.write(&mPaintType, sizeof(PaintType));
    dst.write(&mGradientType, sizeof(GradientType));
    const int gradRWId = mGradient ? mGradient->getReadWriteId() : -1;
    const int gradDocId = mGradient ? mGradient->getDocumentId() : -1;
    dst << gradRWId;
    dst << gradDocId;
    mGradientPoints->prp_writeProperty_impl(dst);
    mGradientTransform->prp_writeProperty_impl(dst);
}

void PaintSettingsAnimator::prp_readProperty_impl(eReadStream& src)
{
    mColor->prp_readProperty_impl(src);
    PaintType paintType;
    src.read(&paintType, sizeof(PaintType));
    src.read(&mGradientType, sizeof(GradientType));
    int gradRWId; src >> gradRWId;
    int gradDocId; src >> gradDocId;
    SimpleTask::sScheduleContexted(this, [this, gradRWId, gradDocId]() {
        auto parentScene = getParentScene();
        if (!parentScene) { return; }
        SceneBoundGradient* gradient = nullptr;
        if (gradRWId != -1) {
            gradient = parentScene->getGradientWithRWId(gradRWId);
        }
        if (!gradient && gradDocId != -1) {
            gradient = parentScene->getGradientWithDocumentId(gradDocId);
            if (!gradient) { // gradient is in a different scene
                gradient = parentScene->getGradientWithDocumentSceneId(gradDocId);
                if (gradient) {
                    const auto newGrad = parentScene->createNewGradient();
                    const auto clipboard = enve::make_shared<PropertyClipboard>(gradient);
                    clipboard->paste(newGrad);
                    gradient = newGrad;
                }
            }
        }
        setGradientVar(gradient);
    });

    mGradientPoints->prp_readProperty_impl(src);
    if (src.evFileVersion() > 7) {
        mGradientTransform->prp_readProperty_impl(src);
    }
    setPaintType(paintType);
}

QDomElement PaintSettingsAnimator::prp_writePropertyXML_impl(const Friction::Core::XmlExporter& exp) const {
    auto result = exp.createElement(prp_tagNameXML());

    result.setAttribute("type", static_cast<int>(mPaintType));

    switch(mPaintType) {
    case PaintType::FLATPAINT: {
        const auto color = mColor->prp_writePropertyXML(exp);
        result.appendChild(color);
    } break;
    case PaintType::BRUSHPAINT: {
        const auto brushSettings = writeBrushPaint(exp);
        result.appendChild(brushSettings);
    } break;
    case PaintType::GRADIENTPAINT: {
        auto gradient = exp.createElement("Gradient");
        gradient.setAttribute("type", static_cast<int>(mGradientType));
        const int gradRWId = mGradient ? mGradient->getReadWriteId() : -1;
        gradient.setAttribute("id", gradRWId);
        result.appendChild(gradient);

        const auto transform = mGradientTransform->prp_writePropertyXML(exp);
        result.appendChild(transform);

        const auto gradPoints = mGradientPoints->prp_writeNamedPropertyXML(
                                    "GradientPoints", exp);
        result.appendChild(gradPoints);
    } break;
    default: break;
    }

    return result;
}


void PaintSettingsAnimator::prp_readPropertyXML_impl(const QDomElement& ele,
                                                const Friction::Core::XmlImporter& imp) {
    const auto typeStr = ele.attribute("type");
    const int typeInt = Friction::Core::XmlExportHelpers::stringToInt(typeStr);
    const auto type = static_cast<PaintType>(typeInt);
    switch(type) {
    case PaintType::FLATPAINT: {
        const auto color = ele.firstChildElement("Color");
        mColor->prp_readPropertyXML(color, imp);
    } break;
    case PaintType::BRUSHPAINT: {
        readBrushPaint(ele, imp);
    } break;
    case PaintType::GRADIENTPAINT: {
        const auto gradient = ele.firstChildElement("Gradient");

        const auto typeStr = gradient.attribute("type");
        const int typeInt = Friction::Core::XmlExportHelpers::stringToInt(typeStr);
        const auto type = static_cast<GradientType>(typeInt);
        mGradientType = type;

        const auto gradIdStr = gradient.attribute("id");
        const int gradId = Friction::Core::XmlExportHelpers::stringToInt(gradIdStr);

        SimpleTask::sScheduleContexted(this, [this, gradId]() {
            const auto parentScene = getParentScene();
            if(!parentScene) return;
            const auto gradient = parentScene->getGradientWithRWId(gradId);
            setGradientVar(gradient);
        });

        const auto transform = ele.firstChildElement("Transform");
        mGradientTransform->prp_readPropertyXML(transform, imp);

        const auto gradPoints = ele.firstChildElement("GradientPoints");
        mGradientPoints->prp_readPropertyXML(gradPoints, imp);
    } break;
    default: break;
    }
    setPaintType(type);
}

void PaintSettingsAnimator::updateGradientPoint() {
    if(mPaintType == GRADIENTPAINT && mGradient) {
        mGradientPoints->setColors(mGradient->getFirstQGradientStopQColor(),
                                   mGradient->getLastQGradientStopQColor());
        mGradientPoints->enable();
    } else {
        mGradientPoints->disable();
    }
}

void PaintSettingsAnimator::setGradientVar(Gradient * const grad) {
    if(grad == mGradient) return;
    if(mGradient) {
        ca_removeChild(mGradient->ref<Gradient>());
        ca_removeChild(mGradientPoints);
        ca_removeChild(mGradientTransform);
    }
    auto& conn = mGradient.assign(grad);
    if(grad) {
        ca_addChild(grad->ref<Gradient>());
        ca_addChild(mGradientPoints);
        ca_addChild(mGradientTransform);
        conn << connect(grad, &Gradient::prp_currentFrameChanged,
                        this, [this]() { updateGradientPoint(); });
        conn << connect(grad, &Gradient::removed,
                        this, [this]() { setGradient(nullptr); });
    }
    updateGradientPoint();
    mTarget_k->requestGlobalFillStrokeUpdateIfSelected();
}

QColor PaintSettingsAnimator::getColor() const {
    return mColor->getColor();
}

QColor PaintSettingsAnimator::getColor(const qreal relFrame) const {
    return mColor->getColor(relFrame);
}

void PaintSettingsAnimator::setGradientType(const GradientType type) {
    if(mGradientType == type) return;
    {
        UndoRedo ur;
        const auto oldValue = mGradientType;
        const auto newValue = type;
        ur.fUndo = [this, oldValue]() {
            setGradientType(oldValue);
        };
        ur.fRedo = [this, newValue]() {
            setGradientType(newValue);
        };
        prp_addUndoRedo(ur);
    }
    mGradientType = type;
    prp_afterWholeInfluenceRangeChanged();
}

QMatrix PaintSettingsAnimator::getGradientTransform(const qreal relFrame) const {
    return mGradientTransform->getRelativeTransformAtFrame(relFrame);
}

void PaintSettingsAnimator::setGradientTransform(const TransformValues &transform) {
    mGradientTransform->setValues(transform);
}

void PaintSettingsAnimator::setupPaintSettings(const qreal relFrame,
                                               UpdatePaintSettings &settings) {
    const qreal absFrame = prp_relFrameToAbsFrameF(relFrame);

    settings.fPaintColor = getColor(relFrame);
    settings.fPaintType = mPaintType;
    if(mGradient && mPaintType == PaintType::GRADIENTPAINT) {
        const auto gradientStops = mGradient->getQGradientStops(absFrame);
        const auto startPoint = mGradientPoints->getStartPoint(relFrame);
        const auto endPoint = mGradientPoints->getEndPoint(relFrame);
        const auto gradientType = getGradientType();
        const auto gradientTransform = getGradientTransform(relFrame);
        settings.updateGradient(gradientStops, startPoint, endPoint,
                                    gradientType, gradientTransform);
    }
}

void PaintSettingsAnimator::duplicatePaintSettingsNotAnim(
        PaintSettingsAnimator * const settings) {
    if(!settings) {
        setPaintType(NOPAINT);
    } else {
        const PaintType paintType = settings->getPaintType();
        setPaintType(paintType);
        if(paintType == FLATPAINT) {
            setCurrentColor(settings->getColor());
        } else if(paintType == GRADIENTPAINT) {
            setGradient(settings->getGradient());
            setGradientType(settings->getGradientType());
        }
    }
}

void PaintSettingsAnimator::applyTransform(const QMatrix &transform) {
    mGradientPoints->applyTransform(transform);
}

#include "Expressions/expression.h"
#include "svgexporthelpers.h"

void PaintSettingsAnimator::saveSVG(SvgExporter& exp,
                                    QDomElement& parent,
                                    const FrameRange& visRange,
                                    const QString& name) const {
    if(mPaintType == NOPAINT) {
        parent.setAttribute(name, "none");
    } else if(mPaintType == GRADIENTPAINT) {
        if(mGradient) {
            const auto baseGradId = SvgExportHelpers::ptrToStr(mGradient);
            const auto thisGradId = SvgExportHelpers::ptrToStr(this);

            QDomElement grad;

            const auto p1 = mGradientPoints->startAnimator();
            const auto p2 = mGradientPoints->endAnimator();

            const auto x1 = p1->getXAnimator();
            const auto y1 = p1->getYAnimator();

            const auto x2 = p2->getXAnimator();
            const auto y2 = p2->getYAnimator();

            switch(mGradientType) {
            case GradientType::LINEAR: {
                grad = exp.createElement("linearGradient");

                x1->saveQrealSVG(exp, grad, visRange, "x1");
                y1->saveQrealSVG(exp, grad, visRange, "y1");
                x2->saveQrealSVG(exp, grad, visRange, "x2");
                y2->saveQrealSVG(exp, grad, visRange, "y2");
            } break;
            case GradientType::RADIAL: {
                grad = exp.createElement("radialGradient");

//                const QPointF distPt = p2 - p1;
//                const qreal radius = qSqrt(pow2(distPt.x()) + pow2(distPt.y()));
                x1->saveQrealSVG(exp, grad, visRange, "cx");
                y1->saveQrealSVG(exp, grad, visRange, "cy");

                PropertyBindingMap bindings;

                bindings.insert({"p1", PropertyBinding::sCreate(p1)});
                bindings.insert({"p2", PropertyBinding::sCreate(p2)});

                const auto rScript = "var distPt = [p2[0] - p1[0], p2[1] - p1[1]];"
                                     "return Math.sqrt(distPt[0]*distPt[0] + "
                                                      "distPt[1]*distPt[1]);";

                auto engine = std::make_unique<QJSEngine>();
                Expression::sAddDefinitionsTo("", *engine);
                QJSValue eEvaluate;
                Expression::sAddScriptTo(rScript, bindings, *engine, eEvaluate,
                                         Expression::sQrealAnimatorTester);
                const auto rExpr = Expression::sCreate("", rScript,
                                                       std::move(bindings),
                                                       std::move(engine),
                                                       std::move(eEvaluate));

                const auto rAnim = enve::make_shared<QrealAnimator>("");
                rAnim->setExpression(rExpr);

                rAnim->saveQrealSVG(exp, grad, visRange, "r");
            } break;
            }
            grad.setAttribute("gradientUnits", "userSpaceOnUse");
            grad.setAttribute("xlink:href", "#" + baseGradId);
            grad.setAttribute("id", thisGradId);
            exp.addToDefs(grad);
            parent.setAttribute(name, QString("url(#%1)").arg(thisGradId));
        } else {
            parent.setAttribute(name, "#000000");
        }
    } else {
        mColor->saveColorSVG(exp, parent, visRange, name);
    }
}

PaintType PaintSettingsAnimator::getPaintType() const {
    return mPaintType;
}

Gradient *PaintSettingsAnimator::getGradient() const {
    return *mGradient;
}

void PaintSettingsAnimator::resetGradientPoints() {
    const QRectF relRect = mTarget_k->getRelBoundingRect();
    mGradientPoints->anim_setRecording(false);
    mGradientPoints->setPositions(relRect.topLeft(),
                                  relRect.bottomRight());
}

void PaintSettingsAnimator::setGradient(Gradient* gradient) {
    if(gradient == mGradient) return;
    if(gradient && !mGradient) resetGradientPoints();
    {
        UndoRedo ur;
        const qptr<Gradient> oldValue = *mGradient;
        const qptr<Gradient> newValue = gradient;
        ur.fUndo = [this, oldValue]() {
            setGradientVar(oldValue);
        };
        ur.fRedo = [this, newValue]() {
            setGradientVar(newValue);
        };
        prp_addUndoRedo(ur);
    }
    setGradientVar(gradient);
}

void PaintSettingsAnimator::setCurrentColor(const QColor &color,
                                            const bool &history)
{
    if (history) {
        UndoRedo ur;
        const auto oldValue = mColor->getColor();
        const auto newValue = color;
        ur.fUndo = [this, oldValue]() { setCurrentColor(oldValue); };
        ur.fRedo = [this, newValue]() { setCurrentColor(newValue); };
        prp_addUndoRedo(ur);
    }
    mColor->setColor(color);
}

void PaintSettingsAnimator::showHideChildrenBeforeChangingPaintType(
        const PaintType newPaintType) {
    if(mPaintType == GRADIENTPAINT)
        setGradient(nullptr);
    if(mPaintType == FLATPAINT || mPaintType == BRUSHPAINT)
        ca_removeChild(mColor);
    if(newPaintType == FLATPAINT || newPaintType == BRUSHPAINT)
        ca_addChild(mColor);
}

void PaintSettingsAnimator::setPaintType(const PaintType paintType)
{
    if (paintType == mPaintType) { return; }
    {
        UndoRedo ur;
        const auto oldValue = mPaintType;
        const auto newValue = paintType;
        ur.fUndo = [this, oldValue]() {
            setPaintType(oldValue);
        };
        ur.fRedo = [this, newValue]() {
            setPaintType(newValue);
        };
        prp_addUndoRedo(ur);
    }

    showHideChildrenBeforeChangingPaintType(paintType);

    mPaintType = paintType;
    updateGradientPoint();
    mTarget_k->requestGlobalFillStrokeUpdateIfSelected();
    prp_afterWholeInfluenceRangeChanged();

    SWT_setDisabled(paintType == PaintType::NOPAINT);

    emit paintTypeChanged(mPaintType);
}

ColorAnimator *PaintSettingsAnimator::getColorAnimator() {
    return mColor.data();
}

void PaintSettingsAnimator::setGradientPointsPos(const QPointF &pt1,
                                                 const QPointF &pt2) {
    if(!mGradientPoints) return;
    mGradientPoints->setPositions(pt1, pt2);
}

UpdatePaintSettings::UpdatePaintSettings(const QColor &paintColor,
                                         const PaintType paintType) {
    fPaintColor = paintColor;
    fPaintType = paintType;
}

UpdatePaintSettings::UpdatePaintSettings() {}

UpdatePaintSettings::~UpdatePaintSettings() {}

void UpdatePaintSettings::applyPainterSettingsSk(
        SkPaint& paint, const float opacity) {
    if(fPaintType == GRADIENTPAINT) {
        paint.setShader(fGradient);
        paint.setAlphaf(opacity);
    } else if(fPaintType == FLATPAINT) {
        paint.setColor(toSkColor(fPaintColor));
        paint.setAlphaf(paint.getAlphaf()*opacity);
    } else {
        paint.setColor(SkColorSetARGB(0, 0, 0, 0));
    }
}

void UpdatePaintSettings::updateGradient(const QGradientStops &stops,
                                         const QPointF &start,
                                         const QPointF &finalStop,
                                         const GradientType gradientType,
                                         const QMatrix& transform) {
    const int nStops = stops.count();
    QVector<SkPoint> gradPoints(nStops);
    QVector<SkColor> gradColors(nStops);
    QVector<float> gradPos(nStops);

    const QMatrix invertedTransform = transform.inverted();
    const QPointF mappedStart = invertedTransform.map(start);
    const QPointF mappedEnd = invertedTransform.map(finalStop);

    const float xInc = static_cast<float>(mappedEnd.x() - mappedStart.x());
    const float yInc = static_cast<float>(mappedEnd.y() - mappedStart.y());
    float currX = static_cast<float>(mappedStart.x());
    float currY = static_cast<float>(mappedStart.y());
    float currT = 0;
    const float tInc = 1.f/(nStops - 1);

    for(int i = 0; i < nStops; i++) {
        const QGradientStop &stopT = stops.at(i);
        const QColor col = stopT.second;
        gradPoints[i] = SkPoint::Make(currX, currY);
        gradColors[i] = toSkColor(col);
        gradPos[i] = currT;

        currX += xInc;
        currY += yInc;
        currT += tInc;
    }
    const SkMatrix skTransform = toSkMatrix(transform);
    if(gradientType == GradientType::LINEAR) {
        fGradient = SkGradientShader::MakeLinear(gradPoints.data(),
                                                 gradColors.data(),
                                                 gradPos.data(), nStops,
                                                 SkTileMode::kClamp,
                                                 0, &skTransform);
    } else {
        const QPointF distPt = mappedEnd - mappedStart;
        const qreal radius = qSqrt(pow2(distPt.x()) + pow2(distPt.y()));
        fGradient = SkGradientShader::MakeRadial(
                        toSkPoint(start), toSkScalar(radius),
                        gradColors.data(), gradPos.data(),
                        nStops, SkTileMode::kClamp,
                        0, &skTransform);
    }
}

UpdateStrokeSettings::UpdateStrokeSettings(const qreal width,
                   const QColor &paintColorT,
                   const PaintType paintTypeT,
                   const QPainter::CompositionMode &outlineCompositionModeT) :
    UpdatePaintSettings(paintColorT, paintTypeT), fOutlineWidth(width) {
    fOutlineCompositionMode = outlineCompositionModeT;
}

UpdateStrokeSettings::UpdateStrokeSettings() {}
