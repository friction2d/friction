#include "ewritestream.h"

#include "exceptions.h"
#include "Paint/simplebrushwrapper.h"
#include "Paint/brushescontext.h"
#include "filefooter.h"
#include "framerange.h"

void eWriteFutureTable::write(eWriteStream &dst) {
    for(const auto& future : mFutures) {
        dst.write(&future, sizeof(eFuturePos));
    }
    dst << mFutures.count();
}

eWriteStream::eWriteStream(QIODevice * const dst) : mDst(dst), mFutureTable(dst) {}

void eWriteStream::setPath(const QString& path) {
    mDir.setPath(QFileInfo(path).path());
}

void eWriteStream::writeFutureTable() {
    mFutureTable.write(*this);
}

eWriteStream::FuturePosId eWriteStream::planFuturePos() {
    return mFutureTable.planFuturePos();
}

void eWriteStream::assignFuturePos(const eWriteStream::FuturePosId id) {
    mFutureTable.assignFuturePos(id.fId);
}

void eWriteStream::writeCheckpoint() {
    const qint64 pos = mDst->pos();
    write(&pos, sizeof(qint64));
}

qint64 eWriteStream::writeFile(QFile * const file) {
    if(!file) RuntimeThrow("No file to write");
    const bool openRes = file->open(QIODevice::ReadOnly);
    if(!openRes) RuntimeThrow("Could not open file");
    const qint64 size = file->size();
    const qint64 lineSize = 1024;
    char line[lineSize];
    const int nLines = static_cast<int>(size/lineSize);
    const qint64 rem = size - nLines*lineSize;
    for(int i = 0; i < nLines; i++) {
        file->read(&line[0], lineSize);
        write(&line[0], lineSize);
    }
    if(rem > 0) {
        file->read(&line[0], rem);
        write(&line[0], rem);
    }
    file->close();
    return size;
}

qint64 eWriteStream::writeCompressed(const void* const data, const qint64 len) {
    const auto charData = reinterpret_cast<const char*>(data);
    const auto ba = QByteArray::fromRawData(charData, len);
    const auto compressed = qCompress(ba);
    *this << compressed;
    return compressed.size();
}

void eWriteStream::writeFilePath(const QString& absPath) {
    const QString relPath = mDir.relativeFilePath(absPath);
    *this << absPath;
    *this << relPath;
}

eWriteStream& eWriteStream::operator<<(const QByteArray& val) {
    const int size = val.size();
    *this << size;
    mDst->write(val.data(), size);
    return *this;
}

eWriteStream &eWriteStream::operator<<(const bool val) {
    write(&val, sizeof(bool));
    return *this;
}

eWriteStream &eWriteStream::operator<<(const int val) {
    write(&val, sizeof(int));
    return *this;
}

eWriteStream &eWriteStream::operator<<(const uint val) {
    write(&val, sizeof(uint));
    return *this;
}

eWriteStream &eWriteStream::operator<<(const uint64_t val) {
    write(&val, sizeof(uint64_t));
    return *this;
}

eWriteStream &eWriteStream::operator<<(const int64_t val)
{
    int32_t safeVal = static_cast<int32_t>(val);
    write(&safeVal, sizeof(int32_t));
    return *this;
}

eWriteStream &eWriteStream::operator<<(const long long val)
{
    int32_t safeVal = static_cast<int32_t>(val);
    write(&safeVal, sizeof(int32_t));
    return *this;
}

eWriteStream &eWriteStream::operator<<(const iValueRange val) {
    write(&val, sizeof(iValueRange));
    return *this;
}

eWriteStream &eWriteStream::operator<<(const qreal val) {
    write(&val, sizeof(qreal));
    return *this;
}

eWriteStream &eWriteStream::operator<<(const QPointF &val) {
    write(&val, sizeof(QPointF));
    return *this;
}

eWriteStream &eWriteStream::operator<<(const QRectF &val) {
    write(&val, sizeof(QRectF));
    return *this;
}

eWriteStream &eWriteStream::operator<<(const QTransform &val)
{
    // QMatrix (48 bytes)
    qreal matrix2d[6] = {
        val.m11(), val.m12(),
        val.m21(), val.m22(),
        val.dx(),  val.dy()
    };

    write(matrix2d, sizeof(matrix2d));
    return *this;
}

eWriteStream &eWriteStream::operator<<(const QColor &val)
{
    struct Qt5QColorLayout {
        int32_t spec;
        uint16_t alpha;
        uint16_t red;
        uint16_t green;
        uint16_t blue;
        uint16_t pad;
        uint16_t pad2; // 16-bytes
    };

    Qt5QColorLayout oldColor;
    oldColor.spec = static_cast<int32_t>(val.spec());
    oldColor.alpha = (val.alpha() << 8) | val.alpha();
    oldColor.red   = (val.red() << 8) | val.red();
    oldColor.green = (val.green() << 8) | val.green();
    oldColor.blue  = (val.blue() << 8) | val.blue();
    oldColor.pad   = 0;
    oldColor.pad2  = 0;

    write(&oldColor, sizeof(Qt5QColorLayout));
    return *this;
}

eWriteStream &eWriteStream::operator<<(const QString &val) {
    const uint nChars = static_cast<uint>(val.length());
    write(&nChars, sizeof(uint));
    if(nChars != 0) write(val.utf16(), nChars*sizeof(ushort));
    return *this;
}

eWriteStream &eWriteStream::operator<<(SimpleBrushWrapper * const brush) {
    *this << (brush ? brush->getCollectionName() : "");
    *this << (brush ? brush->getBrushName() : "");
    return *this;
}
