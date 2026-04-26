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

#include "canvas.h"

#include "MovablePoints/pathpivot.h"
#include "eevent.h"
#include "Private/document.h"

using namespace Friction::Core;

void Canvas::renderGizmos(SkCanvas * const canvas,
                          const qreal qInvZoom,
                          const float invZoom)
{
    if (!mGizmos.fState.visible) { return; }
    updateRotateHandleGeometry(qInvZoom);
    auto drawAxisLine = [&](const Gizmos::LineGeometry &geom,
                             const QColor &baseColor,
                             Gizmos::AxisConstraint axis,
                             bool hovered,
                             bool horizontal) {
        if (!geom.visible || geom.strokeWidth <= 0.0) { return; }

        const bool active = (mGizmos.fState.axisConstraint == axis);
        QColor color = baseColor;

        const qreal strokeAlpha = (active || hovered)
                                  ? mGizmos.fTheme.colorAlphaFillHover/2
                                  : mGizmos.fTheme.colorAlphaFillNormal/2;
        color.setAlpha(static_cast<int>(strokeAlpha));

        SkPaint linePaint;
        linePaint.setAntiAlias(true);
        linePaint.setStyle(SkPaint::kStroke_Style);
        linePaint.setStrokeCap(SkPaint::kRound_Cap);
        linePaint.setStrokeWidth(toSkScalar(geom.strokeWidth));
        linePaint.setColor(toSkColor(color));

        QPointF startPoint = geom.start;
        QPointF endPoint = geom.end;

        SkIRect deviceClip;
        if (canvas->getDeviceClipBounds(&deviceClip)) {
            SkRect deviceRect = SkRect::Make(deviceClip);
            SkMatrix invMatrix;
            if (canvas->getTotalMatrix().invert(&invMatrix)) {
                SkRect worldRect = invMatrix.mapRect(deviceRect);
                if (horizontal) {
                    startPoint.setX(worldRect.left());
                    startPoint.setY(geom.start.y());
                    endPoint.setX(worldRect.right());
                    endPoint.setY(geom.start.y());
                } else {
                    startPoint.setY(worldRect.top());
                    endPoint.setY(worldRect.bottom());
                    startPoint.setX(geom.start.x());
                    endPoint.setX(geom.start.x());
                }
            }
        }

        canvas->drawLine(toSkScalar(startPoint.x()),
                         toSkScalar(startPoint.y()),
                         toSkScalar(endPoint.x()),
                         toSkScalar(endPoint.y()),
                         linePaint);
    };


    if (mGizmos.fState.rotateHandleVisible) {
        if (mGizmos.fState.showRotate) {
            if (mGizmos.fState.rotateHandlePolygon.size() >= 3) {
                QColor fillColor = mGizmos.fTheme.colorZ;
                fillColor.setAlpha(mGizmos.fState.rotateHandleHovered ? static_cast<int>(mGizmos.fTheme.colorAlphaFillHover)
                                                                      : static_cast<int>(mGizmos.fTheme.colorAlphaFillNormal));

                SkPaint fillPaint;
                fillPaint.setAntiAlias(true);
                fillPaint.setStyle(SkPaint::kFill_Style);
                fillPaint.setColor(toSkColor(fillColor));

                SkPaint borderPaint;
                borderPaint.setAntiAlias(true);
                borderPaint.setStyle(SkPaint::kStroke_Style);
                borderPaint.setStrokeJoin(SkPaint::kRound_Join);
                borderPaint.setStrokeCap(SkPaint::kRound_Cap);
                borderPaint.setStrokeWidth(toSkScalar(mGizmos.fConfig.rotateStrokePx * qInvZoom * 0.2f));
                const int strokeLighten = mGizmos.fState.rotateHandleHovered ? mGizmos.fTheme.colorLightenHover : mGizmos.fTheme.colorLightenNormal;
                QColor strokeColor = mGizmos.fTheme.colorZ.darker(strokeLighten);
                strokeColor.setAlpha(mGizmos.fState.rotateHandleHovered ? static_cast<int>(mGizmos.fTheme.colorAlphaStrokeHover)
                                                                        : static_cast<int>(mGizmos.fTheme.colorAlphaStrokeNormal));
                borderPaint.setColor(toSkColor(strokeColor));

                SkPath path;
                bool first = true;
                for (const QPointF &pt : mGizmos.fState.rotateHandlePolygon) {
                    const SkPoint skPt = SkPoint::Make(toSkScalar(pt.x()), toSkScalar(pt.y()));
                    if (first) {
                        path.moveTo(skPt);
                        first = false;
                    } else {
                        path.lineTo(skPt);
                    }
                }
                path.close();

                canvas->drawPath(path, fillPaint);
                canvas->drawPath(path, borderPaint);
            }
        }

        auto drawAxisRect = [&](Gizmos::AxisConstraint handle,
                                const Gizmos::AxisGeometry &geom,
                                const QColor &baseColor) {
            if (!geom.visible) { return; }
            bool hovered = false;
            switch (handle) {
            case Gizmos::AxisConstraint::X: hovered = mGizmos.fState.axisXHovered; break;
            case Gizmos::AxisConstraint::Y: hovered = mGizmos.fState.axisYHovered; break;
            case Gizmos::AxisConstraint::Uniform: hovered = mGizmos.fState.axisUniformHovered; break;
            case Gizmos::AxisConstraint::None: default: hovered = false; break;
            }
            const bool active = (mGizmos.fState.axisConstraint == handle);
            QColor color = baseColor;
            if (active) {
                color = color.lighter(135);
            } else if (hovered) {
                color = color.lighter(120);
            }

            SkPaint fillPaint;
            fillPaint.setAntiAlias(true);
            fillPaint.setStyle(SkPaint::kFill_Style);
            fillPaint.setColor(toSkColor(color));

            SkPaint borderPaint;
            borderPaint.setAntiAlias(true);
            borderPaint.setStyle(SkPaint::kStroke_Style);
            borderPaint.setStrokeWidth(toSkScalar(mGizmos.fConfig.rotateStrokePx * invZoom * 0.2f));

            const int borderLighten = hovered ? mGizmos.fTheme.colorLightenHover : mGizmos.fTheme.colorLightenNormal;
            QColor borderColor = color.darker(borderLighten);
            const qreal strokeAlphaAxis = hovered ? mGizmos.fTheme.colorAlphaStrokeHover : mGizmos.fTheme.colorAlphaStrokeNormal;
            borderColor.setAlpha(static_cast<int>(strokeAlphaAxis));
            borderPaint.setColor(toSkColor(borderColor));

            SkPath path;
            if (geom.usePolygon && geom.polygonPoints.size() >= 3) {
                bool first = true;
                for (const QPointF &pt : geom.polygonPoints) {
                    const SkPoint skPt = SkPoint::Make(toSkScalar(pt.x()), toSkScalar(pt.y()));
                    if (first) {
                        path.moveTo(skPt);
                        first = false;
                    } else {
                        path.lineTo(skPt);
                    }
                }
                path.close();
            } else {
                const qreal halfW = geom.size.width() * 0.5;
                const qreal halfH = geom.size.height() * 0.5;
                const qreal angleRadGeom = qDegreesToRadians(geom.angleDeg);
                const qreal cosG = std::cos(angleRadGeom);
                const qreal sinG = std::sin(angleRadGeom);
                auto mapPoint = [&](qreal localX, qreal localY) -> SkPoint {
                    const qreal worldX = geom.center.x() + localX * cosG - localY * sinG;
                    const qreal worldY = geom.center.y() + localX * sinG + localY * cosG;
                    return SkPoint::Make(toSkScalar(worldX), toSkScalar(worldY));
                };

                path.moveTo(mapPoint(-halfW, -halfH));
                path.lineTo(mapPoint(halfW, -halfH));
                path.lineTo(mapPoint(halfW, halfH));
                path.lineTo(mapPoint(-halfW, halfH));
                path.close();
            }

            canvas->drawPath(path, fillPaint);
            canvas->drawPath(path, borderPaint);
        };
        auto drawScaleSquare = [&](Gizmos::ScaleHandle handle,
                                   const Gizmos::ScaleGeometry &geom,
                                   const QColor &baseColor) {
            if (!geom.visible) { return; }
            bool hovered = false;
            switch (handle) {
            case Gizmos::ScaleHandle::X: hovered = mGizmos.fState.scaleXHovered; break;
            case Gizmos::ScaleHandle::Y: hovered = mGizmos.fState.scaleYHovered; break;
            case Gizmos::ScaleHandle::Uniform: hovered = mGizmos.fState.scaleUniformHovered; break;
            case Gizmos::ScaleHandle::None: default: hovered = false; break;
            }
            const bool active = (mGizmos.fState.scaleConstraint == handle);
            QColor color = baseColor;
            if (active) {
                color = color.lighter(135);
            } else if (hovered) {
                color = color.lighter(120);
            }

            SkPaint fillPaint;
            fillPaint.setAntiAlias(true);
            fillPaint.setStyle(SkPaint::kFill_Style);
            fillPaint.setColor(toSkColor(color));

            SkPaint borderPaint;
            borderPaint.setAntiAlias(true);
            borderPaint.setStyle(SkPaint::kStroke_Style);
            borderPaint.setStrokeWidth(toSkScalar(mGizmos.fConfig.rotateStrokePx * invZoom * 0.2f));
            const int borderLighten = hovered ? mGizmos.fTheme.colorLightenHover : mGizmos.fTheme.colorLightenNormal;
            QColor borderColor = color.darker(borderLighten);
            const qreal strokeAlphaScale = hovered ? mGizmos.fTheme.colorAlphaStrokeHover : mGizmos.fTheme.colorAlphaStrokeNormal;
            borderColor.setAlpha(static_cast<int>(strokeAlphaScale));
            borderPaint.setColor(toSkColor(borderColor));

            if (geom.usePolygon && geom.polygonPoints.size() >= 3) {
                SkPath path;
                bool first = true;
                for (const QPointF &pt : geom.polygonPoints) {
                    const SkPoint skPt = SkPoint::Make(toSkScalar(pt.x()), toSkScalar(pt.y()));
                    if (first) {
                        path.moveTo(skPt);
                        first = false;
                    } else {
                        path.lineTo(skPt);
                    }
                }
                path.close();
                canvas->drawPath(path, fillPaint);
                canvas->drawPath(path, borderPaint);
            } else {
                const SkRect skRect = SkRect::MakeLTRB(toSkScalar(geom.center.x() - geom.halfExtent),
                                                       toSkScalar(geom.center.y() - geom.halfExtent),
                                                       toSkScalar(geom.center.x() + geom.halfExtent),
                                                       toSkScalar(geom.center.y() + geom.halfExtent));
                canvas->drawRect(skRect, fillPaint);
                canvas->drawRect(skRect, borderPaint);
            }
        };

        auto drawShearCircle = [&](Gizmos::ShearHandle handle,
                                   const Gizmos::ShearGeometry &geom,
                                   const QColor &baseColor) {
            if (!geom.visible) { return; }
            bool hovered = (handle == Gizmos::ShearHandle::X) ? mGizmos.fState.shearXHovered : mGizmos.fState.shearYHovered;
            const bool active = (mGizmos.fState.shearConstraint == handle);
            QColor color = baseColor;
            if (active) {
                color = color.lighter(135);
            } else if (hovered) {
                color = color.lighter(120);
            }

            SkPaint fillPaint;
            fillPaint.setAntiAlias(true);
            fillPaint.setStyle(SkPaint::kFill_Style);
            fillPaint.setColor(toSkColor(color));

            SkPaint borderPaint;
            borderPaint.setAntiAlias(true);
            borderPaint.setStyle(SkPaint::kStroke_Style);
            borderPaint.setStrokeWidth(toSkScalar(mGizmos.fConfig.rotateStrokePx * invZoom * 0.2f));
            const int borderLighten = hovered ? mGizmos.fTheme.colorLightenHover : mGizmos.fTheme.colorLightenNormal;
            QColor borderColor = color.darker(borderLighten);
            const qreal strokeAlphaShear = hovered ? mGizmos.fTheme.colorAlphaStrokeHover : mGizmos.fTheme.colorAlphaStrokeNormal;
            borderColor.setAlpha(static_cast<int>(strokeAlphaShear));
            borderPaint.setColor(toSkColor(borderColor));

            if (geom.usePolygon && geom.polygonPoints.size() >= 3) {
                SkPath path;
                bool first = true;
                for (const QPointF &pt : geom.polygonPoints) {
                    const SkPoint skPt = SkPoint::Make(toSkScalar(pt.x()), toSkScalar(pt.y()));
                    if (first) {
                        path.moveTo(skPt);
                        first = false;
                    } else {
                        path.lineTo(skPt);
                    }
                }
                path.close();
                canvas->drawPath(path, fillPaint);
                canvas->drawPath(path, borderPaint);
            } else {
                const SkRect skRect = SkRect::MakeLTRB(toSkScalar(geom.center.x() - geom.radius),
                                                       toSkScalar(geom.center.y() - geom.radius),
                                                       toSkScalar(geom.center.x() + geom.radius),
                                                       toSkScalar(geom.center.y() + geom.radius));
                canvas->drawOval(skRect, fillPaint);
                canvas->drawOval(skRect, borderPaint);
            }
        };
        drawAxisRect(Gizmos::AxisConstraint::Y,
                     mGizmos.fState.axisYGeom,
                     QColor(mGizmos.fTheme.colorY.red(), mGizmos.fTheme.colorY.green(), mGizmos.fTheme.colorY.blue(),
                            mGizmos.fState.axisYHovered ? mGizmos.fTheme.colorAlphaFillHover : mGizmos.fTheme.colorAlphaFillNormal));
        drawAxisRect(Gizmos::AxisConstraint::X,
                     mGizmos.fState.axisXGeom,
                     QColor(mGizmos.fTheme.colorX.red(), mGizmos.fTheme.colorX.green(), mGizmos.fTheme.colorX.blue(),
                            mGizmos.fState.axisXHovered ? mGizmos.fTheme.colorAlphaFillHover : mGizmos.fTheme.colorAlphaFillNormal));
        drawAxisRect(Gizmos::AxisConstraint::Uniform,
                     mGizmos.fState.axisUniformGeom,
                     QColor(mGizmos.fTheme.colorZ.red(), mGizmos.fTheme.colorZ.green(), mGizmos.fTheme.colorZ.blue(),
                            mGizmos.fState.axisUniformHovered ? mGizmos.fTheme.colorAlphaFillHover : mGizmos.fTheme.colorAlphaFillNormal));
        drawScaleSquare(Gizmos::ScaleHandle::Y,
                        mGizmos.fState.scaleYGeom,
                        QColor(mGizmos.fTheme.colorY.red(), mGizmos.fTheme.colorY.green(), mGizmos.fTheme.colorY.blue(),
                               mGizmos.fState.scaleYHovered ? mGizmos.fTheme.colorAlphaFillHover : mGizmos.fTheme.colorAlphaFillNormal));
        drawScaleSquare(Gizmos::ScaleHandle::X,
                        mGizmos.fState.scaleXGeom,
                        QColor(mGizmos.fTheme.colorX.red(), mGizmos.fTheme.colorX.green(), mGizmos.fTheme.colorX.blue(),
                               mGizmos.fState.scaleXHovered ? mGizmos.fTheme.colorAlphaFillHover : mGizmos.fTheme.colorAlphaFillNormal));
        drawScaleSquare(Gizmos::ScaleHandle::Uniform,
                        mGizmos.fState.scaleUniformGeom,
                        QColor(mGizmos.fTheme.colorUniform.red(), mGizmos.fTheme.colorUniform.green(), mGizmos.fTheme.colorUniform.blue(),
                               mGizmos.fState.scaleUniformHovered ? mGizmos.fTheme.colorAlphaFillHover : mGizmos.fTheme.colorAlphaFillNormal));
        drawShearCircle(Gizmos::ShearHandle::Y,
                        mGizmos.fState.shearYGeom,
                        QColor(mGizmos.fTheme.colorY.red(), mGizmos.fTheme.colorY.green(), mGizmos.fTheme.colorY.blue(),
                               mGizmos.fState.shearYHovered ? mGizmos.fTheme.colorAlphaFillHover : mGizmos.fTheme.colorAlphaFillNormal));
        drawShearCircle(Gizmos::ShearHandle::X,
                        mGizmos.fState.shearXGeom,
                        QColor(mGizmos.fTheme.colorX.red(), mGizmos.fTheme.colorX.green(), mGizmos.fTheme.colorX.blue(),
                               mGizmos.fState.shearXHovered ? mGizmos.fTheme.colorAlphaFillHover : mGizmos.fTheme.colorAlphaFillNormal));
    }

    drawAxisLine(mGizmos.fState.xLineGeom,
                 mGizmos.fTheme.colorX,
                 Gizmos::AxisConstraint::X,
                 mGizmos.fState.axisXHovered,
                 true);
    drawAxisLine(mGizmos.fState.yLineGeom,
                 mGizmos.fTheme.colorY,
                 Gizmos::AxisConstraint::Y,
                 mGizmos.fState.axisYHovered,
                 false);
}

