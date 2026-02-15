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

#include "boxsinglewidget.h"
#include "swt_abstraction.h"
#include "singlewidgettarget.h"
#include "optimalscrollarena/scrollwidgetvisiblepart.h"
#include "widgets/colorsettingswidget.h"

#include "Boxes/containerbox.h"
#include "widgets/qrealanimatorvalueslider.h"
#include "boxscroller.h"
#include "GUI/keysview.h"
#include "pointhelpers.h"
#include "GUI/BoxesList/boolpropertywidget.h"
#include "boxtargetwidget.h"
#include "Properties/boxtargetproperty.h"
#include "Properties/comboboxproperty.h"
#include "Animators/qstringanimator.h"
#include "RasterEffects/rastereffectcollection.h"
#include "Properties/boolproperty.h"
#include "Properties/boolpropertycontainer.h"
#include "Animators/qpointfanimator.h"
#include "Boxes/pathbox.h"
#include "canvas.h"
#include "BlendEffects/blendeffectcollection.h"
#include "BlendEffects/blendeffectboxshadow.h"
#include "Sound/eindependentsound.h"
#include "GUI/propertynamedialog.h"
#include "Animators/SmartPath/smartpathcollection.h"

#include "typemenu.h"
#include "themesupport.h"

#include <QMessageBox>

QPixmap* BoxSingleWidget::VISIBLE_ICON;
QPixmap* BoxSingleWidget::INVISIBLE_ICON;
QPixmap* BoxSingleWidget::BOX_CHILDREN_VISIBLE_ICON;
QPixmap* BoxSingleWidget::BOX_CHILDREN_HIDDEN_ICON;
QPixmap* BoxSingleWidget::ANIMATOR_CHILDREN_VISIBLE_ICON;
QPixmap* BoxSingleWidget::ANIMATOR_CHILDREN_HIDDEN_ICON;
QPixmap* BoxSingleWidget::LOCKED_ICON;
QPixmap* BoxSingleWidget::UNLOCKED_ICON;
QPixmap* BoxSingleWidget::MUTED_ICON;
QPixmap* BoxSingleWidget::UNMUTED_ICON;
QPixmap* BoxSingleWidget::ANIMATOR_RECORDING_ICON;
QPixmap* BoxSingleWidget::ANIMATOR_NOT_RECORDING_ICON;
QPixmap* BoxSingleWidget::ANIMATOR_DESCENDANT_RECORDING_ICON;
QPixmap* BoxSingleWidget::C_ICON;
QPixmap* BoxSingleWidget::G_ICON;
QPixmap* BoxSingleWidget::CG_ICON;
QPixmap* BoxSingleWidget::GRAPH_PROPERTY_ICON;
QPixmap* BoxSingleWidget::PROMOTE_TO_LAYER_ICON;

bool BoxSingleWidget::sStaticPixmapsLoaded = false;

#include "GUI/global.h"
#include "GUI/mainwindow.h"
#include "clipboardcontainer.h"
#include "Timeline/durationrectangle.h"
#include "GUI/coloranimatorbutton.h"
#include "canvas.h"
#include "PathEffects/patheffect.h"
#include "PathEffects/patheffectcollection.h"
#include "Sound/esoundobjectbase.h"

#include "widgets/ecombobox.h"

#include <QApplication>
#include <QDrag>
#include <QMenu>
#include <QInputDialog>

eComboBox* createCombo(QWidget* const parent)
{
    const auto result = new eComboBox(parent);
    result->setWheelMode(eComboBox::WheelMode::enabledWithCtrl);
    result->setFocusPolicy(Qt::NoFocus);
    return result;
}

