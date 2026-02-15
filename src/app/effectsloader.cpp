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

#include "effectsloader.h"

#include <QFileSystemWatcher>
#include <QFileSystemModel>
#include <iostream>

#include "widgets/colorwidgetshaders.h"
#include "ShaderEffects/shadereffectcreator.h"
#include "PathEffects/custompatheffect.h"
#include "PathEffects/custompatheffectcreator.h"
#include "RasterEffects/rastereffect.h"
#include "RasterEffects/customrastereffect.h"
#include "RasterEffects/customrastereffectcreator.h"
#include "Boxes/ecustombox.h"
#include "Boxes/customboxcreator.h"
#include "appsupport.h"

EffectsLoader* EffectsLoader::sInstance = nullptr;

EffectsLoader::EffectsLoader()
{
    Q_ASSERT(!sInstance);
    sInstance = this;
    reloadDisabledShaders();
}

EffectsLoader::~EffectsLoader()
{
    makeCurrent();
    glDeleteBuffers(1, &GL_PLAIN_SQUARE_VBO);
    glDeleteVertexArrays(1, &mPlainSquareVAO);
    glDeleteBuffers(1, &GL_TEXTURED_SQUARE_VBO);
    glDeleteVertexArrays(1, &mTexturedSquareVAO);

    glUseProgram(0);

    glDeleteProgram(HUE_PROGRAM.fID);
    glDeleteProgram(HSV_SATURATION_PROGRAM.fID);
    glDeleteProgram(VALUE_PROGRAM.fID);

    glDeleteProgram(HSL_SATURATION_PROGRAM.fID);
    glDeleteProgram(LIGHTNESS_PROGRAM.fID);

    glDeleteProgram(RED_PROGRAM.fID);
    glDeleteProgram(GREEN_PROGRAM.fID);
    glDeleteProgram(BLUE_PROGRAM.fID);

    glDeleteProgram(ALPHA_PROGRAM.fID);

    glDeleteProgram(PLAIN_PROGRAM.fID);

    glDeleteProgram(GRADIENT_PROGRAM.fID);

    glDeleteProgram(BORDER_PROGRAM.fID);
    glDeleteProgram(DOUBLE_BORDER_PROGRAM.fID);

    for (const auto& shaderEffect : qAsConst(ShaderEffectCreator::sEffectCreators)) {
        glDeleteProgram(shaderEffect->fProgram->fId);
    }

    doneCurrent();
}

void EffectsLoader::initializeGpu()
{
    //std::cout << "Entered initializeGpu" << std::endl;

    OffscreenQGL33c::initialize();
    //std::cout << "OffscreenQGL33c initialized" << std::endl;
    makeCurrent();
    //std::cout << "Make OffscreenQGL33c current" << std::endl;

    iniPlainVShaderVBO(this);
    //std::cout << "iniPlainVShaderVBO" << std::endl;
    iniPlainVShaderVAO(this, mPlainSquareVAO);
    //std::cout << "iniPlainVShaderVAO" << std::endl;
    iniTexturedVShaderVBO(this);
    //std::cout << "iniTexturedVShaderVBO" << std::endl;
    iniTexturedVShaderVAO(this, mTexturedSquareVAO);
    //std::cout << "iniTexturedVShaderVAO" << std::endl;
    iniColorPrograms(this);
    //std::cout << "iniColorPrograms" << std::endl;

    doneCurrent();
    //std::cout << "Done OffscreenQGL33c current" << std::endl;
}

void iniCustomBox(const QString &path)
{
    try {
        CustomBoxCreator::sLoadCustom(path);
    } catch(...) {
        RuntimeThrow("Error while loading Box from '" + path + "'");
    }
}

void iniIfCustomBox(const QString &path)
{
    if (!QFile::exists(path) ||
        !QLibrary::isLibrary(path)) { return; }
    try {
        iniCustomBox(path);
    } catch(const std::exception& e) {
        gPrintExceptionCritical(e);
    }
}

void EffectsLoader::iniCustomBoxes()
{
    // ignore for now
    /*QDir(eSettings::sSettingsDir()).mkdir("Boxes");
    const QString dirPath = eSettings::sSettingsDir() + "/Boxes";
    QDirIterator dirIt(dirPath, QDirIterator::NoIteratorFlags);
    while(dirIt.hasNext()) iniIfCustomBox(dirIt.next());

    const auto newFileWatcher = QSharedPointer<QFileSystemModel>(
                new QFileSystemModel);
    newFileWatcher->setRootPath(dirPath);
    connect(newFileWatcher.get(), &QFileSystemModel::directoryLoaded,
            this, [this, newFileWatcher]() {
        disconnect(newFileWatcher.get(), &QFileSystemModel::directoryLoaded,
                   this, nullptr);
        connect(newFileWatcher.get(), &QFileSystemModel::rowsInserted, this,
        [newFileWatcher](const QModelIndex &parent, int first, int last) {
            for(int row = first; row <= last; row++) {
                const auto rowIndex = newFileWatcher->index(row, 0, parent);
                const QString path = newFileWatcher->filePath(rowIndex);
                iniIfCustomBox(path);
            }
        });
    });*/
}

