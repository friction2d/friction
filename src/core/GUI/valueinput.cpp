/*
#
# Friction - https://friction.graphics
#
# Copyright (c) Ole-André Rodlie and contributors
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# See 'README.md' for more information.
#
*/

// Fork of enve - Copyright (C) 2016-2020 Maurycy Liebner

#include "valueinput.h"

#include <QPainter>
#include <QApplication>
#include <QScreen>

#include "skia/skiaincludes.h"
#include "skia/skqtconversions.h"
#include "GUI/global.h"
#include <QGuiApplication>

ValueInput::ValueInput() {
    QScreen *screen = QGuiApplication::primaryScreen();
    qreal dpi = screen ? screen->logicalDotsPerInchX() : 96.0;
    qreal dpr = screen ? screen->devicePixelRatio() : 1.0;
    mFont = toSkFont(QApplication::font(),
                     dpi * dpr,
                     72);
}

void ValueInput::draw(SkCanvas *canvas, const int y) {
    SkPaint paint;
    QScreen *screen = QGuiApplication::primaryScreen();
    const qreal pixelRatio = screen ? screen->devicePixelRatio() : 1.0;
    const auto transStr = getText();
    const int textWidth = QApplication::fontMetrics().horizontalAdvance(transStr)*pixelRatio;
    const SkRect inputRect = SkRect::MakeXYWH(2*eSizesUI::widget*pixelRatio,
                                              y*pixelRatio,
                                              textWidth + eSizesUI::widget*pixelRatio,
                                              eSizesUI::widget*pixelRatio);
    paint.setStyle(SkPaint::kFill_Style);
    paint.setColor(SkColorSetRGB(225, 225, 225));
    canvas->drawRect(inputRect, paint);
    paint.setColor(SK_ColorBLACK);
    paint.setStyle(SkPaint::kStrokeAndFill_Style);
    paint.setStrokeWidth(.1f*pixelRatio);

    const auto stdStr = transStr.toStdString();
    canvas->drawString(stdStr.c_str(),
           inputRect.x() + eSizesUI::widget*0.5f*pixelRatio,
           inputRect.y() + inputRect.height()*0.5f + eSizesUI::font*0.2f*pixelRatio,
           mFont, paint);
}

void ValueInput::draw(QPainter *p, const int y) {
    p->setFont(QApplication::font());
    const auto transStr = getText();
    const int textWidth = QApplication::fontMetrics().horizontalAdvance(transStr);
    const QRect inputRect(2*eSizesUI::widget, y,
                          textWidth + eSizesUI::widget, eSizesUI::widget);
    p->fillRect(inputRect, QColor(255, 255, 255, 55));
    p->drawText(qRound(inputRect.x() + eSizesUI::widget*0.5),
                qRound(inputRect.y() + inputRect.height()*0.5 + eSizesUI::font*0.2),
                transStr);
}

void ValueInput::clearAndDisableInput() {
    mXYMode = DirectionMode::xy;
    mInputEnabled = false;
    mInputText = "";
}

void ValueInput::updateInputValue() {
    if(mInputText.isEmpty()) {
        mInputEnabled = false;
    } else {
        mInputEnabled = true;
        if(mInputText == "-") mInputValue = -1;
        else mInputValue = mInputText.toDouble();
    }
}
#include <QKeyEvent>
bool ValueInput::handleTransormationInputKeyEvent(const int key) {
    if(key == Qt::Key_Minus) {
        if( ((mInputText.isEmpty()) ? false : mInputText.at(0) == '-') ) {
            mInputText.remove("-");
        } else {
            mInputText.prepend("-");
        }
        updateInputValue();
    } else if(key == Qt::Key_0) {
        mInputText += "0";
        if(mInputText == "0" || mInputText == "-0") mInputText += ".";
        updateInputValue();
    } else if(key == Qt::Key_1) {
        mInputText += "1";
        updateInputValue();
    } else if(key == Qt::Key_2) {
        mInputText += "2";
        updateInputValue();
    } else if(key == Qt::Key_3) {
        mInputText += "3";
        updateInputValue();
    } else if(key == Qt::Key_4) {
        mInputText += "4";
        updateInputValue();
    } else if(key == Qt::Key_5) {
        mInputText += "5";
        updateInputValue();
    } else if(key == Qt::Key_6) {
        mInputText += "6";
        updateInputValue();
    } else if(key == Qt::Key_7) {
        mInputText += "7";
        updateInputValue();
    } else if(key == Qt::Key_8) {
        mInputText += "8";
        updateInputValue();
    } else if(key == Qt::Key_9) {
        mInputText += "9";
        updateInputValue();
    } else if(key == Qt::Key_Period || key == Qt::Key_Comma) {
        if(!mInputText.contains(".")) {
            mInputText += ".";
            updateInputValue();
        }
    } else if(key == Qt::Key_Backspace) {
        mInputText.chop(1);
        if(mInputText == "0" ||
           mInputText == "-" ||
           mInputText == "-0") mInputText = "";
        updateInputValue();
    } else {
        return false;
    }

    return true;
}
