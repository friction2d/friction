#ifndef EWRITESTREAM_H
#define EWRITESTREAM_H

#include <QFile>
#include <QDir>

#include "efuturepos.h"
#include "../XML/runtimewriteid.h"

#include <functional>

class SimpleBrushWrapper;
struct iValueRange;
class eWriteStream;

class CORE_EXPORT eWriteFutureTable {
    friend class eWriteStream;
    eWriteFutureTable(QIODevice * const main) :
        mMain(main) {}

    void write(eWriteStream& dst);

    //! @brief Returns id to be used with assignFuturePos
    int planFuturePos() {
        const int id = mFutures.count();
        mFutures.append({-1, -1});
        return id;
    }

    void assignFuturePos(const int id) {
        mFutures.replace(id, {mMain->pos(), id});
    }
private:
    QList<eFuturePos> mFutures;
    QIODevice* const mMain;
};

class CORE_EXPORT eWriteStream {
    friend class MainWindow;
public:
    class FuturePosId {
        friend class eWriteStream;
        FuturePosId(const int id) : fId(id) {}
        const int fId;
    };

    eWriteStream(QIODevice* const dst);

    void setPath(const QString& path);

    Friction::Core::RuntimeIdToWriteId& objListIdConv() { return mObjectListIdConv; }

    void writeFutureTable();

    //! @brief Returns id to be used with assignFuturePos
    FuturePosId planFuturePos();

    void assignFuturePos(const FuturePosId id);

    void writeCheckpoint();

    qint64 writeFile(QFile* const file);

    inline qint64 write(const void* const data, const qint64 len) {
        return mDst->write(reinterpret_cast<const char*>(data), len);
    }

    qint64 writeCompressed(const void* const data, const qint64 len);

    eWriteStream& operator<<(const bool val);
    eWriteStream& operator<<(const int val);
    eWriteStream& operator<<(const uint val);
    eWriteStream& operator<<(const uint64_t val);
    eWriteStream& operator<<(const iValueRange val);
    eWriteStream& operator<<(const qreal val);
    eWriteStream& operator<<(const QPointF& val);
    eWriteStream& operator<<(const QRectF& val);
    eWriteStream& operator<<(const QMatrix& val);
    eWriteStream& operator<<(const QColor& val);
    eWriteStream& operator<<(const QString& val);
    eWriteStream& operator<<(const QByteArray& val);
    eWriteStream& operator<<(SimpleBrushWrapper* const brush);

    void writeFilePath(const QString& absPath);

    template <typename T>
    eWriteStream& operator<<(const T& value) {
        value.write(*this);
        return *this;
    }

private:
    QIODevice* const mDst;
    QDir mDir;
    eWriteFutureTable mFutureTable;
    Friction::Core::RuntimeIdToWriteId mObjectListIdConv;
};

#endif // EWRITESTREAM_H
