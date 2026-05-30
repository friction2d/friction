#include "ereadstream.h"
#include "Paint/simplebrushwrapper.h"
#include "Paint/brushescontext.h"
#include "filefooter.h"
#include "framerange.h"
#include "evformat.h"
#include "Boxes/boundingbox.h"

eReadFutureTable::eReadFutureTable(QIODevice * const main) : mMain(main) {}

void eReadFutureTable::read() {
    const qint64 savedPos = mMain->pos();
    int nFutures;
    mMain->read(reinterpret_cast<char*>(&nFutures), sizeof(int));

    mMain->seek(savedPos - nFutures*qint64(sizeof(eFuturePos)));
    for(int i = 0; i < nFutures; i++) {
        eFuturePos pos;
        mMain->read(reinterpret_cast<char*>(&pos), sizeof(eFuturePos));
        mFutures << pos;
    }
    mMain->seek(savedPos);
}

eReadStream::eReadStream(const int evFileVersion, QIODevice * const src) :
    mEvFileVersion(evFileVersion), mSrc(src), mFutureTable(src) {}

eReadStream::eReadStream(QIODevice * const src) :
    eReadStream(EvFormat::version, src) {}

eReadStream::~eReadStream() {
    for(const auto& task : mDoneTasks) task(*this);
}

void eReadStream::addReadBox(const int readId, BoundingBox* const box) {
    mReadBoxes[readId] = box;
}

BoundingBox *eReadStream::getBoxByReadId(const int readId) const {
    const auto it = mReadBoxes.find(readId);
    if(it == mReadBoxes.end()) return nullptr;
    else return it->second;
}

void eReadStream::addReadStreamDoneTask(const ReadStreamDoneTask& task) {
    mDoneTasks << task;
}

void eReadStream::setPath(const QString& path) {
    mDir.setPath(QFileInfo(path).path());
}

void eReadStream::readFutureTable() { mFutureTable.read(); }

eFuturePos eReadStream::readFuturePos() { return mFutureTable.readFuturePos(); }

bool eReadStream::seek(const eFuturePos &pos) {
    return mFutureTable.seek(pos);
}

void eReadStream::readCheckpoint(const QString &errMsg) {
    const qint64 sPos = mSrc->pos();
    qint64 pos; read(&pos, sizeof(qint64));
    if(pos != sPos)
        RuntimeThrow("The QIODevice::pos '" + QString::number(sPos) +
                     "' does not match the written QIODevice::pos '" +
                     QString::number(pos) + "'.\n" + errMsg);
}

QByteArray eReadStream::readCompressed() {
    QByteArray compressed; *this >> compressed;
    return qUncompress(compressed);
}

QString eReadStream::readFilePath() {
    QString readAbsPath; *this >> readAbsPath;
    if(mEvFileVersion < EvFormat::relativeFilePathSave) {
        return readAbsPath;
    }
    QString relPath; *this >> relPath;
    const QString absPath = mDir.absoluteFilePath(relPath);
    const QFileInfo fi(absPath);
    if(fi.exists()) return fi.absoluteFilePath();
    return readAbsPath;
}

eReadStream& eReadStream::operator>>(QByteArray& val) {
    int size; *this >> size;
    val.resize(size);
    mSrc->read(val.data(), size);
    return *this;
}

eReadStream &eReadStream::operator>>(bool &val) {
    read(&val, sizeof(bool));
    return *this;
}

eReadStream &eReadStream::operator>>(int &val) {
    read(&val, sizeof(int));
    return *this;
}

eReadStream &eReadStream::operator>>(uint &val) {
    read(&val, sizeof(uint));
    return *this;
}

eReadStream &eReadStream::operator>>(uint64_t &val) {
    read(&val, sizeof(uint64_t));
    return *this;
}

eReadStream &eReadStream::operator>>(iValueRange &val) {
    read(&val, sizeof(iValueRange));
    return *this;
}

eReadStream &eReadStream::operator>>(qreal &val) {
    read(&val, sizeof(qreal));
    return *this;
}

eReadStream &eReadStream::operator>>(QPointF &val) {
    read(&val, sizeof(QPointF));
    return *this;
}

eReadStream &eReadStream::operator>>(QRectF &val) {
    read(&val, sizeof(QRectF));
    return *this;
}

eReadStream &eReadStream::operator>>(QTransform &val)
{
    // Get QMatrix (48 bytes)
    qreal m[6];
    read(m, sizeof(m));

    // QMatrix => QTransform (m31=dx, m32=dy, m33=1.0)
    val.setMatrix(m[0], m[1], 0.0,
                  m[2], m[3], 0.0,
                  m[4], m[5], 1.0);

    return *this;
}

eReadStream &eReadStream::operator>>(QColor &val)
{
    struct Qt5QColorLayout {
        int32_t spec;
        uint16_t alpha;
        uint16_t red;
        uint16_t green;
        uint16_t blue;
        uint16_t pad;
        uint16_t pad2;
    };

    Qt5QColorLayout oldColor;
    read(&oldColor, sizeof(Qt5QColorLayout)); // 16 bytes

    int a = oldColor.alpha >> 8;
    int r = oldColor.red >> 8;
    int g = oldColor.green >> 8;
    int b = oldColor.blue >> 8;

    if (oldColor.spec == 2) { // HSV
        val = QColor::fromHsv(r, g, b, a);
    } else if (oldColor.spec == 4) { // HSL
        val = QColor::fromHsl(r, g, b, a);
    } else if (oldColor.spec == 1) { // RGB
        val = QColor::fromRgb(r, g, b, a);
    } else {
        val = QColor();
    }
    return *this;
}

eReadStream &eReadStream::operator>>(QString &val) {
    uint nChars; read(&nChars, sizeof(uint));
    if(nChars == 0) val = "";
    else {
        ushort * const chars = new ushort[nChars];
        read(chars, nChars*sizeof(ushort));
        val = QString::fromUtf16(chars, static_cast<int>(nChars));
        delete[] chars;
    }
    return *this;
}

eReadStream &eReadStream::operator>>(SimpleBrushWrapper *&brush) {
    QString brushCollection; *this >> brushCollection;
    QString brushName; *this >> brushName;
    brush = BrushCollectionData::sGetBrush(brushCollection, brushName);
    return *this;
}

int eReadStream::evFileVersion() const { return mEvFileVersion; }