void Canvas::setGizmoVisibility(const Gizmos::Interact &ti,
                                const bool visibility)
{
    switch (ti) {
    case Gizmos::Interact::Position:
        if (mGizmos.fState.showPosition == visibility) { return; }
        mGizmos.fState.showPosition = visibility;
        if (!visibility) {
            mGizmos.fState.axisHandleActive = false;
            mGizmos.fState.axisConstraint = Gizmos::AxisConstraint::None;
            setAxisGizmoHover(Gizmos::AxisConstraint::X, false);
            setAxisGizmoHover(Gizmos::AxisConstraint::Y, false);
            setAxisGizmoHover(Gizmos::AxisConstraint::Uniform, false);
        }
        break;
    case Gizmos::Interact::Rotate:
        if (mGizmos.fState.showRotate == visibility) { return; }
        mGizmos.fState.showRotate = visibility;
        if (!visibility) {
            setRotateHandleHover(false);
            mGizmos.fState.rotatingFromHandle = false;
        }
        break;
    case Gizmos::Interact::Scale:
        if (mGizmos.fState.showScale == visibility) { return; }
        mGizmos.fState.showScale = visibility;
        if (!visibility) {
            mGizmos.fState.scaleHandleActive = false;
            mGizmos.fState.scaleConstraint = Gizmos::ScaleHandle::None;
            setScaleGizmoHover(Gizmos::ScaleHandle::X, false);
            setScaleGizmoHover(Gizmos::ScaleHandle::Y, false);
            setScaleGizmoHover(Gizmos::ScaleHandle::Uniform, false);
        }
        break;
    case Gizmos::Interact::Shear:
        if (mGizmos.fState.showShear == visibility) { return; }
        mGizmos.fState.showShear = visibility;
        if (!visibility) {
            mGizmos.fState.shearHandleActive = false;
            mGizmos.fState.shearConstraint = Gizmos::ShearHandle::None;
            setShearGizmoHover(Gizmos::ShearHandle::X, false);
            setShearGizmoHover(Gizmos::ShearHandle::Y, false);
        }
        break;
    case Gizmos::Interact::All:
        if (mGizmos.fState.visible == visibility) { return; }
        mGizmos.fState.visible = visibility;
        break;
    default: return;
    }

    if (!visibility) { setGizmosSuppressed(false); }
    emit requestUpdate();
}

