#include "infinitemousemovement.h"

#include <QCursor>
#include <QScreen>
#include <QPoint>
#include <QSize>

bool isCursorInLeft(int x, QSize *screenSize) { return x == 0; }
bool isCursorInRight(int x, QSize *screenSize) { return x == screenSize->width(); }
bool isCursorInTop(int y, QSize *screenSize) { return y == 0; }
bool isCursorInBottom(int y, QSize *screenSize) { return y == screenSize->height(); }

void setCursorMovementInfinite(QScreen *screen) {
    auto size = screen.availableSize();
    auto point = QCursor::pos();
    auto x = point.x();
    auto y = point.y();

    // Depending on which border the cursor is at (left, right, top, down)
    // Move the cursor to the inverse direction + 1 px (not to get in an infinite loop)
    if (isCursorInLeft(point, size)) {
        // We set the cursor to the edge of the screen minus 1px
        QCursor::setPos(screen, size.width() - 1, y);
    };
    if (isCursorInRight(point, size)) {
        // We set the cursor to the edge of the screen plus 1px
        QCursor::setPos(screen, 1, y);
    }
    if(isCursorInTop(point, size)) {
        // We set the cursor to the edge of the screen minus 1px
        QCursor::setPos(screen, x, size.height() - 1);
    }
    if(isCursorInBottom(point, size)) {
        // We set the cursor to the edge of the screen plus 1px
        QCursor::setPos(screen, x, 1);
    }
}

void setCursorMovementDefault() {}