BoxSingleWidget::BoxSingleWidget(BoxScroller * const parent)
    : SingleWidget(parent)
    , mParent(parent)
{
    mMainLayout = new QHBoxLayout(this);
    setLayout(mMainLayout);
    mMainLayout->setSpacing(0);
    mMainLayout->setContentsMargins(0, 0, 0, 0);
    mMainLayout->setAlignment(Qt::AlignLeft);

    mRecordButton = new PixmapActionButton(this);
    mRecordButton->setPixmapChooser([this]() {
        if (!mTarget) { return static_cast<QPixmap*>(nullptr); }
        const auto target = mTarget->getTarget();
        if (enve_cast<eBoxOrSound*>(target)) {
            return static_cast<QPixmap*>(nullptr);
        } else if (const auto asCAnim = enve_cast<ComplexAnimator*>(target)) {
            if (asCAnim->anim_isRecording()) {
                return BoxSingleWidget::ANIMATOR_RECORDING_ICON;
            } else {
                if (asCAnim->anim_isDescendantRecording()) {
                    return BoxSingleWidget::ANIMATOR_DESCENDANT_RECORDING_ICON;
                }
                return BoxSingleWidget::ANIMATOR_NOT_RECORDING_ICON;
            }
        } else if (const auto asAnim = enve_cast<Animator*>(target)) {
            if (asAnim->anim_isRecording()) {
                return BoxSingleWidget::ANIMATOR_RECORDING_ICON;
            }
            return BoxSingleWidget::ANIMATOR_NOT_RECORDING_ICON;
        }
        return static_cast<QPixmap*>(nullptr);
    });

    mMainLayout->addWidget(mRecordButton);
    connect(mRecordButton, &BoxesListActionButton::pressed,
            this, &BoxSingleWidget::switchRecordingAction);

    mContentButton = new PixmapActionButton(this);
    mContentButton->setPixmapChooser([this]() {
        if (!mTarget) { return static_cast<QPixmap*>(nullptr); }
        if (mTarget->childrenCount() == 0) {
            return static_cast<QPixmap*>(nullptr);
        }
        const auto target = mTarget->getTarget();
        if (enve_cast<eBoxOrSound*>(target)) {
            if (mTarget->contentVisible()) {
                return BoxSingleWidget::BOX_CHILDREN_VISIBLE_ICON;
            }
            return BoxSingleWidget::BOX_CHILDREN_HIDDEN_ICON;
        } else {
            if (mTarget->contentVisible()) {
                return BoxSingleWidget::ANIMATOR_CHILDREN_VISIBLE_ICON;
            }
            return BoxSingleWidget::ANIMATOR_CHILDREN_HIDDEN_ICON;
        }
    });

    mMainLayout->addWidget(mContentButton);
    connect(mContentButton, &BoxesListActionButton::pressed,
            this, &BoxSingleWidget::switchContentVisibleAction);

    mVisibleButton = new PixmapActionButton(this);
    mVisibleButton->setPixmapChooser([this]() {
        if (!mTarget) { return static_cast<QPixmap*>(nullptr); }
        const auto target = mTarget->getTarget();
        if (const auto ebos = enve_cast<eBoxOrSound*>(target)) {
            if (enve_cast<eSound*>(target)) {
                if (ebos->isVisible()) { return BoxSingleWidget::UNMUTED_ICON; }
                return BoxSingleWidget::MUTED_ICON;
            } else if (ebos->isVisible()) { return BoxSingleWidget::VISIBLE_ICON; }
            return BoxSingleWidget::INVISIBLE_ICON;
        } else if (const auto eEff = enve_cast<eEffect*>(target)) {
            if (eEff->isVisible()) { return BoxSingleWidget::VISIBLE_ICON; }
            return BoxSingleWidget::INVISIBLE_ICON;
        } /*else if (enve_cast<GraphAnimator*>(target)) {
            const auto bsvt = static_cast<BoxScroller*>(mParent);
            const auto keysView = bsvt->getKeysView();
            if (keysView) { return BoxSingleWidget::GRAPH_PROPERTY_ICON; }
            return static_cast<QPixmap*>(nullptr);
        }*/
        return static_cast<QPixmap*>(nullptr);
    });

    mMainLayout->addWidget(mVisibleButton);
    connect(mVisibleButton, &BoxesListActionButton::pressed,
            this, &BoxSingleWidget::switchBoxVisibleAction);

    mLockedButton = new PixmapActionButton(this);
    mLockedButton->setPixmapChooser([this]() {
        if (!mTarget) { return static_cast<QPixmap*>(nullptr); }
        const auto target = mTarget->getTarget();
        if (const auto box = enve_cast<BoundingBox*>(target)) {
            if (box->isLocked()) { return BoxSingleWidget::LOCKED_ICON; }
            return BoxSingleWidget::UNLOCKED_ICON;
        }
        return static_cast<QPixmap*>(nullptr);
    });

    mMainLayout->addWidget(mLockedButton);
    connect(mLockedButton, &BoxesListActionButton::pressed,
            this, &BoxSingleWidget::switchBoxLockedAction);

    mHwSupportButton = new PixmapActionButton(this);
    mHwSupportButton->setToolTip(tr("Adjust GPU/CPU Processing"));
    mHwSupportButton->setPixmapChooser([this]() {
        if (!mTarget) { return static_cast<QPixmap*>(nullptr); }
        const auto target = mTarget->getTarget();
        if (const auto rEff = enve_cast<RasterEffect*>(target)) {
            if (rEff->instanceHwSupport() == HardwareSupport::cpuOnly) {
                return BoxSingleWidget::C_ICON;
            } else if (rEff->instanceHwSupport() == HardwareSupport::gpuOnly) {
                return BoxSingleWidget::G_ICON;
            }
            return BoxSingleWidget::CG_ICON;
        }
        return static_cast<QPixmap*>(nullptr);
    });

    mMainLayout->addWidget(mHwSupportButton);
    connect(mHwSupportButton, &BoxesListActionButton::pressed, this, [this]() {
        if (!mTarget) { return; }
        const auto target = mTarget->getTarget();
        if (enve_cast<ShaderEffect*>(target)) { return; }
        if (const auto rEff = enve_cast<RasterEffect*>(target)) {
            rEff->switchInstanceHwSupport();
            Document::sInstance->actionFinished();
        }
    });

    mFillWidget = new QWidget(this);
    mMainLayout->addWidget(mFillWidget);
    mFillWidget->setObjectName("transparentWidget");

    mPromoteToLayerButton = new PixmapActionButton(this);
    mPromoteToLayerButton->setToolTip(tr("Promote to Layer"));
    mPromoteToLayerButton->setPixmapChooser([this]() {
        const auto targetGroup = getPromoteTargetGroup();
        if (targetGroup) {
            return BoxSingleWidget::PROMOTE_TO_LAYER_ICON;
        }
        return static_cast<QPixmap*>(nullptr);
    });

    mMainLayout->addWidget(mPromoteToLayerButton);
    connect(mPromoteToLayerButton, &BoxesListActionButton::pressed,
            this, [this]() {
        const auto targetGroup = getPromoteTargetGroup();
        if (targetGroup) {
            targetGroup->promoteToLayer();
            Document::sInstance->actionFinished();
        }
    });

    mValueSlider = new QrealAnimatorValueSlider(nullptr, this);
    mMainLayout->addWidget(mValueSlider, Qt::AlignRight);

    mSecondValueSlider = new QrealAnimatorValueSlider(nullptr, this);
    mMainLayout->addWidget(mSecondValueSlider, Qt::AlignRight);

    mColorButton = new ColorAnimatorButton(nullptr, this);
    mMainLayout->addWidget(mColorButton, Qt::AlignRight);
    mColorButton->setFixedHeight(mColorButton->height() - 6);
    mColorButton->setContentsMargins(0, 3, 0, 3);

    mPropertyComboBox = createCombo(this);
    mMainLayout->addWidget(mPropertyComboBox);

    mBlendModeCombo = createCombo(this);
    mMainLayout->addWidget(mBlendModeCombo);
    mBlendModeCombo->setObjectName("blendModeCombo");

    for(int modeId = int(SkBlendMode::kSrcOver);
        modeId <= int(SkBlendMode::kLastMode); modeId++) {
        const auto mode = static_cast<SkBlendMode>(modeId);
        mBlendModeCombo->addItem(SkBlendMode_Name(mode), modeId);
    }

    mBlendModeCombo->insertSeparator(8);
    mBlendModeCombo->insertSeparator(14);
    mBlendModeCombo->insertSeparator(21);
    mBlendModeCombo->insertSeparator(25);
    connect(mBlendModeCombo, qOverload<int>(&QComboBox::activated),
            this, &BoxSingleWidget::setCompositionMode);
    mBlendModeCombo->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);

    mPathBlendModeCombo = createCombo(this);
    mMainLayout->addWidget(mPathBlendModeCombo);
    mPathBlendModeCombo->addItems(QStringList() << "Normal" <<
                                  "Add" << "Remove" << "Remove reverse" <<
                                  "Intersect" << "Exclude" << "Divide");
    connect(mPathBlendModeCombo, qOverload<int>(&QComboBox::activated),
            this, &BoxSingleWidget::setPathCompositionMode);
    mPathBlendModeCombo->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);

    mFillTypeCombo = createCombo(this);
    mMainLayout->addWidget(mFillTypeCombo);
    mFillTypeCombo->addItems(QStringList() << "Winding" << "Even-odd");
    connect(mFillTypeCombo, qOverload<int>(&QComboBox::activated),
            this, &BoxSingleWidget::setFillType);
    mFillTypeCombo->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);

    mPropertyComboBox->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);
    mBoxTargetWidget = new BoxTargetWidget(this);
    mMainLayout->addWidget(mBoxTargetWidget);

    mCheckBox = new BoolPropertyWidget(this);
    mMainLayout->addWidget(mCheckBox);

    //eSizesUI::widget.addHalfSpacing(mMainLayout);

    hide();
    connectAppFont(this);
}

