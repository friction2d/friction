#ifndef BLENDEFFECT_H
#define BLENDEFFECT_H

#include "Animators/eeffect.h"

#include "Properties/boxtargetproperty.h"

class PathBox;
struct ChildRenderData;

enum class BlendEffectType {
    move, targeted
};

class CORE_EXPORT BlendEffect : public eEffect {
    Q_OBJECT
    e_OBJECT
protected:
    BlendEffect(const QString &name,
                const BlendEffectType type);
public:
    virtual void blendSetup(ChildRenderData &data,
                            const int index,
                            const qreal relFrame,
                            QList<ChildRenderData> &delayed) const = 0;

    using UIDelayed = std::function<
        BlendEffect*(int id, BoundingBox* prev,
                     BoundingBox* next)>;
    virtual void detachedBlendUISetup(
            const qreal relFrame, const int drawId,
            QList<UIDelayed> &delayed) = 0;

    using Delayed = std::function<
        bool(int id, BoundingBox* prev,
             BoundingBox* next)>;
    virtual void detachedBlendSetup(const BoundingBox* const boxToDraw,
                                const qreal relFrame,
                                SkCanvas * const canvas,
                                const SkFilterQuality filter,
                                const int drawId,
                                QList<Delayed> &delayed) const = 0;
    virtual void drawBlendSetup(const qreal relFrame,
                                SkCanvas * const canvas) const = 0;

    void prp_setupTreeViewMenu(PropertyMenu * const menu);
    void prp_readProperty_impl(eReadStream& src);

    void writeIdentifier(eWriteStream &dst) const;
    void writeIdentifierXML(QDomElement& ele) const;

    bool isPathValid() const;
    SkPath clipPath(const qreal relFrame) const;
private:
    PathBox* clipPathSource() const;

    ConnContextQPtr<BoundingBox> mClipBox;
    const BlendEffectType mType;
    qsptr<BoxTargetProperty> mClipPath;
};

#endif // BLENDEFFECT_H
