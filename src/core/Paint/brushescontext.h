#ifndef BRUSHESCONTEXT_H
#define BRUSHESCONTEXT_H

//#include "libmypaintincludes.h"
#include "Paint/brushcontexedwrapper.h"

struct CORE_EXPORT BrushCollectionData {
    QString fName;
    QList<BrushData> fBrushes;

    static QList<BrushCollectionData> sData;
    static SimpleBrushWrapper * sGetBrush(const QString& collectionName,
                                          const QString& brushName) {
        for(const auto& coll : qAsConst(sData)) {
            if(coll.fName != collectionName) continue;
            for(const auto& brush : coll.fBrushes) {
                if(brush.fName == brushName) return brush.fWrapper.get();
            }
        }
        return nullptr;
    }
};

struct CORE_EXPORT BrushContexedCollection {
    BrushContexedCollection(BrushesContext* const context,
                            const BrushCollectionData& raw);
    QString fName;
    QList<qsptr<BrushContexedWrapper>> fBrushes;
};

class CORE_EXPORT BrushesContext : public SelfRef {
    Q_OBJECT
public:
    BrushesContext(const QList<BrushCollectionData>& raw);

    BrushContexedWrapper* brushWrapper(const SimpleBrushWrapper * const brush);
    bool setSelectedWrapper(SimpleBrushWrapper* const wrapper);
    bool setSelectedName(const QString& name);

    QList<BrushContexedCollection> fCollections;
signals:
    void bookmarkAdded(BrushContexedWrapper*);
    void bookmarkRemoved(BrushContexedWrapper*);
};
#endif // BRUSHESCONTEXT_H
