#ifndef EREADSTREAM_H
#define EREADSTREAM_H

#include "../core_global.h"
#include "efuturepos.h"
#include "../XML/runtimewriteid.h"

#include <functional>

#include <QIODevice>
#include <QDir>

class SimpleBrushWrapper;
struct iValueRange;
class BoundingBox;

class CORE_EXPORT eReadFutureTable {
    friend class eReadStream;
    eReadFutureTable(QIODevice* const main);

    bool seek(const eFuturePos& pos) {
        mFutureId = int(pos.fFutureTable) + 1;
        return mMain->seek(pos.fMain);
    }

    eFuturePos readFuturePos() {
        return mFutures.at(mFutureId++);
    }

    void read();
private:
    int mFutureId = 0;
    QList<eFuturePos> mFutures;
    QIODevice* const mMain;
};

class CORE_EXPORT eReadStream {
public:
    eReadStream(const int evFileVersion, QIODevice* const src);
    eReadStream(QIODevice* const src);

    ~eReadStream();

    void addReadBox(const int readId, BoundingBox * const box);
    BoundingBox *getBoxByReadId(const int readId) const;
    using ReadStreamDoneTask = std::function<void(eReadStream&)>;
    void addReadStreamDoneTask(const ReadStreamDoneTask& task);

    void setPath(const QString& path);

    Friction::Core::RuntimeIdToWriteId& objListIdConv() { return mObjectListIdConv; }

    void readFutureTable();

    eFuturePos readFuturePos();

    bool seek(const eFuturePos& pos);

    void readCheckpoint(const QString& errMsg);

    inline qint64 read(void* const data, const qint64 len) {
        return mSrc->read(reinterpret_cast<char*>(data), len);
    }

    QByteArray readCompressed();

    eReadStream& operator>>(bool &val);
    eReadStream& operator>>(int &val);
    eReadStream& operator>>(uint& val);
    eReadStream& operator>>(uint64_t& val);
    eReadStream& operator>>(iValueRange& val);
    eReadStream& operator>>(qreal &val);
    eReadStream& operator>>(QPointF &val);
    eReadStream& operator>>(QRectF &val);
    eReadStream& operator>>(QMatrix &val);
    eReadStream& operator>>(QColor& val);
    eReadStream& operator>>(QString &val);
    eReadStream& operator>>(QByteArray &val);
    eReadStream& operator>>(SimpleBrushWrapper*& brush);

    QString readFilePath();

    template <typename T>
    eReadStream& operator>>(T& value) {
        value.read(*this);
        return *this;
    }

    int evFileVersion() const;
private:
    std::map<int, BoundingBox*> mReadBoxes;
    QList<ReadStreamDoneTask> mDoneTasks;

    const int mEvFileVersion;
    QIODevice* const mSrc;
    QDir mDir;
    eReadFutureTable mFutureTable;
    Friction::Core::RuntimeIdToWriteId mObjectListIdConv;
};

#endif // EREADSTREAM_H
