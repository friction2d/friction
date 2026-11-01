#include "shadoweffect.h"

#include "Boxes/containerbox.h"
#include "svgexporter.h"
#include "svgexporthelpers.h"
#include "appsupport.h"

class ShadowEffectCaller : public RasterEffectCaller {
public:
    ShadowEffectCaller(const HardwareSupport hwSupport,
                       const qreal radius,
                       const QColor& color,
                       const QPointF& translation,
                       const qreal opacity,
                       const QMargins& margin) :
        RasterEffectCaller(hwSupport, true, margin),
        mRadius(static_cast<float>(radius)),
        mColor(toSkColor(color)),
        mTranslation(toSkPoint(translation)),
        mOpacity(static_cast<float>(opacity)) {}

    void processGpu(QGL33 * const gl,
                    GpuRenderTools& renderTools);
    void processCpu(CpuRenderTools& renderTools,
                    const CpuRenderData &data);
private:
    void setupPaint(SkPaint& paint) const;

    const float mRadius;
    const SkColor mColor;
    const SkPoint mTranslation;
    const SkScalar mOpacity;
};

ShadowEffect::ShadowEffect() :
    RasterEffect("shadow",
                 AppSupport::getRasterEffectHardwareSupport("Shadow",
                                                            HardwareSupport::cpuOnly),
                 false,
                 RasterEffectType::SHADOW)
{
    mBlurRadius = enve::make_shared<QrealAnimator>("blur radius");
    mOpacity = enve::make_shared<QrealAnimator>(1, 0, 1, 0.01, "opacity");
    mColor = enve::make_shared<ColorAnimator>();
    mTranslation = enve::make_shared<QPointFAnimator>("translation");

    mBlurRadius->setValueRange(0, 300);
    mBlurRadius->setCurrentBaseValue(10);
    ca_addChild(mBlurRadius);

    mTranslation->setValuesRange(-1000, 1000);
    mTranslation->setBaseValue(QPointF(0, 0));
    ca_addChild(mTranslation);

    mColor->setColor(Qt::black);
    ca_addChild(mColor);

    ca_addChild(mOpacity);

    ca_setGUIProperty(mColor.data());
}

QDomElement ShadowEffect::saveShadowSVG(
        SvgExporter& exp, const FrameRange& visRange,
        const QDomElement& child) const {
    auto result = exp.createElement("g");

    const QString filterId = SvgExportHelpers::ptrToStr(this);
    auto filter = exp.createElement("filter");
    filter.setAttribute("id", filterId);
    filter.setAttribute("filterUnits", "userSpaceOnUse");

    qreal scale = 1.;
    if(const auto parent = getFirstAncestor<BoundingBox>()) {
        if(const auto grandParent = parent->getParentGroup()) {
            scale /= grandParent->getTotalTransform().m11();
        }
    }

    auto shadow = exp.createElement("feDropShadow");
    const auto x = mTranslation->getXAnimator();
    x->saveQrealSVG(exp, shadow, visRange, "dx", scale);
    const auto y = mTranslation->getYAnimator();
    y->saveQrealSVG(exp, shadow, visRange, "dy", scale);
    mColor->saveColorSVG(exp, shadow, visRange, "flood-color", !exp.fColors11);
    mOpacity->saveQrealSVG(exp, shadow, visRange, "flood-opacity");
    mBlurRadius->saveQrealSVG(exp, shadow, visRange, "stdDeviation", scale/3);
    filter.appendChild(shadow);

    exp.addToDefs(filter);
    result.setAttribute("filter", "url(#" + filterId + ")");

    result.appendChild(child);
    return result;
}