bool Canvas::getGizmoVisibility(const Gizmos::Interact &ti)
{
    switch (ti) {
    case Gizmos::Interact::Position:
        return mGizmos.fState.showPosition;
    case Gizmos::Interact::Rotate:
        return mGizmos.fState.showRotate;
    case Gizmos::Interact::Scale:
        return mGizmos.fState.showScale;
    case Gizmos::Interact::Shear:
        return mGizmos.fState.showShear;
    case Gizmos::Interact::All:
        return mGizmos.fState.visible;
    default:;
    }
    return false;
}

void Canvas::updateRotateHandleHover(const QPointF &pos,
                                     qreal invScale)
{
    updateRotateHandleGeometry(invScale);
    setRotateHandleHover(pointOnRotateGizmo(pos, invScale));
    setAxisGizmoHover(Gizmos::AxisConstraint::X,
                      pointOnAxisGizmo(Gizmos::AxisConstraint::X,
                                       pos, invScale));
    setAxisGizmoHover(Gizmos::AxisConstraint::Y,
                      pointOnAxisGizmo(Gizmos::AxisConstraint::Y,
                                       pos, invScale));
    setAxisGizmoHover(Gizmos::AxisConstraint::Uniform,
                      pointOnAxisGizmo(Gizmos::AxisConstraint::Uniform,
                                       pos, invScale));
    setScaleGizmoHover(Gizmos::ScaleHandle::X,
                       pointOnScaleGizmo(Gizmos::ScaleHandle::X,
                                         pos, invScale));
    setScaleGizmoHover(Gizmos::ScaleHandle::Y,
                       pointOnScaleGizmo(Gizmos::ScaleHandle::Y,
                                         pos, invScale));
    setScaleGizmoHover(Gizmos::ScaleHandle::Uniform,
                       pointOnScaleGizmo(Gizmos::ScaleHandle::Uniform,
                                         pos, invScale));
    setShearGizmoHover(Gizmos::ShearHandle::X,
                       pointOnShearGizmo(Gizmos::ShearHandle::X,
                                         pos, invScale));
    setShearGizmoHover(Gizmos::ShearHandle::Y,
                       pointOnShearGizmo(Gizmos::ShearHandle::Y,
                                         pos, invScale));
}

bool Canvas::pointOnRotateGizmo(const QPointF &pos,
                                qreal invScale) const
{
    if (!mGizmos.fState.rotateHandleVisible || !mGizmos.fState.showRotate) { return false; }
    if (mGizmos.fState.rotateHandleHitPolygon.size() >= 3) {
        QPolygonF hitPoly(mGizmos.fState.rotateHandleHitPolygon);
        if (hitPoly.containsPoint(pos, Qt::OddEvenFill)) { return true; }
    }
    const qreal halfThicknessWorld = (mGizmos.fConfig.rotateHitWidthPx * invScale) * 0.5;
    const QPointF center = mGizmos.fState.rotateHandleAnchor;
    const qreal radius = mGizmos.fState.rotateHandleRadius;
    if (radius <= 0) { return false; }

    const qreal dx = pos.x() - center.x();
    const qreal dy = pos.y() - center.y();
    const qreal distance = std::hypot(dx, dy);
    if (distance < radius - halfThicknessWorld || distance > radius + halfThicknessWorld) {
        return false;
    }

    const double angleCCW = qRadiansToDegrees(std::atan2(center.y() - pos.y(), pos.x() - center.x()));
    const double skAngle = std::fmod(360.0 + (360.0 - angleCCW), 360.0);
    const double arcStart = std::fmod(mGizmos.fState.mRotateHandleStartOffsetDeg + mGizmos.fState.rotateHandleAngleDeg, 360.0);
    const double normalizedStart = arcStart < 0 ? arcStart + 360.0 : arcStart;
    const double delta = std::fmod((skAngle - normalizedStart + 360.0), 360.0);

    double extraAngleDeg = 0.0;
    if (radius > 0) {
        const qreal halfStrokeWorld = (mGizmos.fConfig.rotateStrokePx * invScale) * 0.5;
        extraAngleDeg = qRadiansToDegrees(halfStrokeWorld / radius);
    }

    if (delta <= mGizmos.fState.rotateHandleSweepDeg + extraAngleDeg) { return true; }
    if (extraAngleDeg > 0.0 && delta >= 360.0 - extraAngleDeg) { return true; }
    return false;
}

