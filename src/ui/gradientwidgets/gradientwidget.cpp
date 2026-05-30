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

#include "gradientwidget.h"
#include <QMouseEvent>
#include <QMenu>
#include <QAction>
#include <QPushButton>
#include <QHBoxLayout>

#include "gradientwidgets/displayedgradientswidget.h"
#include "Animators/gradient.h"

GradientWidget::GradientWidget(QWidget * const parent)
    : QWidget(parent)
    , mGradientsListWidget(nullptr)
    , mCurrentGradientWidget(nullptr)
    , mReordering(false)
    , mFirstMove(false)
    , mNumberVisibleGradients(6)
    , mHalfHeight(64)
    , mQuorterHeight(32)
    , mCurrentGradient(nullptr)
    , mCurrentColor(nullptr)
    , mCurrentColorId(0)
    , mCenterGradientId(1)
{
    setContentsMargins(0, 0, 0, 0);

    const auto mMainLayout = new QVBoxLayout(this);
    mMainLayout->setContentsMargins(0, 0, 0, 0);

    mGradientsListWidget = new GradientsListWidget(this);
    mCurrentGradientWidget = new CurrentGradientWidget(this);

    const auto aWidget = new QWidget(this);
    aWidget->setContentsMargins(0, 0, 0 ,0);
    aWidget->setFocusPolicy(Qt::NoFocus);

    const auto aLayout = new QHBoxLayout(aWidget);
    aLayout->setContentsMargins(0, 0, 0, 0);

    const auto bWidget = new QWidget(this);
    bWidget->setContentsMargins(0, 0, 0 ,0);
    bWidget->setFocusPolicy(Qt::NoFocus);

    const auto bLayout = new QHBoxLayout(bWidget);
    bLayout->setContentsMargins(0, 0, 0, 0);

    const auto cWidget = new QWidget(this);
    cWidget->setContentsMargins(0, 0, 0 ,0);
    cWidget->setFocusPolicy(Qt::NoFocus);

    const auto cLayout = new QVBoxLayout(cWidget);
    cLayout->setContentsMargins(0, 0, 0, 0);

    const auto buttonAdd = new QPushButton(QIcon::fromTheme("plus"), QString(""), this);
    const auto buttonRem = new QPushButton(QIcon::fromTheme("minus"), QString(""), this);
    const auto buttonDup = new QPushButton(QIcon::fromTheme("duplicate"), QString(""), this);
    const auto buttonAddColor = new QPushButton(QIcon::fromTheme("plus"), QString(""), this);
    const auto buttonRemColor = new QPushButton(QIcon::fromTheme("minus"), QString(""), this);

    buttonAdd->setToolTip(tr("Add Gradient"));
    buttonRem->setToolTip(tr("Remove Gradient"));
    buttonDup->setToolTip(tr("Duplicate Gradient"));
    buttonAddColor->setToolTip(tr("Add Color"));
    buttonRemColor->setToolTip(tr("Remove Color"));

    buttonAdd->setFocusPolicy(Qt::NoFocus);
    buttonRem->setFocusPolicy(Qt::NoFocus);
    buttonDup->setFocusPolicy(Qt::NoFocus);
    buttonAddColor->setFocusPolicy(Qt::NoFocus);
    buttonRemColor->setFocusPolicy(Qt::NoFocus);

    buttonAdd->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    buttonRem->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    buttonDup->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    buttonAddColor->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    buttonRemColor->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    mCurrentGradientWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mGradientsListWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    aLayout->addWidget(mGradientsListWidget);
    aLayout->addWidget(cWidget);

    bLayout->addWidget(mCurrentGradientWidget);
    bLayout->addWidget(buttonAddColor);
    bLayout->addWidget(buttonRemColor);

    cLayout->addWidget(buttonAdd);
    cLayout->addWidget(buttonRem);
    cLayout->addWidget(buttonDup);
    cLayout->addStretch();

    mMainLayout->addWidget(aWidget);
    mMainLayout->addWidget(bWidget);
    mMainLayout->addStretch();

    /*eSizesUI::widget.add(this, [this](const int size) {
        const qreal mult = (3 + mNumberVisibleGradients + 0.5);
        setFixedHeight(qRound(mult*size));
    });*/

    eSizesUI::widget.add(buttonAdd,
                         [&buttonAddColor,
                          &buttonRemColor,
                          &buttonAdd,
                          &buttonRem,
                          &buttonDup](const int size) {
        Q_UNUSED(size)
        buttonAddColor->setFixedSize(eSizesUI::button, eSizesUI::button);
        buttonRemColor->setFixedSize(eSizesUI::button, eSizesUI::button);
        buttonAdd->setFixedHeight(eSizesUI::button);
        buttonRem->setFixedHeight(eSizesUI::button);
        buttonDup->setFixedHeight(eSizesUI::button);
        /*buttonAddColor->setIconSize(QSize(size, size));
        buttonRemColor->setIconSize(QSize(size, size));
        buttonAdd->setIconSize(QSize(size, size));
        buttonRem->setIconSize(QSize(size, size));
        buttonDup->setIconSize(QSize(size, size));*/
    });

    const auto list = mGradientsListWidget->getList();
    connect(list, &DisplayedGradientsWidget::triggered,
            this, &GradientWidget::setCurrentGradient);
    connect(this, &GradientWidget::selectionChanged,
            list, &DisplayedGradientsWidget::setSelectedGradient);
    connect(list, &DisplayedGradientsWidget::triggered,
            this, &GradientWidget::triggered);

    connect(this, &GradientWidget::selectionChanged,
            mCurrentGradientWidget, &CurrentGradientWidget::setCurrentGradient);
    connect(mCurrentGradientWidget, &CurrentGradientWidget::selectedColorChanged,
            this, &GradientWidget::selectedColorChanged);

    connect(buttonAdd, &QPushButton::clicked,
            list, &DisplayedGradientsWidget::newGradient);
    connect(buttonRem, &QPushButton::clicked,
            list, &DisplayedGradientsWidget::removeSelectedGradient);
    connect(buttonDup, &QPushButton::clicked,
            list, &DisplayedGradientsWidget::duplicateSelectedGradient);

    connect(buttonAddColor, &QPushButton::clicked,
            mCurrentGradientWidget, &CurrentGradientWidget::colorAdd);
    connect(buttonRemColor, &QPushButton::clicked,
            mCurrentGradientWidget, &CurrentGradientWidget::colorRemove);
}

void GradientWidget::clearAll()
{
    mCurrentGradient = nullptr;
    mCenterGradientId = 1;
    mCurrentColorId = 0;
}

void GradientWidget::setCurrentGradient(Gradient *gradient)
{
    if (mCurrentGradient == gradient) { return; }
    mCurrentGradient = gradient;
    emit selectionChanged(gradient);
}

Gradient *GradientWidget::getCurrentGradient()
{
    return mCurrentGradient;
}

ColorAnimator *GradientWidget::getColorAnimator()
{
    return mCurrentGradientWidget->getColorAnimator();
}

void GradientWidget::showEvent(QShowEvent *e)
{
    Q_UNUSED(e)
    mCurrentGradientWidget->update();
    return QWidget::showEvent(e);
}