stdsptr<RasterEffectCaller>
ShadowEffect::getEffectCaller(const qreal relFrame,
                              const qreal resolution,
                              const qreal influence,
                              BoxRenderData * const data) const
{
    Q_UNUSED(data)

    qreal blur = mBlurRadius->getEffectiveValue(relFrame)*resolution;
    const QColor color = mColor->getColor(relFrame);
    QPointF trans = mTranslation->getEffectiveValue(relFrame)*resolution;
    const qreal opacity = mOpacity->getEffectiveValue(relFrame)*influence;

    if (std::isnan(blur) || std::isinf(blur) ||
        std::isnan(trans.x()) || std::isinf(trans.x()) ||
        std::isnan(trans.y()) || std::isinf(trans.y())) {
        return nullptr;
    }

    blur = qBound(0.0, blur, 2048.0);
    trans.setX(qBound(-8192.0, trans.x(), 8192.0));
    trans.setY(qBound(-8192.0, trans.y(), 8192.0));

    const int iL = qMax(0, qCeil(blur - trans.x()));
    const int iT = qMax(0, qCeil(blur - trans.y()));
    const int iR = qMax(0, qCeil(blur + trans.x()));
    const int iB = qMax(0, qCeil(blur + trans.y()));

    return enve::make_shared<ShadowEffectCaller>(
                instanceHwSupport(), blur,
                color, trans, opacity,
                QMargins(iL, iT, iR, iB));
}

void ShadowEffectCaller::setupPaint(SkPaint &paint) const
{
    const float sigma = mRadius * 0.3333333f;
    paint.setImageFilter(SkImageFilters::Blur(sigma, sigma, nullptr));

    const uint8_t alpha = static_cast<uint8_t>(SkColorGetA(mColor) * mOpacity);
    const SkColor shadowColor = SkColorSetARGB(alpha,
                                               SkColorGetR(mColor),
                                               SkColorGetG(mColor),
                                               SkColorGetB(mColor));

    paint.setColorFilter(SkColorFilters::Blend(shadowColor, SkBlendMode::kSrcIn));
}

void ShadowEffectCaller::processGpu(QGL33 * const gl,
                                    GpuRenderTools &renderTools) {
    Q_UNUSED(gl)

    renderTools.switchToSkia();
    const auto canvas = renderTools.requestTargetCanvas();
    canvas->clear(SK_ColorTRANSPARENT);

    const auto srcTex = renderTools.requestSrcTextureImageWrapper();

    SkPaint paint;
    setupPaint(paint);
    canvas->drawImage(srcTex, mTranslation.x(), -mTranslation.y(), &paint);
    canvas->drawImage(srcTex, 0, 0);
    canvas->flush();

    renderTools.swapTextures();
}

void ShadowEffectCaller::processCpu(CpuRenderTools &renderTools,
                                    const CpuRenderData &data)
{
    Q_UNUSED(data)

    const auto& srcBtmp = renderTools.fSrcBtmp;
    const auto& dstBtmp = renderTools.fDstBtmp;

    if (srcBtmp.empty() || srcBtmp.getPixels() == nullptr ||
        dstBtmp.empty() || dstBtmp.getPixels() == nullptr) {
        return;
    }

    SkCanvas canvas(dstBtmp);
    canvas.clear(SK_ColorTRANSPARENT);

    const int radCeil = static_cast<int>(ceil(mRadius));
    const auto& texTile = data.fTexTile;

    const int ceilDX = static_cast<int>(ceil(abs(mTranslation.x())));
    const int ceilDY = static_cast<int>(ceil(abs(mTranslation.y())));
    auto srcRect = texTile.makeOutset(radCeil + ceilDX, radCeil + ceilDY);

    if (srcRect.intersect(srcRect, srcBtmp.bounds())) {

        SkBitmap packedTile;
        packedTile.allocPixels(srcBtmp.info().makeWH(srcRect.width(),
                                                     srcRect.height()));

        if (srcBtmp.readPixels(packedTile.info(),
                               packedTile.getPixels(),
                               packedTile.rowBytes(),
                               srcRect.left(),
                               srcRect.top())) {

            const int drawX = srcRect.left() - texTile.left();
            const int drawY = srcRect.top() - texTile.top();

            SkPaint paint;
            setupPaint(paint);

            canvas.drawBitmap(packedTile,
                              mTranslation.x() + drawX,
                              mTranslation.y() + drawY,
                              &paint);
            canvas.drawBitmap(packedTile, drawX, drawY);
        }
    }
}