ContainerBox* BoxSingleWidget::getPromoteTargetGroup() {
    if(!mTarget) return nullptr;
    const auto target = mTarget->getTarget();
    ContainerBox* targetGroup = nullptr;
    if(const auto box = enve_cast<ContainerBox*>(target)) {
        if(box->isGroup()) targetGroup = box;
    } else if(enve_cast<RasterEffectCollection*>(target) ||
              enve_cast<BlendEffectCollection*>(target)) {
        const auto pTarget = static_cast<Property*>(target);
        const auto parentBox = pTarget->getFirstAncestor<BoundingBox>();
        if(parentBox && parentBox->isGroup()) {
            targetGroup = static_cast<ContainerBox*>(parentBox);
        }
    }
    return targetGroup;
}

void BoxSingleWidget::setCompositionMode(const int id) {
    if(!mTarget) return;
    const auto target = mTarget->getTarget();

    if(const auto boxTarget = enve_cast<BoundingBox*>(target)) {
        const int modeId = mBlendModeCombo->itemData(id).toInt();
        const auto mode = static_cast<SkBlendMode>(modeId);
        boxTarget->setBlendModeSk(mode);
    }
    Document::sInstance->actionFinished();
}

void BoxSingleWidget::setPathCompositionMode(const int id) {
    if(!mTarget) return;
    const auto target = mTarget->getTarget();

    if(const auto pAnim = enve_cast<SmartPathAnimator*>(target)) {
        pAnim->setMode(static_cast<SmartPathAnimator::Mode>(id));
    }
    Document::sInstance->actionFinished();
}

void BoxSingleWidget::setFillType(const int id) {
    if(!mTarget) return;
    const auto target = mTarget->getTarget();

    if(const auto pAnim = enve_cast<SmartPathCollection*>(target)) {
        pAnim->setFillType(static_cast<SkPathFillType>(id));
    }
    Document::sInstance->actionFinished();
}

void BoxSingleWidget::setComboProperty(ComboBoxProperty* const combo) {
    if(!combo) return mPropertyComboBox->hide();
    mPropertyComboBox->clear();
    mPropertyComboBox->addItems(combo->getValueNames());
    mPropertyComboBox->setCurrentIndex(combo->getCurrentValue());
    mTargetConn << connect(combo, &ComboBoxProperty::valueChanged,
                           mPropertyComboBox, &QComboBox::setCurrentIndex);
    mTargetConn << connect(mPropertyComboBox,
                           qOverload<int>(&QComboBox::activated),
                           this, [combo](const int id) {
        combo->setCurrentValue(id);
        Document::sInstance->actionFinished();
    });
    mPropertyComboBox->show();
}

