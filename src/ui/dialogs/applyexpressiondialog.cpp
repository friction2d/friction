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

#include "applyexpressiondialog.h"

#include <QDialogButtonBox>
#include <QLabel>

#include "widgets/twocolumnlayout.h"
#include "GUI/global.h"
#include "Private/scene.h"
#include "Private/document.h"

ApplyExpressionDialog::ApplyExpressionDialog(QrealAnimator* const target,
                                             QWidget * const parent)
    : Friction::Ui::Dialog(parent)
    , mTarget(target)
{
    setWindowTitle("Apply Expression " + target->prp_getName());
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    const auto mainLayout = new QVBoxLayout(this);
    setLayout(mainLayout);

    const auto layout = new TwoColumnLayout();

    const auto scene = target->getParentScene();

    mFirstFrame = new QSpinBox(this);
    mLastFrame = new QSpinBox(this);

    const int minFrame = scene ? scene->getMinFrame() : 0;
    const int maxFrame = scene ? scene->getMaxFrame() : 200;

    mFirstFrame->setRange(-99999, maxFrame);
    mFirstFrame->setValue(minFrame);
    mLastFrame->setRange(minFrame, 99999);
    mLastFrame->setValue(maxFrame);

    connect(mFirstFrame, qOverload<int>(&QSpinBox::valueChanged),
            mLastFrame, &QSpinBox::setMinimum);
    connect(mLastFrame, qOverload<int>(&QSpinBox::valueChanged),
            mFirstFrame, &QSpinBox::setMaximum);

    mAccuracy = new QDoubleSpinBox(this);
    mAccuracy->setRange(0.1, 100);
    mAccuracy->setValue(10);

    layout->addPair(new QLabel("First Frame:"), mFirstFrame);
    layout->addPair(new QLabel("Last Frame:"), mLastFrame);
    eSizesUI::widget.addSpacing(layout);
    layout->addPair(new QLabel("Accuracy:"), mAccuracy);

    const auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok |
                                              QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        const int firstFrame = mFirstFrame->value();
        const int lastFrame = mLastFrame->value();
        const qreal accuracy = mAccuracy->value();
        const FrameRange absRange{firstFrame, lastFrame};
        const auto relRange = mTarget->prp_absRangeToRelRange(absRange);
        mTarget->applyExpression(relRange, accuracy, true);
        accept();
        Document::sInstance->actionFinished();
    });

    mainLayout->addLayout(layout);
    eSizesUI::widget.addSpacing(mainLayout);
    mainLayout->addWidget(buttons);

    connect(target, &QObject::destroyed, this, &QDialog::reject);
}