const QStringList EffectsLoader::getLoadedShaderEffects()
{
    return mLoadedGREPaths;
}

void EffectsLoader::reloadDisabledShaders()
{
    mShadersDisabled = AppSupport::getSettings("settings",
                                               "DisabledShaders").toStringList();
}

void iniCustomPathEffect(const QString &path)
{
    try {
        CustomPathEffectCreator::sLoadCustom(path);
    } catch(...) {
        RuntimeThrow("Error while loading PathEffect from '" + path + "'");
    }
}

void iniIfCustomPathEffect(const QString &path)
{
    if (!QFile::exists(path) ||
        !QLibrary::isLibrary(path)) { return; }
    try {
        iniCustomPathEffect(path);
    } catch(const std::exception& e) {
        gPrintExceptionCritical(e);
    }
}

void EffectsLoader::iniCustomPathEffects()
{
    // ignore for now
    /*QDir(eSettings::sSettingsDir()).mkdir("PathEffects");
    const QString dirPath = eSettings::sSettingsDir() + "/PathEffects";
    QDirIterator dirIt(dirPath, QDirIterator::NoIteratorFlags);
    while(dirIt.hasNext()) iniIfCustomPathEffect(dirIt.next());

    const auto newFileWatcher = QSharedPointer<QFileSystemModel>(
                new QFileSystemModel);
    newFileWatcher->setRootPath(dirPath);
    connect(newFileWatcher.get(), &QFileSystemModel::directoryLoaded,
            this, [this, newFileWatcher]() {
        disconnect(newFileWatcher.get(), &QFileSystemModel::directoryLoaded,
                   this, nullptr);
        connect(newFileWatcher.get(), &QFileSystemModel::rowsInserted, this,
        [newFileWatcher](const QModelIndex &parent, int first, int last) {
            for(int row = first; row <= last; row++) {
                const auto rowIndex = newFileWatcher->index(row, 0, parent);
                const QString path = newFileWatcher->filePath(rowIndex);
                iniIfCustomPathEffect(path);
            }
        });
    });*/
}

void EffectsLoader::iniCustomRasterEffect(const QString &soPath)
{
    try {
        CustomRasterEffectCreator::sLoadCustom(soPath);
    } catch(...) {
        RuntimeThrow("Error while loading RasterEffect from '" + soPath + "'");
    }
}

void EffectsLoader::iniIfCustomRasterEffect(const QString &path)
{
    if (!QFile::exists(path) ||
        !QLibrary::isLibrary(path)) { return; }
    try {
        iniCustomRasterEffect(path);
    } catch(const std::exception& e) {
        gPrintExceptionCritical(e);
    }
}

void EffectsLoader::iniCustomRasterEffects()
{
    // ignore for now
    /*QDir(eSettings::sSettingsDir()).mkdir("RasterEffects");
    const QString dirPath = eSettings::sSettingsDir() + "/RasterEffects";
    QDirIterator dirIt(dirPath, QDirIterator::NoIteratorFlags);
    while(dirIt.hasNext()) iniIfCustomRasterEffect(dirIt.next());

    const auto newFileWatcher = QSharedPointer<QFileSystemModel>(new QFileSystemModel);
    newFileWatcher->setRootPath(dirPath);
    connect(newFileWatcher.get(), &QFileSystemModel::directoryLoaded,
            this, [this, newFileWatcher]() {
        disconnect(newFileWatcher.get(), &QFileSystemModel::directoryLoaded,
                   this, nullptr);
        connect(newFileWatcher.get(), &QFileSystemModel::rowsInserted, this,
        [this, newFileWatcher](const QModelIndex &parent, int first, int last) {
            for(int row = first; row <= last; row++) {
                const auto rowIndex = newFileWatcher->index(row, 0, parent);
                const QString path = newFileWatcher->filePath(rowIndex);
                iniIfCustomRasterEffect(path);
            }
        });
    });*/
}

