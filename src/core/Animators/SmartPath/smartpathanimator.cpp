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

#include "smartpathanimator.h"
#include "Animators/qrealpoint.h"
#include "smartpathcollection.h"
#include "Private/esettings.h"
#include "MovablePoints/pathpointshandler.h"

SmartPathAnimator::SmartPathAnimator() :
    InterOptimalAnimatorT<SmartPath>("path") {
    const auto ptsHandler = enve::make_shared<PathPointsHandler>(this);
    connect(this, &Property::prp_currentFrameChanged,
            this, [ptsHandler] {
        ptsHandler->updateAllPoints();
    });
    connect(eSettings::sInstance, &eSettings::settingsChanged,
            this, [ptsHandler] {
        ptsHandler->updateAllPointsRadius();
    });

    setPointsHandler(ptsHandler);
}

SmartPathAnimator::SmartPathAnimator(const SkPath &path) :
    SmartPathAnimator() {
    baseValue().setPath(path);
    mResultPath = path;
    updateAllPoints();
}

SmartPathAnimator::SmartPathAnimator(const SmartPath &baseValue) :
    SmartPathAnimator() {
    this->baseValue() = baseValue;
    setResultUpToDate(false);
    updateAllPoints();
}

QMimeData *SmartPathAnimator::SWT_createMimeData() {
    return new eMimeData(QList<SmartPathAnimator*>() << this);
}

#include "typemenu.h"
#include "Private/document.h"
#include <QInputDialog>
#include "GUI/propertynamedialog.h"

void SmartPathAnimator::prp_setupTreeViewMenu(PropertyMenu * const menu) {
    const auto parentWidget = menu->getParentWidget();
    menu->addPlainAction(QIcon::fromTheme("dialog-information"), tr("Rename"), [this, parentWidget]() {
        PropertyNameDialog::sRenameProperty(this, parentWidget);
    });
    const auto spClipboard = Document::sInstance->getSmartPathClipboard();
    menu->addPlainAction(QIcon::fromTheme("paste"), tr("Paste Path"), [this, spClipboard]() {
        pastePath(spClipboard->path());
    })->setEnabled(spClipboard);
    menu->addPlainAction(QIcon::fromTheme("copy"), tr("Copy Path"), [this] {
        const auto spClipboard = enve::make_shared<SmartPathClipboard>(baseValue());
        Document::sInstance->replaceClipboard(spClipboard);
    });
    menu->addSeparator();
    const PropertyMenu::PlainSelectedOp<SmartPathAnimator> dOp =
    [](SmartPathAnimator* const eff) {
        const auto parent = eff->getParent<DynamicComplexAnimatorBase<SmartPathAnimator>>();
        parent->removeChild(eff->ref<SmartPathAnimator>());
    };
    menu->addPlainAction(QIcon::fromTheme("trash"), tr("Delete Path(s)"), dOp);
    menu->addSeparator();
    Animator::prp_setupTreeViewMenu(menu);
}

void SmartPathAnimator::prp_drawCanvasControls(
        SkCanvas * const canvas, const CanvasMode mode,
        const float invScale, const bool ctrlPressed) {
    SkiaHelpers::drawOutlineOverlay(canvas, mResultPath, invScale,
                                    toSkMatrix(getTransform()),
                                    toSkColor(mPathColor));
    Property::prp_drawCanvasControls(canvas, mode, invScale, ctrlPressed);
}

void SmartPathAnimator::prp_readProperty_impl(eReadStream& src) {
    anim_readKeys(src);
    src >> baseValue();
    src.read(&mMode, sizeof(Mode));
    if(src.evFileVersion() > 3) {
        QString name; src >> name;
        prp_setName(name);
    }
    prp_afterWholeInfluenceRangeChanged();
}

void SmartPathAnimator::prp_writeProperty_impl(eWriteStream &dst) const {
    anim_writeKeys(dst);
    dst << baseValue();
    dst.write(&mMode, sizeof(Mode));
    dst << prp_getName();
}

void SmartPathAnimator::prp_readPropertyXML_impl(
        const QDomElement& ele, const Friction::Core::XmlImporter& imp) {
    Q_UNUSED(imp)
    const bool closed = ele.attribute("closed", "true") == "true";
    prp_setName(ele.attribute("name", "path"));
    const QString modeStr = ele.attribute("mode", "0");
    const int modeInt = Friction::Core::XmlExportHelpers::stringToInt(modeStr);
    mMode = static_cast<Mode>(modeInt);

    readValuesXML(ele, [closed](SmartPath& path, const QStringRef& str) {
        path.loadXML(str);
        path.setClosed(closed);
    });
}

QDomElement SmartPathAnimator::prp_writePropertyXML_impl(const Friction::Core::XmlExporter& exp) const {
    auto result = exp.createElement("Path");
    result.setAttribute("closed", isClosed() ? "true" : "false");
    result.setAttribute("name", prp_getName());
    result.setAttribute("mode", int(mMode));

    writeValuesXML(result, [](const SmartPath& path) {
        return path.toXML();
    });

    return result;
}

