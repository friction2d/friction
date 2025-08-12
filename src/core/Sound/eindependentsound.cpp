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

#include "eindependentsound.h"

#include <QInputDialog>

#include "ReadWrite/evformat.h"
#include "typemenu.h"
#include "Timeline/fixedlenanimationrect.h"
#include "fileshandler.h"

SoundFileHandler* soundFileHandlerGetter(const QString& path)
{
    return FilesHandler::sInstance->getFileHandler<SoundFileHandler>(path);
}

qsptr<FixedLenAnimationRect> createIndependentSoundDur(eIndependentSound* const sound)
{
    const auto result = enve::make_shared<FixedLenAnimationRect>(*sound, true);
    return result;
}

eIndependentSound::eIndependentSound()
    : eSoundObjectBase(createIndependentSoundDur(this))
    , mFileHandler(this,
                   [](const QString& path) {
                       return soundFileHandlerGetter(path);
                   },
                   [this](SoundFileHandler* obj) {
                       fileHandlerAfterAssigned(obj);
                   },
                   [this](ConnContext& conn, SoundFileHandler* obj) {
                       fileHandlerConnector(conn, obj);
                   })
{}

void eIndependentSound::fileHandlerConnector(ConnContext &conn,
                                             SoundFileHandler *obj)
{
    conn << connect(obj, &SoundFileHandler::pathChanged,
                    this, &eSoundObjectBase::prp_afterWholeInfluenceRangeChanged);
    conn << connect(obj, &SoundFileHandler::reloaded,
                    this, &eSoundObjectBase::prp_afterWholeInfluenceRangeChanged);
}

void eIndependentSound::fileHandlerAfterAssigned(SoundFileHandler *obj)
{
    if(obj) {
        const auto newDataHandler = FileDataCacheHandler::sGetDataHandler<SoundDataHandler>(obj->path());
        setSoundDataHandler(newDataHandler);
    } else {
        setSoundDataHandler(nullptr);
    }
}


void eIndependentSound::prp_setupTreeViewMenu(PropertyMenu * const menu)
{
    if (menu->hasActionsForType<eIndependentSound>()) { return; }
    menu->addedActionsForType<eIndependentSound>();
    eSoundObjectBase::prp_setupTreeViewMenu(menu);

    const PropertyMenu::PlainTriggeredOp stretchOp = [this]() {
        bool ok = false;
        const int stretch = QInputDialog::getInt(nullptr,
                                                 tr("Stretch"),
                                                 tr("Stretch"),
                                                 qRound(getStretch() * 100),
                                                 -1000,
                                                 1000,
                                                 1,
                                                 &ok);
        if (!ok) { return; }
        setStretch(stretch * 0.01);
    };
    menu->addPlainAction(QIcon::fromTheme("width"),
                         tr("Stretch"),
                         stretchOp);

    const PropertyMenu::PlainTriggeredOp deleteOp = [this]() {
        removeFromParent_k();
    };
    menu->addPlainAction(QIcon::fromTheme("trash"),
                         tr("Delete"),
                         deleteOp);
}

bool eIndependentSound::SWT_shouldBeVisible(const SWT_RulesCollection &rules,
                                            const bool parentSatisfies,
                                            const bool parentMainTarget) const
{
    Q_UNUSED(parentMainTarget);
    if (rules.fRule == SWT_BoxRule::visible && !isVisible()) { return false; }
    if (rules.fRule == SWT_BoxRule::selected && !isSelected()) { return false; }
    if (rules.fType == SWT_Type::sound) { return true; }
    if (rules.fType == SWT_Type::graphics) { return false; }
    return parentSatisfies;
}

void eIndependentSound::setFilePathNoRename(const QString &path)
{
    mFileHandler.assign(path);
}

void eIndependentSound::setFilePath(const QString &path)
{
    setFilePathNoRename(path);
    rename(QFileInfo(path).completeBaseName());
}

void eIndependentSound::updateDurationRectLength()
{
    if (cacheHandler() && getParentScene()) {
        const qreal secs = durationSeconds();
        const qreal fps = getCanvasFPS();
        const int frames = qCeil(qAbs(secs * fps * getStretch()));
        const auto flaRect = static_cast<FixedLenAnimationRect*>(getDurationRectangle());
        flaRect->setAnimationFrameDuration(frames);
    }
}

void eIndependentSound::prp_writeProperty_impl(eWriteStream& dst) const
{
    eBoxOrSound::prp_writeProperty_impl(dst);
    dst.writeFilePath(mFileHandler.path());
    dst << getStretch();
}

void eIndependentSound::prp_readProperty_impl(eReadStream& src)
{
    eBoxOrSound::prp_readProperty_impl(src);
    const QString filePath = src.readFilePath();
    if (!filePath.isEmpty()) { setFilePathNoRename(filePath); }
    if (src.evFileVersion() >= EvFormat::avStretch) {
        qreal stretch;
        src >> stretch;
        setStretch(stretch);
    }
}

QDomElement eIndependentSound::prp_writePropertyXML_impl(const Friction::Core::XmlExporter& exp) const
{
    auto result = eBoxOrSound::prp_writePropertyXML_impl(exp);
    const QString& absSrc = mFileHandler.path();
    Friction::Core::XevExportHelpers::setAbsAndRelFileSrc(absSrc, result, exp);

    result.setAttribute("stretch", QString::number(getStretch()));

    return result;
}

void eIndependentSound::prp_readPropertyXML_impl(const QDomElement& ele,
                                                 const Friction::Core::XmlImporter& imp)
{
    eBoxOrSound::prp_readPropertyXML_impl(ele, imp);
    const QString absSrc = Friction::Core::XevExportHelpers::getAbsAndRelFileSrc(ele, imp);
    if (!absSrc.isEmpty()) { setFilePathNoRename(absSrc); }

    setStretch(ele.attribute("stretch", "0").toDouble());
}