void Canvas::setRotateHandleHover(bool hovered)
{
    if (mGizmos.fState.rotateHandleHovered == hovered) { return; }
    mGizmos.fState.rotateHandleHovered = hovered;
    emit requestUpdate();
}

bool Canvas::shouldShowXLineGizmo() const
{
    if (mValueInput.xOnlyMode()) {
        return true;
    }
    return mGizmos.fState.gizmosSuppressed &&
           mGizmos.fState.axisHandleActive &&
           mGizmos.fState.axisConstraint == Gizmos::AxisConstraint::X;
}

bool Canvas::shouldShowYLineGizmo() const
{
    if (mValueInput.yOnlyMode()) {
        return true;
    }
    return mGizmos.fState.gizmosSuppressed &&
           mGizmos.fState.axisHandleActive &&
           mGizmos.fState.axisConstraint == Gizmos::AxisConstraint::Y;
}

bool Canvas::updateLineGizmoVisibility()
{
    const bool desiredX = shouldShowXLineGizmo();
    const bool desiredY = shouldShowYLineGizmo();

    bool changed = false;
    if (mGizmos.fState.xLineGeom.visible != desiredX) {
        mGizmos.fState.xLineGeom.visible = desiredX;
        changed = true;
    }
    if (mGizmos.fState.yLineGeom.visible != desiredY) {
        mGizmos.fState.yLineGeom.visible = desiredY;
        changed = true;
    }
    return changed;
}

void Canvas::setGizmosSuppressed(bool suppressed)
{
    if (mGizmos.fState.gizmosSuppressed == suppressed) {
        if (updateLineGizmoVisibility()) {
            emit requestUpdate();
        }
        return;
    }

    mGizmos.fState.gizmosSuppressed = suppressed;
    updateLineGizmoVisibility();

    if (suppressed) {
        mGizmos.fState.rotateHandleHovered = false;
        mGizmos.fState.axisXHovered = false;
        mGizmos.fState.axisYHovered = false;
        mGizmos.fState.axisUniformHovered = false;
        mGizmos.fState.scaleXHovered = false;
        mGizmos.fState.scaleYHovered = false;
        mGizmos.fState.scaleUniformHovered = false;
        mGizmos.fState.shearXHovered = false;
        mGizmos.fState.shearYHovered = false;
    }

    emit requestUpdate();
}

