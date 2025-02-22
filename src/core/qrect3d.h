#ifndef QRECT3D_H
#define QRECT3D_H

#include <QRectF>

class QRect3D : public QRectF {
protected:
    QRect3D(int x, int y, int z, int width, int height) :
        QRectF(x, y, width, height),
        m_z(z) {}

    auto z() { return m_z; }
private:
    int m_z;
};

#endif // QRECT3D_H
