/*
#
# Friction - https://friction.graphics
#
# Copyright (c) Ole-André Rodlie and contributors
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3.
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

#include "alignwidget.h"
#include "Private/document.h"

#include <QLabel>
#include <QHBoxLayout>
#include <QPushButton>
#include <QStandardItemModel>

#include "GUI/global.h"
#include "themesupport.h"

#define INDEX_ALIGN_GEOMETRY 0
#define INDEX_ALIGN_GEOMETRY_PIVOT 1
#define INDEX_ALIGN_PIVOT 2

#define INDEX_REL_SCENE 0
#define INDEX_REL_LAST_SELECTED 1
#define INDEX_REL_LAST_SELECTED_PIVOT 2
#define INDEX_REL_BOUNDINGBOX 3

using namespace Friction::Ui;

AlignWidget::AlignWidget(QWidget* const parent,
                         QToolBar* const toolbar)
    : QWidget(parent)
    , mAlignPivot(nullptr)
    , mRelativeTo(nullptr)
    , mToolbar(toolbar)
{
    if (mToolbar) { setupToolbar(); }
    else { setup(); }

    connect(this, &AlignWidget::alignTriggered,
            this, [](const Qt::Alignment align,
                   const AlignPivot pivot,
                   const AlignRelativeTo relativeTo) {
        const auto scene = *Document::sInstance->fActiveScene;
        if (!scene) { return; }
        scene->alignSelectedBoxes(align, pivot, relativeTo);
        Document::sInstance->actionFinished();
    });
}

void AlignWidget::setup()
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    const auto mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    setLayout(mainLayout);

    const auto combosLay = new QHBoxLayout;
    mainLayout->addLayout(combosLay);

    combosLay->addWidget(new QLabel(tr("Align"), this));
    mAlignPivot = new QComboBox(this);
    mAlignPivot->setMinimumWidth(20);

    mAlignPivot->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mAlignPivot->setFocusPolicy(Qt::NoFocus);
    mAlignPivot->addItem(tr("Geometry")); // INDEX_ALIGN_GEOMETRY
    mAlignPivot->addItem(tr("Geometry by Pivot")); // INDEX_ALIGN_GEOMETRY_PIVOT
    mAlignPivot->addItem(tr("Pivot")); // INDEX_ALIGN_PIVOT
    combosLay->addWidget(mAlignPivot);

    combosLay->addWidget(new QLabel(tr("To"), this));
    mRelativeTo = new QComboBox(this);
    mRelativeTo->setMinimumWidth(20);

    mRelativeTo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mRelativeTo->setFocusPolicy(Qt::NoFocus);
    mRelativeTo->addItem(tr("Scene")); // INDEX_REL_SCENE
    mRelativeTo->addItem(tr("Last Selected")); // INDEX_REL_LAST_SELECTED
    mRelativeTo->addItem(tr("Last Selected Pivot")); // INDEX_REL_LAST_SELECTED_PIVOT
    mRelativeTo->addItem(tr("Bounding Box")); // INDEX_REL_BOUNDINGBOX
    combosLay->addWidget(mRelativeTo);

    setComboBoxItemState(mRelativeTo, INDEX_REL_LAST_SELECTED_PIVOT, false);
    setComboBoxItemState(mRelativeTo, INDEX_REL_BOUNDINGBOX, false);

    const auto buttonsLay = new QHBoxLayout;
    mainLayout->addLayout(buttonsLay);
    mainLayout->addStretch();

    const auto leftButton = addAlignButton(Qt::AlignLeft,
                                           "pivot-align-left",
                                           tr("Align Left"));
    const auto hcenterButton = addAlignButton(Qt::AlignHCenter,
                                              "pivot-align-hcenter",
                                              tr("Align Horizontal Center"));
    const auto rightButton = addAlignButton(Qt::AlignRight,
                                            "pivot-align-right",
                                            tr("Align Right"));
    const auto topButton = addAlignButton(Qt::AlignTop,
                                          "pivot-align-top",
                                          tr("Align Top"));
    const auto vcenterButton = addAlignButton(Qt::AlignVCenter,
                                              "pivot-align-vcenter",
                                              tr("Align Vertical Center"));
    const auto bottomButton = addAlignButton(Qt::AlignBottom,
                                             "pivot-align-bottom",
                                             tr("Align Bottom"));

    buttonsLay->addWidget(leftButton);
    buttonsLay->addWidget(hcenterButton);
    buttonsLay->addWidget(rightButton);
    buttonsLay->addWidget(topButton);
    buttonsLay->addWidget(vcenterButton);
    buttonsLay->addWidget(bottomButton);

    eSizesUI::widget.add(leftButton, [leftButton,
                                      hcenterButton,
                                      rightButton,
                                      topButton,
                                      vcenterButton,
                                      bottomButton](const int size) {
        Q_UNUSED(size)
        leftButton->setFixedHeight(eSizesUI::button);
        hcenterButton->setFixedHeight(eSizesUI::button);
        rightButton->setFixedHeight(eSizesUI::button);
        topButton->setFixedHeight(eSizesUI::button);
        vcenterButton->setFixedHeight(eSizesUI::button);
        bottomButton->setFixedHeight(eSizesUI::button);
    });

    connectAlignPivot();
    connect(mRelativeTo,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [leftButton,
                   rightButton,
                   topButton,
                   bottomButton](int index) {
        leftButton->setEnabled(index != INDEX_REL_LAST_SELECTED_PIVOT);
        rightButton->setEnabled(index != INDEX_REL_LAST_SELECTED_PIVOT);
        topButton->setEnabled(index != INDEX_REL_LAST_SELECTED_PIVOT);
        bottomButton->setEnabled(index != INDEX_REL_LAST_SELECTED_PIVOT);
    });
}

void AlignWidget::setupToolbar()
{
    if (!mToolbar) { return; }

    ThemeSupport::setToolbarButtonStyle("FlatButton",
                                        mToolbar,
                                        mToolbar->addAction(QIcon::fromTheme("alignCenter"),
                                                            tr("Align")));

    mAlignPivot = new QComboBox(this);
    mAlignPivot->setMaximumWidth(200);

    mAlignPivot->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mAlignPivot->setFocusPolicy(Qt::NoFocus);
    mAlignPivot->addItem(tr("Geometry")); // INDEX_ALIGN_GEOMETRY
    mAlignPivot->addItem(tr("Geometry by Pivot")); // INDEX_ALIGN_GEOMETRY_PIVOT
    mAlignPivot->addItem(tr("Pivot")); // INDEX_ALIGN_PIVOT
    mToolbar->addSeparator();
    mToolbar->addWidget(mAlignPivot);

    mRelativeTo = new QComboBox(this);
    mRelativeTo->setMaximumWidth(200);

    mRelativeTo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    mRelativeTo->setFocusPolicy(Qt::NoFocus);
    mRelativeTo->addItem(tr("To Scene")); // INDEX_REL_SCENE
    mRelativeTo->addItem(tr("To Last Selected")); // INDEX_REL_LAST_SELECTED
    mRelativeTo->addItem(tr("To Last Selected Pivot")); // INDEX_REL_LAST_SELECTED_PIVOT
    mRelativeTo->addItem(tr("To Bounding Box")); // INDEX_REL_BOUNDINGBOX
    mToolbar->addSeparator();
    mToolbar->addWidget(mRelativeTo);

    setComboBoxItemState(mRelativeTo,
                         INDEX_REL_LAST_SELECTED_PIVOT,
                         false);
    setComboBoxItemState(mRelativeTo,
                         INDEX_REL_BOUNDINGBOX,
                         false);

    const auto leftButton = addAlignButton(Qt::AlignLeft,
                                           "pivot-align-left",
                                           tr("Align Left"));
    const auto hcenterButton = addAlignButton(Qt::AlignHCenter,
                                              "pivot-align-hcenter",
                                              tr("Align Horizontal Center"));
    const auto rightButton = addAlignButton(Qt::AlignRight,
                                            "pivot-align-right",
                                            tr("Align Right"));
    const auto topButton = addAlignButton(Qt::AlignTop,
                                          "pivot-align-top",
                                          tr("Align Top"));
    const auto vcenterButton = addAlignButton(Qt::AlignVCenter,
                                              "pivot-align-vcenter",
                                              tr("Align Vertical Center"));
    const auto bottomButton = addAlignButton(Qt::AlignBottom,
                                             "pivot-align-bottom",
                                             tr("Align Bottom"));

    mToolbar->addSeparator();
    mToolbar->addWidget(leftButton);
    mToolbar->addSeparator();
    mToolbar->addWidget(hcenterButton);
    mToolbar->addSeparator();
    mToolbar->addWidget(rightButton);
    mToolbar->addSeparator();
    mToolbar->addWidget(topButton);
    mToolbar->addSeparator();
    mToolbar->addWidget(vcenterButton);
    mToolbar->addSeparator();
    mToolbar->addWidget(bottomButton);
    mToolbar->addSeparator();

    eSizesUI::widget.add(mAlignPivot, [this](const int size) {
        Q_UNUSED(size)
        mAlignPivot->setFixedHeight(eSizesUI::button);
        mRelativeTo->setFixedHeight(eSizesUI::button);
    });

    connectAlignPivot();
    connect(mRelativeTo,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [leftButton,
                   rightButton,
                   topButton,
                   bottomButton](int index) {
        leftButton->setEnabled(index != INDEX_REL_LAST_SELECTED_PIVOT);
        rightButton->setEnabled(index != INDEX_REL_LAST_SELECTED_PIVOT);
        topButton->setEnabled(index != INDEX_REL_LAST_SELECTED_PIVOT);
        bottomButton->setEnabled(index != INDEX_REL_LAST_SELECTED_PIVOT);
    });
}

QAction *AlignWidget::addAlignAction(const Qt::Alignment &align,
                                     const QString &icon,
                                     const QString &title)
{
    const auto act = mToolbar->addAction(QIcon::fromTheme(icon), QString());
    act->setToolTip(title);
    connect(act, &QAction::triggered,
            this, [this, align](){ triggerAlign(align); });
    return act;
}

QPushButton *AlignWidget::addAlignButton(const Qt::Alignment &align,
                                         const QString &icon,
                                         const QString &title)
{
    const auto button = new QPushButton(this);
    button->setFocusPolicy(Qt::NoFocus);
    button->setIcon(QIcon::fromTheme(icon));
    button->setToolTip(title);
    connect(button, &QPushButton::pressed,
            this, [this, align]() { triggerAlign(align); });
    return button;
}

void AlignWidget::connectAlignPivot()
{
    connect(mAlignPivot,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        setComboBoxItemState(mRelativeTo,
                             INDEX_REL_LAST_SELECTED_PIVOT,
                             index == INDEX_ALIGN_PIVOT);
        setComboBoxItemState(mRelativeTo,
                             INDEX_REL_BOUNDINGBOX,
                             index == INDEX_ALIGN_PIVOT);
        if (index == INDEX_ALIGN_PIVOT) { mRelativeTo->setCurrentIndex(INDEX_REL_BOUNDINGBOX); }
        else { mRelativeTo->setCurrentIndex(INDEX_REL_SCENE); }
    });
}

void AlignWidget::triggerAlign(const Qt::Alignment align)
{
    const auto alignPivot = static_cast<AlignPivot>(mAlignPivot->currentIndex());
    const auto relativeTo = static_cast<AlignRelativeTo>(mRelativeTo->currentIndex());
    emit alignTriggered(align, alignPivot, relativeTo);
}

void AlignWidget::setComboBoxItemState(QComboBox *box,
                                       int index,
                                       bool enabled)
{
    auto model = qobject_cast<QStandardItemModel*>(box->model());
    if (!model) { return; }

    if (index >= box->count() || index < 0) { return; }

    auto item = model->item(index);
    if (!item) { return; }

    item->setEnabled(enabled);
}