void BoxSingleWidget::handlePropertySelectedChanged(const Property *prop)
{
    if (const auto graph = enve_cast<GraphAnimator*>(prop)) {
        const auto bsvt = static_cast<BoxScroller*>(mParent);
        const auto keysView = bsvt->getKeysView();
        if (keysView) {
            const bool graphSelected = keysView->graphIsSelected(graph);
            const bool isSelected = prop->prp_isSelected();
            if (graphSelected) {
                if (!isSelected) { keysView->graphRemoveViewedAnimator(graph); }
            } else {
                if (isSelected) { keysView->graphAddViewedAnimator(graph); }
            }
            Document::sInstance->actionFinished();
        }
    }
}

ColorAnimator *BoxSingleWidget::getColorTarget() const {
    const auto swt = mTarget->getTarget();
    ColorAnimator * color = nullptr;
    if(const auto ca = enve_cast<ComplexAnimator*>(swt)) {
        color = enve_cast<ColorAnimator*>(swt);
        if(!color) {
            const auto guiProp = ca->ca_getGUIProperty();
            color = enve_cast<ColorAnimator*>(guiProp);
        }
    }
    return color;
}

void BoxSingleWidget::clearAndHideValueAnimators() {
    mValueSlider->setTarget(nullptr);
    mValueSlider->hide();
    mSecondValueSlider->setTarget(nullptr);
    mSecondValueSlider->hide();
}