void EffectsLoader::reloadProgram(ShaderEffectCreator* const loaded,
                                  const QString &fragPath)
{
    try {
        makeCurrent();
        loaded->reloadProgram(this, fragPath);
        emit programChanged(&*loaded->fProgram);
        doneCurrent();
    } catch(const std::exception& e) {
        doneCurrent();
        gPrintExceptionCritical(e);
    }
}

void EffectsLoader::iniShaderEffects()
{
    makeCurrent();

    for (const auto &path : AppSupport::getFilesFromPath(AppSupport::getAppShaderPresetsPath(),
                                                         QStringList() << "*.gre")) {
        try {
            iniShaderEffectProgramExec(path);
        } catch(const std::exception& e) {
            gPrintExceptionCritical(e);
        }
    }

    const QString dirPath = AppSupport::getAppShaderEffectsPath();
    for (const auto &path : AppSupport::getFilesFromPath(dirPath,
                                                         QStringList() << "*.gre")) {
        try {
            iniShaderEffectProgramExec(path);
        } catch(const std::exception& e) {
            gPrintExceptionCritical(e);
        }
    }

    //qDebug() << "Shaders paths" << mLoadedGREPaths;
    qDebug() << "Shaders total" << mLoadedShaders.count();

    // TODO: rewrite this
    /*const auto newFileWatcher = QSharedPointer<QFileSystemModel>(
                new QFileSystemModel);
    newFileWatcher->setRootPath(dirPath);
    connect(newFileWatcher.get(), &QFileSystemModel::directoryLoaded,
            this, [this, newFileWatcher]() {
        disconnect(newFileWatcher.get(), &QFileSystemModel::directoryLoaded,
                   this, nullptr);
        connect(newFileWatcher.get(), &QFileSystemModel::rowsInserted, this,
        [this, newFileWatcher](const QModelIndex &parent, int first, int last) {
            for (int row = first; row <= last; row++) {
                const auto rowIndex = newFileWatcher->index(row, 0, parent);
                const QString path = newFileWatcher->filePath(rowIndex);
                const QFileInfo fileInfo(path);
                if (!fileInfo.isFile()) { return; }
                const QString suffix = fileInfo.suffix();
                QString grePath;
                QString fragPath;
                if (suffix == "gre") {
                    fragPath = QString("%1/%2.frag").arg(fileInfo.path(),
                                                         fileInfo.completeBaseName());
                    if (!QFile::exists(fragPath)) { continue; }
                    grePath = path;
                } else if (suffix == "frag") {
                    grePath = QString("%1/%2.gre").arg(fileInfo.path(),
                                                       fileInfo.completeBaseName());
                    fragPath = path;
                    if (!QFile::exists(grePath)) { continue; }
                } else { continue; }
                if (mLoadedGREPaths.contains(grePath)) { continue; }
                iniSingleRasterEffectProgram(grePath);
            }
        });
    });*/
    doneCurrent();
}

void EffectsLoader::iniSingleRasterEffectProgram(const QString &grePath)
{
    try {
        makeCurrent();
        iniShaderEffectProgramExec(grePath);
        doneCurrent();
    } catch(const std::exception& e) {
        doneCurrent();
        gPrintExceptionCritical(e);
    }
}

void EffectsLoader::iniShaderEffectProgramExec(const QString &grePath)
{
    if (!QFile::exists(grePath) || mLoadedGREPaths.contains(grePath)) { return; }

    const auto shaderID = AppSupport::getShaderID(grePath);
    if (shaderID.first.isEmpty() || shaderID.second.isEmpty()) { return; }

    if (mShadersDisabled.contains(grePath)) {
        qDebug() << "SKIP DISABLED SHADER:" << shaderID.first << shaderID.second << grePath;
        mLoadedGREPaths << grePath;
        return;
    }

    if (mLoadedShaders.contains(shaderID)) {
        qDebug() << "SKIP DUPLICATE SHADER:" << shaderID.first << shaderID.second << grePath;
        return;
    }

    const QFileInfo fileInfo(grePath);
    const QString fragPath = QString::fromUtf8("%1/%2.frag").arg(fileInfo.path(),
                                                                 fileInfo.completeBaseName());
    if (!QFile::exists(fragPath)) { return; }
    qDebug() << "Loading Shader" << shaderID.first << shaderID.second << grePath;
    try {
        // TODO: we should keep each creator in an list, replace shaderID
        /*const auto loaded =*/ ShaderEffectCreator::sLoadFromFile(this, grePath).get();
        mLoadedGREPaths << grePath;
        mLoadedShaders << shaderID;
    } catch(...) {
        RuntimeThrow("Error while loading ShaderEffect from '" + grePath + "'");
    }
}
