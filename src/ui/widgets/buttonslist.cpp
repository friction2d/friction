/*
# enve2d - https://github.com/enve2d
#
# Copyright (c) enve2d developers
# Copyright (C) 2016-2020 Maurycy Liebner
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
*/

#include "buttonslist.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QApplication>
#include <QResizeEvent>

#include "GUI/global.h"

class ElidedButton : public QPushButton {
public:
    ElidedButton(const QString& text, QWidget* parent) :
        QPushButton(parent), mFullText(text) {
        setSizePolicy(QSizePolicy::MinimumExpanding,
                      QSizePolicy::Fixed);
    }
protected:
    void resizeEvent(QResizeEvent* e) {
        const int width = e->size().width();
        const auto fm = fontMetrics();
        const auto text = fm.elidedText(mFullText, Qt::ElideLeft, width);
        setText(text);
    }
private:
    const QString mFullText;
};

ButtonsList::ButtonsList(const TextTriggerGetter& getter,
                         const int count,
                         QWidget* const parent)
    : QWidget(parent)
{
    const auto recentLay = new QVBoxLayout;
    recentLay->setSpacing(2);
    recentLay->setAlignment(Qt::AlignTop);
    recentLay->setContentsMargins(0, 0, 0, 0);

    setLayout(recentLay);

    for (int i = 0; i < count; i++) {
        const auto textTrigger = getter(i);
        const auto pathButton = new ElidedButton(textTrigger.first, this);
        pathButton->setObjectName("vmiddleButton");
        pathButton->setToolTip(gSingleLineTooltip(textTrigger.first));
        connect(pathButton, &QPushButton::released, textTrigger.second);
        recentLay->addWidget(pathButton, 0, Qt::AlignTop);
        if (i == 0) {
            if (i == count - 1) { break; }
            pathButton->setObjectName("topButton");
        } else if (i == count - 1) {
            pathButton->setObjectName("bottomButton");
        } else {
            pathButton->setObjectName("vmiddleButton");
        }
    }
}