void BoxSingleWidget::setTargetAbstraction(SWT_Abstraction *abs) {
    mTargetConn.clear();
    SingleWidget::setTargetAbstraction(abs);
    if(!abs) return;
    const auto target = abs->getTarget();
    const auto prop = enve_cast<Property*>(target);
    if(!prop) return;
    mTargetConn << connect(prop, &SingleWidgetTarget::SWT_changedDisabled,
                           this, qOverload<>(&QWidget::update));
    mTargetConn << connect(prop, &Property::prp_nameChanged,
                           this, qOverload<>(&QWidget::update));

    const auto boolProperty = enve_cast<BoolProperty*>(prop);
    const auto boolPropertyContainer = enve_cast<BoolPropertyContainer*>(prop);
    const auto boxTargetProperty = enve_cast<BoxTargetProperty*>(prop);
    const auto comboBoxProperty = enve_cast<ComboBoxProperty*>(prop);
    const auto animator = enve_cast<Animator*>(prop);
    const auto graphAnimator = enve_cast<GraphAnimator*>(prop);
    const auto complexAnimator = enve_cast<ComplexAnimator*>(prop);
    const auto eboxOrSound = enve_cast<eBoxOrSound*>(prop);
    const auto eindependentSound = enve_cast<eIndependentSound*>(prop);
    const auto eeffect = enve_cast<eEffect*>(prop);
    const auto rasterEffect = enve_cast<RasterEffect*>(prop);
    const auto boundingBox = enve_cast<BoundingBox*>(prop);

    mMainLayout->setContentsMargins(0, 0, boundingBox ? 0 : 5, 0);
    mContentButton->setVisible(complexAnimator);
    mRecordButton->setVisible(animator && !eboxOrSound);
    mVisibleButton->setVisible(eboxOrSound || eeffect || graphAnimator);
    mLockedButton->setVisible(boundingBox);
    mHwSupportButton->setVisible(rasterEffect);
    {
        const auto targetGroup = getPromoteTargetGroup();
        if(boundingBox && targetGroup) {
            mTargetConn << connect(targetGroup,
                                   &ContainerBox::switchedGroupLayer,
                                   this, [this](const eBoxType type) {
                mBlendModeCombo->setEnabled(type == eBoxType::layer);
            });
        }
        mPromoteToLayerButton->setVisible(targetGroup);
        if(targetGroup) {
            mTargetConn << connect(targetGroup, &ContainerBox::switchedGroupLayer,
                                   this, [this](const eBoxType type) {
                mPromoteToLayerButton->setVisible(type == eBoxType::group);
            });
        }
    }
    mBoxTargetWidget->setVisible(boxTargetProperty);
    mCheckBox->setVisible(boolProperty || boolPropertyContainer);

    mPropertyComboBox->setVisible(comboBoxProperty);

    mPathBlendModeVisible = false;
    mBlendModeVisible = false;
    mFillTypeVisible = false;
    mSelected = false;

    mColorButton->setColorTarget(nullptr);
    mValueSlider->setTarget(nullptr);
    mSecondValueSlider->setTarget(nullptr);

    bool valueSliderVisible = false;
    bool secondValueSliderVisible = false;
    bool colorButtonVisible = false;

    if(boundingBox) {
        mBlendModeVisible = true;
        const auto blendName = SkBlendMode_Name(boundingBox->getBlendMode());
        mBlendModeCombo->setCurrentText(blendName);
        mBlendModeCombo->setEnabled(!boundingBox->isGroup());
        mTargetConn << connect(boundingBox, &BoundingBox::blendModeChanged,
                               this, [this](const SkBlendMode mode) {
            mBlendModeCombo->setCurrentText(SkBlendMode_Name(mode));
        });
    } else if(enve_cast<eSoundObjectBase*>(prop)) {
    } else if(boolProperty) {
        mCheckBox->setTarget(boolProperty);
        mTargetConn << connect(boolProperty, &BoolProperty::valueChanged,
                               this, [this]() { mCheckBox->update(); });
    } else if(boolPropertyContainer) {
        mCheckBox->setTarget(boolPropertyContainer);
        mTargetConn << connect(boolPropertyContainer,
                               &BoolPropertyContainer::valueChanged,
                               this, [this]() { mCheckBox->update(); });
    } else if(comboBoxProperty) {
        setComboProperty(comboBoxProperty);
    } else if(const auto qra = enve_cast<QrealAnimator*>(prop)) {
        mValueSlider->setTarget(qra);
        valueSliderVisible = true;
        mValueSlider->setIsLeftSlider(false);
    } else if(complexAnimator) {
        if(const auto col = enve_cast<ColorAnimator*>(prop)) {
            colorButtonVisible = true;
            mColorButton->setColorTarget(col);
        } else if(const auto coll = enve_cast<SmartPathCollection*>(prop)) {
            mFillTypeVisible = true;
            mFillTypeCombo->setCurrentIndex(static_cast<int>(coll->getFillType()));
            mTargetConn << connect(coll, &SmartPathCollection::fillTypeChanged,
                                   this, [this](const SkPathFillType type) {
                mFillTypeCombo->setCurrentIndex(static_cast<int>(type));
            });
        }
        if(complexAnimator && !abs->contentVisible()) {
            if(enve_cast<QPointFAnimator*>(prop)) {
                updateValueSlidersForQPointFAnimator();
                valueSliderVisible = mValueSlider->isVisible();
                secondValueSliderVisible = mSecondValueSlider->isVisible();
            } else {
                const auto guiProp = complexAnimator->ca_getGUIProperty();
                if(const auto qra = enve_cast<QrealAnimator*>(guiProp)) {
                    valueSliderVisible = true;
                    mValueSlider->setTarget(qra);
                    mValueSlider->setIsLeftSlider(false);
                    mSecondValueSlider->setTarget(nullptr);
                } else if(const auto col = enve_cast<ColorAnimator*>(guiProp)) {
                    mColorButton->setColorTarget(col);
                    colorButtonVisible = true;
                } else if(const auto combo = enve_cast<ComboBoxProperty*>(guiProp)) {
                    setComboProperty(combo);
                }
            }
        }
    } else if(boxTargetProperty) {
        mBoxTargetWidget->setTargetProperty(boxTargetProperty);
    } else if(const auto path = enve_cast<SmartPathAnimator*>(prop)) {
        mPathBlendModeVisible = true;
        mPathBlendModeCombo->setCurrentIndex(static_cast<int>(path->getMode()));
        mTargetConn << connect(path, &SmartPathAnimator::pathBlendModeChagned,
                               this, [this](const SmartPathAnimator::Mode mode) {
            mPathBlendModeCombo->setCurrentIndex(static_cast<int>(mode));
        });
    }

    if(animator) {
        mTargetConn << connect(animator, &Animator::anim_isRecordingChanged,
                               this, [this]() { mRecordButton->update(); });
    }
    if(eeffect) {
        if(rasterEffect) {
            mTargetConn << connect(rasterEffect, &RasterEffect::hardwareSupportChanged,
                                   this, [this]() { mHwSupportButton->update(); });
        }

        mTargetConn << connect(eeffect, &eEffect::effectVisibilityChanged,
                               this, [this]() { mVisibleButton->update(); });
    }
    if(boundingBox || eindependentSound) {
        const auto ptr = static_cast<eBoxOrSound*>(prop);
        mTargetConn << connect(ptr, &eBoxOrSound::visibilityChanged,
                               this, [this]() { mVisibleButton->update(); });
        mTargetConn << connect(ptr, &eBoxOrSound::selectionChanged,
                               this, qOverload<>(&QWidget::update));
        mTargetConn << connect(ptr, &eBoxOrSound::lockedChanged,
                               this, [this]() { mLockedButton->update(); });
    }
    if(!boundingBox && !eindependentSound) {
        mTargetConn << connect(prop, &Property::prp_selectionChanged,
                               this, qOverload<>(&QWidget::update));
        mTargetConn << connect(prop, &Property::prp_selectionChanged,
                               this, [this, prop]() { handlePropertySelectedChanged(prop); });
    }

    mValueSlider->setVisible(valueSliderVisible);
    mSecondValueSlider->setVisible(secondValueSliderVisible);
    mColorButton->setVisible(colorButtonVisible);

    updateCompositionBoxVisible();
    updatePathCompositionBoxVisible();
    updateFillTypeBoxVisible();
}

