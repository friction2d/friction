#include "canvaswidget.h"

CanvasWidget::renderSk(SkCanvas* canvas) {
    _baseCanvas->renderSk(canvas);
}

void CanvasWidget::mousePressEvent(QMouseEvent *e) {
    _baseCanvas->mousePressEvent(e);
}

void CanvasWidget::mouseReleaseEvent(QMouseEvent *e) {
    _baseCanvas->mouseReleaseEvent(e);
}

void CanvasWidget::mouseMoveEvent(QMouseEvent *e) {
    _baseCanvas->mouseMoveEvent(e);
}

void CanvasWidget::mouseDoubleClickEvent(QMouseEvent *e) {
    _baseCanvas->mouseDoubleClickEvent(e);
}