void Canvas::updateRotateHandleGeometry(qreal invScale)
{
    mGizmos.fState.rotateHandleVisible = false;
    mGizmos.fState.rotateHandleRadius = 0;
    mGizmos.fState.rotateHandlePolygon.clear();
    mGizmos.fState.rotateHandleHitPolygon.clear();

    auto resetAllGizmos = [&]() {
        setAxisGizmoHover(Gizmos::AxisConstraint::X, false);
        setAxisGizmoHover(Gizmos::AxisConstraint::Y, false);
        setAxisGizmoHover(Gizmos::AxisConstraint::Uniform, false);
        setScaleGizmoHover(Gizmos::ScaleHandle::X, false);
        setScaleGizmoHover(Gizmos::ScaleHandle::Y, false);
        setScaleGizmoHover(Gizmos::ScaleHandle::Uniform, false);
        setShearGizmoHover(Gizmos::ShearHandle::X, false);
        setShearGizmoHover(Gizmos::ShearHandle::Y, false);

        mGizmos.fState.axisXGeom = Gizmos::AxisGeometry();
        mGizmos.fState.axisYGeom = Gizmos::AxisGeometry();
        mGizmos.fState.axisUniformGeom = Gizmos::AxisGeometry();
        mGizmos.fState.scaleXGeom = Gizmos::ScaleGeometry();
        mGizmos.fState.scaleYGeom = Gizmos::ScaleGeometry();
        mGizmos.fState.scaleUniformGeom = Gizmos::ScaleGeometry();
        mGizmos.fState.shearXGeom = Gizmos::ShearGeometry();
        mGizmos.fState.shearYGeom = Gizmos::ShearGeometry();
        mGizmos.fState.xLineGeom = Gizmos::LineGeometry();
        mGizmos.fState.yLineGeom = Gizmos::LineGeometry();

        const bool hadConstraint =
                mGizmos.fState.axisConstraint != Gizmos::AxisConstraint::None ||
                mGizmos.fState.scaleConstraint != Gizmos::ScaleHandle::None ||
                mGizmos.fState.shearConstraint != Gizmos::ShearHandle::None;

        mGizmos.fState.axisConstraint = Gizmos::AxisConstraint::None;
        mGizmos.fState.scaleConstraint = Gizmos::ScaleHandle::None;
        mGizmos.fState.shearConstraint = Gizmos::ShearHandle::None;
        mGizmos.fState.axisHandleActive = false;
        mGizmos.fState.scaleHandleActive = false;
        mGizmos.fState.shearHandleActive = false;
        if (hadConstraint) {
            mValueInput.setForce1D(false);
            mValueInput.setXYMode();
        }
        mGizmos.fState.rotateHandlePolygon.clear();
        mGizmos.fState.rotateHandleHitPolygon.clear();
    };

    if (mGizmos.fState.gizmosSuppressed) { return; }

    if (mCurrentMode != CanvasMode::boxTransform) {
        setRotateHandleHover(false);
        resetAllGizmos();
        return;
    }

    if (mSelectedBoxes.isEmpty()) {
        setRotateHandleHover(false);
        resetAllGizmos();
        return;
    }

    if (!mRotPivot) {
        setRotateHandleHover(false);
        resetAllGizmos();
        return;
    }

    mGizmos.fState.rotateHandleAngleDeg = 0.0; // keep gizmo orientation screen-aligned

    QPointF pivot;
    if (mDocument.fPivotPosForGizmosValid) { pivot = mDocument.fPivotPosForGizmos; }
    else { pivot = mRotPivot->getAbsolutePos(); }

    const qreal axisWidthWorld = mGizmos.fConfig.axisWidthPx * invScale;
    const qreal axisHeightWorld = mGizmos.fConfig.axisHeightPx * invScale;
    const qreal axisGapYWorld = mGizmos.fConfig.axisYOffsetPx * invScale;
    const qreal axisGapXWorld = mGizmos.fConfig.axisXOffsetPx * invScale;
    const qreal xLineLengthWorld = mGizmos.fConfig.xLineLengthPx * invScale;
    const qreal xLineStrokeWorld = mGizmos.fConfig.xLineStrokePx * invScale;
    const qreal yLineLengthWorld = mGizmos.fConfig.yLineLengthPx * invScale;
    const qreal yLineStrokeWorld = mGizmos.fConfig.yLineStrokePx * invScale;

    // const qreal anchorOffset = axisWidthWorld * 0.5;
    const qreal anchorOffset = 2.0 * invScale;
    mGizmos.fState.rotateHandleAnchor = pivot + QPointF(anchorOffset, -anchorOffset);

    const qreal baseRotateRadiusWorld = mGizmos.fConfig.rotateRadiusPx * invScale;
    mGizmos.fState.rotateHandleRadius = baseRotateRadiusWorld;
    mGizmos.fState.rotateHandleSweepDeg = mGizmos.fConfig.rotateSweepDeg; // default sweep angle
    mGizmos.fState.mRotateHandleStartOffsetDeg = mGizmos.fConfig.rotateBaseOffsetDeg; // default base angle offset

    const qreal strokeWorld = mGizmos.fConfig.rotateStrokePx * invScale;
    const qreal sweepDegAbs = std::abs(mGizmos.fState.rotateHandleSweepDeg);
    auto normalizeAngle = [](qreal degrees) {
        degrees = std::fmod(degrees, 360.0);
        if (degrees < 0.0) { degrees += 360.0; }
        return degrees;
    };
    const qreal startAngleSk = normalizeAngle(mGizmos.fState.mRotateHandleStartOffsetDeg + mGizmos.fState.rotateHandleAngleDeg);
    const qreal direction = (mGizmos.fState.rotateHandleSweepDeg >= 0.0) ? 1.0 : -1.0;
    mGizmos.fState.rotateHandlePolygon.clear();
    mGizmos.fState.rotateHandleHitPolygon.clear();

    if (mGizmos.fState.showRotate && sweepDegAbs > std::numeric_limits<qreal>::epsilon()) {
        const int segments = std::max(8, static_cast<int>(std::ceil(sweepDegAbs / 6.0)));
        auto angleToPoint = [&](qreal angleDeg, qreal radius) {
            const qreal angleRad = qDegreesToRadians(angleDeg);
            return QPointF(mGizmos.fState.rotateHandleAnchor.x() + radius * std::cos(angleRad),
                           mGizmos.fState.rotateHandleAnchor.y() + radius * std::sin(angleRad));
        };
        auto buildArcPolygon = [&](qreal halfThickness, QVector<QPointF> &storage) {
            storage.clear();
            const qreal outerRadius = mGizmos.fState.rotateHandleRadius + halfThickness;
            if (outerRadius <= 0.0) { return; }
            const qreal innerRadius = std::max(mGizmos.fState.rotateHandleRadius - halfThickness, 0.0);
            storage.reserve((segments + 1) * 2);
            for (int i = 0; i <= segments; ++i) {
                const qreal angle = normalizeAngle(startAngleSk + direction * (sweepDegAbs * i) / segments);
                storage.append(angleToPoint(angle, outerRadius));
            }
            if (innerRadius > std::numeric_limits<qreal>::epsilon()) {
                for (int i = 0; i <= segments; ++i) {
                    const qreal angle = normalizeAngle(startAngleSk + direction * sweepDegAbs - direction * (sweepDegAbs * i) / segments);
                    storage.append(angleToPoint(angle, innerRadius));
                }
            } else {
                storage.append(mGizmos.fState.rotateHandleAnchor);
            }
        };

        buildArcPolygon(strokeWorld * 0.5, mGizmos.fState.rotateHandlePolygon);
        const qreal hitHalfThickness = (mGizmos.fConfig.rotateHitWidthPx * invScale) * 0.5;
        buildArcPolygon(hitHalfThickness, mGizmos.fState.rotateHandleHitPolygon);
    }

    mGizmos.fState.axisYGeom.center = pivot + QPointF(0.0, -axisGapYWorld);
    mGizmos.fState.axisYGeom.size = QSizeF(axisWidthWorld, axisHeightWorld);
    mGizmos.fState.axisYGeom.angleDeg = 0.0;
    mGizmos.fState.axisYGeom.visible = mGizmos.fState.showPosition;
    mGizmos.fState.axisYGeom.usePolygon = true;
    mGizmos.fState.axisYGeom.polygonPoints = {
        pivot + QPointF(0.0, - 10.0 * invScale),
        pivot + QPointF(-2.0 * invScale, - 11.0 * invScale),
        pivot + QPointF(-2.0 * invScale, - 55.0 * invScale),
        pivot + QPointF(-6.0 * invScale, - 56.5 * invScale),
        pivot + QPointF(0.0, - 70.0 * invScale),
        pivot + QPointF(6.0 * invScale, - 56.5 * invScale),
        pivot + QPointF(2.0 * invScale, - 55.0 * invScale),
        pivot + QPointF(2.0 * invScale, - 11.0 * invScale)
    };

    mGizmos.fState.axisXGeom.center = pivot + QPointF(axisGapXWorld, 0.0);
    mGizmos.fState.axisXGeom.size = QSizeF(axisHeightWorld, axisWidthWorld);
    mGizmos.fState.axisXGeom.angleDeg = 0.0;
    mGizmos.fState.axisXGeom.visible = mGizmos.fState.showPosition;
    mGizmos.fState.axisXGeom.usePolygon = true;
    mGizmos.fState.axisXGeom.polygonPoints = {
        pivot + QPointF(10.0 * invScale, 0.0),
        pivot + QPointF(11.0 * invScale, -2.0 * invScale),
        pivot + QPointF(55.0 * invScale, -2.0 * invScale),
        pivot + QPointF(56.5 * invScale, -6.0 * invScale),
        pivot + QPointF(70.0 * invScale, 0.0),
        pivot + QPointF(56.5 * invScale, 6.0 * invScale),
        pivot + QPointF(55.0 * invScale, 2.0 * invScale),
        pivot + QPointF(11.0 * invScale, 2.0 * invScale)
    };

    mGizmos.fState.axisUniformGeom.center = pivot + QPointF(axisGapXWorld, 0.0);
    mGizmos.fState.axisUniformGeom.size = QSizeF(axisHeightWorld, axisWidthWorld);
    mGizmos.fState.axisUniformGeom.visible = mGizmos.fState.showPosition;
    mGizmos.fState.axisUniformGeom.usePolygon = true;
    mGizmos.fState.axisUniformGeom.polygonPoints = {
        pivot + QPointF((mGizmos.fConfig.axisUniformOffsetPx + mGizmos.fConfig.axisUniformChamferPx) * invScale , - mGizmos.fConfig.axisUniformOffsetPx * invScale),
        pivot + QPointF(mGizmos.fConfig.axisUniformOffsetPx * invScale, - (mGizmos.fConfig.axisUniformOffsetPx + mGizmos.fConfig.axisUniformChamferPx) * invScale),
        pivot + QPointF(mGizmos.fConfig.axisUniformOffsetPx * invScale, - (mGizmos.fConfig.axisUniformWidthPx - mGizmos.fConfig.axisUniformChamferPx) * invScale),
        pivot + QPointF((mGizmos.fConfig.axisUniformOffsetPx + mGizmos.fConfig.axisUniformChamferPx) * invScale, - mGizmos.fConfig.axisUniformWidthPx * invScale),
        pivot + QPointF((mGizmos.fConfig.axisUniformWidthPx - mGizmos.fConfig.axisUniformChamferPx) * invScale, - mGizmos.fConfig.axisUniformWidthPx * invScale),
        pivot + QPointF(mGizmos.fConfig.axisUniformWidthPx * invScale, - (mGizmos.fConfig.axisUniformWidthPx - mGizmos.fConfig.axisUniformChamferPx) * invScale),
        pivot + QPointF(mGizmos.fConfig.axisUniformWidthPx * invScale, - (mGizmos.fConfig.axisUniformOffsetPx + mGizmos.fConfig.axisUniformChamferPx) * invScale),
        pivot + QPointF((mGizmos.fConfig.axisUniformWidthPx - mGizmos.fConfig.axisUniformChamferPx) * invScale, - mGizmos.fConfig.axisUniformOffsetPx * invScale)
    };

    mGizmos.fState.xLineGeom.start = pivot;
    mGizmos.fState.xLineGeom.end = pivot + QPointF(xLineLengthWorld, 0.0);
    mGizmos.fState.xLineGeom.strokeWidth = xLineStrokeWorld;

    mGizmos.fState.yLineGeom.start = pivot;
    mGizmos.fState.yLineGeom.end = pivot + QPointF(0.0, yLineLengthWorld);
    mGizmos.fState.yLineGeom.strokeWidth = yLineStrokeWorld;

    updateLineGizmoVisibility();
    const qreal rotateOffsetWorld = mGizmos.fState.axisYGeom.size.width() * 0.5;
    mGizmos.fState.rotateHandleRadius = baseRotateRadiusWorld + rotateOffsetWorld;

    const QPointF offset(0.0, -mGizmos.fState.rotateHandleRadius);
    mGizmos.fState.rotateHandlePos = pivot + offset;

    const qreal scaleSizeWorld = mGizmos.fConfig.scaleSizePx * invScale;
    const qreal scaleHalf = scaleSizeWorld * 0.5;
    const qreal scaleGapWorld = mGizmos.fConfig.scaleGapPx * invScale;
    const qreal axisYTop = mGizmos.fState.axisYGeom.center.y() - (mGizmos.fState.axisYGeom.size.height() * 0.5);
    const qreal axisXRight = mGizmos.fState.axisXGeom.center.x() + (mGizmos.fState.axisXGeom.size.width() * 0.5);
    const qreal scaleCenterY = axisYTop - scaleGapWorld - scaleHalf;
    const qreal scaleCenterX = axisXRight + scaleGapWorld + scaleHalf;

    mGizmos.fState.scaleYGeom.center = QPointF(mGizmos.fState.axisYGeom.center.x(), scaleCenterY);
    mGizmos.fState.scaleYGeom.halfExtent = scaleHalf;
    mGizmos.fState.scaleYGeom.visible = mGizmos.fState.showScale;
    mGizmos.fState.scaleYGeom.usePolygon = false;
    mGizmos.fState.scaleYGeom.polygonPoints.clear();

    mGizmos.fState.scaleXGeom.center = QPointF(scaleCenterX, mGizmos.fState.axisXGeom.center.y());
    mGizmos.fState.scaleXGeom.halfExtent = scaleHalf;
    mGizmos.fState.scaleXGeom.visible = mGizmos.fState.showScale;
    mGizmos.fState.scaleXGeom.usePolygon = false;
    mGizmos.fState.scaleXGeom.polygonPoints.clear();

    mGizmos.fState.scaleUniformGeom.center = QPointF(scaleCenterX, scaleCenterY);
    mGizmos.fState.scaleUniformGeom.halfExtent = scaleHalf;
    mGizmos.fState.scaleUniformGeom.visible = mGizmos.fState.showScale;
    mGizmos.fState.scaleUniformGeom.usePolygon = true;
    mGizmos.fState.scaleUniformGeom.polygonPoints = {
        QPointF(mGizmos.fState.scaleUniformGeom.center.x() - scaleHalf * 2, mGizmos.fState.scaleUniformGeom.center.y() - scaleHalf),
        QPointF(mGizmos.fState.scaleUniformGeom.center.x() + scaleHalf, mGizmos.fState.scaleUniformGeom.center.y() - scaleHalf),
        QPointF(mGizmos.fState.scaleUniformGeom.center.x() + scaleHalf, mGizmos.fState.scaleUniformGeom.center.y() + scaleHalf * 2),
        QPointF(mGizmos.fState.scaleUniformGeom.center.x() - scaleHalf, mGizmos.fState.scaleUniformGeom.center.y() + scaleHalf * 2),
        QPointF(mGizmos.fState.scaleUniformGeom.center.x() - scaleHalf, mGizmos.fState.scaleUniformGeom.center.y() + scaleHalf),
        QPointF(mGizmos.fState.scaleUniformGeom.center.x() - scaleHalf * 2, mGizmos.fState.scaleUniformGeom.center.y() + scaleHalf)
    };

    const qreal shearRadiusWorld = mGizmos.fConfig.shearRadiusPx * invScale;

    const QPointF scaleYCenter = mGizmos.fState.scaleYGeom.center;
    const QPointF scaleXCenter = mGizmos.fState.scaleXGeom.center;
    const QPointF scaleUniformCenter = mGizmos.fState.scaleUniformGeom.center;

    mGizmos.fState.shearXGeom.center = QPointF((scaleYCenter.x() + scaleUniformCenter.x()) * 0.5,
                                                scaleCenterY);
    mGizmos.fState.shearXGeom.radius = shearRadiusWorld;
    mGizmos.fState.shearXGeom.visible = mGizmos.fState.showShear;
    mGizmos.fState.shearXGeom.usePolygon = true;
    {
        const qreal halfWidth = shearRadiusWorld * 1.5;
        const qreal topHeight = shearRadiusWorld * 0.8;
        //const qreal bottomHeight = shearRadiusWorld * 0.6;
        const QPointF c = mGizmos.fState.shearXGeom.center;
        mGizmos.fState.shearXGeom.polygonPoints = {
            QPointF(c.x() - scaleHalf * 0.5 - halfWidth * 1.5, c.y()),
            QPointF(c.x() - scaleHalf * 0.5 - halfWidth, c.y() - topHeight),
            QPointF(c.x() - scaleHalf * 0.5 + halfWidth, c.y() - topHeight),
            QPointF(c.x() - scaleHalf * 0.5 + halfWidth * 1.5, c.y()),
            QPointF(c.x() - scaleHalf * 0.5 + halfWidth, c.y() + topHeight),
            QPointF(c.x() - scaleHalf * 0.5 - halfWidth, c.y() + topHeight)
        };
    }

    mGizmos.fState.shearYGeom.center = QPointF(scaleCenterX,
                                 (scaleXCenter.y() + scaleUniformCenter.y()) * 0.5);
    mGizmos.fState.shearYGeom.radius = shearRadiusWorld;
    mGizmos.fState.shearYGeom.visible = mGizmos.fState.showShear;
    mGizmos.fState.shearYGeom.usePolygon = true;
    {
        const qreal halfHeight = shearRadiusWorld * 1.5;
        const qreal topWidth = shearRadiusWorld * 0.8;
        //const qreal bottomWidth = shearRadiusWorld * 0.6;
        const QPointF c = mGizmos.fState.shearYGeom.center;
        mGizmos.fState.shearYGeom.polygonPoints = {
            QPointF(c.x(), c.y() + scaleHalf * 0.5 - halfHeight * 1.5),
            QPointF(c.x() + topWidth, c.y() + scaleHalf * 0.5 - halfHeight),
            QPointF(c.x() + topWidth, c.y() + scaleHalf * 0.5 + halfHeight),
            QPointF(c.x(), c.y() + scaleHalf * 0.5 + halfHeight * 1.5),
            QPointF(c.x() - topWidth, c.y() + scaleHalf * 0.5 + halfHeight),
            QPointF(c.x() - topWidth, c.y() + scaleHalf * 0.5 - halfHeight)
        };
    }

    const bool anyGizmoVisible = (mGizmos.fState.showRotate || mGizmos.fState.showPosition ||
                                  mGizmos.fState.showScale || mGizmos.fState.showShear);
    mGizmos.fState.rotateHandleVisible = anyGizmoVisible;
}

