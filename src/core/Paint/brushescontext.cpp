#include "brushescontext.h"

#include "Private/document.h"
QList<BrushCollectionData> BrushCollectionData::sData;

BrushContexedCollection::BrushContexedCollection(BrushesContext * const context,
                                                 const BrushCollectionData &raw) {
    fName = raw.fName;
    for(const auto& brushRaw : raw.fBrushes) {
        fBrushes << enve::make_shared<BrushContexedWrapper>(context, brushRaw);
    }
}

BrushesContext::BrushesContext(const QList<BrushCollectionData> &raw) {
    for(const auto& coll : raw) {
        fCollections << BrushContexedCollection(this, coll);
    }
    connect(Document::sInstance, &Document::bookmarkBrushAdded,
            this, [this](SimpleBrushWrapper* const brush) {
        const auto wrapper = brushWrapper(brush);
        wrapper->setBookmarked(true);
        emit bookmarkAdded(wrapper);
    });
    connect(Document::sInstance, &Document::bookmarkBrushRemoved,
            this, [this](SimpleBrushWrapper* const brush) {
        const auto wrapper = brushWrapper(brush);
        wrapper->setBookmarked(false);
        emit bookmarkRemoved(wrapper);
    });
}

BrushContexedWrapper *BrushesContext::brushWrapper(
        const SimpleBrushWrapper * const brush) {
    for(const auto& coll : qAsConst(fCollections)) {
        for(auto& wrapper : coll.fBrushes) {
            if(wrapper->getSimpleBrush() == brush) {
                return wrapper.get();
            }
        }
    }
    return nullptr;
}

bool BrushesContext::setSelectedWrapper(SimpleBrushWrapper * const wrapper) {
    for(const auto& coll : qAsConst(fCollections)) {
        for(auto& brush : coll.fBrushes) {
            if(brush->getSimpleBrush() == wrapper) {
                brush->setSelected(true);
                return true;
            }
        }
    }
    return false;
}

bool BrushesContext::setSelectedName(const QString &name) {
    for(const auto& coll : qAsConst(fCollections)) {
        for(auto& brush : coll.fBrushes) {
            if(brush->getBrushData().fName == name) {
                brush->setSelected(true);
                return true;
            }
        }
    }
    return false;
}
