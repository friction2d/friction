#include "blureffect.h"

#include "Animators/qrealanimator.h"
#include "Boxes/containerbox.h"
#include "svgexporthelpers.h"
#include "svgexporter.h"
#include "appsupport.h"

class BlurEffectCaller : public RasterEffectCaller {
public:
    BlurEffectCaller(const HardwareSupport hwSupport,
                     const qreal radius);

    void processGpu(QGL33 * const gl,
                    GpuRenderTools& renderTools);
    void processCpu(CpuRenderTools& renderTools,
                    const CpuRenderData &data);
private:
    const float mRadius;
};

BlurEffect::BlurEffect() :
    RasterEffect("blur",
                 AppSupport::getRasterEffectHardwareSupport("Blur",
                                                            HardwareSupport::cpuOnly),
                 false,
                 RasterEffectType::BLUR)
{
    mRadius = enve::make_shared<QrealAnimator>(10, 0, 999.999, 1, "radius");
    ca_addChild(mRadius);
    connect(mRadius.get(), &QrealAnimator::effectiveValueChanged,
            this, &RasterEffect::forcedMarginChanged);
    ca_setGUIProperty(mRadius.get());
}

stdsptr<RasterEffectCaller> BlurEffect::getEffectCaller(
        const qreal relFrame, const qreal resolution,
        const qreal influence, BoxRenderData * const data) const {
    Q_UNUSED(data)
    const qreal radius = mRadius->getEffectiveValue(relFrame)*resolution*influence;
    if(isZero4Dec(radius)) return nullptr;
    return enve::make_shared<BlurEffectCaller>(instanceHwSupport(), radius);
}

QMargins radiusToMargin(const qreal radius) {
    return QMargins() + qCeil(radius);
}

QMargins BlurEffect::getMargin() const {
    return radiusToMargin(mRadius->getEffectiveValue());
}

QDomElement BlurEffect::saveBlurSVG(
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

    auto shadow = exp.createElement("feGaussianBlur");
    mRadius->saveQrealSVG(exp, shadow, visRange, "stdDeviation", scale/3);

    filter.appendChild(shadow);

    exp.addToDefs(filter);
    result.setAttribute("filter", "url(#" + filterId + ")");

    result.appendChild(child);
    return result;
}

BlurEffectCaller::BlurEffectCaller(const HardwareSupport hwSupport,
                                   const qreal radius) :
    RasterEffectCaller(hwSupport, true, radiusToMargin(radius)),
    mRadius(static_cast<float>(radius)) {}


void BlurEffectCaller::processGpu(QGL33 * const gl,
                                  GpuRenderTools &renderTools) {
    Q_UNUSED(gl)

    const float sigma = mRadius*0.3333333f;
    const auto filter = SkBlurImageFilter::Make(sigma, sigma, nullptr);

    SkPaint paint;
    paint.setImageFilter(filter);

    renderTools.switchToSkia();
    const auto canvas = renderTools.requestTargetCanvas();
    canvas->clear(SK_ColorTRANSPARENT);
    const auto srcTex = renderTools.requestSrcTextureImageWrapper();
    canvas->drawImage(srcTex, 0, 0, &paint);
    canvas->flush();

    renderTools.swapTextures();
}

void BlurEffectCaller::processCpu(CpuRenderTools &renderTools,
                                  const CpuRenderData &data) {
    Q_UNUSED(data)

    const float sigma = mRadius*0.3333333f;
    const auto filter = SkBlurImageFilter::Make(sigma, sigma, nullptr);

    SkPaint paint;
    paint.setImageFilter(filter);

    SkCanvas canvas(renderTools.fDstBtmp);
    canvas.clear(SK_ColorTRANSPARENT);

    const int radCeil = static_cast<int>(ceil(mRadius));
    const auto& srcBtmp = renderTools.fSrcBtmp;
    const auto& texTile = data.fTexTile;
    auto srcRect = texTile.makeOutset(radCeil, radCeil);
    if(srcRect.intersect(srcRect, srcBtmp.bounds())) {
        SkBitmap tileSrc;
        srcBtmp.extractSubset(&tileSrc, srcRect);
        const int drawX = srcRect.left() - texTile.left();
        const int drawY = srcRect.top() - texTile.top();
        canvas.drawBitmap(tileSrc, drawX, drawY, &paint);
    }
}