bool Canvas::tryStartRotateWithGizmo(const eMouseEvent &e,
                                     qreal invScale)
{
    updateRotateHandleGeometry(invScale);
    const bool hovered = pointOnRotateGizmo(e.fPos, invScale);
    setRotateHandleHover(hovered);
    if (!hovered) { return false; }

    if (!prepareRotation(e.fPos, true)) {
        setRotateHandleHover(false);
        return false;
    }
    e.fGrabMouse();
    return true;
}

bool Canvas::tryStartScaleGizmo(const eMouseEvent &e,
                                qreal invScale)
{
    updateRotateHandleGeometry(invScale);
    if (!mGizmos.fState.rotateHandleVisible) { return false; }

    if (pointOnScaleGizmo(Gizmos::ScaleHandle::Y, e.fPos, invScale)) {
        return startScaleConstrainedMove(e, Gizmos::ScaleHandle::Y);
    }
    if (pointOnScaleGizmo(Gizmos::ScaleHandle::X, e.fPos, invScale)) {
        return startScaleConstrainedMove(e, Gizmos::ScaleHandle::X);
    }
    if (pointOnScaleGizmo(Gizmos::ScaleHandle::Uniform, e.fPos, invScale)) {
        return startScaleConstrainedMove(e, Gizmos::ScaleHandle::Uniform);
    }
    return false;
}