void BoxSingleWidget::loadStaticPixmaps(int iconSize)
{
    if (sStaticPixmapsLoaded) { return; }
    if (!ThemeSupport::hasIconSize(iconSize)) {
        QMessageBox::warning(nullptr,
                             tr("Icon issues"),
                             tr("<p>Requested icon size <b>%1</b> is not available,"
                                " expect blurry icons.</p>"
                                "<p>Note that this may happen if you change the display scaling"
                                " in Windows without restarting."
                                " If you still have issues after restarting please report this issue.</p>").arg(iconSize));
    }
    const auto pixmapSize = ThemeSupport::getIconSize(iconSize);
    qDebug() << "pixmaps size" << pixmapSize;
    VISIBLE_ICON = new QPixmap(QIcon::fromTheme("visible").pixmap(pixmapSize));
    INVISIBLE_ICON = new QPixmap(QIcon::fromTheme("hidden").pixmap(pixmapSize));
    BOX_CHILDREN_VISIBLE_ICON = new QPixmap(QIcon::fromTheme("visible-child").pixmap(pixmapSize));
    BOX_CHILDREN_HIDDEN_ICON = new QPixmap(QIcon::fromTheme("hidden-child").pixmap(pixmapSize));
    ANIMATOR_CHILDREN_VISIBLE_ICON = new QPixmap(QIcon::fromTheme("visible-child-small").pixmap(pixmapSize));
    ANIMATOR_CHILDREN_HIDDEN_ICON = new QPixmap(QIcon::fromTheme("hidden-child-small").pixmap(pixmapSize));
    LOCKED_ICON = new QPixmap(QIcon::fromTheme("locked").pixmap(pixmapSize));
    UNLOCKED_ICON = new QPixmap(QIcon::fromTheme("unlocked").pixmap(pixmapSize));
    MUTED_ICON = new QPixmap(QIcon::fromTheme("muted").pixmap(pixmapSize));
    UNMUTED_ICON = new QPixmap(QIcon::fromTheme("unmuted").pixmap(pixmapSize));
    ANIMATOR_RECORDING_ICON = new QPixmap(QIcon::fromTheme("record").pixmap(pixmapSize));
    ANIMATOR_NOT_RECORDING_ICON = new QPixmap(QIcon::fromTheme("norecord").pixmap(pixmapSize));
    ANIMATOR_DESCENDANT_RECORDING_ICON = new QPixmap(QIcon::fromTheme("record-child").pixmap(pixmapSize));
    C_ICON = new QPixmap(QIcon::fromTheme("cpu-active").pixmap(pixmapSize));
    G_ICON = new QPixmap(QIcon::fromTheme("gpu-active").pixmap(pixmapSize));
    CG_ICON = new QPixmap(QIcon::fromTheme("cpu-gpu").pixmap(pixmapSize));
    GRAPH_PROPERTY_ICON = new QPixmap(QIcon::fromTheme("graph_property_2").pixmap(pixmapSize));
    PROMOTE_TO_LAYER_ICON = new QPixmap(QIcon::fromTheme("layer").pixmap(pixmapSize));

    sStaticPixmapsLoaded = true;
}

void BoxSingleWidget::clearStaticPixmaps()
{
    if (!sStaticPixmapsLoaded) { return; }

    delete VISIBLE_ICON;
    delete INVISIBLE_ICON;
    delete BOX_CHILDREN_VISIBLE_ICON;
    delete BOX_CHILDREN_HIDDEN_ICON;
    delete ANIMATOR_CHILDREN_VISIBLE_ICON;
    delete ANIMATOR_CHILDREN_HIDDEN_ICON;
    delete LOCKED_ICON;
    delete UNLOCKED_ICON;
    delete MUTED_ICON;
    delete UNMUTED_ICON;
    delete ANIMATOR_RECORDING_ICON;
    delete ANIMATOR_NOT_RECORDING_ICON;
    delete ANIMATOR_DESCENDANT_RECORDING_ICON;
    delete PROMOTE_TO_LAYER_ICON;
    delete C_ICON;
    delete G_ICON;
    delete CG_ICON;
    delete GRAPH_PROPERTY_ICON;
}

void BoxSingleWidget::mousePressEvent(QMouseEvent *event) {
    if(!mTarget) return;
    if(event->x() < mFillWidget->x() ||
       event->x() > mFillWidget->x() + mFillWidget->width()) return;
    const auto target = mTarget->getTarget();
    if(event->button() == Qt::RightButton) {
        setSelected(true);
        QMenu menu(this);

        if(const auto pTarget = enve_cast<Property*>(target)) {
            const bool shiftPressed = event->modifiers() & Qt::ShiftModifier;
            if(enve_cast<BoundingBox*>(target) || enve_cast<eIndependentSound*>(target)) {
                const auto box = static_cast<eBoxOrSound*>(target);
                if(!box->isSelected()) box->selectionChangeTriggered(shiftPressed);
            } else {
                if(!pTarget->prp_isSelected()) pTarget->prp_selectionChangeTriggered(shiftPressed);
            }
            PropertyMenu pMenu(&menu, mParent->currentScene(), MainWindow::sGetInstance());
            pTarget->prp_setupTreeViewMenu(&pMenu);
        }
        menu.exec(event->globalPos());
        setSelected(false);
    } else {
        mDragPressPos = event->pos().x() > mFillWidget->x();
        mDragStartPos = event->pos();
    }
    Document::sInstance->actionFinished();
}

void BoxSingleWidget::mouseMoveEvent(QMouseEvent *event) {
    if(!mTarget) return;
    if(!mDragPressPos) return;
    if(!(event->buttons() & Qt::LeftButton)) return;
    const auto dist = (event->pos() - mDragStartPos).manhattanLength();
    if(dist < QApplication::startDragDistance()) return;
    const auto drag = new QDrag(this);
    {
        const auto prop = static_cast<Property*>(mTarget->getTarget());
        const QString name = prop->prp_getName();
        const int nameWidth = QApplication::fontMetrics().horizontalAdvance(name);
        QPixmap pixmap(mFillWidget->x() + nameWidth + eSizesUI::widget, height());
        render(&pixmap);
        drag->setPixmap(pixmap);
    }
    connect(drag, &QDrag::destroyed, this, &BoxSingleWidget::clearSelected);

    const auto mimeData = mTarget->getTarget()->SWT_createMimeData();
    if(!mimeData) return;
    setSelected(true);
    drag->setMimeData(mimeData);

    drag->installEventFilter(MainWindow::sGetInstance());
    drag->exec(Qt::CopyAction | Qt::MoveAction);
}

void BoxSingleWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (!mTarget) { return; }
    const auto target = mTarget->getTarget();

    const auto bbox = enve_cast<BoundingBox*>(target);
    if (event->button() == Qt::MidButton && bbox) {
        PropertyNameDialog::sRenameBox(bbox, this);
        return;
    }

    if (event->x() < mFillWidget->x() ||
        event->x() > mFillWidget->x() + mFillWidget->width()) { return; }
    setSelected(false);

    if (pointToLen(event->pos() - mDragStartPos) > eSizesUI::widget/2) { return; }

    const bool shiftPressed = event->modifiers() & Qt::ShiftModifier;
    if (enve_cast<BoundingBox*>(target) || enve_cast<eIndependentSound*>(target)) {
        const auto boxTarget = static_cast<eBoxOrSound*>(target);
        boxTarget->selectionChangeTriggered(shiftPressed);
        Document::sInstance->actionFinished();
    } else if (const auto pTarget = enve_cast<Property*>(target)) {
        pTarget->prp_selectionChangeTriggered(shiftPressed);
    }
}

void BoxSingleWidget::enterEvent(QEvent *)
{
#ifdef Q_OS_MAC
    setFocus();
#endif
    mHover = true;
    update();
}

void BoxSingleWidget::leaveEvent(QEvent *)
{
#ifdef Q_OS_MAC
    KeyFocusTarget::KFT_sSetLastTarget();
#endif
    mHover = false;
    update();
}

#ifdef Q_OS_MAC
void BoxSingleWidget::keyPressEvent(QKeyEvent *event)
{
    if (mHover) {
        MainWindow::sGetInstance()->processBoxesListKeyEvent(event);
    }
    SingleWidget::keyPressEvent(event);
}
#endif

void BoxSingleWidget::mouseDoubleClickEvent(QMouseEvent *e)
{
    Q_UNUSED(e)
    switchContentVisibleAction();
}

void BoxSingleWidget::prp_drawTimelineControls(QPainter * const p,
                               const qreal pixelsPerFrame,
                               const FrameRange &viewedFrames) {
    if(isHidden() || !mTarget) return;
    const auto target = mTarget->getTarget();
    if(const auto asAnim = enve_cast<Animator*>(target)) {
        asAnim->prp_drawTimelineControls(
                    p, pixelsPerFrame, viewedFrames, eSizesUI::widget);
    }
}

Key* BoxSingleWidget::getKeyAtPos(const int pressX,
                                  const qreal pixelsPerFrame,
                                  const int minViewedFrame) {
    if(isHidden() || !mTarget) return nullptr;
    const auto target = mTarget->getTarget();
    if(const auto asAnim = enve_cast<Animator*>(target)) {
        return asAnim->anim_getKeyAtPos(pressX, minViewedFrame,
                                        pixelsPerFrame, KEY_RECT_SIZE);
    }
    return nullptr;
}

TimelineMovable* BoxSingleWidget::getRectangleMovableAtPos(
                            const int pressX,
                            const qreal pixelsPerFrame,
                            const int minViewedFrame) {
    if(isHidden() || !mTarget) return nullptr;
    const auto target = mTarget->getTarget();
    if(const auto asAnim = enve_cast<Animator*>(target)) {
        return asAnim->anim_getTimelineMovable(
                    pressX, minViewedFrame, pixelsPerFrame);
    }
    return nullptr;
}

void BoxSingleWidget::getKeysInRect(const QRectF &selectionRect,
                                    const qreal pixelsPerFrame,
                                    QList<Key*>& listKeys) {
    if(isHidden() || !mTarget) return;
    const auto target = mTarget->getTarget();
    if(const auto asAnim = enve_cast<Animator*>(target)) {
        asAnim->anim_getKeysInRect(selectionRect, pixelsPerFrame,
                                   listKeys, KEY_RECT_SIZE);
    }
}