SkPath SmartPathAnimator::getPathAtRelFrame(const qreal frame) {
    const auto diff = prp_differencesBetweenRelFrames(
                qRound(frame), anim_getCurrentRelFrame());
    if(!diff) return getCurrentPath();
    const auto pn = anim_getPrevAndNextKeyIdF(frame);
    const int prevId = pn.first;
    const int nextId = pn.second;

    const auto prevKey = anim_getKeyAtIndex<SmartPathKey>(prevId);
    const auto nextKey = anim_getKeyAtIndex<SmartPathKey>(nextId);
    const bool adjKeys = pn.second - pn.first == 1;
    const auto keyAtRelFrame = adjKeys ? nullptr :
           anim_getKeyAtIndex<SmartPathKey>(pn.first + 1);
    if(keyAtRelFrame) return keyAtRelFrame->getValue().getPathAt();
    if(prevKey && nextKey) {
        SkPath result;
        const qreal nWeight = graph_prevKeyWeight(prevKey, nextKey, frame);
        SmartPath sPath;
        const auto& prevPath = prevKey->getValue();
        const auto& nextPath = nextKey->getValue();
        gInterpolate(prevPath, nextPath, nWeight, sPath);
        return sPath.getPathAt();
    } else if(!prevKey && nextKey) {
        return nextKey->getValue().getPathAt();
    } else if(prevKey && !nextKey) {
        return prevKey->getValue().getPathAt();
    }
    return baseValue().getPathAt();
}

void SmartPathAnimator::actionSetNormalNodeCtrlsMode(const int nodeId,
                                                     const CtrlsMode mode)
{
    prp_pushUndoRedoName(tr("Set Node Ctrls Mode"));

    prp_startTransform();
    getCurrentlyEdited()->actionSetNormalNodeCtrlsMode(nodeId, mode);
    prp_finishTransform();
    changed();
}

void SmartPathAnimator::actionDemoteToDissolved(const int nodeId,
                                                const bool approx)
{
    prp_pushUndoRedoName(tr("Demote Node"));

    prp_startTransform();
    getCurrentlyEdited()->actionDemoteToDissolved(nodeId, approx);
    prp_finishTransform();
    changed();
}

void SmartPathAnimator::actionPromoteToNormal(const int nodeId)
{
    prp_pushUndoRedoName(tr("Promote Node"));

    prp_startTransform();
    getCurrentlyEdited()->actionPromoteDissolvedNodeToNormal(nodeId);
    prp_finishTransform();
    changed();
}

qsptr<SmartPathAnimator> SmartPathAnimator::createFromDetached() {
    if(!hasDetached()) return nullptr;
    const auto baseDetached = baseValue().getAndClearLastDetached();
    SmartPath baseSmartPath(baseDetached);
    const auto newAnim = enve::make_shared<SmartPathAnimator>(baseSmartPath);
    const auto& keys = anim_getKeys();
    for(const auto &key : keys) {
        const auto spKey = static_cast<SmartPathKey*>(key);
        auto& sp = spKey->getValue();
        const auto keyDetached = sp.getAndClearLastDetached();
        SmartPath keySmartPath(keyDetached);
        const auto newKey = enve::make_shared<SmartPathKey>(
                    keySmartPath, key->getRelFrame(), newAnim.get());
        newAnim->anim_appendKey(newKey);
    }
    return newAnim;
}

void SmartPathAnimator::applyTransform(const QMatrix &transform) {
    const auto& keys = anim_getKeys();
    for(const auto &key : keys) {
        const auto spKey = static_cast<SmartPathKey*>(key);
        spKey->getValue().applyTransform(transform);
    }
    if(keys.isEmpty()) {
        prp_startTransform();
        baseValue().applyTransform(transform);
        prp_finishTransform();
    } else baseValue().applyTransform(transform);
    prp_afterWholeInfluenceRangeChanged();
}

const SkPath &SmartPathAnimator::getCurrentPath() {
    if(!resultUpToDate()) {
        mResultPath = getCurrentlyEdited()->getPathAt();
        setResultUpToDate(true);
    }
    return mResultPath;
}

void SmartPathAnimator::setMode(const SmartPathAnimator::Mode mode)
{
    if (mMode == mode) { return; }
    {
        prp_pushUndoRedoName("Set Path Blend Mode");
        UndoRedo ur;
        const auto oldValue = mMode;
        const auto newValue = mode;
        ur.fUndo = [this, oldValue]() {
            setMode(oldValue);
        };
        ur.fRedo = [this, newValue]() {
            setMode(newValue);
        };
        prp_addUndoRedo(ur);
    }
    mMode = mode;
    prp_afterWholeInfluenceRangeChanged();
    emit pathBlendModeChagned(mode);
}

void SmartPathAnimator::pastePath(const int frame, SmartPath path) {
    if(!anim_isRecording()) {
        baseValue() = path;
        return prp_afterWholeInfluenceRangeChanged();
    }
    const bool pasteClosed = path.isClosed();
    const bool baseClosed = baseValue().isClosed();

    if(pasteClosed != baseClosed) {
        if(pasteClosed) path.actionOpen();
        else {
            const auto& keys = anim_getKeys();
            for(const auto &key : keys) {
                const auto spKey = static_cast<SmartPathKey*>(key);
                auto& sp = spKey->getValue();
                sp.actionOpen();
            }
        }
    }

    const int pasteNodes = path.getNodeCount();
    const int baseNodes = baseValue().getNodeCount();
    const int addNodes = pasteNodes - baseNodes;
    if(addNodes > 0) {
        const auto& keys = anim_getKeys();
        for(const auto &key : keys) {
            const auto spKey = static_cast<SmartPathKey*>(key);
            auto& sp = spKey->getValue();
            sp.addDissolvedNodes(addNodes);
        }
    } else if(addNodes < 0) {
        path.addDissolvedNodes(-addNodes);
    }

    auto key = anim_getKeyAtRelFrame<SmartPathKey>(frame);
    if(key) key->setValue(path);
    else {
        const auto newKey = enve::make_shared<SmartPathKey>(path, frame, this);
        anim_appendKey(newKey);
    }
}

void SmartPathAnimator::updateAllPoints() {
    const auto handler = getPointsHandler();
    const auto pathHandler = static_cast<PathPointsHandler*>(handler);
    pathHandler->updateAllPoints();
}