bool Canvas::tryStartShearGizmo(const eMouseEvent &e,
                                qreal invScale)
{
    updateRotateHandleGeometry(invScale);
    if (!mGizmos.fState.rotateHandleVisible) { return false; }

    if (pointOnShearGizmo(Gizmos::ShearHandle::Y, e.fPos, invScale)) {
        return startShearConstrainedMove(e, Gizmos::ShearHandle::Y);
    }
    if (pointOnShearGizmo(Gizmos::ShearHandle::X, e.fPos, invScale)) {
        return startShearConstrainedMove(e, Gizmos::ShearHandle::X);
    }
    return false;
}

bool Canvas::tryStartAxisGizmo(const eMouseEvent &e,
                               qreal invScale)
{
    updateRotateHandleGeometry(invScale);
    if (!mGizmos.fState.rotateHandleVisible) { return false; }

    if (pointOnAxisGizmo(Gizmos::AxisConstraint::Uniform, e.fPos, invScale)) {
        return startAxisConstrainedMove(e, Gizmos::AxisConstraint::Uniform);
    }
    if (pointOnAxisGizmo(Gizmos::AxisConstraint::Y, e.fPos, invScale)) {
        return startAxisConstrainedMove(e, Gizmos::AxisConstraint::Y);
    }
    if (pointOnAxisGizmo(Gizmos::AxisConstraint::X, e.fPos, invScale)) {
        return startAxisConstrainedMove(e, Gizmos::AxisConstraint::X);
    }
    return false;
}

bool Canvas::startScaleConstrainedMove(const eMouseEvent &e,
                                       Gizmos::ScaleHandle handle)
{
    if (mCurrentMode != CanvasMode::boxTransform &&
        mCurrentMode != CanvasMode::pointTransform) { return false; }
    if (mSelectedBoxes.isEmpty() && mSelectedPoints_d.isEmpty()) { return false; }

    mValueInput.clearAndDisableInput();
    mValueInput.setupScale();

    if (handle == Gizmos::ScaleHandle::Uniform) {
        mValueInput.setForce1D(false);
        mValueInput.setXYMode();
    } else {
        mValueInput.setForce1D(true);
        if (handle == Gizmos::ScaleHandle::X) { mValueInput.setXOnlyMode(); }
        else { mValueInput.setYOnlyMode(); }
    }

    mTransMode = TransformMode::scale;
    mDoubleClick = false;
    mStartTransform = true;

    mGizmos.fState.scaleConstraint = handle;
    mGizmos.fState.scaleHandleActive = true;
    setScaleGizmoHover(handle, true);
    mRotPivot->setMousePos(e.fPos);
    setGizmosSuppressed(true);

    e.fGrabMouse();
    return true;
}

bool Canvas::startShearConstrainedMove(const eMouseEvent &e,
                                       Gizmos::ShearHandle handle)
{
    if (mCurrentMode != CanvasMode::boxTransform &&
        mCurrentMode != CanvasMode::pointTransform) { return false; }
    if (mSelectedBoxes.isEmpty() && mSelectedPoints_d.isEmpty()) { return false; }

    mValueInput.clearAndDisableInput();
    mValueInput.setupShear();
    mValueInput.setForce1D(true);
    if (handle == Gizmos::ShearHandle::X) { mValueInput.setXOnlyMode(); }
    else { mValueInput.setYOnlyMode(); }

    mTransMode = TransformMode::shear;
    mDoubleClick = false;
    mStartTransform = true;

    mGizmos.fState.shearConstraint = handle;
    mGizmos.fState.shearHandleActive = true;
    setShearGizmoHover(handle, true);
    mRotPivot->setMousePos(e.fPos);
    setGizmosSuppressed(true);

    e.fGrabMouse();
    return true;
}

bool Canvas::startAxisConstrainedMove(const eMouseEvent &e,
                                      Gizmos::AxisConstraint axis)
{
    if (mCurrentMode != CanvasMode::boxTransform &&
        mCurrentMode != CanvasMode::pointTransform) { return false; }
    if (mSelectedBoxes.isEmpty() && mSelectedPoints_d.isEmpty()) { return false; }

    mValueInput.clearAndDisableInput();
    mValueInput.setupMove();
    mValueInput.setForce1D(true);
    if (axis == Gizmos::AxisConstraint::X) {
        mValueInput.setXOnlyMode();
    } else {
        if (axis == Gizmos::AxisConstraint::Y) { mValueInput.setYOnlyMode(); }
        else { mValueInput.setXYMode(); }
    }

    mTransMode = TransformMode::move;
    mDoubleClick = false;
    mStartTransform = true;
    mGizmos.fState.axisConstraint = axis;
    mGizmos.fState.axisHandleActive = true;
    setAxisGizmoHover(axis, true);
    setGizmosSuppressed(true);
    e.fGrabMouse();
    return true;
}

bool Canvas::pointOnScaleGizmo(Gizmos::ScaleHandle handle,
                               const QPointF &pos,
                               qreal invScale) const
{
    Q_UNUSED(invScale);
    if (!mGizmos.fState.rotateHandleVisible) { return false; }

    const Gizmos::ScaleGeometry *geom = nullptr;
    switch (handle) {
    case Gizmos::ScaleHandle::X:
        geom = &mGizmos.fState.scaleXGeom;
        break;
    case Gizmos::ScaleHandle::Y:
        geom = &mGizmos.fState.scaleYGeom;
        break;
    case Gizmos::ScaleHandle::Uniform:
        geom = &mGizmos.fState.scaleUniformGeom;
        break;
    case Gizmos::ScaleHandle::None:
    default:
        return false;
    }

    if (!geom->visible || geom->halfExtent <= 0.0) { return false; }

    if (geom->usePolygon && geom->polygonPoints.size() >= 3) {
        QPolygonF poly(geom->polygonPoints);
        return poly.containsPoint(pos, Qt::OddEvenFill);
    }

    const qreal half = geom->halfExtent;
    return std::abs(pos.x() - geom->center.x()) <= half &&
           std::abs(pos.y() - geom->center.y()) <= half;
}