void BoxSingleWidget::paintEvent(QPaintEvent *) {
    if(!mTarget) return;
    QPainter p(this);
    const auto target = mTarget->getTarget();
    const auto prop = enve_cast<Property*>(target);
    if(!prop) return;
    if(prop->SWT_isDisabled()) p.setOpacity(.5);

    int nameX = mFillWidget->x();

    if (mHover) { p.fillRect(rect(), ThemeSupport::getThemeHighlightColor(40)); }

    const auto bsTarget = enve_cast<eBoxOrSound*>(prop);
    if (!bsTarget && prop->prp_isSelected()) {
        p.fillRect(mFillWidget->geometry(),
                   ThemeSupport::getThemeHighlightSelectedColor(25));
    }
    if (bsTarget) {
        nameX += eSizesUI::widget/4;
        const bool ss = enve_cast<eSoundObjectBase*>(prop);
        if (ss || enve_cast<BoundingBox*>(prop)) {
            p.fillRect(rect(), QColor(0, 0, 0, 50));
            if (bsTarget->isSelected()) {
                p.fillRect(mFillWidget->geometry(),
                           ThemeSupport::getThemeHighlightSelectedColor(50));
                p.setPen(Qt::white);
            } else {
                p.setPen(Qt::white);
            }
        } else if (enve_cast<BlendEffectBoxShadow*>(prop)) {
            p.fillRect(rect(), QColor(0, 255, 125, 50));
            nameX += eSizesUI::widget;
        }
    } else if(!enve_cast<ComplexAnimator*>(prop)) {
        if(const auto graphAnim = enve_cast<GraphAnimator*>(prop)) {
            const auto bswvp = static_cast<BoxScroller*>(mParent);
            const auto keysView = bswvp->getKeysView();
            if(keysView) {
                const bool selected = keysView->graphIsSelected(graphAnim);
                if(selected) {
                    const int id = keysView->graphGetAnimatorId(graphAnim);
                    const auto color = id >= 0 ?
                                keysView->sGetAnimatorColor(id) :
                                QColor(Qt::black);
                    const QRect visRect(mVisibleButton->pos(),
                                        mVisibleButton->size());
                    const int adj = qRound(4*qreal(mVisibleButton->width())/20);
                    p.fillRect(visRect.adjusted(adj, adj, -adj, -adj), color);
                }
            }
            if(const auto path = enve_cast<SmartPathAnimator*>(prop)) {
                const QRect colRect(QPoint{nameX, 0},
                                    QSize{eSizesUI::widget, eSizesUI::widget});
                p.setPen(Qt::NoPen);
                p.setRenderHint(QPainter::Antialiasing, true);
                p.setBrush(path->getPathColor());
                const int radius = qRound(eSizesUI::widget*0.2);
                p.drawEllipse(colRect.center() + QPoint(0, 2),
                              radius, radius);
                p.setRenderHint(QPainter::Antialiasing, false);
                nameX += eSizesUI::widget;
            }
        } else nameX += eSizesUI::widget;

        if(!enve_cast<Animator*>(prop)) nameX += eSizesUI::widget;
        p.setPen(Qt::white);
    } else { //if(enve_cast<ComplexAnimator*>(target)) {
        p.setPen(Qt::white);
    }

    const QRect textRect(nameX, 0, width() - nameX - eSizesUI::widget, eSizesUI::widget);
    const QString& name = prop->prp_getName();
    QTextOption opts(Qt::AlignVCenter);
    opts.setWrapMode(QTextOption::NoWrap);
    p.drawText(textRect, name, opts);
    if(mSelected) {
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(Qt::lightGray));
        p.drawRect(rect().adjusted(0, 0, -1, -1));
    }
    p.end();
}

void BoxSingleWidget::switchContentVisibleAction() {
    if(!mTarget) return;
    mTarget->switchContentVisible();
    Document::sInstance->actionFinished();
    //mParent->callUpdaters();
}

void BoxSingleWidget::switchRecordingAction() {
    if(!mTarget) return;
    const auto target = mTarget->getTarget();
    if(!target) return;
    if(const auto asAnim = enve_cast<Animator*>(target)) {
        asAnim->anim_switchRecording();
        Document::sInstance->actionFinished();
        update();
    }
}

void BoxSingleWidget::switchBoxVisibleAction() {
    if(!mTarget) return;
    const auto target = mTarget->getTarget();
    if(!target) return;
    if(const auto ebos = enve_cast<eBoxOrSound*>(target)) {
        ebos->switchVisible();
    } else if(const auto eEff = enve_cast<eEffect*>(target)) {
        eEff->switchVisible();
    } /*else if(const auto graph = enve_cast<GraphAnimator*>(target)) {
        const auto bsvt = static_cast<BoxScroller*>(mParent);
        const auto keysView = bsvt->getKeysView();
        if(keysView) {
            if(keysView->graphIsSelected(graph)) {
                keysView->graphRemoveViewedAnimator(graph);
            } else {
                keysView->graphAddViewedAnimator(graph);
            }
            Document::sInstance->actionFinished();
        }
    }*/
    Document::sInstance->actionFinished();
    update();
}

void BoxSingleWidget::switchBoxLockedAction() {
    if(!mTarget) return;
    static_cast<BoundingBox*>(mTarget->getTarget())->switchLocked();
    Document::sInstance->actionFinished();
    update();
}

void BoxSingleWidget::updateValueSlidersForQPointFAnimator() {
    if(!mTarget) return;
    const auto target = mTarget->getTarget();
    const auto asQPointFAnim = enve_cast<QPointFAnimator*>(target);
    if(!asQPointFAnim || mTarget->contentVisible()) return;
    if(width() - mFillWidget->x() > 10*eSizesUI::widget) {
        mValueSlider->setTarget(asQPointFAnim->getXAnimator());
        mValueSlider->show();
        mValueSlider->setIsLeftSlider(true);
        mSecondValueSlider->setTarget(asQPointFAnim->getYAnimator());
        mSecondValueSlider->show();
        mSecondValueSlider->setIsRightSlider(true);
    } else {
        clearAndHideValueAnimators();
    }
}

void BoxSingleWidget::updatePathCompositionBoxVisible() {
    if(!mTarget) return;
    if(mPathBlendModeVisible && width() - mFillWidget->x() > 8*eSizesUI::widget) {
        mPathBlendModeCombo->show();
    } else mPathBlendModeCombo->hide();
}

void BoxSingleWidget::updateCompositionBoxVisible() {
    if(!mTarget) return;
    if(mBlendModeVisible && width() - mFillWidget->x() > 10*eSizesUI::widget) {
        mBlendModeCombo->show();
    } else mBlendModeCombo->hide();
}

void BoxSingleWidget::updateFillTypeBoxVisible() {
    if(!mTarget) return;
    if(mFillTypeVisible && width() - mFillWidget->x() > 8*eSizesUI::widget) {
        mFillTypeCombo->show();
    } else mFillTypeCombo->hide();
}

void BoxSingleWidget::resizeEvent(QResizeEvent *) {
    updateCompositionBoxVisible();
    updatePathCompositionBoxVisible();
    updateFillTypeBoxVisible();
    updateValueSlidersForQPointFAnimator();
}