bool Canvas::pointOnShearGizmo(Gizmos::ShearHandle handle,
                               const QPointF &pos,
                               qreal invScale) const
{
    Q_UNUSED(invScale);
    if (!mGizmos.fState.rotateHandleVisible) { return false; }

    const Gizmos::ShearGeometry *geom = nullptr;
    switch (handle) {
    case Gizmos::ShearHandle::X:
        geom = &mGizmos.fState.shearXGeom;
        break;
    case Gizmos::ShearHandle::Y:
        geom = &mGizmos.fState.shearYGeom;
        break;
    case Gizmos::ShearHandle::None:
    default:
        return false;
    }

    if (!geom->visible) { return false; }

    if (geom->usePolygon && geom->polygonPoints.size() >= 3) {
        QPolygonF poly(geom->polygonPoints);
        return poly.containsPoint(pos, Qt::OddEvenFill);
    }

    if (geom->radius <= 0.0) { return false; }

    const qreal distance = std::hypot(pos.x() - geom->center.x(),
                                      pos.y() - geom->center.y());
    return distance <= geom->radius;
}

bool Canvas::pointOnAxisGizmo(Gizmos::AxisConstraint axis,
                              const QPointF &pos,
                              qreal invScale) const
{
    Q_UNUSED(invScale);
    if (!mGizmos.fState.rotateHandleVisible) { return false; }

    const Gizmos::AxisGeometry* geomPtr;
    switch (axis) {
    case Gizmos::AxisConstraint::X:
        geomPtr = &mGizmos.fState.axisXGeom;
        break;
    case Gizmos::AxisConstraint::Y:
        geomPtr = &mGizmos.fState.axisYGeom;
        break;
    case Gizmos::AxisConstraint::Uniform:
        geomPtr = &mGizmos.fState.axisUniformGeom;
        break;
    default:
        return false;
    }
    const Gizmos::AxisGeometry &geom = *geomPtr;

    if (!geom.visible) { return false; }

    if (geom.usePolygon && geom.polygonPoints.size() >= 3) {
        QPolygonF poly(geom.polygonPoints);
        return poly.containsPoint(pos, Qt::OddEvenFill);
    }

    const QPointF relative = pos - geom.center;
    const qreal angleRadGeom = qDegreesToRadians(geom.angleDeg);
    const qreal cosG = std::cos(angleRadGeom);
    const qreal sinG = std::sin(angleRadGeom);
    const qreal localX = relative.x() * cosG + relative.y() * sinG;
    const qreal localY = -relative.x() * sinG + relative.y() * cosG;
    const qreal halfW = geom.size.width() * 0.5;
    const qreal halfH = geom.size.height() * 0.5;
    return std::abs(localX) <= halfW && std::abs(localY) <= halfH;
}

void Canvas::setScaleGizmoHover(Gizmos::ScaleHandle handle,
                                bool hovered)
{
    bool *target = nullptr;
    switch (handle) {
    case Gizmos::ScaleHandle::X:
        target = &mGizmos.fState.scaleXHovered;
        break;
    case Gizmos::ScaleHandle::Y:
        target = &mGizmos.fState.scaleYHovered;
        break;
    case Gizmos::ScaleHandle::Uniform:
        target = &mGizmos.fState.scaleUniformHovered;
        break;
    case Gizmos::ScaleHandle::None:
    default:
        return;
    }

    if (*target == hovered) { return; }
    *target = hovered;
    emit requestUpdate();
}

void Canvas::setShearGizmoHover(Gizmos::ShearHandle handle,
                                bool hovered)
{
    bool *target = nullptr;
    switch (handle) {
    case Gizmos::ShearHandle::X:
        target = &mGizmos.fState.shearXHovered;
        break;
    case Gizmos::ShearHandle::Y:
        target = &mGizmos.fState.shearYHovered;
        break;
    case Gizmos::ShearHandle::None:
    default:
        return;
    }

    if (*target == hovered) { return; }
    *target = hovered;
    emit requestUpdate();
}

void Canvas::setAxisGizmoHover(Gizmos::AxisConstraint axis,
                               bool hovered)
{
    bool *target = nullptr;
    switch (axis) {
    case Gizmos::AxisConstraint::X:
        target = &mGizmos.fState.axisXHovered;
        break;
    case Gizmos::AxisConstraint::Y:
        target = &mGizmos.fState.axisYHovered;
        break;
    case Gizmos::AxisConstraint::Uniform:
        target = &mGizmos.fState.axisUniformHovered;
        break;
    case Gizmos::AxisConstraint::None:
    default:
        return;
    }

    if (*target == hovered) { return; }
    *target = hovered;
    emit requestUpdate();
}

void Canvas::handleLeftMouseGizmos()
{
    mGizmos.fState.rotatingFromHandle = false;
    mGizmos.fState.axisHandleActive = false;
    mGizmos.fState.scaleHandleActive = false;
    mGizmos.fState.shearHandleActive = false;

    setGizmosSuppressed(false);

    if (mGizmos.fState.axisConstraint != Gizmos::AxisConstraint::None) {
        mGizmos.fState.axisConstraint = Gizmos::AxisConstraint::None;
        mValueInput.setForce1D(false);
        mValueInput.setXYMode();
        setAxisGizmoHover(Gizmos::AxisConstraint::X, false);
        setAxisGizmoHover(Gizmos::AxisConstraint::Y, false);
    }
    if (mGizmos.fState.scaleConstraint != Gizmos::ScaleHandle::None) {
        mGizmos.fState.scaleConstraint = Gizmos::ScaleHandle::None;
        mValueInput.setForce1D(false);
        mValueInput.setXYMode();
        setScaleGizmoHover(Gizmos::ScaleHandle::X, false);
        setScaleGizmoHover(Gizmos::ScaleHandle::Y, false);
        setScaleGizmoHover(Gizmos::ScaleHandle::Uniform, false);
    }
    if (mGizmos.fState.shearConstraint != Gizmos::ShearHandle::None) {
        mGizmos.fState.shearConstraint = Gizmos::ShearHandle::None;
        mValueInput.setForce1D(false);
        mValueInput.setXYMode();
        setShearGizmoHover(Gizmos::ShearHandle::X, false);
        setShearGizmoHover(Gizmos::ShearHandle::Y, false);
    }
}

void Canvas::cancelCurrentTransformGimzos()
{
    mGizmos.fState.axisHandleActive = false;
    mGizmos.fState.scaleHandleActive = false;
    mGizmos.fState.shearHandleActive = false;
    setGizmosSuppressed(false);

    if (mGizmos.fState.axisConstraint != Gizmos::AxisConstraint::None) {
        mGizmos.fState.axisConstraint = Gizmos::AxisConstraint::None;
        mValueInput.setForce1D(false);
        mValueInput.setXYMode();
        setAxisGizmoHover(Gizmos::AxisConstraint::X, false);
        setAxisGizmoHover(Gizmos::AxisConstraint::Y, false);
    }
    if (mGizmos.fState.scaleConstraint != Gizmos::ScaleHandle::None) {
        mGizmos.fState.scaleConstraint = Gizmos::ScaleHandle::None;
        mValueInput.setForce1D(false);
        mValueInput.setXYMode();
        setScaleGizmoHover(Gizmos::ScaleHandle::X, false);
        setScaleGizmoHover(Gizmos::ScaleHandle::Y, false);
        setScaleGizmoHover(Gizmos::ScaleHandle::Uniform, false);
    }
    if (mGizmos.fState.shearConstraint != Gizmos::ShearHandle::None) {
        mGizmos.fState.shearConstraint = Gizmos::ShearHandle::None;
        mValueInput.setForce1D(false);
        mValueInput.setXYMode();
        setShearGizmoHover(Gizmos::ShearHandle::X, false);
        setShearGizmoHover(Gizmos::ShearHandle::Y, false);
    }
}
